/*
 * Actions.h
 *
 * Actions are how an XML view asks for something to happen. A view never calls code; it names an action:
 *
 *     <ds_button translation_tag="tr.fan.max">
 *         <action trigger="clicked" name="presenter.fans.set_speed" arg="100"/>
 *     </ds_button>
 *
 * The name says where the action is:
 *   - `presenter.<action>`: the presenter of the nearest enclosing XmlView that handles it, then the next one out, ...
 *   - `actions.<group>.<action>`: a global action, added with Actions::addGlobal() in `<Group>Actions.cpp`, e.g.
 *     `actions.gcode.send` in GcodeActions.cpp
 *
 * The handler gets an ActionArgs, which can also find the list rows the widget is inside: `args.row<FanRow>()`.
 *
 * Triggers are LVGL's event names (`clicked`, `value_changed`, `long_pressed`, ...) plus `value_confirmed`, which
 * DuetScreen widgets raise once an edit is finished (a slider is released, or a value is typed in), and `dismissed`,
 * which a `<ds_modal>` raises when the user closes it (see Widgets/DsModal.h).
 *
 * `arg` may contain `{name}` placeholders. `{event.value}` is the edited value; any other name is replaced by the
 * current value of the subject of that name, looked up from where the `<action>` was written. E.g. in a fan row:
 * `<action name="actions.gcode.send" arg="M106 P{number} S{event.value}"/>`.
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "lvgl/lvgl.h"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** What a handler is told about the action it is handling */
	struct ActionArgs
	{
		std::string_view name;		 // The action's name
		std::string arg;			 // The `arg` attribute, with placeholders filled in; empty if none
		lv_event_t* event = nullptr; // The event that triggered it, or nullptr if dispatched directly
		lv_obj_t* target = nullptr;	 // The widget the `<action>` belongs to

		/** The value an edit produced (`value_confirmed`; `value_changed` on a ds_slider, slider, arc, dropdown (the
		 * option's index) or checkable), or the value tapped (`value_tapped`), else `arg` as a number */
		std::optional<float> value() const;

		/** `arg` as an integer, if it is one */
		std::optional<int32_t> argInt() const;

		/** The text a text area holds, or the option a dropdown has chosen (an edit's result, e.g. on
		 * `value_changed`), else `arg` */
		std::string text() const;

		/** Where on the screen the widget was touched, for an event from a touch such as `clicked`, else nothing */
		std::optional<lv_point_t> point() const;

		/** The innermost list row the widget is in, or nullptr */
		Row* row() const;

		/** The innermost list row of type R the widget is in, or nullptr. Rows of other types are skipped, so a heater
		 * row's handler can ask for its tool with `row<ToolRow>()`. */
		template <typename R>
			requires std::derived_from<R, Row>
		R* row() const
		{
			for (lv_obj_t* obj = target; obj != nullptr; obj = outerOf(obj))
			{
				if (auto* r = dynamic_cast<R*>(rowOf(obj)))
				{
					return r;
				}
			}
			return nullptr;
		}

	  private:
		/** The row `obj` is the root widget of, or nullptr */
		static Row* rowOf(lv_obj_t* obj);

	  public:
		/** Where looking outwards from `obj` continues: its owner (see Actions::setOwner), else its parent */
		static lv_obj_t* outerOf(lv_obj_t* obj);
	};

	/** Handles one action */
	using ActionHandler = std::function<void(const ActionArgs&)>;

	/** A set of named action handlers, e.g. one presenter's */
	class ActionHandlerSet
	{
	  public:
		/**
		 * Handle actions called `name` with `handler`, replacing any earlier handler
		 * @param description What the action does and what it takes (`arg`, the edited value, the row it is in), for
		 * the UI API reference
		 */
		void add(std::string name, std::string description, ActionHandler handler, std::string file = {})
		{
			m_handlers[std::move(name)] = Entry{std::move(description), std::move(handler), std::move(file)};
		}

		/** @return whether there is a handler for `name` */
		bool has(std::string_view name) const { return m_handlers.find(name) != m_handlers.end(); }

		/** Run the handler for `name` with `args`. @return whether there was one */
		bool dispatch(std::string_view name, const ActionArgs& args) const;

		/** Call `fn(name, description, file)` for every action, in name order; `file` is empty if not known */
		void forEach(
			const std::function<void(std::string_view name, std::string_view description, std::string_view file)>& fn)
			const
		{
			for (const auto& [name, entry] : m_handlers)
			{
				fn(name, entry.description, entry.file);
			}
		}

	  private:
		/** One action */
		struct Entry
		{
			std::string description; // What it does, for the UI API reference
			ActionHandler handler;	 // Runs it
			std::string file;		 // The source file that added it, or empty
		};

		std::map<std::string, Entry, std::less<>> m_handlers; // Actions by name
	};

	namespace Actions
	{
		/** What an action of the presenter of an enclosing view is written after: `presenter.<action>` */
		inline constexpr std::string_view PRESENTER_PREFIX = "presenter.";

		/** What a global action is written after: `actions.<group>.<action>`, added in `<Group>Actions.cpp` */
		inline constexpr std::string_view GLOBAL_PREFIX = "actions.";

		/** Register the `<action>` element and the `value_confirmed` event. Call after lv_xml_init(). */
		void init();

		/** Forget every widget registration. Global actions are kept, like presenters, for the life of the program. */
		void deinit();

		/** The event DuetScreen widgets send, with a `float*` parameter, when an edit is confirmed */
		lv_event_code_t valueConfirmedEvent();

		/** The event a `<ds_modal>` sends when it is closed other than by its subject (tapped outside, back, …) */
		lv_event_code_t dismissedEvent();

		/** The event a `<ds_slider numberpad="own">` sends when its value is tapped, for the view to ask for one */
		lv_event_code_t valueTappedEvent();

		/** Actions from `obj` and the widgets inside it try `handlers` before outer ones. Until `obj` is deleted. */
		void setHandlers(lv_obj_t* obj, const ActionHandlerSet* handlers);

		/** `obj` is the root widget of `row` in a `<ds_list>`. Until `obj` is deleted. */
		void setRow(lv_obj_t* obj, Row* row);

		/**
		 * Actions (and rows) from `obj` and the widgets inside it are looked for outwards from `owner` instead of from
		 * `obj`'s parent: for widgets written in one view but shown elsewhere, such as a dialog moved to the modal
		 * host. `owner` must outlive `obj`. Until `obj` is deleted.
		 */
		void setOwner(lv_obj_t* obj, lv_obj_t* owner);

		/**
		 * Handle `actions.<name>` from any view, replacing any earlier handler. Lasts for the life of the program, so
		 * it may be called before init().
		 * @param name `<group>.<action>`, e.g. "heater.toggle"; a group's actions are added in `<Group>Actions.cpp`
		 *        (HeaterActions.cpp), so `actions.heater.toggle` says where to look
		 * @param description As for ActionHandlerSet::add()
		 * @param where Where it is added, recorded for the UI API reference
		 */
		void addGlobal(std::string name,
					   std::string description,
					   ActionHandler handler,
					   std::source_location where = std::source_location::current());

		/** The global actions (see addGlobal()), by their full names (`actions.<group>.<action>`) */
		const ActionHandlerSet& globals();

		/** Every name `<action trigger="…">` accepts: LVGL's event names, `value_confirmed` and `dismissed` */
		std::vector<std::string> triggers();

		/**
		 * Resolve and run an action as if an `<action>` on `from` had fired (placeholders in `arg` are not filled in)
		 * @param name As written in XML: `presenter.<action>` (the presenters of the views around `from`, innermost
		 *        first) or `actions.<group>.<action>` (the global actions)
		 * @return whether a handler ran
		 */
		bool dispatch(lv_obj_t* from, std::string_view name, std::string_view arg, lv_event_t* event = nullptr);
	} // namespace Actions
} // namespace UI::Xml
