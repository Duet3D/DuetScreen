/*
 * GlobalActions.cpp
 */

#include "GlobalActions.h"
#include "Debug.h"
#include "Services.h"
#include "UI/Core/Navigation.h"
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

	/** `nav.open`: open the screen named by the argument */
	static void navOpen(const ActionArgs& args)
	{
		openScreen(args.arg, true);
	}

	void registerAll()
	{
		Actions::addGlobal(
			"gcode", "Send `arg` to the printer as G-code, e.g. `arg=\"M106 P{number} S{value}\"`", gcode);
		Actions::addGlobal("nav.open", "Open the screen registered as `arg`, e.g. `arg=\"control\"`", navOpen);
		Actions::addGlobal(
			"nav.home", "Close every screen and show the home screens", [](const ActionArgs&) { home(); });
		Actions::addGlobal(
			"nav.back", "Close the top modal, or else go back a screen", [](const ActionArgs&) { back(); });
	}
} // namespace UI::Xml::GlobalActions
