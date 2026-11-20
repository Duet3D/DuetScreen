/*
 * Patch.h
 *
 * One change to the object model: an M409 response or rr_model reply, or a DSF WebSocket update, read whole (a reply
 * cut short is never a patch). Tree::apply() merges it in.
 *
 * A response looks like {"key":"heat","flags":"v","result":{…},"next":0}. The key says where in the object model the
 * result belongs (empty for the whole model), the flags say whether it is a live response ('f': only the fields that
 * change often) or a detailed one, and for an array result 'a<n>' says the first element sent is element n; `next`,
 * if more than 0, says the array continues in another response.
 */

#pragma once

#include "Node.h"
#include "Path.h"
#include <optional>

namespace OM
{
	class Patch
	{
	  public:
		/**
		 * The patch an object model reply makes: {"key":…,"flags":…,"result":…,"next":…} from M409 or rr_model
		 * @param reply The reply, read as JSON; its result is taken
		 * @return the patch, or nothing if `reply` isn't an object model reply (it has no `key` or no `result`)
		 */
		static std::optional<Patch> fromReply(Node& reply);

		/**
		 * A live patch of the whole object model, as the DSF WebSocket sends: objects update only the fields they
		 * send, arrays take the length sent
		 * @param model The patch; it is taken
		 */
		static Patch liveModel(Node& model);

		/** Where in the object model the result belongs */
		const Path& root() const { return m_root; }

		/** Whether this is a live response, which only updates what it sends */
		bool live() const { return m_live; }

		/** For an array result: the index the first element sent has in the whole array */
		size_t first() const { return m_first; }

		/** Whether an array result is only part of the array: it didn't start at 0, or continues in another response */
		bool partial() const { return m_result.isArray() && (m_first > 0 || m_next > 0); }

		/** Whether a partial array result is the last part */
		bool last() const { return m_next == 0; }

		/** Where an array result continues, or 0 if it is complete */
		size_t next() const { return m_next; }

		/** The result as sent: for a partial array, its element i is element first() + i of the whole array */
		const Node& result() const { return m_result; }

	  private:
		Patch() = default;

		Path m_root;		 // Where the result belongs
		bool m_live = false; // 'f' flag: a live response
		size_t m_first = 0; // 'a' flag: index of the first array element sent
		size_t m_next = 0;	 // `next`: where the array continues, or 0
		Node m_result;		 // The result
	};
} // namespace OM
