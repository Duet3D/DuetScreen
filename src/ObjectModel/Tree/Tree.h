/*
 * Tree.h
 *
 * DuetScreen's copy of the printer's object model: every field RepRapFirmware has sent, as one tree of Nodes, kept in
 * the shape the firmware sends it, so the UI can read any field by its RRF path (`heat.heaters[1].current`) without
 * code for that field.
 *
 *   connection ──Patch──▶ Tree::apply() ──changes──▶ listeners (the UI's OM bindings, …)
 *
 * The connection makes one Patch per response (see Patch.h) and gives it to apply(). apply() merges it
 * in and notes which paths changed; listeners are then told, once per response, and take the changes with
 * takeChanges().
 *
 * How a response is merged:
 *   - a detailed response (no 'f' flag) replaces everything under its key, so a field it doesn't send is removed:
 *     without the 'n' flag RepRapFirmware leaves null fields out
 *   - a live response ('f') only updates what it sends; arrays take the length sent
 *   - a partial array result ('a' flag, or `next` > 0) replaces only the elements sent; the last part sets the length
 *
 * One field is filled in by the screen: RepRapFirmware 3.7 replaced heat.bedHeaters and heat.chamberHeaters (a heater
 * per bed or chamber) with heat.bedHeaterMapping and heat.chamberHeaterMapping (a list of heaters per bed or
 * chamber). It still sends the old fields when asked for obsolete ones, but not reliably: at times they are all -1
 * while the mapping names the bed's heater. Where the firmware sends a mapping, apply() fills the old field from it,
 * each bed's or chamber's first heater, so everything that reads it (views, and user bundles, as om.* is the UI's API)
 * works with 3.7 as with 3.6 and earlier, which send no mapping and whose own lists stand.
 *
 * Threads: the connection applies patches on its own thread, and the UI reads from another, so reading the tree
 * (root(), and every Node it leads to) needs the model lock (MODEL_LOCK) held for as long as the nodes are used.
 */

#pragma once

#include "ChangeSet.h"
#include "Node.h"
#include "Patch.h"
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
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

		/** The root of the object model, always an object. Hold the model lock while using it or anything it leads to.
		 */
		const Node& root() const { return m_root; }

		/** Merge a complete response into the tree (see the file comment) and tell the listeners if anything changed */
		void apply(const Patch& patch);

		/** Forget everything (e.g. when disconnected) and tell the listeners */
		void clear();

		/** The paths changed since the last call, emptied */
		ChangeSet takeChanges();

		/**
		 * Tell `listener` whenever the tree changes. Listeners are called on the thread that changed the tree.
		 * @return what removeListener() takes
		 */
		size_t addListener(Listener listener);

		/** Stop telling the listener addListener() returned `id` for */
		void removeListener(size_t id);

		/**
		 * Call `callback` after each response (or clear()) that changes something at, below or above one of `paths`,
		 * for what DuetScreen does itself as the printer changes (see Followers.h), not for showing it: it runs on the
		 * connection's thread, without the model lock, as the response is applied. (The UI's watchers run on its own
		 * thread: see UI/Xml/Om/OmBindings.h.)
		 * @return what unwatch() takes
		 */
		size_t watch(std::vector<std::string> paths, Listener callback);

		/** Stop the watcher watch() returned `id` for */
		void unwatch(size_t id);

		/**
		 * Ask for the object model at `paths` (e.g. "heat.heaters", "move") to be kept up to date: the connection only
		 * fetches the top-level keys something wants (see Comm/ModelSync.h). Watching paths, here or through the UI's
		 * bindings, wants them too.
		 * @return what unwant() takes
		 */
		size_t want(const std::vector<std::string>& paths);

		/** Stop wanting what want() returned `id` for */
		void unwant(size_t id);

		/** The top-level keys something wants, e.g. "heat" for "heat.heaters[1].current" */
		std::set<std::string> wantedKeys() const;

		/** A number that changes whenever what is wanted changes, so wantedKeys() is only read again when it has */
		uint64_t wantsVersion() const { return m_wantsVersion.load(); }

	  private:
		/** Tell the listeners, if there are changes they haven't been told about */
		void notify();

		/** Fill heat.bedHeaters and heat.chamberHeaters from the firmware's mappings, if it sends them, noting any
		 * change in `changes` (see the file comment; the model lock held) */
		void deriveHeaterLists(ChangeSet& changes);

		/** Call the watchers of anything in `changes` */
		void tellWatchers(const ChangeSet& changes);

		/** One watch() */
		struct Watcher
		{
			size_t id;						// What unwatch() takes
			std::vector<std::string> paths; // Paths watched
			Listener callback;				// Called after a change to one
			size_t want;					// What want() gave for the paths
		};

		Node m_root;										  // The object model
		ChangeSet m_changes;								  // Changed since the last takeChanges()
		bool m_notified = false;							  // Whether listeners have been told about m_changes
		std::vector<std::pair<size_t, Listener>> m_listeners; // Told about changes, with their ids
		size_t m_nextListenerId = 1;						  // Id of the next listener added
		std::vector<Watcher> m_watchers;					  // Called after the changes they watch
		std::mutex m_listenersMutex;						  // Guards m_listeners and m_watchers
		std::map<size_t, std::vector<std::string>> m_wants;	  // The top-level keys each want() asked for, by id
		size_t m_nextWantId = 1;							  // Id of the next want()
		mutable std::mutex m_wantsMutex;					  // Guards m_wants
		std::atomic<uint64_t> m_wantsVersion{1};			  // Changes whenever m_wants does
	};
} // namespace OM
