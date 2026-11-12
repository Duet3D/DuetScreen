/*
 * XmlActions.cpp
 */

#include "XmlActions.h"
#include "Debug.h"
#include "helix_xml.h"
#include "src/xml/lv_xml_base_types.h"
#include "src/xml/lv_xml_parser.h"
#include "src/xml/lv_xml_utils.h"
#include <charconv>
#include <unordered_map>

namespace UI::Xml
{
	namespace
	{
		struct Scope
		{
			const ActionHandlerSet* handlers;
			std::optional<size_t> item;
		};

		struct Binding
		{
			std::string name;
			std::string arg;
		};

		std::unordered_map<lv_obj_t*, Scope> s_scopes;
		ActionHandlerSet s_globals;
		lv_event_code_t s_valueConfirmed = LV_EVENT_LAST;

		void scopeDeletedCb(lv_event_t* e)
		{
			s_scopes.erase(static_cast<lv_obj_t*>(lv_event_get_current_target(e)));
		}

		void bindingDeletedCb(lv_event_t* e)
		{
			delete static_cast<Binding*>(lv_event_get_user_data(e));
		}

		void actionEventCb(lv_event_t* e)
		{
			const auto* binding = static_cast<const Binding*>(lv_event_get_user_data(e));
			Actions::dispatch(static_cast<lv_obj_t*>(lv_event_get_current_target(e)), binding->name, binding->arg, e);
		}

		lv_event_code_t parseTrigger(const char* trigger)
		{
			if (trigger == nullptr)
			{
				return LV_EVENT_CLICKED;
			}
			if (lv_streq(trigger, "value_confirmed"))
			{
				return s_valueConfirmed;
			}
			return lv_xml_trigger_text_to_enum_value(trigger);
		}

		/* <action name="..." trigger="clicked" arg="..."/> as a child of any widget */
		void* actionCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
		{
			return lv_xml_state_get_parent(state);
		}

		void actionApply(lv_xml_parser_state_t* state, const char** attrs)
		{
			const char* name = lv_xml_get_value_of(attrs, "name");
			if (name == nullptr || name[0] == '\0')
			{
				LOG_WARN("<action> without a name is ignored");
				return;
			}

			const char* trigger = lv_xml_get_value_of(attrs, "trigger");
			const lv_event_code_t code = parseTrigger(trigger);
			if (code == LV_EVENT_LAST)
			{
				LOG_WARN("<action name=\"{:s}\"> has an unknown trigger \"{:s}\"", name, trigger);
				return;
			}

			const char* arg = lv_xml_get_value_of(attrs, "arg");
			auto* binding = new Binding{name, arg ? arg : ""};

			auto* obj = static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));
			lv_obj_add_event_cb(obj, actionEventCb, code, binding);
			lv_obj_add_event_cb(obj, bindingDeletedCb, LV_EVENT_DELETE, binding);
		}

		std::optional<float> parseFloat(std::string_view text)
		{
			float value = 0;
			const auto* end = text.data() + text.size();
			auto [ptr, ec] = std::from_chars(text.data(), end, value);
			if (ec != std::errc() || ptr != end)
			{
				return std::nullopt;
			}
			return value;
		}
	} // namespace

	std::optional<float> ActionArgs::value() const
	{
		if (event != nullptr)
		{
			const lv_event_code_t code = lv_event_get_code(event);
			if (code == s_valueConfirmed && lv_event_get_param(event) != nullptr)
			{
				return *static_cast<const float*>(lv_event_get_param(event));
			}

			if (code == LV_EVENT_VALUE_CHANGED && target != nullptr)
			{
				if (lv_obj_check_type(target, &lv_slider_class))
				{
					return static_cast<float>(lv_slider_get_value(target));
				}
				if (lv_obj_check_type(target, &lv_arc_class))
				{
					return static_cast<float>(lv_arc_get_value(target));
				}
				if (lv_obj_has_flag(target, LV_OBJ_FLAG_CHECKABLE))
				{
					return lv_obj_has_state(target, LV_STATE_CHECKED) ? 1.0f : 0.0f;
				}
			}
		}

		return parseFloat(arg);
	}

	std::optional<int32_t> ActionArgs::argInt() const
	{
		int32_t value = 0;
		const auto* end = arg.data() + arg.size();
		auto [ptr, ec] = std::from_chars(arg.data(), end, value);
		if (arg.empty() || ec != std::errc() || ptr != end)
		{
			return std::nullopt;
		}
		return value;
	}

	bool ActionHandlerSet::dispatch(const ActionArgs& args) const
	{
		auto it = m_handlers.find(args.name);
		if (it == m_handlers.end())
		{
			return false;
		}
		it->second(args);
		return true;
	}

	namespace Actions
	{
		void init()
		{
			s_valueConfirmed = static_cast<lv_event_code_t>(lv_event_register_id());
			lv_xml_register_widget("lv_obj-action", actionCreate, actionApply);
		}

		void deinit()
		{
			s_scopes.clear();
			s_globals = ActionHandlerSet{};
			s_valueConfirmed = LV_EVENT_LAST;
		}

		lv_event_code_t valueConfirmedEvent()
		{
			return s_valueConfirmed;
		}

		void registerScope(lv_obj_t* obj, const ActionHandlerSet* handlers, std::optional<size_t> item)
		{
			if (obj == nullptr)
			{
				return;
			}
			const bool existed = s_scopes.contains(obj);
			s_scopes[obj] = Scope{handlers, item};
			if (!existed)
			{
				lv_obj_add_event_cb(obj, scopeDeletedCb, LV_EVENT_DELETE, nullptr);
			}
		}

		void unregisterScope(lv_obj_t* obj)
		{
			if (s_scopes.erase(obj) > 0 && lv_is_initialized())
			{
				lv_obj_remove_event_cb(obj, scopeDeletedCb);
			}
		}

		void registerGlobal(std::string name, ActionHandler handler)
		{
			s_globals.add(std::move(name), std::move(handler));
		}

		bool dispatch(lv_obj_t* from, std::string_view name, std::string_view arg, lv_event_t* event)
		{
			ZoneScoped;
			ActionArgs args{.name = name, .arg = arg, .item = std::nullopt, .event = event, .target = from};

			// The innermost list item the event came from
			for (lv_obj_t* obj = from; obj != nullptr; obj = lv_obj_get_parent(obj))
			{
				auto it = s_scopes.find(obj);
				if (it != s_scopes.end() && it->second.item.has_value())
				{
					args.item = it->second.item;
					break;
				}
			}

			// The innermost presenter that handles it
			for (lv_obj_t* obj = from; obj != nullptr; obj = lv_obj_get_parent(obj))
			{
				auto it = s_scopes.find(obj);
				if (it != s_scopes.end() && it->second.handlers != nullptr && it->second.handlers->dispatch(args))
				{
					return true;
				}
			}

			if (s_globals.dispatch(args))
			{
				return true;
			}

			LOG_WARN("No handler for action '{:s}'", name);
			return false;
		}
	} // namespace Actions
} // namespace UI::Xml
