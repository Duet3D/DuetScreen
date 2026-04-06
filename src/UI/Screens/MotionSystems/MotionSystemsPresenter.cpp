/*
 * MotionSystemsPresenter.cpp
 *
 *  Created on: 2026-03-24
 */

#include "MotionSystemsPresenter.h"
#include "Debug.h"
#include "MotionSystemsView.h"
#include "ObjectModel/MotionSystem.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <cmath>

namespace UI
{
	void MotionSystemsPresenter::onInit()
	{
		ZoneScoped;
		registerEventListener<EventType::MotionSystemData>(this, &MotionSystemsPresenter::newMotionSystemData);
		registerEventListener<EventType::AxesData>(this, &MotionSystemsPresenter::newAxesData);
	}

	void MotionSystemsPresenter::onActivate()
	{
		ZoneScoped;
		refreshTool();
		refreshSpeedFactor();
		refreshSpeeds();
	}

	void MotionSystemsPresenter::onDisconnect()
	{
		ZoneScoped;
	}

	void MotionSystemsPresenter::newMotionSystemData()
	{
		ZoneScoped;
		refreshTool();
		refreshSpeedFactor();
		refreshSpeeds();
	}

	void MotionSystemsPresenter::newAxesData()
	{
		ZoneScoped;
		{
			auto& panel1 = m_view->getMotionSystemPanel(0);
			const auto x = OM::Move::GetAxisByLetter('X');
			const auto y = OM::Move::GetAxisByLetter('Y');
			panel1.setPosition(0, 'X', x == nullptr ? 0.0f : x->machinePosition);
			panel1.setPosition(1, 'Y', y == nullptr ? 0.0f : y->machinePosition);
		}
		{
			auto& panel2 = m_view->getMotionSystemPanel(1);
			const auto x = OM::Move::GetAxisByLetter('U');
			const auto y = OM::Move::GetAxisByLetter('V');
			panel2.setPosition(0, 'U', x == nullptr ? 0.0f : x->machinePosition);
			panel2.setPosition(1, 'V', y == nullptr ? 0.0f : y->machinePosition);
		}
	}

	void MotionSystemsPresenter::refreshTool()
	{
		ZoneScoped;
		MODEL_LOCK();

		const size_t toolCount = OM::GetToolCount();

		OM::Move::IterateMotionSystemsWhile(
			[this, toolCount](OM::Move::MotionSystemPtr ms, size_t idx)
			{
				if (idx >= m_view->getMotionSystemPanels().size())
					return false;

				auto& panel = m_view->getMotionSystemPanel(idx);

				const int32_t currentTool = ms->currentTool;
				if (currentTool < 0)
				{
					panel.setTool(static_cast<size_t>(-1));
				}
				else
				{
					// Map tool index to slot index in the tool list
					const size_t slot = idx == 0 ? ms->currentTool : ms->currentTool - getView()->MS_TOOL_COUNT[0];
					panel.setTool(slot);
				}

				return true;
			});
	}

	void MotionSystemsPresenter::refreshSpeedFactor()
	{
		ZoneScoped;
		MODEL_LOCK();

		OM::Move::IterateMotionSystemsWhile(
			[this](OM::Move::MotionSystemPtr ms, size_t idx)
			{
				if (idx >= m_view->getMotionSystemPanels().size())
					return false;

				auto& panel = m_view->getMotionSystemPanel(idx);
				panel.setSpeedFactor(static_cast<uint32_t>(std::lround(100 * ms->speedFactor)));
				return true;
			});
	}

	void MotionSystemsPresenter::refreshSpeeds()
	{
		ZoneScoped;
		MODEL_LOCK();

		OM::Move::IterateMotionSystemsWhile(
			[this](OM::Move::MotionSystemPtr ms, size_t idx)
			{
				if (idx >= m_view->getMotionSystemPanels().size())
					return false;

				auto& panel = m_view->getMotionSystemPanel(idx);
				panel.setSpeeds(ms->currentMove.topSpeed, ms->currentMove.requestedSpeed);
				return true;
			});
	}
} // namespace UI