/*
 * LayoutEditor.h
 *
 * Editing the layouts on the screen: what the user changes while customising, before it is kept or thrown away.
 *
 * While editing, every slot has a draft, a copy of its layout that each change edits. Each change is checked (a widget
 * only moves where it fits, locked items can't be removed or reconfigured) and the draft is shown at once through
 * Layouts::preview(), so every <ds_layout> follows it. done() saves the drafts that differ from the saved layouts;
 * cancel() shows the saved layouts again. Undo goes back one change at a time; a drag (beginGesture() to
 * endGesture()) is one change, however many steps it takes.
 *
 * The editor knows nothing of widgets on the screen: <ds_layout> turns touches into calls here, and the customise
 * presenter (UI/Presenters/Customise) its toolbar's buttons. Both learn of changes as an EditorListener.
 */

#pragma once

#include "Layout.h"
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** Told when anything about the edit changes: starting or ending, the selection, a draft, the undo history */
	class EditorListener
	{
	  public:
		virtual ~EditorListener() = default;

		/** Something changed */
		virtual void onEditorChanged() = 0;
	};

	class LayoutEditor
	{
	  public:
		/** What is selected: an item, or a place in a slot to add one */
		struct Selection
		{
			std::string slot;							// The slot; empty if nothing is selected
			std::string item;							// The item's id; empty for a place
			std::optional<std::pair<int32_t, int32_t>> cell; // A grid place's cell, column then row

			/** Equal if every member is */
			bool operator==(const Selection&) const = default;
		};

		/** The editor */
		static LayoutEditor& get();

		/** Whether an edit is going on */
		bool isEditing() const { return m_editing; }

		/** Start editing every slot that has a layout, with nothing selected */
		void begin();

		/** Stop editing and show the saved layouts again */
		void cancel();

		/**
		 * Stop editing and save each draft that differs from its saved layout
		 * @param error Set to what went wrong, for the first draft that couldn't be saved
		 * @return whether every draft was saved (the drafts are shown either way)
		 */
		bool done(std::string& error);

		/** The draft of `slot`, or nullptr if it has none or no edit is going on */
		const Layout* draft(std::string_view slot) const;

		/** The selection */
		const Selection& selection() const { return m_selection; }

		/** The selected item, or nullptr */
		const LayoutItem* selectedItem() const;

		/** Select item `id` of `slot` */
		void select(const std::string& slot, const std::string& id);

		/** Select a place in `slot` to add a widget: grid cell (x, y), or the end of a strip */
		void selectPlace(const std::string& slot, std::optional<std::pair<int32_t, int32_t>> cell = std::nullopt);

		/** Select nothing */
		void clearSelection();

		/** Whether the selected item can be removed: it isn't locked */
		bool canRemove() const;

		/** Whether the selected item has settings the user can change: its widget has some, and it isn't locked */
		bool canConfigure() const;

		/** The slot add() adds to: the selected one, else the first grid slot, else the first slot; empty if none */
		std::string targetSlot() const;

		/** Start a drag: the changes until endGesture() are undone as one */
		void beginGesture();

		/** End a drag */
		void endGesture();

		/**
		 * Put grid item `id` in the w by h cells from (x, y), e.g. when one of its edges is dragged
		 * @return whether it changed: the size fits its widget, and the cells are in the grid and free
		 */
		bool place(const std::string& slot, const std::string& id, int32_t x, int32_t y, int32_t w, int32_t h);

		/** Move grid item `id` so its top left cell is (x, y). @return whether it fits there and was moved */
		bool move(const std::string& slot, const std::string& id, int32_t x, int32_t y);

		/** Resize grid item `id` to w by h cells. @return whether that size fits its widget and the grid */
		bool resize(const std::string& slot, const std::string& id, int32_t w, int32_t h);

		/** Move strip item `id` to position `index` in its strip. @return whether it moved */
		bool moveTo(const std::string& slot, const std::string& id, size_t index);

		/**
		 * Add widget `widget` where the selection says (the selected place or slot), at its default size or, if that
		 * doesn't fit, the first place its smallest size does. The new item is selected.
		 * @return the new item's id, or nullopt if it can't go in the slot or doesn't fit anywhere
		 */
		std::optional<std::string> add(const std::string& widget);

		/** Remove the selected item, unless it is locked. @return whether it was removed */
		bool removeSelected();

		/** Set the selected item's setting `name` to `value`. @return whether it is a value of that setting */
		bool setSetting(const std::string& name, const nlohmann::json& value);

		/** Change grid `slot` to `columns` by `rows` cells, scaling its items. @return whether it is a grid */
		bool setGrid(const std::string& slot, int32_t columns, int32_t rows);

		/** Put `slot` back to its default layout (see Layouts::defaultFor()) */
		void reset(const std::string& slot);

		/** Whether there is a change to undo */
		bool canUndo() const { return !m_undo.empty(); }

		/** Undo the last change */
		void undo();

		/** Tell `listener` about changes until it is removed */
		void addListener(EditorListener* listener);

		/** Stop telling `listener` about changes */
		void removeListener(EditorListener* listener);

	  private:
		using Drafts = std::map<std::string, Layout, std::less<>>; // Each slot's draft, by slot

		/** The draft of `slot` to change, or nullptr */
		Layout* editable(std::string_view slot);

		/** Before a change to `slot`: remember the drafts, for undo, unless a gesture already did */
		void record();

		/** After a change to `slot`'s draft: show it and tell the listeners */
		void changed(const std::string& slot);

		/** Tell every listener something changed */
		void notify();

		bool m_editing = false;					// Between begin() and done() or cancel()
		Drafts m_drafts;						// Each slot's draft
		Selection m_selection;					// What is selected
		std::vector<Drafts> m_undo;				// The drafts before each change, oldest first
		std::optional<Drafts> m_gestureStart;	// The drafts when the current gesture began
		std::vector<EditorListener*> m_listeners; // Told about changes
	};
} // namespace UI::Xml
