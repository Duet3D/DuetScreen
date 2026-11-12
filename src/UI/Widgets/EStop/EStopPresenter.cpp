/*
 * EStopPresenter.cpp
 *
 *  Created on: 2025-08-27
 *      Author: Andy Everitt
 */

#include "EStopPresenter.h"
#include "Debug.h"
#include "EStop.h"
#include "Hardware/Duet.h"
#include "i18n/i18n.h"

namespace UI
{
	void EStopPresenter::eStop()
	{
		ZoneScoped;
		LOG_WARN("EStop Pressed!");
		Comm::DUET.Estop();
		Model::get().post<EventType::Response>(ResponseType::ERROR, _("estop.message"));
	}

} // namespace UI
