/*
 * Patch.cpp
 */

#include "Patch.h"
#include "tracy/Tracy.hpp"
#include <charconv>

namespace OM
{
	std::optional<Patch> Patch::fromReply(Node& reply)
	{
		ZoneScoped;
		const Node& key = reply["key"];
		if (key.kind() != Node::Kind::string || reply["result"].kind() == Node::Kind::missing)
		{
			return std::nullopt;
		}
		const auto root = Path::parse(key.text());
		if (!root)
		{
			return std::nullopt;
		}

		Patch patch;
		patch.m_root = *root;
		const std::string_view flags = reply["flags"].text();
		patch.m_live = flags.find('f') != std::string_view::npos;
		if (const size_t a = flags.find('a'); a != std::string_view::npos)
		{
			std::from_chars(flags.data() + a + 1, flags.data() + flags.size(), patch.m_first);
		}
		patch.m_next = static_cast<size_t>(std::max<int64_t>(reply["next"].integer(0), 0));
		patch.m_result = std::move(reply.field("result"));
		return patch;
	}

	Patch Patch::wholeModel(Node& model)
	{
		ZoneScoped;
		Patch patch;
		patch.m_result = std::move(model);
		return patch;
	}

	Patch Patch::liveModel(Node& model)
	{
		ZoneScoped;
		Patch patch;
		patch.m_live = true;
		patch.m_result = std::move(model);
		return patch;
	}
} // namespace OM
