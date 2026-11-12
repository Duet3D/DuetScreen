/*
 * ToolListPresenter.h
 *
 * Behaviour for the tool list (assets/ui/components/tool_list.xml). The tools, beds and chambers themselves are the
 * global `tools`, `beds` and `chambers` lists (see UI/Xml/Data/ToolSource.h and HeatSource.h).
 *
 * Actions, raised from a row of those lists
 *   tools.toggle          select or deselect the tool
 *   heaters.toggle        cycle the heater through active, standby and off
 *   heaters.set_active    ask for the heater's active temperature with the number pad called tool_numberpad
 *   heaters.set_standby   ask for the heater's standby temperature the same way
 */

#pragma once

#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class ToolListPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter and register its actions */
		ToolListPresenter();

	  private:
		/** `tools.toggle` */
		void toggleTool(const Xml::ActionArgs& args);

		/** `heaters.toggle` */
		void toggleHeater(const Xml::ActionArgs& args);

		/** `heaters.set_active` and `heaters.set_standby` */
		void setTemperature(const Xml::ActionArgs& args, bool active);
	};
} // namespace UI
