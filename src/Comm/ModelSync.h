/*
 * ModelSync.h
 *
 * Keeps DuetScreen's copy of the object model (OM::Tree) up to date over a connection that is asked for it: M409 over
 * USB and UART, rr_model over the network. (Through DSF the model is pushed instead: see DsfLink.h.)
 *
 *   - The live model (flags "d99f": the fields that change often, and `seqs`) is asked for every poll interval.
 *   - `seqs` holds a number per top-level key that changes whenever anything of the key changes. A key something wants
 *     (OM::Tree::want(): the UI binding to it, C++ watching it) is fetched in detail when first wanted on a connection,
 *     and again whenever its number changes. Keys nothing wants are never fetched.
 *   - Detailed fetches go one after another, each as soon as the last is answered, so a whole model arrives in a few
 *     round trips rather than one per poll.
 *   - An array result the firmware splits (`next` > 0) is fetched on from where it stopped, whatever the key.
 *   - `move` holds only the first few axes: the rest are fetched from `move.axes`.
 *
 * It runs on the connection's thread: everything here is called there, and the link's answers arrive there.
 */

#pragma once

#include "Comm/Replies.h"
#include "ObjectModel/Tree/Node.h"
#include "ObjectModel/Tree/Patch.h"
#include "ObjectModel/Tree/Tree.h"
#include <chrono>
#include <cstdint>
#include <expected>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Comm
{
	/** What ModelSync needs of a connection */
	class ModelLink
	{
	  public:
		/** Receives the reply to requestModel(): the reply's JSON, or why there is none (e.g. it timed out) */
		using ModelHandler = std::function<void(std::expected<OM::Node, ReplyError> reply)>;

		virtual ~ModelLink() = default;

		/**
		 * Ask for the object model at `key` (empty for the whole model) with M409's `flags`
		 * @param handler Given the reply, on the connection's thread, once
		 */
		virtual void requestModel(std::string_view key, std::string_view flags, ModelHandler handler) = 0;

		/** Fetch the responses the printer holds for the screen (rr_reply); nothing where responses arrive anyway */
		virtual void fetchResponses() {}
	};

	class ModelSync
	{
	  public:
		using Clock = std::chrono::steady_clock; // What times are measured with

		/** The live model's flags */
		static constexpr std::string_view liveFlags = "d99f";

		/**
		 * @param link Where the model is asked for
		 * @param tree The model kept up to date, and what says which keys are wanted
		 */
		explicit ModelSync(ModelLink& link, OM::Tree& tree = OM::Tree::get());

		/** How often the live model is asked for */
		void setPollInterval(std::chrono::milliseconds interval) { m_pollInterval = interval; }

		/** Let time pass: ask for the live model if it is due and isn't already asked for, and fetch what is due */
		void tick(Clock::time_point now);

		/** Ask for the live model now, unless it is already asked for */
		void pollNow();

		/** Merge an object model reply nothing asked for: a busy channel's status report, or one that came too late */
		void receiveUnasked(const OM::Patch& patch);

		/** Whether nothing is asked for and waiting for its reply */
		bool idle() const { return !m_liveAsked && !m_fetching; }

		/** Whether a live model has arrived: the printer answers, and its `seqs` are known */
		bool hasLiveModel() const { return m_liveSeen; }

		/** When tick() next has something to do: the next live poll */
		Clock::time_point nextPoll() const;

		/** The flags a detailed fetch of `key` uses */
		static std::string_view detailFlags(std::string_view key);

	  private:
		/** What is known about one top-level key */
		struct KeyState
		{
			std::optional<int64_t> seq; // Its number in `seqs`, if the firmware reports one
			std::optional<int64_t>
				fetched; // The number it had when last fetched; set (to 0 if unreported) once fetched
		};

		/** Ask for the live model, as of `now` */
		void askLive(Clock::time_point now);

		/** The live model arrived (or didn't) */
		void liveArrived(std::expected<OM::Node, ReplyError> reply);

		/** Note what the live model's `seqs` say has changed */
		void readSeqs(const OM::Node& seqs);

		/** Fetch the next wanted key that is due, unless a fetch is in flight */
		void fetchNext();

		/**
		 * Ask for (part of) a detailed fetch
		 * @param key The key fetched
		 * @param path What is asked for: the key, or `move.axes` while fetching `move`
		 * @param from The index of the first array element asked for, or 0
		 */
		void fetch(const std::string& key, const std::string& path, size_t from);

		/** Part of a detailed fetch arrived (or didn't) */
		void fetchArrived(const std::string& key, const std::string& path, std::expected<OM::Node, ReplyError> reply);

		/** A detailed fetch is complete: the key is up to date as of the number it had when asked for */
		void fetchDone(const std::string& key);

		ModelLink& m_link;							   // Where the model is asked for
		OM::Tree& m_tree;							   // The model kept up to date
		std::chrono::milliseconds m_pollInterval{250}; // How often the live model is asked for
		std::optional<Clock::time_point> m_lastPoll;   // When the live model was last asked for
		bool m_liveAsked = false;					   // The live model is asked for and not yet answered
		bool m_liveSeen = false;					   // A live model has arrived: `seqs` are known
		bool m_fetching = false;					   // A detailed fetch is in flight
		bool m_fetchFailed = false;		   // A fetch failed: none is asked for again until the next live model arrives
		uint64_t m_wantsVersion = 0;	   // The tree's wantsVersion() when m_wanted was read
		std::vector<std::string> m_wanted; // The keys the tree wants, as of m_wantsVersion
		std::optional<int64_t> m_fetchSeq; // The fetched key's number when its fetch started
		std::map<std::string, KeyState, std::less<>> m_keys; // What is known of each key
		std::optional<int64_t> m_replySeq;					 // `seqs.reply` when last seen
	};
} // namespace Comm
