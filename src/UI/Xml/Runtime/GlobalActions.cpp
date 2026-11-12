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
		Actions::addGlobal("gcode", gcode);
		Actions::addGlobal("nav.open", navOpen);
		Actions::addGlobal("nav.home", [](const ActionArgs&) { home(); });
		Actions::addGlobal("nav.back", [](const ActionArgs&) { back(); });
	}
} // namespace UI::Xml::GlobalActions
