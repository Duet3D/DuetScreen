/*
 * FanPresenter.h
 *
 * Behaviour for the fan screen (assets/ui/screens/fan_screen.xml). The fan data itself is the global `fans` list
 * (see UI/Xml/Data/FanSource.h).
 *
 * Actions
 *   fans.set_speed    set the speed of the fan in the row the action came from, in percent: the edited value
 *                     (`value_confirmed`) or the action's `arg`
 */

#pragma once

#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class FanPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter and register its actions */
		FanPresenter();

	  private:
		/** `fans.set_speed` */
		void setSpeed(const Xml::ActionArgs& args);
	};
} // namespace UI
