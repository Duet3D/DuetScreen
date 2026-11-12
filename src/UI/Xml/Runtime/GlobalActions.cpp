/*
 * GlobalActions.cpp
 */

#include "GlobalActions.h"
#include "Debug.h"
#include "Services.h"
#include "UI/Xml/View/Actions.h"

namespace UI::Xml::GlobalActions
{
	/** `gcode`: send the argument, placeholders already filled in */
	static void gcode(const ActionArgs& args)
	{
		if (args.arg.empty())
		{
			LOG_WARN("gcode action without an arg");
			return;
		}
		Services::sendGcode(args.arg);
	}

	void registerAll()
	{
		Actions::addGlobal("gcode", gcode);
	}
} // namespace UI::Xml::GlobalActions
