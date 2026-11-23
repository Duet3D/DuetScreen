/*
 * CustomiseActions.cpp
 */

#include "CustomiseActions.h"
#include "CustomisePresenter.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Layout/LayoutEditor.h"
#include "UI/Xml/View/Actions.h"

namespace UI::CustomiseActions
{
	void registerAll()
	{
		Xml::Actions::addGlobal("customise.start",
								"Go to the home screen and start customising it: its widgets can be moved, resized, "
								"added and removed, and their settings changed",
								[](const Xml::ActionArgs&)
								{
									home();
									Xml::LayoutEditor::get().begin();
								});
		Xml::Actions::addGlobal("customise.open_catalogue",
								"While customising, open the widget pool to add a widget where the user chose",
								[](const Xml::ActionArgs&)
								{
									if (auto* customise = CustomisePresenter::shared())
									{
										customise->openCatalogue();
									}
								});
	}
} // namespace UI::CustomiseActions
