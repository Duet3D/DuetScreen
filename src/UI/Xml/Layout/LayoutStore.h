/*
 * LayoutStore.h
 *
 * The layout each slot shows, chosen from the layout files of every bundle (see Layout.h), and where the user's own
 * layouts are saved.
 *
 * For each slot:
 *   - its *locks* are the locked items of the first bundle's layout of the slot (the built-in bundle's, for the
 *     built-in slots), so no later bundle can remove the E-stop
 *   - its *default* is the layout of the last bundle that has one, not counting the user's own: a shared bundle's
 *     layout replaces the built-in one, and "reset" goes back to it
 *   - it *shows* the user's own layout if there is one, else the default
 *
 * The user's own layouts are a bundle of their own, the local bundle (`local/` in the user bundle folder), which the
 * on-screen editor writes and which loads after every other bundle. Like every other customisation it is only files
 * in the user bundle folder, so copying that folder copies everything the user has set up.
 *
 * Saving is safe against power loss: the new file is written beside the old one and renamed over it, after a copy of
 * the old one is kept as `<slot>.json.bak`, which is read if the layout file can't be.
 */

#pragma once

#include "Layout.h"
#include "UI/Xml/Runtime/Bundles.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** Told when the layout a slot shows changes */
	class LayoutListener
	{
	  public:
		virtual ~LayoutListener() = default;

		/** The layout `slot` shows has changed */
		virtual void onLayoutChanged(std::string_view slot) = 0;
	};

	namespace Layouts
	{
		/** The local bundle's directory name, in the user bundle folder */
		inline constexpr std::string_view LOCAL_BUNDLE = "local";

		/**
		 * Use the layout files read from the bundles, in load order (the built-in bundle's first). Each is repaired
		 * (see repair()), and what was changed is added to `issues`. Call after WidgetCatalogue::set(). Listeners
		 * are told every slot has changed.
		 * @param localDir The local bundle's directory, where save() writes; empty to save nothing
		 */
		void set(std::vector<LayoutFile> files, std::filesystem::path localDir, std::vector<LoadIssue>& issues);

		/** Forget every layout */
		void clear();

		/** The layout `slot` shows, or nullptr if it has none */
		const Layout* find(std::string_view slot);

		/** The layout `slot` shows when the user hasn't changed it, or nullptr */
		const Layout* defaultFor(std::string_view slot);

		/** The built-in layout of `slot`, whose locked items every layout of the slot keeps, or nullptr */
		const Layout* locksFor(std::string_view slot);

		/** Every slot with a layout, in name order */
		std::vector<std::string> slots();

		/**
		 * Make `layout` the one its slot shows, and save it as the user's own: or, if it is the default, delete the
		 * user's own so the default shows. It is repaired first. Listeners are told.
		 * @param error Set to what went wrong if false is returned; the layout is shown anyway
		 */
		bool save(Layout layout, std::string& error);

		/**
		 * Show `layout` in its slot without saving it, e.g. a layout being edited. It is repaired first. Listeners are
		 * told. revert() goes back to the slot's saved layout.
		 */
		void preview(Layout layout);

		/** Show `slot`'s saved layout again after preview(). Listeners are told if it changes. */
		void revert(std::string_view slot);

		/** The layout `slot` shows when nothing is being previewed: the user's own, else the default; or nullptr */
		const Layout* saved(std::string_view slot);

		/** Tell `listener` about changes until it is removed */
		void addListener(LayoutListener* listener);

		/** Stop telling `listener` about changes */
		void removeListener(LayoutListener* listener);

		/**
		 * Write `text` to `file` so that a reader finds the old or the new contents, never a mix: write a temporary
		 * file, flush it to the card, then rename it over `file`. Any earlier contents are first kept the same way
		 * as `file` + ".bak". Missing folders on the way to `file` are created, and flushed to the card too.
		 * @param error Set to what went wrong if false is returned
		 */
		bool writeSafely(const std::filesystem::path& file, std::string_view text, std::string& error);

		/**
		 * Delete `dir` and everything in it, if it is there, and flush its removal to the card
		 * @param error Set to what went wrong if false is returned
		 */
		bool removeSafely(const std::filesystem::path& dir, std::string& error);
	} // namespace Layouts
} // namespace UI::Xml
