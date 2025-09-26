/*
 * ToolControlPresenter.cpp
 *
 *  Created on: 2025-07-14
 *      Author: Andy Everitt
 */

#include "ToolControlPresenter.h"
#include "Debug.h"
#include "ToolControl.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	void ToolControlPresenter::onActivate()
	{
		if (m_tool != nullptr)
		{
			setToolIndex(m_toolSlot);
		}
		newToolData();

		for (auto& heater : m_view->getHeaters())
		{
			heater->activate();
		}
	}

	void ToolControlPresenter::onDeactivate()
	{
		UI_LOCK();
		for (auto& heater : m_view->getHeaters())
		{
			auto presenter = heater->getPresenter();
			presenter->deactivate();
		}
	}

	void ToolControlPresenter::reset()
	{
		UI_LOCK();
		m_tool = nullptr;
		m_view->getHeaters().clear();
		m_toolSlot = -1;
	}

	void ToolControlPresenter::setToolIndex(size_t toolIndex)
	{
		UI_LOCK();
		reset();
		m_tool = OM::GetToolBySlot(toolIndex);
		if (m_tool == nullptr)
		{
			LOG_ERROR("Tool with index {:d} not found", toolIndex);
			return;
		}
		LOG_DBG("Set tool index to {} for presenter '{}'", toolIndex, getName());
		m_toolSlot = (ssize_t)toolIndex;
	}

	void ToolControlPresenter::toggleToolState()
	{
		if (m_tool == nullptr)
		{
			LOG_ERROR("Tool is null for presenter '{}'", getName());
			return;
		}

		m_tool->ToggleState();
	}

	void ToolControlPresenter::newToolData()
	{
		if (m_tool == nullptr)
		{
			LOG_ERROR("Tool is null for presenter '{}'", getName());
			return;
		}

		LOG_VERBOSE("Updating tool control for tool index {:d}", m_tool->index);
		m_view->setToolName(m_tool->GetName());
		m_view->setToolState(m_tool->status, _(m_tool->GetStatusStr()));

		auto& heaters = m_view->getHeaters();

		heaters.setItemCount(m_tool->GetHeaterCount(),
							 [this](size_t index, LvObj& parent)
							 {
								 auto control = std::make_shared<HeaterSlider>(fmt::format("{}", index), parent);
								 auto presenter = control->getPresenter();
								 presenter->setToolHeaterIndex(m_tool->index, index);
								 control->setNumberPad(m_view->getNumberPad());
								 control->activate();
								 return control;
							 });
	}

	void ToolControlPresenter::newToolHeaterData(size_t toolIndex)
	{
		if (m_tool == nullptr)
		{
			LOG_ERROR("Tool is null for presenter '{}'", getName());
			return;
		}
		if (m_tool->index != toolIndex)
		{
			LOG_VERBOSE("Tool index mismatch: expected {:d}, got {:d}", m_tool->index, toolIndex);
			return;
		}

		LOG_VERBOSE("Updating tool heaters for tool index {:d}", m_tool->index);
		m_view->getHeaters().clear();
		newToolData();
	}
} // namespace UI
