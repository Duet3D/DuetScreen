/*
 * Actions.h
 *
 * Actions are how an XML view asks for something to happen. A view never calls code; it names an action:
 *
 *     <ds_button translation_tag="fan.max">
 *         <action trigger="clicked" name="fans.set_speed" arg="100"/>
 *     </ds_button>
 *
 * When the trigger fires, the action is resolved from the widget outwards, like a name in a scope:
 *   1. the presenter of the nearest enclosing XmlView, then the next one out, ...
 *   2. the global actions (registered with Actions::addGlobal(), e.g. `gcode`)
 *
 * The handler gets an ActionArgs, which can also find the list rows the widget is inside: `args.row<FanRow>()`.
 *
 * Triggers are LVGL's event names (`clicked`, `value_changed`, `long_pressed`, ...) plus `value_confirmed`, which
 * DuetScreen widgets raise once an edit is finished (a slider is released, or a value is typed in), and `dismissed`,
 * which a `<ds_modal>` raises when the user closes it (see Widgets/DsModal.h).
 *
 * `arg` may contain `{name}` placeholders. `{value}` is the edited value; any other name is replaced by the current
 * value of the subject of that name, looked up from where the `<action>` was written (row, then presenter, then
 * global). E.g. in a fan row: `<action name="gcode" arg="M106 P{number} S{value}"/>`.
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "lvgl/lvgl.h"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace UI::Xml
{
	/** What a handler is told about the action it is handling */
	struct ActionArgs
	{
		std::string_view name;		 // The action's name
		std::string arg;			 // The `arg` attribute, with placeholders filled in; empty if none
		lv_event_t* event = nullptr; // The event that triggered it, or nullptr if dispatched directly
		lv_obj_t* target = nullptr;	 // The widget the `<action>` belongs to

		/** The value an edit produced (`value_confirmed`, or `value_changed` on a slider, arc or checkable), else `arg`
		 * as a number */
		std::optional<float> value() const;

		/** `arg` as an integer, if it is one */
		std::optional<int32_t> argInt() const;

		/** The text a text area holds (an edit's result, e.g. on `value_changed`), else `arg` */
		std::string_view text() const;

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
		/** Handle actions called `name` with `handler`, replacing any earlier handler */
		void add(std::string name, ActionHandler handler) { m_handlers[std::move(name)] = std::move(handler); }

		/** @return whether there is a handler for `name` */
		bool has(std::string_view name) const { return m_handlers.find(name) != m_handlers.end(); }

		/** Run the handler for `args.name`. @return whether there was one */
		bool dispatch(const ActionArgs& args) const;

	  private:
		std::map<std::string, ActionHandler, std::less<>> m_handlers; // Handlers by action name
	};

	namespace Actions
	{
		/** Register the `<action>` element and the `value_confirmed` event. Call after lv_xml_init(). */
		void init();

		/** Forget every handler and widget registration */
		void deinit();

		/** The event DuetScreen widgets send, with a `float*` parameter, when an edit is confirmed */
		lv_event_code_t valueConfirmedEvent();

		/** The event a `<ds_modal>` sends when it is closed other than by its subject (tapped outside, back, …) */
		lv_event_code_t dismissedEvent();

		/** Actions from `obj` and the widgets inside it try `handlers` before outer ones. Until `obj` is deleted. */
		void setHandlers(lv_obj_t* obj, const ActionHandlerSet* handlers);

		/** `obj` is the root widget of `row` in a `<ds_list>`. Until `obj` is deleted. */
		void setRow(lv_obj_t* obj, Row* row);

		/**
		 * Actions (and rows) from `obj` and the widgets inside it are looked for outwards from `owner` instead of from
		 * `obj`'s parent: for widgets written in one view but shown elsewhere, such as a dialog moved to the modal host.
		 * `owner` must outlive `obj`. Until `obj` is deleted.
		 */
		void setOwner(lv_obj_t* obj, lv_obj_t* owner);

		/** Handle `name` everywhere it isn't handled by a presenter */
		void addGlobal(std::string name, ActionHandler handler);

		/**
		 * Resolve and run an action as if an `<action>` on `from` had fired (placeholders in `arg` are not filled in)
		 * @return whether a handler ran
		 */
		bool dispatch(lv_obj_t* from, std::string_view name, std::string_view arg, lv_event_t* event = nullptr);
	} // namespace Actions
} // namespace UI::Xml
