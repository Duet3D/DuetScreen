/*
 * PrinterModel.cpp
 */

#include "PrinterModel.h"
#include "ObjectModel/Tree/Tree.h"
#include "i18n/i18n.h"
#include <cctype>

namespace UI::Xml::PrinterModel
{
	namespace
	{
		/** The beds or chambers that have a heater, from `heat.bedHeaters` or `heat.chamberHeaters` */
		std::vector<HeatedPlace> heated(std::string_view list)
		{
			std::vector<HeatedPlace> places;
			const OM::Node& heaters = root()["heat"][list];
			for (size_t number = 0; number < heaters.size(); ++number)
			{
				const int64_t index = heaters[number].integer(-1);
				if (index >= 0 && heater(static_cast<size_t>(index)).exists())
				{
					places.push_back({.number = number, .heater = static_cast<size_t>(index)});
				}
			}
			return places;
		}
	} // namespace

	const OM::Node& root()
	{
		return OM::Tree::get().root();
	}

	const OM::Node& heater(size_t number)
	{
		return root()["heat"]["heaters"][number];
	}

	const OM::Node& tool(size_t number)
	{
		return root()["tools"][number];
	}

	std::vector<size_t> tools()
	{
		std::vector<size_t> numbers;
		const OM::Node& all = root()["tools"];
		for (size_t number = 0; number < all.size(); ++number)
		{
			if (all[number].exists())
			{
				numbers.push_back(number);
			}
		}
		return numbers;
	}

	std::vector<HeatedPlace> beds()
	{
		return heated("bedHeaters");
	}

	std::vector<HeatedPlace> chambers()
	{
		return heated("chamberHeaters");
	}

	std::optional<size_t> currentTool()
	{
		const int64_t number = root()["state"]["currentTool"].integer(-1);
		if (number < 0 || !tool(static_cast<size_t>(number)).exists())
		{
			return std::nullopt;
		}
		return static_cast<size_t>(number);
	}

	std::string heaterName(size_t number)
	{
		const int64_t sensor = heater(number)["sensor"].integer(-1);
		const std::string_view name =
			sensor >= 0 ? root()["sensors"]["analog"][static_cast<size_t>(sensor)]["name"].text() : "";
		return name.empty() ? _("temperature.heater", number) : std::string(name);
	}

	std::string toolName(size_t number)
	{
		const std::string_view name = tool(number)["name"].text();
		return name.empty() ? _("temperature.default_tool_name", number) : std::string(name);
	}

	std::string heaterState(size_t number)
	{
		const std::string_view state = heater(number)["state"].text();
		return state.empty() ? "off" : std::string(state);
	}

	int32_t heaterTarget(size_t number)
	{
		const std::string state = heaterState(number);
		const OM::Node& node = heater(number);
		if (state == "active")
		{
			return static_cast<int32_t>(node["active"].integer(0));
		}
		if (state == "standby")
		{
			return static_cast<int32_t>(node["standby"].integer(0));
		}
		return 0;
	}

	std::optional<size_t> toolHeater(size_t number, size_t slot)
	{
		const int64_t index = tool(number)["heaters"][slot].integer(-1);
		if (index < 0 || !heater(static_cast<size_t>(index)).exists())
		{
			return std::nullopt;
		}
		return static_cast<size_t>(index);
	}

	bool takesFilament(size_t number)
	{
		return tool(number)["filamentExtruder"].integer(-1) >= 0;
	}

	std::string_view filament(size_t number)
	{
		const int64_t extruder = tool(number)["filamentExtruder"].integer(-1);
		if (extruder < 0)
		{
			return "";
		}
		return root()["move"]["extruders"][static_cast<size_t>(extruder)]["filament"].text();
	}

	std::optional<size_t> axisNumber(char letter)
	{
		const OM::Node& axes = root()["move"]["axes"];
		for (size_t number = 0; number < axes.size(); ++number)
		{
			const std::string_view axisLetter = axes[number]["letter"].text();
			if (!axisLetter.empty() && axisLetter.front() == letter)
			{
				return number;
			}
		}
		return std::nullopt;
	}

	const OM::Node& axis(char letter)
	{
		const auto number = axisNumber(letter);
		return number ? root()["move"]["axes"][*number] : OM::Node::missing();
	}

	float toolOffset(char letter)
	{
		const auto tool = currentTool();
		const auto number = axisNumber(letter);
		return tool && number ? static_cast<float>(PrinterModel::tool(*tool)["offsets"][*number].number()) : 0.0f;
	}

	std::vector<size_t> visibleAxes()
	{
		std::vector<size_t> numbers;
		const OM::Node& axes = root()["move"]["axes"];
		for (size_t number = 0; number < axes.size(); ++number)
		{
			if (axes[number].exists() && axes[number]["visible"].boolean(true))
			{
				numbers.push_back(number);
			}
		}
		return numbers;
	}

	std::string axisGcode(char letter)
	{
		return std::islower(static_cast<unsigned char>(letter)) ? "'" + std::string(1, letter) : std::string(1, letter);
	}

	bool noMovesBeforeHoming()
	{
		return root()["move"]["noMovesBeforeHoming"].boolean(false);
	}
} // namespace UI::Xml::PrinterModel
