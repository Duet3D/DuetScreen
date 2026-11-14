/*
 * FactorsActions.cpp
 */

#include "FactorsActions.h"
#include "FactorsPresenter.h"
#include "UI/Xml/Data/ToolSource.h"
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
			"Open the dialog to change extrusion factors: the tool's, from a row of a list of tools, else every "
			"extruder's",
			[](const Xml::ActionArgs& args)
			{
				auto* factors = FactorsPresenter::shared();
				if (factors == nullptr)
				{
					return;
				}
				const auto* row = args.row<Xml::ToolRow>();
				factors->openExtrusion(row != nullptr ? std::optional(static_cast<size_t>(row->number.get()))
													  : std::nullopt);
			});
	}
} // namespace UI::FactorsActions
