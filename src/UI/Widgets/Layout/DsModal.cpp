/*
 * DsModal.cpp
 */

#include "DsModal.h"
#include "UI/Core/Navigation.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/View/Actions.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsModal::DsModal(const std::string& name, lv_obj_t* owner, bool inPlace)
		: DsModal(name, owner, inPlace, LvObjRef(owner))
	{
	}

	DsModal::DsModal(const std::string& name, lv_obj_t* owner, bool inPlace, LvObjRef&& ownerRef)
		: Modal<LvContainer>(name, ownerRef)
		, m_owner(owner)
	{
		LvObj* host = inPlace ? nullptr : getModalHost();
		if (host != nullptr)
		{
			setParent(*host);
		}
		// Centred by the theme's `modal` style rather than Modal's local property, which would outrank every style, so
		// a view's own styles can place it, e.g. <bind_style> moving it out of the keyboard's way
		lv_obj_remove_local_style_prop(getRootPtr(), LV_STYLE_ALIGN, LV_PART_MAIN);

		// Actions raised in the dialog go to the view it was written in, not the modal host's
		Actions::setOwner(getRootPtr(), owner);
		lv_obj_add_event_cb(owner, ownerDeletedCb, LV_EVENT_DELETE, this);
		lv_obj_add_event_cb(getRootPtr(), cardDeletedCb, LV_EVENT_DELETE, this);
	}

	DsModal::~DsModal()
	{
		forgetView(this);
		if (getRootPtr() != nullptr)
		{
			lv_obj_remove_event_cb_with_user_data(getRootPtr(), cardDeletedCb, this);
		}
		lv_obj_remove_event_cb_with_user_data(m_owner, ownerDeletedCb, this);
	}

	void DsModal::registerXml()
	{
		Widgets::add(
			{.name = "ds_modal",
			 .extends = "lv_obj",
			 .description = "A dialog over the main window with a dimmed backdrop. Its subject alone decides whether "
							"it is open, so a presenter opens and closes it by setting a subject. Closed any other way "
							"(tapped outside, the back button, navigation closing every modal), it raises `dismissed` "
							"so the presenter can update the subject; closing it by its subject raises nothing. The "
							"content is laid out in the dialog's card, which the lv_obj attributes apply to. Unless "
							"`in_place`, it is deleted with the widget it was written in.",
			 .example = R"(<ds_modal name="file_dialog" bind_open="dialog" width="70%" height="content">
	<action trigger="dismissed" name="files.close_dialog"/>
	…the dialog's content…
</ds_modal>)",
			 .attributes =
				 {{"bind_open",
				   "Open while this subject is non-zero (int) or non-empty (text), or, with `open_value`, equal to it",
				   AttributeKind::Subject},
				  {"open_value", "The value `bind_open` must have for the dialog to be open"},
				  {"blocking", "\"true\" so that tapping outside doesn't close it (default false)"},
				  {"in_place",
				   "\"true\" to stay in the widget it is written in rather than move to the main window, so the "
				   "backdrop covers only that widget, e.g. alerts covering the whole screen (default false)"}}},
			xmlCreate,
			xmlApply);
	}

	void DsModal::onHide()
	{
		Modal<LvContainer>::onHide();
		if (!m_closingBySubject)
		{
			lv_obj_send_event(getRootPtr(), Actions::dismissedEvent(), nullptr);
		}
	}

	void* DsModal::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* modal = new DsModal(
			attrOr(attrs, "name", "modal"), parentOf(state), lv_xml_to_bool(attrOr(attrs, "in_place", "false")));
		return modal->getRootPtr();
	}

	void DsModal::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		auto& modal = static_cast<DsModal&>(*LvObj::fromPtr(itemOf(state)));
		modal.setBlocking(lv_xml_to_bool(attrOr(attrs, "blocking", "false")));
		if (const char* value = lv_xml_get_value_of(attrs, "open_value"))
		{
			modal.m_openValue = value;
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_open"))
		{
			lv_subject_add_observer_obj(subject, openObserverCb, modal.getRootPtr(), &modal);
		}
	}

	void DsModal::openObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsModal*>(lv_observer_get_user_data(observer))->follow(subject);
	}

	void DsModal::follow(lv_subject_t* subject)
	{
		bool open = false;
		switch (subject->type)
		{
		case LV_SUBJECT_TYPE_INT:
			open = m_openValue ? std::to_string(lv_subject_get_int(subject)) == *m_openValue
							   : lv_subject_get_int(subject) != 0;
			break;
		case LV_SUBJECT_TYPE_STRING:
		{
			const std::string_view text = lv_subject_get_string(subject);
			open = m_openValue ? text == *m_openValue : !text.empty();
			break;
		}
		default:
			LOG_WARN("<ds_modal bind_open>: the subject must be an integer or a string");
			return;
		}

		if (open)
		{
			this->open();
		}
		else if (isVisible())
		{
			m_closingBySubject = true;
			close();
			m_closingBySubject = false;
		}
	}

	void DsModal::ownerDeletedCb(lv_event_t* e)
	{
		delete static_cast<DsModal*>(lv_event_get_user_data(e));
	}

	void DsModal::cardDeletedCb(lv_event_t* e)
	{
		delete static_cast<DsModal*>(lv_event_get_user_data(e));
	}
} // namespace UI::Xml
