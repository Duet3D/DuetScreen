/*
 * EStopAction.cpp
 */

#include "EStopAction.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Model.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"

namespace UI::EStopAction
{
	namespace
	{
		constexpr float TAP = 0.5f; // Dragged less than this far, it was a tap: say how to use it

		/** `estop.dragged` */
		void dragged(const Xml::ActionArgs& args)
		{
			const float fraction = args.value().value_or(0.0f);
			if (fraction >= 1.0f)
			{
				LOG_WARN("EStop Pressed!");
				Comm::DUET.Estop();
				Model::get().post<EventType::Response>(ResponseType::ERROR, _("estop.message"));
			}
			else if (fraction < TAP)
			{
				Model::get().post<EventType::Response>(ResponseType::WARNING, _("estop.prompt"));
			}
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("estop.dragged",
								"Emergency stop the printer when a ds_drag_button was dragged all the way "
								"(`value_confirmed` 1); if it was only tapped, say to drag it",
								dragged);
	}
} // namespace UI::EStopAction
