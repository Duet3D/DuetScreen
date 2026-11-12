/*
 * DashboardGraph.cpp
 */

#include "DashboardGraph.h"
#include "Configuration.h"

namespace UI
{
	DashboardGraph::DashboardGraph(const std::string& name, LvObj& parent)
		: TemperatureGraph(name, parent)
	{
		setXRange({.min = -60, .max = 0});
		setYRange({.min = 0, .max = 300});
		setXCount(-getXRange().min * MODEL_TICK_HZ * 2);
	}

	void DashboardGraph::onHide()
	{
		ZoneScoped;
		activate();
	}
} // namespace UI
