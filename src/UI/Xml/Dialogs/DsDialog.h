/*
 * DsDialog.h
 *
 * <ds_dialog>: a dialog written in XML where it belongs, opened while its subject says so. Its content stays where it
 * is written, hidden, until it opens: then the dialog surface (DialogHost.h) shows it in its card, as the dialog or as
 * the next step of the one open. Actions raised in it still go to the view it is written in. Its attributes are in
 * registerXml(). When the user closes it, it empties its subject (0, or empty text), so the code that opens it need
 * not close it, then raises `dismissed`.
 *
 *     <ds_dialog name="file_dialog" bind_open="presenter.dialog" bind_title="presenter.dialog_title">
 *         …the content…
 *     </ds_dialog>
 *
 * It is deleted with the widget it is written in.
 */

#pragma once

#include "DialogStep.h"
#include "helix_xml.h"
#include <optional>
#include <string>

namespace UI::Xml
{
	class DialogHost;

	class DsDialog : public DialogStep
	{
	  public:
		/** Take the dialog off the surface */
		~DsDialog() override;

		DsDialog(const DsDialog&) = delete;
		DsDialog& operator=(const DsDialog&) = delete;

		/** Register `<ds_dialog>` with the XML engine */
		static void registerXml();

		/** The widget holding what the dialog shows */
		lv_obj_t* content() override { return m_content; }

		/** The widget the dialog is written in */
		lv_obj_t* home() override { return m_home; }

		/** The title subject, or nullptr */
		lv_subject_t* title() override { return m_title; }

		/** The card's size, from `width` and `height` */
		Size size() const override { return m_size; }

		/** From `blocking` */
		bool blocking() const override { return m_blocking; }

		/** From `alert` */
		bool alert() const override { return m_alert; }

		/** Empty the subject, then raise `dismissed` */
		void dismissed() override;

	  private:
		/** A dialog written in `home`, showing `content` */
		DsDialog(lv_obj_t* home, lv_obj_t* content);

		/** `<ds_dialog>` create callback: the content's container, hidden where it is written */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_dialog>` apply callback: lv_obj attributes on the content (but its size), then the dialog's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_open` observer */
		static void openObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Open, if still meant to be, once the views being built are */
		static void openLaterCb(void* dialog);

		/** The widget it is written in is going: take the dialog off the surface, so its content goes too */
		static void homeDeletedCb(lv_event_t* e);

		/** The content has gone: delete the dialog */
		static void contentDeletedCb(lv_event_t* e);

		/** Whether `subject` says the dialog is open */
		bool wantsOpen(lv_subject_t* subject) const;

		/** Open or close to match the subject */
		void follow(lv_subject_t* subject);

		lv_obj_t* m_home;						// Where it is written
		lv_obj_t* m_content;					// What it shows
		lv_subject_t* m_title = nullptr;		// Its title, or nullptr
		lv_subject_t* m_open = nullptr;			// `bind_open`'s subject, or nullptr
		Size m_size;							// The card's size
		bool m_blocking = false;				// `blocking`
		bool m_alert = false;					// `alert`
		std::optional<std::string> m_openValue; // `open_value`, if given
	};
} // namespace UI::Xml
