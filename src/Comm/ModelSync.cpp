/*
 * ModelSync.cpp
 */

#include "ModelSync.h"
#include "Configuration.h"
#include "Debug.h"
#include "tracy/Tracy.hpp"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <vector>

namespace Comm
{
	namespace
	{
		/** Keys fetched before others when several are due, most needed first; others follow by name */
		constexpr std::array<std::string_view, 13> fetchOrder = {
			"state",
			"move",
			"heat",
			"tools",
			"job",
			"fans",
			"directories",
			"boards",
			"network",
			"sensors",
			"spindles",
			"inputs",
			"volumes",
		};

		/** A key never fetched in detail: it comes with every live model */
		constexpr std::string_view liveOnlyKey = "seqs";

		/** Where `key` comes in fetchOrder; keys not in it come after all of them */
		size_t rank(std::string_view key)
		{
			ZoneScoped;
			return static_cast<size_t>(std::ranges::find(fetchOrder, key) - fetchOrder.begin());
		}
	} // namespace

	ModelSync::ModelSync(ModelLink& link, OM::Tree& tree)
		: m_link(link)
		, m_tree(tree)
	{
		ZoneScoped;
	}

	std::string_view ModelSync::detailFlags(std::string_view key)
	{
		ZoneScoped;
		if (key == "heat")
		{
			return "vo"; // 'o' for heat.bedHeaters and heat.chamberHeaters, obsolete since RepRapFirmware 3.7
		}
		if (key == "move" || key == "job" || key == "state")
		{
			return "vn"; // 'n' for their null fields, which say something (no job file, no current tool, …)
		}
		return "v";
	}

	void ModelSync::reset()
	{
		ZoneScoped;
		m_generation++;
		m_lastPoll.reset();
		m_liveAsked = false;
		m_liveSeen = false;
		m_fetching = false;
		m_fetchSeq.reset();
		m_keys.clear();
		m_replySeq.reset();
	}

	void ModelSync::tick(Clock::time_point now)
	{
		ZoneScoped;
		if (!m_liveAsked && (!m_lastPoll || now - *m_lastPoll >= m_pollInterval))
		{
			askLive(now);
		}
		fetchNext(); // something may be wanted that wasn't before
	}

	void ModelSync::pollNow()
	{
		ZoneScoped;
		if (!m_liveAsked)
		{
			askLive(Clock::now());
		}
	}

	void ModelSync::askLive(Clock::time_point now)
	{
		ZoneScoped;
		m_liveAsked = true;
		m_lastPoll = now;
		m_link.requestModel("",
							liveFlags,
							[this, generation = m_generation](std::expected<OM::Node, ReplyError> reply)
							{
								ZoneScopedN("askLive_cb");
								if (generation == m_generation)
								{
									liveArrived(std::move(reply));
								}
							});
	}

	void ModelSync::receiveUnasked(const OM::Patch& patch)
	{
		ZoneScoped;
		m_tree.apply(patch);
		if (patch.root().empty())
		{
			readSeqs(patch.result()["seqs"]);
			fetchNext();
		}
	}

	void ModelSync::liveArrived(std::expected<OM::Node, ReplyError> reply)
	{
		ZoneScoped;
		m_liveAsked = false;
		if (!reply)
		{
			LOG_WARN("No live object model: {:s}", reply.error().message);
			return;
		}
		const auto patch = OM::Patch::fromReply(*reply);
		if (!patch || !patch->root().empty())
		{
			LOG_WARN("The live object model was asked for, but something else came: {:s}", reply->toJson());
			return;
		}
		m_tree.apply(*patch);
		readSeqs(patch->result()["seqs"]);
		m_liveSeen = true;
		fetchNext();
	}

	void ModelSync::readSeqs(const OM::Node& seqs)
	{
		ZoneScoped;
		seqs.forEachField(
			[this](std::string_view name, const OM::Node& value)
			{
				if (name == "reply")
				{
					if (m_replySeq != value.integer())
					{
						m_replySeq = value.integer();
						m_link.fetchResponses();
					}
					return;
				}
				m_keys[std::string(name)].seq = value.integer();
			});
	}

	void ModelSync::fetchNext()
	{
		ZoneScoped;
		if (m_fetching || !m_liveSeen)
		{
			return;
		}
		std::vector<std::string> due;
		for (const std::string& key : m_tree.wantedKeys())
		{
			if (key == liveOnlyKey)
			{
				continue;
			}
			const KeyState& state = m_keys[key];
			if (!state.fetched || (state.seq && *state.seq != *state.fetched))
			{
				due.push_back(key);
			}
		}
		if (due.empty())
		{
			return;
		}
		const auto next = std::ranges::min_element(due,
												   [](const std::string& a, const std::string& b)
												   { return std::pair(rank(a), a) < std::pair(rank(b), b); });
		m_fetchSeq = m_keys[*next].seq;
		fetch(*next, *next, 0);
	}

	void ModelSync::fetch(const std::string& key, const std::string& path, size_t from)
	{
		ZoneScoped;
		ZoneText(path.data(), path.size());
		m_fetching = true;
		std::string flags(detailFlags(key));
		if (from > 0)
		{
			flags += fmt::format("a{:d}", from);
		}
		LOG_DBG("Fetching {:s} ({:s})", path, flags);
		m_link.requestModel(path,
							flags,
							[this, key, path, generation = m_generation](std::expected<OM::Node, ReplyError> reply)
							{
								ZoneScopedN("fetch_cb");
								if (generation == m_generation)
								{
									fetchArrived(key, path, std::move(reply));
								}
							});
	}

	void ModelSync::fetchArrived(const std::string& key,
								 const std::string& path,
								 std::expected<OM::Node, ReplyError> reply)
	{
		ZoneScoped;
		if (!reply)
		{
			// Tried again once the next live model has arrived
			LOG_WARN("Fetching {:s} failed: {:s}", path, reply.error().message);
			m_fetching = false;
			return;
		}
		const auto patch = OM::Patch::fromReply(*reply);
		if (!patch || patch->root().str() != path)
		{
			LOG_WARN("{:s} was asked for, but something else came: {:s}", path, reply->toJson());
			m_fetching = false;
			return;
		}
		m_tree.apply(*patch);

		if (patch->partial() && patch->next() > 0)
		{
			fetch(key, path, patch->next()); // the firmware split the array: fetch on from where it stopped
			return;
		}
		if (path == "move")
		{
			// `move` holds the first MAX_REPORTED_AXES axes only: there may be more
			const size_t sent = patch->result()["axes"].size();
			if (sent >= MAX_REPORTED_AXES)
			{
				fetch(key, "move.axes", sent);
				return;
			}
		}
		fetchDone(key);
	}

	void ModelSync::fetchDone(const std::string& key)
	{
		ZoneScoped;
		m_keys[key].fetched = m_fetchSeq.value_or(0);
		m_fetching = false;
		fetchNext();
	}
} // namespace Comm
