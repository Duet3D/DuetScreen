/*
 * AlertMessageBox.cpp
 *
 *  Created on: 2025-08-12
 *      Author: Andy Everitt
 */

#include "AlertMessageBox.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "lv_i18n/lv_i18n.h"
#include "lvgl/src/lvgl_private.h"

namespace UI
{
	static const float s_jogAmounts[6] = {-2.0f, -0.2f, -0.02f, 0.02f, 0.2f, 2.0f};

	AlertMessageBox::AlertMessageBox(const std::string& name, LvObj& parent, layout_t layout)
		: MessageBox(name, parent, layout)
		, m_inputCont("input_cont", getBody())
		, m_axisJogList("axis_jog_list", getBody())
		, m_choicesList("choices_list", getBody())
		, m_warningText("warning_text", m_inputCont)
		, m_minText("min_text", m_inputCont)
		, m_maxText("max_text", m_inputCont)
		, m_input("input", m_inputCont)
		, m_kb(nullptr)
	{
		UI_LOCK();
		init();
	}

	void AlertMessageBox::init()
	{
		UI_LOCK();

		m_okBtn.addStyle(Themes::getLvglStyles().actionBtn);
		m_cancelBtn.addStyle(Themes::getLvglStyles().actionBtn);

		// Body
		m_axisJogList.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_choicesList.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_choicesList.setListFlow(LV_FLEX_FLOW_ROW);

		// Input Container
		m_inputCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		static const int32_t inputColDsc[] = {LV_GRID_FR(1), LV_GRID_FR(3), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		static const int32_t inputRowDsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		m_inputCont.setGridDsc(inputColDsc, inputRowDsc);
		m_inputCont.setGridCell(m_warningText, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
		m_inputCont.setGridCell(m_minText, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
		m_inputCont.setGridCell(m_input, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
		m_inputCont.setGridCell(m_maxText, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);

		m_warningText.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_minText.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_input.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_maxText.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);

		m_input.setOneLine(true);
		m_input.addStyle(Themes::getLvglStyles().pad_zero);

		// Callbacks
		m_input.addEventCallback(onInputEvent, LV_EVENT_ALL, this);

		clear();
		setMode(OM::Alert::Mode::None);
	}

	void AlertMessageBox::setInputValidationCallback(std::function<bool(std::string_view)> cb)
	{
		UI_LOCK();
		m_inputValidationCb = cb;
		m_okBtn.setDisabled(!validate());
	}

	bool AlertMessageBox::validate()
	{
		UI_LOCK();
		if (!m_inputValidationCb)
		{
			return true;
		}
		std::string_view text = m_input.getText();
		return m_inputValidationCb(text);
	}

	void AlertMessageBox::setKeyboard(LvKeyboard* kb)
	{
		UI_LOCK();
		LOG_INFO("Setting keyboard {:p}", static_cast<const void*>(kb));
		m_kb = kb;

		if (m_kb)
		{
			m_kb->setTextArea(&m_input);
		}
	}

	bool AlertMessageBox::isBlocking() const
	{
		switch (m_mode)
		{
		case OM::Alert::Mode::InfoConfirm:
		case OM::Alert::Mode::ConfirmCancel:
		case OM::Alert::Mode::Choices:
		case OM::Alert::Mode::NumberInt:
		case OM::Alert::Mode::NumberFloat:
		case OM::Alert::Mode::Text:
			// These alerts are blocking
			return true;
		default:
			break;
		}
		return false;
	}

	bool AlertMessageBox::isResponse() const
	{
		return m_mode == OM::Alert::Mode::None;
	}

	void AlertMessageBox::clear()
	{
		UI_LOCK();
		m_mode = OM::Alert::Mode::None;
		MessageBox::clear();
		setMinText("");
		setMaxText("");
		setWarningText("");
		setOkBtnText(_("msgbox_ok"));
		setCancelBtnText(_("msgbox_cancel"));

		warningTextVisible(false);
		minTextVisible(false);
		maxTextVisible(false);
		inputVisible(false);
		axisJogVisible(false);
		selectionVisible(false);

		setChoiceCallback(nullptr);
		setInputValidationCallback(nullptr);
	}

	void AlertMessageBox::setMode(OM::Alert::Mode mode)
	{
		UI_LOCK();
		LOG_INFO("Seting mode to {:d}", (int)mode);
		m_mode = mode;

		// Hide all containers
		inputVisible(false);
		axisJogVisible(false);
		selectionVisible(false);
		okVisible(false);
		cancelVisible(false);
		warningTextVisible(false);
		progressVisible(false);
		imageVisible(false);

		switch (mode)
		{
		case OM::Alert::Mode::None:
			cancelVisible(true);
			break;
		case OM::Alert::Mode::Info:
			break;
		case OM::Alert::Mode::InfoClose:
		case OM::Alert::Mode::InfoConfirm:
			okVisible(true);
			break;
		case OM::Alert::Mode::ConfirmCancel:
			okVisible(true);
			cancelVisible(true);
			break;
		case OM::Alert::Mode::Choices:
			selectionVisible(true);
			break;
		case OM::Alert::Mode::NumberInt:
			lv_textarea_set_accepted_chars(m_input, "+-0123456789");
			inputVisible(true);
			okVisible(true);
			break;
		case OM::Alert::Mode::NumberFloat:
			lv_textarea_set_accepted_chars(m_input, "+-0123456789.");
			inputVisible(true);
			okVisible(true);
			break;
		case OM::Alert::Mode::Text:
			lv_textarea_set_accepted_chars(m_input, NULL);
			inputVisible(true);
			okVisible(true);
			break;
		}
	}

	void AlertMessageBox::selectionVisible(bool visible)
	{
		UI_LOCK();
		m_choicesList.setVisible(visible);
		updateVisibility();
	}

	void AlertMessageBox::inputVisible(bool visible)
	{
		UI_LOCK();
		m_inputCont.setVisible(visible);
		updateVisibility();
	}

	void AlertMessageBox::warningTextVisible(bool visible)
	{
		UI_LOCK();
		m_warningText.setVisible(visible);
		updateVisibility();
	}

	void AlertMessageBox::minTextVisible(bool visible)
	{
		UI_LOCK();
		m_minText.setVisible(visible);
		updateVisibility();
	}

	void AlertMessageBox::maxTextVisible(bool visible)
	{
		UI_LOCK();
		m_maxText.setVisible(visible);
		updateVisibility();
	}

	void AlertMessageBox::axisJogVisible(bool visible)
	{
		UI_LOCK();
		m_axisJogList.setVisible(visible);
		updateVisibility();
	}

	void AlertMessageBox::setWarningText(std::string_view format)
	{
		m_warningText.setText(format);
	}

	void AlertMessageBox::setMinText(std::string_view format)
	{
		m_minText.setText(format);
	}

	void AlertMessageBox::setMaxText(std::string_view format)
	{
		m_maxText.setText(format);
	}

	size_t AlertMessageBox::getJogAxisCount() const
	{
		return m_axisJogList.getItemCount();
	}

	void AlertMessageBox::setJogAxisCount(size_t count)
	{
		m_axisJogList.setItemCount(count, *this);
	}

	std::string_view AlertMessageBox::getJogAxisLetter(size_t index) const
	{
		UI_LOCK();
		auto control = m_axisJogList.getItem(index);
		if (!control)
		{
			LOG_ERROR("Index {:d} out of range", index);
			return "";
		}
		return control->getAxisLetter();
	}

	void AlertMessageBox::setJogAxisLetter(size_t index, char letter)
	{
		UI_LOCK();
		if (index >= getJogAxisCount())
		{
			setJogAxisCount(index + 1);
		}
		auto control = m_axisJogList.getItem(index);
		control->setAxisLetter(letter);
	}

	void AlertMessageBox::setJogAxisPosition(size_t index, float position)
	{
		UI_LOCK();
		if (index >= getJogAxisCount())
		{
			setJogAxisCount(index + 1);
		}
		auto control = m_axisJogList.getItem(index);
		control->setPosition(position);
	}

	void AlertMessageBox::setJogAxisEnabled(size_t index, bool enabled)
	{
		UI_LOCK();
		if (index >= getJogAxisCount())
		{
			setJogAxisCount(index + 1);
		}
		auto control = m_axisJogList.getItem(index);
		control->setEnabled(enabled);
	}

	size_t AlertMessageBox::getChoiceCount() const
	{
		return m_choicesList.getItemCount();
	}

	void AlertMessageBox::setChoiceCount(size_t count)
	{
		m_choicesList.setItemCount(count,
								   [this](size_t i, LvObj& parent)
								   {
									   auto btn =
										   std::make_shared<Button>(fmt::format("msgbox_choice_{:d}", i), parent, "");
									   btn->setSize(LV_PCT(20), LV_SIZE_CONTENT);
									   btn->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
									   btn->addClickedCallback(onChoiceEvent, this);
									   return btn;
								   });
	}

	void AlertMessageBox::setChoice(size_t index, std::string_view text)
	{
		UI_LOCK();
		auto control = m_choicesList.getItem(index);
		if (!control)
		{
			LOG_ERROR("Index {:d} out of range", index);
			return;
		}
		control->setText(text);
	}

	void AlertMessageBox::setInput(int32_t val)
	{
		UI_LOCK();
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", val);
		m_input.setText(buf);
	}

	void AlertMessageBox::setInput(float val)
	{
		UI_LOCK();
		char buf[32];
		snprintf(buf, sizeof(buf), "%.1f", val);
		m_input.setText(buf);
	}

	void AlertMessageBox::setInput(std::string_view text)
	{
		UI_LOCK();
		m_input.setText(text);
	}

	std::string_view AlertMessageBox::getInput() const
	{
		UI_LOCK();
		return m_input.getText();
	}

	void AlertMessageBox::onChoiceEvent(lv_event_t* e)
	{
		UI_LOCK();
		AlertMessageBox* msgBox = static_cast<AlertMessageBox*>(lv_event_get_user_data(e));
		lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
		uintptr_t index = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn));
		if (msgBox->m_choiceCb)
		{
			msgBox->m_choiceCb(index);
		}
		msgBox->close();
	}

	void AlertMessageBox::onInputEvent(lv_event_t* e)
	{
		UI_LOCK();
		AlertMessageBox* msgBox = static_cast<AlertMessageBox*>(lv_event_get_user_data(e));
		lv_event_code_t code = lv_event_get_code(e);

		switch (code)
		{
		case LV_EVENT_READY:
			if (msgBox->m_showKeyboardCb)
			{
				msgBox->m_showKeyboardCb(false);
			}
			break;
		case LV_EVENT_FOCUSED:
			if (msgBox->m_kb)
			{
				msgBox->m_kb->setTextArea(&msgBox->m_input);
			}
			if (msgBox->m_showKeyboardCb)
			{
				msgBox->m_showKeyboardCb(true);
			}
			break;
		case LV_EVENT_DEFOCUSED:
			if (msgBox->m_kb)
			{
				msgBox->m_kb->setTextArea(nullptr);
			}
			if (msgBox->m_showKeyboardCb)
			{
				msgBox->m_showKeyboardCb(false);
			}
			break;
		case LV_EVENT_VALUE_CHANGED:
		{
			msgBox->m_okBtn.setDisabled(!msgBox->validate());
			break;
		}
		}
	}

	AlertMessageBox::AxisJog::AxisJog(const size_t index, LvObj& parent, AlertMessageBox& msgBox)
		: LvObj(lv_obj_create, utils::format("msgbox_axis_jog_%u", index), parent)
		, m_index(index)
		, m_msgBox(msgBox)
		, m_label("label", getRoot())
		, m_relMove{Button(utils::format("msgbox_axis_%u_rel_move_1", index), getRoot(), "", layout_t(0, 0, 0, 100)),
					Button(utils::format("msgbox_axis_%u_rel_move_2", index), getRoot(), "", layout_t(0, 0, 0, 100)),
					Button(utils::format("msgbox_axis_%u_rel_move_3", index), getRoot(), "", layout_t(0, 0, 0, 100)),
					Button(utils::format("msgbox_axis_%u_rel_move_4", index), getRoot(), "", layout_t(0, 0, 0, 100)),
					Button(utils::format("msgbox_axis_%u_rel_move_5", index), getRoot(), "", layout_t(0, 0, 0, 100)),
					Button(utils::format("msgbox_axis_%u_rel_move_6", index), getRoot(), "", layout_t(0, 0, 0, 100))}

	{
		UI_LOCK();

		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		setSize(LV_PCT(100), LV_SIZE_CONTENT);
		setStylePad(2, 0, Padding::ALL);
		setStylePad(2, 0, Padding::COLUMN);

		setUserData(this);

		for (size_t i = 0; i < ARRAY_SIZE(m_relMove); i++)
		{
			Button& btn = m_relMove[i];
			btn.setText(utils::format("%.2f", s_jogAmounts[i]).c_str());
			btn.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
			btn.setFlexGrow(1);
			btn.setHeight(LV_SIZE_CONTENT);
			btn.setUserData(reinterpret_cast<void*>(const_cast<float*>(&s_jogAmounts[i])));
			btn.addClickedCallback(onRelMoveEvent, &btn);
		}

		m_label.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_label.setFlexGrow(3);
		m_label.moveToIndex(3);
	}

	void AlertMessageBox::AxisJog::setAxisLetter(char letter)
	{
		UI_LOCK();
		m_axisLetter[0] = letter;
		m_axisLetter[1] = '\0';
		m_label.setText(fmt::format("{:s} = {:.2f}", m_axisLetter, m_position).c_str());
	}

	void AlertMessageBox::AxisJog::setPosition(float position)
	{
		UI_LOCK();
		m_position = position;
		m_label.setText(fmt::format("{:s} = {:.2f}", m_axisLetter, m_position).c_str());
	}

	void AlertMessageBox::AxisJog::setEnabled(bool enabled)
	{
		UI_LOCK();
		for (size_t i = 0; i < ARRAY_SIZE(m_relMove); i++)
		{
			m_relMove[i].setDisabled(!enabled);
		}
	}

	void AlertMessageBox::AxisJog::onRelMoveEvent(lv_event_t* e)
	{
		UI_LOCK();
		Button* btn = static_cast<Button*>(lv_event_get_user_data(e));
		AlertMessageBox::AxisJog* axisJog =
			static_cast<AlertMessageBox::AxisJog*>(lv_obj_get_user_data(btn->getParent()));
		float amount = *reinterpret_cast<float*>(btn->getUserData());

		// TODO: I am breaking the rule of no logic in the view but I'm being lazy. I should create a presenter for
		// this.
		Comm::DUET.SendGcode(
			fmt::format("M120\nG91\nG1 {:c}{:g} F{:d}\nM121\n", axisJog->m_axisLetter[0], amount, 300));
	}

} // namespace UI
