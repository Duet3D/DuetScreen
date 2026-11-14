/*
 * FactorsActions.cpp
 */

#include "FactorsActions.h"
#include "FactorsPresenter.h"
#include "UI/Xml/View/Actions.h"
#include <optional>

namespace UI::FactorsActions
{
	void registerAll()
	{
		Xml::Actions::addGlobal("factors.speed",
								"Open the dialog to change the speed factor",
								[](const Xml::ActionArgs&)
								{
									if (auto* factors = FactorsPresenter::shared())
									{
										factors->openSpeed();
									}
								});
		Xml::Actions::addGlobal(
			"factors.extrusion",
			"Open the dialog to change extrusion factors: those of the tool numbered `arg`, if given, else every "
			"extruder's",
			[](const Xml::ActionArgs& args)
			{
				auto* factors = FactorsPresenter::shared();
				if (factors == nullptr)
				{
					return;
				}
				const auto tool = args.argInt();
				factors->openExtrusion(tool && *tool >= 0 ? std::optional(static_cast<size_t>(*tool)) : std::nullopt);
			});
	}
} // namespace UI::FactorsActions
