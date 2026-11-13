/*
 * DsModal.h
 *
 * <ds_modal>: a dialog over the main window (Navigation's modal host, see getModalHost()) with a dimmed backdrop. Its
 * attributes are in registerXml().
 *
 * Unless in_place, the dialog lives in the modal host but belongs to where it is written: it is deleted with the
 * widget it was written in (e.g. when that view is rebuilt), or with the modal host, whichever goes first.
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
		 * Create a closed dialog belonging to `owner`
		 * @param owner The widget the dialog is written in; deleting it deletes the dialog
		 * @param inPlace Stay in `owner`; otherwise move to the modal host, if there is one
		 */
		DsModal(const std::string& name, lv_obj_t* owner, bool inPlace = false);

		/** Stop belonging to the owner, and leave navigation's list of modals */
		~DsModal() override;

		/** Register `<ds_modal>` with the XML engine */
		static void registerXml();

	  protected:
		/** Close the backdrop; raise `dismissed` unless the subject closed the dialog */
		void onHide() override;

	  private:
		/** Pass a temporary C++ wrapper for `owner` to Modal, which only uses it during construction */
		DsModal(const std::string& name, lv_obj_t* owner, bool inPlace, LvObjRef&& ownerRef);

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
