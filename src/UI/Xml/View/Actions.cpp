/*
 * Actions.cpp
 */

#include "Actions.h"
#include "Debug.h"
#include "helix_xml.h"
#include "src/xml/lv_xml_base_types.h"
#include "src/xml/lv_xml_parser.h"
#include "src/xml/lv_xml_utils.h"
#include <charconv>
#include <fmt/format.h>
#include <unordered_map>

namespace UI::Xml
{
	namespace
	{
		/** What Actions knows about a widget: the handlers of the view it is the root of, or the row it shows */
		struct Target
		{
			const ActionHandlerSet* handlers = nullptr; // Presenter actions, if the widget is a view root
			Row* row = nullptr;							// The row, if the widget is a list row's root
			lv_obj_t* owner = nullptr;					// Where to look outwards from instead of the parent, if set
		};

		/** One `<action>` element, attached to its widget */
		struct Binding
		{
			std::string name;			 // Action name
			std::string arg;			 // `arg` as written, placeholders unfilled
			lv_xml_subject_ctx_t* scope; // Where the element was written, for placeholders; holds a reference
		};

		std::unordered_map<lv_obj_t*, Target> s_targets;  // Widgets that are view or row roots
		ActionHandlerSet s_globals;						  // Actions no presenter handles
		lv_event_code_t s_valueConfirmed = LV_EVENT_LAST; // Registered at init()
		lv_event_code_t s_dismissed = LV_EVENT_LAST;	  // Registered at init()

		/** Forget a deleted widget */
		void targetDeletedCb(lv_event_t* e)
		{
			s_targets.erase(static_cast<lv_obj_t*>(lv_event_get_current_target(e)));
		}

		/** Get the entry for `obj`, watching for its deletion the first time */
		Target& targetFor(lv_obj_t* obj)
		{
			auto [it, inserted] = s_targets.try_emplace(obj);
			if (inserted)
			{
				lv_obj_add_event_cb(obj, targetDeletedCb, LV_EVENT_DELETE, nullptr);
			}
			return it->second;
		}

		/** Free a Binding when its widget is deleted */
		void bindingDeletedCb(lv_event_t* e)
		{
			auto* binding = static_cast<Binding*>(lv_event_get_user_data(e));
			lv_xml_subject_ctx_unref(binding->scope);
			delete binding;
		}

		/** The value of the subject called `name` as seen from `scope`, as text; nullopt if there is no such subject */
		std::optional<std::string> subjectText(lv_xml_subject_ctx_t* scope, const std::string& name)
		{
			lv_xml_subject_ctx_t* previous = lv_xml_subject_ctx_activate(scope);
			lv_subject_t* subject = lv_xml_find_subject(nullptr, name.c_str());
			lv_xml_subject_ctx_restore(previous);
			if (subject == nullptr)
			{
				return std::nullopt;
			}

			switch (subject->type)
			{
			case LV_SUBJECT_TYPE_INT:
				return std::to_string(lv_subject_get_int(subject));
			case LV_SUBJECT_TYPE_STRING:
				return std::string(lv_subject_get_string(subject));
			default:
				return std::nullopt;
			}
		}

		/** Replace each `{name}` in `text`: `{value}` by `value` (if any), others by the subject of that name */
		std::string fillPlaceholders(std::string_view text, lv_xml_subject_ctx_t* scope, std::optional<float> value)
		{
			std::string out;
			size_t pos = 0;
			while (pos < text.size())
			{
				const size_t open = text.find('{', pos);
				const size_t close = open == std::string_view::npos ? open : text.find('}', open);
				if (close == std::string_view::npos)
				{
					out += text.substr(pos);
					break;
				}

				out += text.substr(pos, open - pos);
				const std::string name(text.substr(open + 1, close - open - 1));
				std::optional<std::string> replacement;
				if (name == "value")
				{
					replacement = value ? std::optional(fmt::format("{:g}", *value)) : std::nullopt;
				}
				else
				{
					replacement = subjectText(scope, name);
				}

				if (replacement)
				{
					out += *replacement;
				}
				else
				{
					LOG_WARN("Action argument '{:s}': nothing to put in {{{:s}}}", text, name);
					out += text.substr(open, close - open + 1);
				}
				pos = close + 1;
			}
			return out;
		}

