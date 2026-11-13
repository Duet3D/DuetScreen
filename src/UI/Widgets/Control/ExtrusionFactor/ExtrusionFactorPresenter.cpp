/*
 * ExtrusionFactorPresenter.cpp
 *
 *  Created on: 2025-11-10
 *      Author: Andy Everitt
 */

#include "ExtrusionFactorPresenter.h"
#include "Debug.h"
#include "ExtrusionFactor.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Tool.h"
#include "i18n/i18n.h"

namespace UI
{
	void ExtrusionFactorPresenter::setTool(OM::ToolPtr tool)
	{
		ZoneScoped;
		m_tool = std::move(tool);

		newExtruderData();
	}

	void ExtrusionFactorPresenter::newExtruderData()
	{
		ZoneScoped;
		auto callback = [&](OM::Move::ExtruderAxisPtr extruder, size_t index)
		{
			m_view->setExtruderLabel(index, _("fine_tune.extruder", extruder->index));
			m_view->setExtruderValue(index, static_cast<uint32_t>(std::round(100 * extruder->factor)));
			return;
		};

		if (m_tool == nullptr)
		{
			m_view->setExtruderCount(OM::Move::GetExtruderAxisCount());

			OM::Move::IterateExtruderAxesWhile(
				[&](OM::Move::ExtruderAxisPtr extruder, size_t index)
				{
					callback(std::move(extruder), index);
					return true;
				});
		}
		else
		{
			size_t extruderCount = m_tool->GetExtruderCount();
			m_view->setExtruderCount(extruderCount);

			m_tool->IterateExtruders(callback);
		}
	}

	void ExtrusionFactorPresenter::setExtruderFactor(size_t slot, uint32_t value)
	{
		ZoneScoped;
		auto extruder = m_tool ? m_tool->GetExtruder(slot) : OM::Move::GetExtruderAxisBySlot(slot);
		if (extruder == nullptr || value == static_cast<uint32_t>(std::lround(100 * extruder->factor)))
		{
			return;
		}

		LOG_DBG("Setting extruder {:d} slider factor to {:d}", extruder->index, value);
		Comm::DUET.SendGcodef("M221 D{:d} S{:d}\n", extruder->index, value);
	}

	void ModalExtrusionFactorPresenter::configureNumberPad(size_t index)
	{
		ZoneScoped;
		auto tool = getView()->getExtrusionFactor().getPresenter()->m_tool;
		auto extruder = tool ? tool->GetExtruder(index) : OM::Move::GetExtruderAxisBySlot(index);
		configureNumberPad(std::move(extruder));
	};

	void ModalExtrusionFactorPresenter::configureNumberPad(OM::Move::ExtruderAxisPtr extruder)
	{
		ZoneScoped;
		auto& np = m_view->getNumberPad();

		np.setVisible(extruder != nullptr);

		if (!extruder)
		{
			LOG_ERROR("No extruder found for number pad configuration");
			return;
		}

		/** TODO this is effectively duplicated from ExtrusionFactor::configureNumberPad(Slider* slider) but because
		 * we need to configure the number pad on activation, we don't have a slider to pass to it.
		 */

		np.setHeader(_("fine_tune.extruder", extruder->index));
		np.setValue(std::round(100 * extruder->factor));
		np.setMinValue(0);
		np.setConfirmCallback(
			[extruder](float value)
			{
				if (extruder == nullptr || value == std::round(100 * extruder->factor))
				{
					return;
				}

				LOG_DBG("Setting extruder {:d} number pad factor to {:g}", extruder->index, value);
				Comm::DUET.SendGcodef("M221 D{:d} S{:g}\n", extruder->index, value);
			});
	}

	void ModalExtrusionFactorPresenter::onActivate()
	{
		ZoneScoped;
		// The tool the dialog was opened for, else (the job's dialog, which shows every extruder) the selected one
		auto tool = getView()->getExtrusionFactor().getPresenter()->m_tool;
		if (tool == nullptr)
		{
			tool = OM::GetCurrentTool();
		}
		OM::Move::ExtruderAxisPtr extruder;
		if (tool)
		{
			extruder = tool->GetExtruder(0);
		}

		configureNumberPad(std::move(extruder));
	}
} // namespace UI
