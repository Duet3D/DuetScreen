/*
 * ToolActions.cpp
 */

#include "ToolActions.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Data/ToolSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include <fmt/format.h>
#include <string>

namespace UI::ToolActions
{
	/** `actions.tool.toggle`: select the tool, or deselect it if it is selected */
	static void toggleTool(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		const auto* row = args.row<Xml::ToolRow>();
		if (row == nullptr)
		{
			LOG_WARN("actions.tool.toggle must come from a row of the tools list");
			return;
		}
		const auto number = static_cast<size_t>(row->number.get());
		bool active = false;
		{
			MODEL_LOCK();
			active = Xml::PrinterModel::tool(number)["state"].text() == "active";
		}
		Xml::Services::sendGcode(active ? std::string("T-1\n") : fmt::format("T{:d}\n", number));
	}

	void registerAll()
	{
		Xml::Actions::addGlobal(
			"tool.toggle", "Select the tool, or deselect it if selected. From a row of `tools`.", toggleTool);
	}
} // namespace UI::ToolActions
