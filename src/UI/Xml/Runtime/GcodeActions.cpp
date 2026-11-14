/*
 * GcodeActions.cpp
 */

#include "GcodeActions.h"
#include "Debug.h"
#include "Services.h"
#include "UI/Xml/View/Actions.h"

namespace UI::Xml::GcodeActions
{
	/** `actions.gcode.send`: send the argument, placeholders already filled in */
	static void send(const ActionArgs& args)
	{
		if (args.arg.empty())
		{
			LOG_WARN("actions.gcode.send without an arg");
			return;
		}
		Services::sendGcode(args.arg);
	}

	void registerAll()
	{
		Actions::addGlobal("gcode.send",
						   "Send `arg` to the printer as G-code, e.g. `arg=\"M106 P{number} S{event.value}\"`",
						   "tr.actions.gcode_send",
						   send);
	}
} // namespace UI::Xml::GcodeActions
