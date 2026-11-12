/*
 * NotificationsView.h
 *
 * Messages that appear over whatever screen is showing: responses from the Duet (a queue of message boxes that time
 * out), M291 alerts, and firmware update prompts. It is a transparent overlay covering the screen that only takes
 * touches where a message is showing; the behaviour is in NotificationsPresenter.
 *
 * Hosted in XML with `<ds_view name="notifications" view="notifications" shown="true"/>`.
 */

#pragma once

#include "NotificationsPresenter.h"
#include "UI/Components/MessageBox/AlertMessageBox.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Modal/Modal.h"
#include "UI/Core/View.h"
#include <list>
#include <memory>

namespace UI
{
	class NotificationsView : public View<NotificationsPresenter>
	{
	  public:
		friend class NotificationsPresenter;

		/** Create the overlay inside `parent`, covering it */
		NotificationsView(const std::string& name, LvObj& parent);

		/** Remove every queued response */
		void clear();

		/** Queue a new, hidden response message box */
		std::shared_ptr<MessageBox>& createMessageBox();

		/** Number of queued responses */
		size_t getMessageBoxCount() const { return m_messageBoxList.size(); }

		/** The `index`th queued response, or nullptr */
		std::shared_ptr<MessageBox> getMessageBox(size_t index) const;

		/** Remove the oldest response */
		void popMessageBox();

		/** Remove every response */
		void clearMessageBoxes() { m_messageBoxList.clear(); }

		/** The M291 alert */
		Modal<AlertMessageBox>& getAlert() { return m_alert; }

		/** The firmware update prompt */
		Modal<MessageBox>& getUpdatePrompt() { return m_updatePrompt; }

		/** Show or hide the on-screen keyboard, moving the alert out of its way */
		void showKeyboard(bool show);

	  private:
		/** Put the alert at the top, smaller if the keyboard needs the bottom half */
		void placeAlert(bool keyboardShown);

		std::list<std::shared_ptr<MessageBox>> m_messageBoxList; // Queued responses; the first is showing

		Modal<AlertMessageBox> m_alert{"alert", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT)}; // M291 alert
		Modal<MessageBox> m_updatePrompt{
			"update_prompt", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT)}; // Firmware update prompt
	};
} // namespace UI
