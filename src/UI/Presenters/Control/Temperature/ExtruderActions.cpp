/*
 * ExtruderActions.cpp
 */

#include "ExtruderActions.h"
#include "Debug.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/ExtruderSource.h"
#include "UI/Xml/View/Actions.h"

namespace UI::ExtruderActions
{
	namespace
	{
		/** Extrude (`direction` 1) or retract (-1) with the current tool, if the extruder's source is running */
		void extrude(int direction)
		{
			const auto* source = Xml::DataSources::find<Xml::ExtruderSource>();
			if (source == nullptr)
			{
				LOG_WARN("The extruder's source isn't running");
				return;
			}
			source->extrude(direction);
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("extruder.extrude",
								"Extrude the chosen distance (`source.extruder.distance`) at the chosen feed rate with "
								"the current tool",
								"tr.actions.extruder_extrude",
								[](const Xml::ActionArgs&) { extrude(1); });
		Xml::Actions::addGlobal("extruder.retract",
								"Retract the chosen distance at the chosen feed rate with the current tool",
								"tr.actions.extruder_retract",
								[](const Xml::ActionArgs&) { extrude(-1); });
	}
} // namespace UI::ExtruderActions
