/*
 * NotificationsView.cpp
 */

#include "NotificationsView.h"
#include "Debug.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"

namespace UI
{
	NotificationsView::NotificationsView(const std::string& name, LvObj& parent)
		: View(name, parent)
	{
		ZoneScoped;
		// An invisible layer over the whole screen: messages position themselves against it as they would against
		// the screen, and touches go through it to the screen below
		setSize(LV_PCT(100), LV_PCT(100));
		setFlag(LV_OBJ_FLAG_FLOATING, true);
		setFlag(LV_OBJ_FLAG_CLICKABLE, false);
		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
		addStyle(Themes::getLvglStyles().pad_zero);

		placeAlert(false);

		m_updatePrompt.setTitle(_("message.update_available"));
		m_updatePrompt.setText(_("message.update_available_text"));
		m_updatePrompt.setOkBtnText(_("message.update_confirm"));
		m_updatePrompt.setCancelBtnText(_("message.update_cancel"));
		m_updatePrompt.setOkCallback([this]() { m_presenter->update(); });
		m_updatePrompt.okVisible(true);
		m_updatePrompt.cancelVisible(true);
		m_updatePrompt.setMaxHeight(LV_PCT(70));
	}

	void NotificationsView::clear()
	{
		ZoneScoped;
		clearMessageBoxes();
	}

	std::shared_ptr<MessageBox>& NotificationsView::createMessageBox()
	{
		ZoneScoped;
		UI_LOCK();
		m_messageBoxList.emplace_back(
			std::make_shared<MessageBox>("home_message_box", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT)));
		auto& msgBox = m_messageBoxList.back();
		msgBox->setFlag(LV_OBJ_FLAG_FLOATING, true);
		msgBox->setAlign(LV_ALIGN_TOP_MID, 0, 2);
		msgBox->setMaxHeight(LV_PCT(70));
		msgBox->hide();
		return msgBox;
	}

	std::shared_ptr<MessageBox> NotificationsView::getMessageBox(size_t index) const
	{
		ZoneScoped;
		UI_LOCK();
		if (index >= getMessageBoxCount())
		{
			return nullptr;
		}
		auto it = m_messageBoxList.cbegin();
		std::advance(it, index);
		return *it;
	}

	void NotificationsView::popMessageBox()
	{
		ZoneScoped;
		UI_LOCK();
		if (!m_messageBoxList.empty())
		{
			m_messageBoxList.erase(m_messageBoxList.begin());
		}
	}

	void NotificationsView::placeAlert(bool keyboardShown)
	{
		m_alert.setAlign(LV_ALIGN_TOP_MID, 0, keyboardShown ? 5 : 0);
		m_alert.setMaxHeight(keyboardShown ? LV_PCT(45) : LV_PCT(70));
	}

	void NotificationsView::showKeyboard(bool show)
	{
		ZoneScoped;
		UI_LOCK();
		placeAlert(show);
		if (auto* keyboard = findView<LvKeyboard>("keyboard"))
		{
			keyboard->setVisible(show, true);
		}
	}
} // namespace UI
