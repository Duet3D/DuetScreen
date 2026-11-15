/*
 * ToolActions.cpp
 */

#include "ToolActions.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Data/ToolSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include <fmt/format.h>
#include <string>

namespace UI::ToolActions
{
	/** `actions.tool.toggle`: select the tool numbered `arg`, or deselect it if it is selected */
	static void toggleTool(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		const auto tool = args.argInt();
		if (!tool || *tool < 0)
		{
			LOG_WARN("actions.tool.toggle needs the tool's number as its arg, e.g. arg=\"{{row.index}}\"");
			return;
		}
		const auto number = static_cast<size_t>(*tool);
		bool active = false;
		{
			MODEL_LOCK();
			active = Xml::PrinterModel::tool(number)["state"].text() == "active";
		}
		Xml::Services::sendGcode(active ? std::string("T-1\n") : fmt::format("T{:d}\n", number));
	}

	/** `actions.tool.show`: show the tool numbered `arg` in the tools widget, or follow the tool in use (`auto`) */
	static void showTool(const Xml::ActionArgs& args)
	{
		auto* tools = Xml::DataSources::find<Xml::ToolSource>();
		if (tools == nullptr)
		{
			LOG_WARN("The tool source isn't running");
			return;
		}
		const auto number = args.argInt();
		tools->show(number && *number >= 0 ? std::optional(static_cast<size_t>(*number)) : std::nullopt);
	}

	void registerAll()
	{
		Xml::Actions::addGlobal("tool.show",
								"Show the tool numbered `arg` in detail in the dashboard's tools widget, without "
								"selecting it; `auto` to follow the tool in use again",
								showTool);
		Xml::Actions::addGlobal(
			"tool.toggle",
			"Select the tool numbered `arg`, or deselect it if selected, e.g. `arg=\"{row.index}\"` in a row of "
			"`om.tools`",
			"tr.actions.tool_toggle",
			toggleTool);
	}
} // namespace UI::ToolActions
