/*
 * DashboardGraph.h
 *
 * The dashboard's temperature graph: the last minute, 0-300°C. It keeps recording while the dashboard is hidden,
 * so the history is there when it is shown again.
 */

#pragma once

#include "UI/Widgets/Temperature/TemperatureGraph.h"

namespace UI
{
	class DashboardGraph : public TemperatureGraph
	{
	  public:
		/** Create the graph inside `parent` */
		DashboardGraph(const std::string& name, LvObj& parent);

	  protected:
		/** Keep the presenter recording */
		void onHide() override;
	};
} // namespace UI
