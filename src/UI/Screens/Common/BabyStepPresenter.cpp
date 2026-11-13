/*
 * BabyStepPresenter.cpp
 */

#include "BabyStepPresenter.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <algorithm>

namespace UI
{
	void BabyStepRow::fields(Xml::FieldVisitor& f)
	{
		f.field("text", text, "E.g. \"0.01 mm\"");
		f.field("selected", selected, "1 for the step in use");
	}

	BabyStepPresenter::BabyStepPresenter()
		: XmlPresenter("babystep")
	{
		auto step = [this](float sign)
		{
			if (m_selected < m_steps.size())
			{
				Xml::Services::sendGcode(fmt::format("M290 S{:g}", sign * m_steps[m_selected]));
			}
		};
		action("babystep.up", "Move Z up by the chosen step (M290)", [step](const Xml::ActionArgs&) { step(1.0f); });
		action(
			"babystep.down", "Move Z down by the chosen step (M290)", [step](const Xml::ActionArgs&) { step(-1.0f); });
		action("babystep.reset",
			   "Undo all babystepping",
			   [](const Xml::ActionArgs&) { Xml::Services::sendGcode("M290 R0 S0"); });
		action("babystep.select",
			   "Use that step. From a row of `steps`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<BabyStepRow>())
				   {
					   m_selected = static_cast<size_t>(row->index.get());
					   showSteps();
				   }
			   });
		action("babystep.edit",
			   "Ask for a new size for that step with a number pad, and remember it. From a row of `steps`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto* row = args.row<BabyStepRow>();
				   if (row == nullptr)
				   {
					   return;
				   }
				   const auto i = static_cast<size_t>(row->index.get());
				   Xml::Services::askNumber({.header = _("multi_value_selector.numberpad_new_value_header"),
											 .min = 0.001f,
											 .max = 1.0f,
											 .value = m_steps.at(i),
											 .onConfirm = [this, i](float value)
											 {
												 m_steps.at(i) = value;
												 StorageHelper::setData(ID_BABYSTEP_AMOUNT, m_steps);
												 showSteps();
											 }});
			   });
	}

	void BabyStepPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("usable", m_usable, "1 if there is a Z axis and the printer is connected");
		f.field("reset_text", m_resetText, "The reset button's text, with the current offset, e.g. \"Reset 0.02 mm\"");
		f.list("steps", m_stepRows, "Step sizes");
	}

	void BabyStepPresenter::onInit()
	{
		registerEventListener<EventType::AxesData>([this]() { showOffset(); });
	}

	void BabyStepPresenter::onActivate()
	{
		m_steps = StorageHelper::getData(ID_BABYSTEP_AMOUNT);
		showSteps();
		showOffset();
	}

	void BabyStepPresenter::onDisconnect()
	{
		m_usable.set(0);
	}

	void BabyStepPresenter::showOffset()
	{
		MODEL_LOCK();
		auto z = OM::Move::GetAxisByLetter('Z');
		m_usable.set(z && OM::IsConnected() ? 1 : 0);
		m_resetText.set(_("babystep.reset", z ? z->babystep : 0.0f));
	}

	void BabyStepPresenter::showSteps()
	{
		m_selected = std::min(m_selected, m_steps.empty() ? 0 : m_steps.size() - 1);
		m_stepRows.sync(m_steps.size(),
						[this](BabyStepRow& row, size_t i)
						{
							row.text.set(fmt::format("{:g} mm", m_steps[i]));
							row.selected.set(i == m_selected ? 1 : 0);
						});
	}
} // namespace UI
