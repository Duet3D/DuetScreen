/*
 * DsModal.h
 *
 * <ds_modal>: a dialog over the main window (Navigation's modal host, see getModalHost()) with a dimmed backdrop.
 *
 *     <ds_modal name="file_dialog" bind_open="dialog" width="70%" height="content">
 *         <action trigger="dismissed" name="files.close_dialog"/>
 *         …the dialog's content…
 *     </ds_modal>
 *
 * Its subject alone decides whether it is open, so a presenter opens and closes a dialog by setting a subject:
 *   bind_open   open while this subject is non-zero (integer) or non-empty (string), or, with open_value, equal to it
 *   open_value  the value bind_open must have for the dialog to be open
 *   blocking    tapping outside doesn't close it (default false)
 *
 * Closed any other way (tapped outside, the back button, navigation closing every modal), it raises `dismissed`, so
 * the presenter can update the subject; closing it by its subject raises nothing. The content is laid out in the
 * dialog's card, which takes every lv_obj attribute.
 *
 * The dialog lives in the modal host but belongs to where it is written: it is deleted with the widget it was
 * written in (e.g. when that view is rebuilt), or with the modal host, whichever goes first.
 */

#pragma once

#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/Modal/Modal.h"
#include "helix_xml.h"
#include <optional>
#include <string>

namespace UI::Xml
{
	class DsModal : public Modal<LvContainer>
	{
	  public:
		/**
		 * Create a closed dialog belonging to `owner`, in the modal host if there is one, else in `owner`
		 * @param owner The widget the dialog is written in; deleting it deletes the dialog
		 */
		DsModal(const std::string& name, lv_obj_t* owner);

		/** Stop belonging to the owner, and leave navigation's list of modals */
		~DsModal() override;

		/** Register `<ds_modal>` with the XML engine */
		static void registerXml();

	  protected:
		/** Close the backdrop; raise `dismissed` unless the subject closed the dialog */
		void onHide() override;

	  private:
		/** Pass a temporary C++ wrapper for `owner` to Modal, which only uses it during construction */
		DsModal(const std::string& name, lv_obj_t* owner, LvObjRef&& ownerRef);

		/** `<ds_modal>` create callback: create the dialog */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_modal>` apply callback: lv_obj attributes on the card, then the dialog's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_open` observer */
		static void openObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** The owner is being deleted: delete the dialog */
		static void ownerDeletedCb(lv_event_t* e);

		/** The card is being deleted with the modal host: delete the C++ dialog */
		static void cardDeletedCb(lv_event_t* e);

		/** Open or close to match the subject */
		void follow(lv_subject_t* subject);

		lv_obj_t* m_owner;						// The widget the dialog was written in
		std::optional<std::string> m_openValue; // open_value, if given
		bool m_closingBySubject = false;		// True while the subject is closing the dialog
	};
} // namespace UI::Xml
