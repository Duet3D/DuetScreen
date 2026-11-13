/*
 * Patch.h
 *
 * One M409 response (or rr_model reply) while it is being parsed: what it says, built up privately, field by field,
 * by the JSON decoder. Nothing else can see it until the response is complete and Tree::apply() takes it, so a
 * response cut short by an error changes nothing.
 *
 * A response looks like {"key":"heat","flags":"v","result":{…},"next":0}. The key says where in the object model the
 * result belongs (empty for the whole model), the flags say whether it is a live response ('f': only the fields that
 * change often) or a detailed one, and for an array result 'a<n>' says the first element sent is element n; `next`,
 * if more than 0, says the array continues in another response.
 */

#pragma once

#include "Node.h"
#include "Path.h"
#include <string>
#include <string_view>

namespace OM
{
	class Patch
	{
	  public:
		/** Forget everything, ready for the next response */
		void reset();

		/** The response's `key`, e.g. "heat" or "move.axes"; empty for the whole model */
		void setKey(std::string_view key);

		/** The response's `flags`, e.g. "v", "vn" or "d99f" */
		void setFlags(std::string_view flags);

		/** The response's `next`: the index the array result continues from, or 0 if it is complete */
		void setNext(size_t next) { m_next = next; }

		/**
		 * A value of the result
		 * @param path Where, relative to the result; root-array indices are as sent, before the 'a' offset
		 * @param kind null, boolean, number or string
		 * @param text The JSON text of a bool or number, or the string
		 */
		void value(const Path& path, Node::Kind kind, std::string_view text);

		/** An object starts at `path` (relative to the result), so an object without fields is still an object */
		void objectStart(const Path& path);

		/** An array at `path` (relative to the result) ended after `length` items */
		void arrayEnd(const Path& path, size_t length);

		/** Whether any of the result has arrived */
		bool hasResult() const { return m_hasResult; }

		/** Where the result belongs in the object model */
		const Path& root() const { return m_root; }

		/** Whether this is a live response, which only updates what it sends */
		bool live() const { return m_live; }

		/** For an array result: the index of the first element sent */
		size_t first() const { return m_first; }

		/** For an array result: how many elements were sent */
		size_t count() const { return m_count; }

		/** Whether an array result is only part of the array: it didn't start at 0, or continues in another response */
		bool partial() const { return m_result.isArray() && (m_first > 0 || m_next > 0); }

		/** Whether a partial array result is the last part */
		bool last() const { return m_next == 0; }

		/** The result; Tree::apply() takes its children */
		Node& result() { return m_result; }

	  private:
		/** `path` with the 'a' offset added to a root-array index */
		Path offset(const Path& path) const;

		/** The node at `path` in the result, made if needed */
		Node& make(const Path& path);

		Path m_root;			  // Where the result belongs
		bool m_live = false;	  // 'f' flag: a live response
		size_t m_first = 0;		  // 'a' flag: index of the first array element sent
		size_t m_count = 0;		  // Number of array elements sent, for an array result
		size_t m_next = 0;		  // `next`: where the array continues, or 0
		bool m_hasResult = false; // Whether any of the result arrived
		Node m_result;			  // The result
	};
} // namespace OM
