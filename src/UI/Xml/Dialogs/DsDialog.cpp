/*
 * DsDialog.cpp
 */

#include "DsDialog.h"
#include "Debug.h"
#include "DialogHost.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Binding/Constants.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/Actions.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <map>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** Every dialog, by its content */
		std::map<lv_obj_t*, DsDialog*>& dialogs()
		{
			static std::map<lv_obj_t*, DsDialog*> s_dialogs;
			return s_dialogs;
		}

		/** True while a dialog's subject is first followed, as its view is built */
		bool s_building = false;
	} // namespace

	DsDialog::DsDialog(lv_obj_t* home, lv_obj_t* content)
		: m_home(home)
		, m_content(content)
	{
		// Actions raised in it go to the view it is written in, wherever it is shown
		Actions::setOwner(m_content, m_home);
		dialogs()[m_content] = this;
		lv_obj_add_event_cb(m_home, homeDeletedCb, LV_EVENT_DELETE, this);
		lv_obj_add_event_cb(m_content, contentDeletedCb, LV_EVENT_DELETE, this);
	}

	DsDialog::~DsDialog()
	{
		lv_async_call_cancel(openLaterCb, this);
		dialogs().erase(m_content);
		lv_obj_remove_event_cb_with_user_data(m_home, homeDeletedCb, this);
		if (DialogHost* host = DialogHost::showing(*this))
		{
			host->forget(*this);
		}
	}

	void DsDialog::registerXml()
	{
		Widgets::add(
			{.name = "ds_dialog",
			 .extends = "lv_obj",
			 .description =
				 "A dialog, shown by the dialog surface (`<ds_dialog_host>`) while its subject says so, as the dialog "
				 "or "
				 "as the next step of the one open, which the card slides to. Its content stays where it is written, "
				 "hidden, so its actions go to that view. The surface draws its title, a close button (and a back "
				 "button on a later step); closed by them, by tapping outside or by the back key, it raises "
				 "`dismissed` so the view can update the subject. Closing it by its subject takes any steps opened "
				 "from it too. The lv_obj attributes apply to its content.",
			 .example =
				 R"(<ds_dialog name="file_dialog" bind_open="presenter.dialog" bind_title="presenter.dialog_title">
	<action trigger="dismissed" name="presenter.files.close_dialog"/>
	…the dialog's content…
</ds_dialog>)",
			 .attributes =
				 {{"bind_open",
				   "Open while this subject is non-zero (int) or non-empty (text), or, with `open_value`, equal to it",
				   AttributeKind::Subject},
				  {"open_value", "The value `bind_open` must have for the dialog to be open"},
				  {"title", "Its title, as written"},
				  {"title_tag", "Its title, a translation key", AttributeKind::Translation},
				  {"bind_title", "Its title, a subject's text", AttributeKind::Subject},
				  {"width",
				   "The card's width while it shows: a percentage of the room, or pixels (default 70%); its content "
				   "always fills the card"},
				  {"height",
				   "The card's height: `content` (default) to fit the content, scrolling once it reaches the room "
				   "there is, or a percentage or pixels for a card of a set height whose content fills it"},
				  {"blocking",
				   "\"true\" if it must be answered: no close or back button, and tapping outside or the back key do "
				   "nothing, for it and the steps under it (default false)"},
				  {"alert",
				   "\"true\" for an alert: shown before everything else, setting the user's dialog aside until it is "
				   "answered; alerts wait their turn (default false)"}}},
			xmlCreate,
			xmlApply);
	}

	void DsDialog::dismissed()
	{
		lv_obj_send_event(m_content, Actions::dismissedEvent(), nullptr);
	}

	void* DsDialog::xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
	{
		lv_obj_t* home = parentOf(state);
		lv_obj_t* content = lv_obj_create(home);
		lv_obj_add_flag(content, LV_OBJ_FLAG_HIDDEN);
		new DsDialog(home, content);
		return content;
	}

	void DsDialog::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		// The size is the card's: the content fills it
		std::vector<const char*> others;
		for (size_t i = 0; attrs != nullptr && attrs[i] != nullptr; i += 2)
		{
			const std::string_view name = attrs[i];
			if (name != "width" && name != "height")
			{
				others.push_back(attrs[i]);
				others.push_back(attrs[i + 1]);
			}
		}
		others.push_back(nullptr);
		others.push_back(nullptr);
		lv_xml_obj_apply(state, others.data());

		lv_obj_t* content = itemOf(state);
		const auto found = dialogs().find(content);
		if (found == dialogs().end())
		{
			return;
		}
		DsDialog* dialog = found->second;

		dialog->m_size.width = attrOr(attrs, "width", "70%");
		dialog->m_size.height = attrOr(attrs, "height", "content");
		dialog->m_blocking = lv_xml_to_bool(attrOr(attrs, "blocking", "false"));
		dialog->m_alert = lv_xml_to_bool(attrOr(attrs, "alert", "false"));
		if (const char* value = lv_xml_get_value_of(attrs, "open_value"))
		{
			dialog->m_openValue = value;
		}

		// The title: a subject's, a translation's (a constant following the language) or as written
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_title"))
		{
			dialog->m_title = subject;
		}
		else if (const char* tag = lv_xml_get_value_of(attrs, "title_tag"); tag != nullptr && tag[0] != '\0')
		{
			Subject* constant = Constants::find(tag);
			dialog->m_title = constant != nullptr ? constant->raw() : nullptr;
		}
		else if (const char* title = lv_xml_get_value_of(attrs, "title"); title != nullptr && title[0] != '\0')
		{
			dialog->m_title = Constants::find("'" + std::string(title) + "'")->raw();
		}

		if (lv_subject_t* subject = findSubject(state, attrs, "bind_open"))
		{
			dialog->m_open = subject;
			s_building = true;
			lv_subject_add_observer_obj(subject, openObserverCb, content, dialog);
			s_building = false;
		}
	}

	void DsDialog::openObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsDialog*>(lv_observer_get_user_data(observer))->follow(subject);
	}

	bool DsDialog::wantsOpen(lv_subject_t* subject) const
	{
		switch (subject->type)
		{
		case LV_SUBJECT_TYPE_INT:
			return m_openValue ? std::to_string(lv_subject_get_int(subject)) == *m_openValue
							   : lv_subject_get_int(subject) != 0;
		case LV_SUBJECT_TYPE_STRING:
		{
			const std::string_view text = lv_subject_get_string(subject);
			return m_openValue ? text == *m_openValue : !text.empty();
		}
		default:
			LOG_WARN("<ds_dialog bind_open>: the subject must be an integer or a string");
			return false;
		}
	}

	void DsDialog::follow(lv_subject_t* subject)
	{
		if (wantsOpen(subject))
		{
			// While its view is built the surface may not be yet (the home screen builds its pages first), so a
			// dialog open from the start opens once the views are
			if (s_building)
			{
				lv_async_call_cancel(openLaterCb, this);
				lv_async_call(openLaterCb, this);
			}
			else
			{
				DialogHost::of(m_home).open(*this);
			}
			return;
		}
		lv_async_call_cancel(openLaterCb, this);
		if (DialogHost* host = DialogHost::showing(*this))
		{
			host->close(*this);
		}
	}

	void DsDialog::openLaterCb(void* dialog)
	{
		auto* self = static_cast<DsDialog*>(dialog);
		if (self->m_open != nullptr && self->wantsOpen(self->m_open))
		{
			DialogHost::of(self->m_home).open(*self);
		}
	}

	void DsDialog::homeDeletedCb(lv_event_t* e)
	{
		auto* dialog = static_cast<DsDialog*>(lv_event_get_user_data(e));
		// Its content goes home, to be deleted with the rest of it
		if (DialogHost* host = DialogHost::showing(*dialog))
		{
			host->forget(*dialog);
		}
	}

	void DsDialog::contentDeletedCb(lv_event_t* e)
	{
		delete static_cast<DsDialog*>(lv_event_get_user_data(e));
	}
} // namespace UI::Xml
