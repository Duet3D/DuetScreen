/*
 * FanPresenter.h
 *
 * Behaviour for the fan screen (assets/ui/screens/fan_screen.xml). The fan data itself is the global `fans` list
 * (see UI/Xml/Data/FanSource.h).
 *
 * What each action does is in its registration, and in the UI API reference (docs/XML_UI_API.md).
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
