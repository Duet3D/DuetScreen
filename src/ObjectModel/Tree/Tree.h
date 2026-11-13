/*
 * Tree.h
 *
 * DuetScreen's copy of the printer's object model: every field RepRapFirmware has sent, as one tree of Nodes, kept in
 * the shape the firmware sends it, so the UI can read any field by its RRF path (`heat.heaters[1].current`) without
 * code for that field.
 *
 *   JSON decoder ──Patch──▶ Tree::apply() ──changes──▶ listeners (the UI's OM bindings, …)
 *
 * The decoder builds one Patch per response and gives it to apply() when the response is complete. apply() merges it
 * in and notes which paths changed; listeners are then told, once per response, and take the changes with
 * takeChanges().
 *
 * How a response is merged:
 *   - a detailed response (no 'f' flag) replaces everything under its key, so a field it doesn't send is removed:
 *     without the 'n' flag RepRapFirmware leaves null fields out
 *   - a live response ('f') only updates what it sends; arrays take the length sent
 *   - a partial array result ('a' flag, or `next` > 0) replaces only the elements sent; the last part sets the length
 *
 * Threads: the decoder applies patches on its own thread, and the UI reads from another, so reading the tree (root(),
 * and every Node it leads to) needs the model lock (MODEL_LOCK) held for as long as the nodes are used.
 */

#pragma once

#include "ChangeSet.h"
#include "Node.h"
#include "Patch.h"
#include <functional>
#include <mutex>
#include <vector>

namespace OM
{
	class Tree
	{
	  public:
		/** Something told when the tree has changed; it should take the changes (soon, not necessarily now) */
		using Listener = std::function<void()>;

		/** The object model of the printer DuetScreen is connected to */
		static Tree& get();

		/** An empty tree; normally only get()'s, but tests may make their own */
		Tree();

		/** The root of the object model, always an object. Hold the model lock while using it or anything it leads to. */
		const Node& root() const { return m_root; }

		/** Merge a complete response into the tree (see the file comment) and tell the listeners if anything changed */
		void apply(Patch& patch);

		/** Forget everything (e.g. when disconnected) and tell the listeners */
		void clear();

		/** The paths changed since the last call, emptied */
		ChangeSet takeChanges();

		/** Tell `listener` whenever the tree changes. Listeners are called on the thread that changed the tree. */
		void addListener(Listener listener);

		/** Stop telling every listener (e.g. when the UI stops) */
		void clearListeners();

	  private:
		/** Tell the listeners, if there are changes they haven't been told about */
		void notify();

		Node m_root;					 // The object model
		ChangeSet m_changes;			 // Changed since the last takeChanges()
		bool m_notified = false;		 // Whether listeners have been told about m_changes
		std::vector<Listener> m_listeners; // Told about changes
		std::mutex m_listenersMutex;	 // Guards m_listeners
	};
} // namespace OM