		/** An `<action>`'s trigger fired */
		void actionEventCb(lv_event_t* e)
		{
			const auto* binding = static_cast<const Binding*>(lv_event_get_user_data(e));
			auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));

			// The edited value, if any, is needed to fill {value}
			ActionArgs probe{.name = binding->name, .arg = "", .event = e, .target = target};
			const std::string arg = fillPlaceholders(binding->arg, binding->scope, probe.value());
			Actions::dispatch(target, binding->name, arg, e);
		}

		/** The event code for a `trigger` attribute; LV_EVENT_LAST if unknown */
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
			if (lv_streq(trigger, "dismissed"))
			{
				return s_dismissed;
			}
			return lv_xml_trigger_text_to_enum_value(trigger);
		}

		/** `<action>` creates nothing: it decorates its parent widget */
		void* actionCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
		{
			return lv_xml_state_get_parent(state);
		}

		/** Attach the action to its parent widget */
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
			auto* binding = new Binding{name, arg ? arg : "", lv_xml_subject_ctx_ref(lv_xml_subject_ctx_get_active())};

			auto* obj = static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));
			lv_obj_add_event_cb(obj, actionEventCb, code, binding);
			lv_obj_add_event_cb(obj, bindingDeletedCb, LV_EVENT_DELETE, binding);
		}

		/** `text` as a float, if all of it is one */
		std::optional<float> parseFloat(std::string_view text)
		{
			float value = 0;
			const auto* end = text.data() + text.size();
			auto [ptr, ec] = std::from_chars(text.data(), end, value);
			if (text.empty() || ec != std::errc() || ptr != end)
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

	Row* ActionArgs::row() const
	{
		for (lv_obj_t* obj = target; obj != nullptr; obj = outerOf(obj))
		{
			if (Row* r = rowOf(obj))
			{
				return r;
			}
		}
		return nullptr;
	}

	Row* ActionArgs::rowOf(lv_obj_t* obj)
	{
		auto it = s_targets.find(obj);
		return it == s_targets.end() ? nullptr : it->second.row;
	}

	lv_obj_t* ActionArgs::outerOf(lv_obj_t* obj)
	{
		auto it = s_targets.find(obj);
		return it != s_targets.end() && it->second.owner != nullptr ? it->second.owner : lv_obj_get_parent(obj);
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
			s_dismissed = static_cast<lv_event_code_t>(lv_event_register_id());
			lv_xml_register_widget("lv_obj-action", actionCreate, actionApply);
		}

		void deinit()
		{
			s_targets.clear();
			s_globals = ActionHandlerSet{};
			s_valueConfirmed = LV_EVENT_LAST;
			s_dismissed = LV_EVENT_LAST;
		}

		lv_event_code_t valueConfirmedEvent()
		{
			return s_valueConfirmed;
		}

		lv_event_code_t dismissedEvent()
		{
			return s_dismissed;
		}

		void setHandlers(lv_obj_t* obj, const ActionHandlerSet* handlers)
		{
			if (obj != nullptr)
			{
				targetFor(obj).handlers = handlers;
			}
		}

		void setRow(lv_obj_t* obj, Row* row)
		{
			if (obj != nullptr)
			{
				targetFor(obj).row = row;
			}
		}

		void setOwner(lv_obj_t* obj, lv_obj_t* owner)
		{
			if (obj != nullptr)
			{
				targetFor(obj).owner = owner;
			}
		}

		void addGlobal(std::string name, ActionHandler handler)
		{
			s_globals.add(std::move(name), std::move(handler));
		}

		bool dispatch(lv_obj_t* from, std::string_view name, std::string_view arg, lv_event_t* event)
		{
			ZoneScoped;
			const ActionArgs args{.name = name, .arg = std::string(arg), .event = event, .target = from};

			// The innermost presenter that handles it
			for (lv_obj_t* obj = from; obj != nullptr; obj = ActionArgs::outerOf(obj))
			{
				auto it = s_targets.find(obj);
				if (it != s_targets.end() && it->second.handlers != nullptr && it->second.handlers->dispatch(args))
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
