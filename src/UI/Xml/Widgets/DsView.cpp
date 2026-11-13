/*
 * DsView.cpp
 */

#include "DsView.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/NativeViews.h"
#include "UI/Xml/View/XmlView.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml::DsView
{
	using namespace WidgetSupport;

	namespace
	{
		/** The widget `view` created directly inside `parent` (a modal, for example, wraps its root in a backdrop) */
		lv_obj_t* outermost(LvObj& view, lv_obj_t* parent)
		{
			lv_obj_t* obj = view.getRootPtr();
			while (obj != nullptr && lv_obj_get_parent(obj) != parent)
			{
				obj = lv_obj_get_parent(obj);
			}
			return obj != nullptr ? obj : view.getRootPtr();
		}

		/** Whether `obj` or anything it is inside is hidden */
		bool insideHidden(lv_obj_t* obj)
		{
			for (; obj != nullptr; obj = lv_obj_get_parent(obj))
			{
				if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN))
				{
					return true;
				}
			}
			return false;
		}

		/** The ds_view widget is being deleted: forget the view and delete the C++ object */
		void deletedCb(lv_event_t* e)
		{
			auto* view = static_cast<LvObj*>(lv_event_get_user_data(e));
			forgetView(view);
			delete view;
		}

		/** The presenter `presenter=` names, or nullptr to use the component's own */
		std::shared_ptr<XmlPresenter> presenterFor(const char** attrs)
		{
			const char* name = lv_xml_get_value_of(attrs, "presenter");
			if (name == nullptr || name[0] == '\0')
			{
				return nullptr;
			}
			auto presenter = Presenters::create(name);
			if (!presenter)
			{
				LOG_ERROR("<ds_view presenter=\"{:s}\">: no such presenter; using the component's own", name);
			}
			return presenter;
		}

		/** Create the view: an XML component if there is one by that name, else a C++ view */
		void* create(lv_xml_parser_state_t* state, const char** attrs)
		{
			const char* view = lv_xml_get_value_of(attrs, "view");
			if (view == nullptr || view[0] == '\0')
			{
				LOG_WARN("<ds_view> needs a view=");
				return lv_obj_create(parentOf(state));
			}
			const std::string name = attrOr(attrs, "name", view);

			lv_obj_t* parent = parentOf(state);
			LvObj* object = nullptr;
			if (findComponent(view) != nullptr || !NativeViews::exists(view))
			{
				// A missing component still gets an XmlView, which shows what is missing
				object = new XmlView(name, parent, view, presenterFor(attrs));
			}
			else
			{
				object = NativeViews::create(view, name, parent);
			}

			lv_obj_t* root = outermost(*object, parent);
			lv_obj_add_event_cb(root, deletedCb, LV_EVENT_DELETE, object);
			registerView(name, object);
			return root;
		}

		/** lv_obj attributes, then the starting state */
		void apply(lv_xml_parser_state_t* state, const char** attrs)
		{
			lv_xml_obj_apply(state, attrs);

			const char* view = lv_xml_get_value_of(attrs, "view");
			if (view == nullptr)
			{
				return;
			}
			LvObj* object = findView(attrOr(attrs, "name", view));
			if (object == nullptr)
			{
				return;
			}

			if (lv_xml_to_bool(attrOr(attrs, "home", "false")))
			{
				addHomeScreen(object);
				object->show();
			}
			else if (lv_xml_to_bool(attrOr(attrs, "shown", "false")))
			{
				// Shown with whatever it is in: inside something hidden (a closed page, another tab) it comes alive
				// when that is shown, not before
				if (insideHidden(parentOf(state)))
				{
					object->setFlag(LV_OBJ_FLAG_HIDDEN, false);
				}
				else
				{
					object->show();
				}
			}
			else
			{
				object->hide();
			}
		}
	} // namespace

	void registerXml()
	{
		Widgets::add(
			{.name = "ds_view",
			 .extends = "lv_obj",
			 .description =
				 "A view inside a screen, e.g. a page of the home screen, registered with navigation as its `name` "
				 "(default: its `view`) so it can be opened with `nav.open`. A view starts hidden, like any screen "
				 "that isn't open. Every lv_obj attribute works except `hidden`, which navigation owns.",
			 .example = R"(<ds_view name="dashboard_jobs" view="file_browser" presenter="dashboard_jobs"/>)",
			 .attributes =
				 {{"view",
				   "The XML component shown (with its presenter), or if there is no such component, a C++ view"},
				  {"presenter",
				   "Show the component with this registered presenter instead of the one it names, so one component "
				   "can show different data in different places"},
				  {"home", "\"true\" to make it a home screen: shown when nothing else is open (default false)"},
				  {"shown",
				   "\"true\" to show it without taking part in navigation, for always-present parts such as a status "
				   "bar or the page in a tab: shown, and its presenter active, whenever what it is in is shown "
				   "(default false)"}}},
			create,
			apply);
	}
} // namespace UI::Xml::DsView
