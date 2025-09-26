/*
 * BabyStep.cpp
 *
 *  Created on: 2025-09-25
 *      Author: Andy Everitt
 */

#include "BabyStep.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	BabyStep::BabyStep(const std::string& name, LvObj& parent)
		: View(name, parent, layout_t(0, 0, 100, 100))
	{
		UI_LOCK();

		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_header.setText(_("babystep_header"));

		m_buttonPanel.setFlexGrow(1);
		m_buttonPanel.setWidth(LV_PCT(100));
		// m_buttonPanel.setMinWidth(LV_SIZE_CONTENT);

		setBabyStepValue(0);
		m_buttonPanel.setIncrementLabel(_("babystep_increment"));
		m_buttonPanel.setDecrementLabel(_("babystep_decrement"));
		m_buttonPanel.setValueLabelFmt("{:g}mm");
		m_buttonPanel.setIncrementValues({0.01f, 0.05f});
		m_buttonPanel.setValueChangeCallback([this](float change) { m_presenter->babystep(change); });
		m_buttonPanel.setResetCallback([this]() { m_presenter->resetBabystep(); });
	}

	void BabyStep::setBabyStepValue(float value)
	{
		m_buttonPanel.setResetLabel(fmt::format(fmt::runtime(_("babystep_reset")), value));
	}
} // namespace UI
