/*
 * GaugeActions.cpp
 */

#include "GaugeActions.h"
#include "Debug.h"
#include "ObjectModel/Fan.h"
#include "UI/Core/Model.h"
#include "UI/Presenters/Control/Fans/FanPresenter.h"
#include "UI/Presenters/Control/Temperature/HeaterActions.h"
#include "UI/Xml/Data/GaugeSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include <charconv>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::GaugeActions
{
	namespace
	{
		/** `id`'s parts between dots, e.g. "tool.1.0" → tool, 1, 0 */
		std::vector<std::string_view> partsOf(std::string_view id)
		{
			std::vector<std::string_view> parts;
			size_t start = 0;
			while (true)
			{
				const size_t dot = id.find('.', start);
				parts.push_back(id.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start));
				if (dot == std::string_view::npos)
				{
					return parts;
				}
				start = dot + 1;
			}
		}

		/** `text` as a number, if it is one */
		std::optional<size_t> numberOf(std::string_view text)
		{
			size_t value = 0;
			const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
			if (error != std::errc() || end != text.data() + text.size())
			{
				return std::nullopt;
			}
			return value;
		}

		/** Ask for fan `number`'s speed, in percent, on the shared number pad, starting at `current` */
		void askFanSpeed(size_t number, const std::string& name, int32_t current)
		{
			Xml::Services::askNumber(
				{.header = name,
				 .min = 0,
				 .max = 100,
				 .value = static_cast<float>(current),
				 .onConfirm = [number](float value) { setFanSpeed(static_cast<int32_t>(number), value); },
				 .asker = nullptr});
		}

		/** `gauges.edit` */
		void edit(const Xml::ActionArgs& args)
		{
			ZoneScoped;
			const auto* row = args.row<Xml::GaugeRow>();
			if (row == nullptr)
			{
				LOG_WARN("gauges.edit must come from a row of the gauges list");
				return;
			}
			if (row->editable.get() == 0)
			{
				return;
			}

			const std::string id(row->id.get());
			const auto parts = partsOf(id);
			if (id == "speed_factor")
			{
				Xml::Actions::dispatch(args.target, "factors.speed", "");
				return;
			}
			if (id == "extrusion_factor")
			{
				Xml::Actions::dispatch(args.target, "factors.extrusion", "");
				return;
			}

			const auto number = parts.size() > 1 ? numberOf(parts[1]) : std::nullopt;
			if (!number)
			{
				LOG_WARN("gauges.edit: '{:s}' can't be changed", id);
				return;
			}
			// A heater on standby has its standby temperature set, any other its active one
			const bool active = row->state.get() != std::string_view("standby");
			const std::optional<float> value = args.value();
			if (parts[0] == "fan" && value)
			{
				setFanSpeed(static_cast<int32_t>(*number), *value);
			}
			else if (parts[0] == "fan")
			{
				askFanSpeed(*number, std::string(row->name.get()), row->target.get());
			}
			else if (parts[0] == "bed" || parts[0] == "chamber")
			{
				HeaterActions::setTemperature(parts[0] == "bed" ? HeaterActions::Heated::Bed
																: HeaterActions::Heated::Chamber,
											  *number,
											  *number,
											  active,
											  value);
			}
			else if (const auto slot = parts.size() > 2 ? numberOf(parts[2]) : std::nullopt; parts[0] == "tool" && slot)
			{
				HeaterActions::setTemperature(HeaterActions::Heated::ToolHeater, *number, *slot, active, value);
			}
			else
			{
				LOG_WARN("gauges.edit: '{:s}' can't be changed", id);
			}
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("gauges.edit",
								"Change the value a row of `gauges` shows: a heater's target (its standby one while "
								"on standby) or a fan's speed, to the edited value (`value_confirmed`) or `arg`, else "
								"asking for it on the number pad; or open the speed or extrusion factor dialog. Does "
								"nothing for a row that isn't `editable`.",
								edit);
	}
} // namespace UI::GaugeActions
