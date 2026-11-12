/*
 * FansPresenter.h
 *
 * Behaviour for fan views. The fan data itself comes from the `om.fans` model (see OmPublisher.h).
 *
 * Actions
 *   fans.set_speed    set the speed of the fan in the list item the action came from, in percent. The value is the
 *                     edited value (`value_confirmed`) or the action's `arg`.
 */

#pragma once

#include "UI/Xml/XmlPresenter.h"

namespace UI::Xml
{
	class FansPresenter : public XmlPresenter
	{
	  public:
		FansPresenter();

	  private:
		void setSpeed(const ActionArgs& args);
	};
} // namespace UI::Xml
