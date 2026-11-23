/*
 * FanActions.cpp
 */

#include "FanActions.h"
#include "Debug.h"
#include "ObjectModel/Tree/Tree.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Om/OmList.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

namespace UI::FanActions
{
	namespace
	{
		/** Set fan `number`'s speed to `value` percent (clamped to 0-100), unless it is already set to that */
		void setFanSpeed(int32_t number, float value)
		{
			const int32_t percent = std::clamp(static_cast<int32_t>(std::lround(value)), 0, 100);
			{
				MODEL_LOCK();
				const OM::Node& fan = OM::Tree::get().root()["fans"][static_cast<size_t>(number)];
				if (fan.exists() && static_cast<int32_t>(std::lround(100 * fan["requestedValue"].number())) == percent)
				{
					return;
				}
			}
			Xml::Services::sendGcode(fmt::format("M106 P{:d} S{:d}", number, 255 * percent / 100));
		}

		/** The number of the fan whose row of `om.fans` the action is raised in, if it is in one */
		std::optional<int32_t> fanOfRow(const Xml::ActionArgs& args)
		{
			const auto* row = args.row<Xml::OmRow>();
			if (row == nullptr)
			{
				return std::nullopt;
			}
			const auto& segments = row->item().segments();
			const auto* list = segments.size() == 2 ? std::get_if<std::string>(&segments[0]) : nullptr;
			const auto* index = segments.size() == 2 ? std::get_if<size_t>(&segments[1]) : nullptr;
			if (list == nullptr || *list != "fans" || index == nullptr)
			{
				return std::nullopt;
			}
			return static_cast<int32_t>(*index);
		}

		/** `actions.fan.set_speed` */
		void setSpeed(const Xml::ActionArgs& args)
		{
			ZoneScoped;
			// In a fan's row, the fan is the row's and `arg` the speed; elsewhere `arg` names the fan
			const auto inRow = fanOfRow(args);
			const auto number = inRow ? inRow : args.argInt();
			if (!number || *number < 0)
			{
				LOG_WARN("actions.fan.set_speed must come from a row of om.fans, or name the fan in its arg");
				return;
			}

			std::string name;
			float current = 0;
			{
				MODEL_LOCK();
				const OM::Node& fan = OM::Tree::get().root()["fans"][static_cast<size_t>(*number)];
				// A thermostatic fan follows its sensors' temperature, so it can't be set by hand
				if (!fan.exists() || fan["thermostatic"]["lowTemperature"].exists())
				{
					return;
				}
				name = fan["name"].text().empty() ? _("fine_tune.fan", *number) : std::string(fan["name"].text());
				current = static_cast<float>(std::lround(100 * fan["requestedValue"].number()));
			}

			if (const auto speed = inRow ? args.value() : args.edited())
			{
				setFanSpeed(*number, *speed);
				return;
			}
			const int32_t fan = *number;
			Xml::Prompts::askNumber({.header = name,
									 .min = 0,
									 .max = 100,
									 .value = current,
									 .unit = "%",
									 .onConfirm = [fan](float value) { setFanSpeed(fan, value); },
									 .asker = nullptr});
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal(
			"fan.set_speed",
			"Set a fan's speed in percent. In a row of `om.fans` the fan is the row's, and the speed "
			"the edited value (`value_confirmed`), else `arg` (e.g. 0 or 100 for off and full). "
			"Elsewhere `arg` is the fan's number, and the speed the edited value. Without a speed "
			"it is asked for on the number pad. Does nothing for a thermostatic fan, which "
			"follows its sensors.",
			"tr.actions.fan_set_speed",
			setSpeed);
	}
} // namespace UI::FanActions
