/*
 * DialogActions.cpp
 */

#include "DialogActions.h"
#include "Debug.h"
#include "DialogHost.h"
#include "UI/Xml/View/Actions.h"

namespace UI::Xml::DialogActions
{
	void registerAll()
	{
		Actions::addGlobal("dialog.close",
						   "Close the dialog step the widget is in, and any opened from it, as the user closing it "
						   "does: a `<ds_dialog>` empties its subject and raises `dismissed`. E.g. on a Cancel button.",
						   [](const ActionArgs& args)
						   {
							   DialogHost* host = args.target != nullptr ? DialogHost::holding(args.target) : nullptr;
							   if (host == nullptr)
							   {
								   LOG_WARN("actions.dialog.close must come from inside a dialog");
								   return;
							   }
							   host->dismissHolding(args.target);
						   });
	}
} // namespace UI::Xml::DialogActions
