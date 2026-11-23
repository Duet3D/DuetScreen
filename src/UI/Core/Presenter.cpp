/*
 * Presenter.cpp
 *
 *  Created on: 2025-07-14
 *      Author: Andy Everitt
 */

#include "Presenter.h"
#include "Debug.h"
#include "UI/Lvgl/LvObj.h"

namespace UI
{
	void BasePresenter::init()
	{
		ZoneScoped;
		LOG_DBG("Initializing presenter '{}'", getName());
		onInit();
		registerEventListener<EventType::Connected>(this, &BasePresenter::connected);
		registerEventListener<EventType::Disconnected>(this, &BasePresenter::disconnected);
	}

	void BasePresenter::activate()
	{
		ZoneScoped;
		UI_LOCK();
		if (m_state == State::UNINITIALISED)
		{
			init();
			m_state = State::DEACTIVE;
		}

		LOG_DBG("Activating presenter '{}'", getName());

		m_state = State::ACTIVATING;
		onActivate();
		m_state = State::ACTIVE;
	}

	void BasePresenter::deactivate()
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("Deactivating presenter '{}'", getName());
		if (m_state == State::ACTIVE || m_state == State::ACTIVATING)
		{
			m_state = State::DEACTIVATING;
			onDeactivate();
			m_state = State::DEACTIVE;
		}
	}

	std::string_view BasePresenter::getName() const
	{
		static std::string_view name = "BasePresenter";
		return name;
	}

	void BasePresenter::connected()
	{
		ZoneScoped;
		LOG_DBG("{} connected", getName());
		onConnect();
		onActivate();
	}

	void BasePresenter::disconnected()
	{
		ZoneScoped;
		LOG_DBG("{} disconnected", getName());
		onDisconnect();
	}
} // namespace UI
