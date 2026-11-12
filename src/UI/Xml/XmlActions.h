/*
 * XmlActions.h
 *
 * Actions are how an XML view asks for something to happen. A view never calls code directly; it names an action:
 *
 *     <ds_button translation_tag="fan.max">
 *         <action trigger="clicked" name="fans.set_speed" arg="100"/>
 *     </ds_button>
 *
 * When the trigger fires, the action is resolved from the widget outwards, like a subject name:
 *   1. the presenter of the nearest enclosing XmlView, then the next one out, ...
 *   2. the global actions (e.g. `gcode`)
 * If the widget is inside a `<model_list>` item, the handler also receives that item's index.
 *
 * Triggers are LVGL's event names (`clicked`, `value_changed`, ...) plus `value_confirmed`, which DuetScreen widgets
 * raise once an edit is finished (e.g. a slider is released or a value is typed in).
 */

#pragma once

#include "lvgl/lvgl.h"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace UI::Xml
{
	struct ActionArgs
	{
		std::string_view name;
		/** The `arg` attribute, or empty */
		std::string_view arg;
		/** Index of the `<model_list>` item the event came from, if any */
		std::optional<size_t> item;
		lv_event_t* event = nullptr;
		lv_obj_t* target = nullptr;

		/** The value carried by a `value_confirmed`/`value_changed` event, else `arg` parsed as a number */
		std::optional<float> value() const;
		std::optional<int32_t> argInt() const;
	};

	using ActionHandler = std::function<void(const ActionArgs&)>;

	class ActionHandlerSet
	{
	  public:
		void add(std::string name, ActionHandler handler) { m_handlers[std::move(name)] = std::move(handler); }
		bool has(std::string_view name) const { return m_handlers.find(name) != m_handlers.end(); }

		/** @return true if a handler for `args.name` ran */
		bool dispatch(const ActionArgs& args) const;

	  private:
		std::map<std::string, ActionHandler, std::less<>> m_handlers;
	};

	namespace Actions
	{
		/** Register the `<action>` element and the `value_confirmed` event. Call after lv_xml_init(). */
		void init();
		void deinit();

		/** Event code DuetScreen widgets send, with a `float*` param, when an edit is confirmed */
		lv_event_code_t valueConfirmedEvent();

		/**
		 * Mark `obj` as the root of an action scope. Events from `obj` and its descendants try `handlers` (if any) and
		 * report `item` (if any). The scope is removed automatically when `obj` is deleted.
		 */
		void registerScope(lv_obj_t* obj, const ActionHandlerSet* handlers, std::optional<size_t> item = std::nullopt);
		void unregisterScope(lv_obj_t* obj);

		void registerGlobal(std::string name, ActionHandler handler);

		/**
		 * Resolve and run an action as if `<action name=… arg=…>` on `from` had fired.
		 * @return true if a handler ran
		 */
		bool dispatch(lv_obj_t* from, std::string_view name, std::string_view arg, lv_event_t* event = nullptr);
	} // namespace Actions
} // namespace UI::Xml
