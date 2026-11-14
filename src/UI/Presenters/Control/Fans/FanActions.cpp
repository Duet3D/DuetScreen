/*
 * FanActions.cpp
 */

#include "FanActions.h"
#include "Debug.h"
#include "FanPresenter.h"
#include "ObjectModel/Tree/Tree.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Om/OmList.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include <cmath>
#include <optional>
#include <string>

namespace UI::FanActions
{
	/** `actions.fan.set_speed` */
	static void setSpeed(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		// The fan: the row's, else the one `arg` names
		std::optional<int32_t> number = args.argInt();
		if (const auto* row = args.row<Xml::OmRow>())
		{
			const auto& segments = row->item().segments();
			const auto* list = segments.size() == 2 ? std::get_if<std::string>(&segments[0]) : nullptr;
			const auto* index = segments.size() == 2 ? std::get_if<size_t>(&segments[1]) : nullptr;
			if (list != nullptr && *list == "fans" && index != nullptr)
			{
				number = static_cast<int32_t>(*index);
			}
		}
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

		if (const auto edited = args.edited())
		{
			setFanSpeed(*number, *edited);
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

	void registerAll()
	{
		Xml::Actions::addGlobal("fan.set_speed",
								"Set a fan's speed in percent: the edited value (`value_confirmed`), else ask for it "
								"on the number pad. The fan is the row's, in a row of `om.fans`, else fan number "
								"`arg`. Does nothing for a thermostatic fan, which follows its sensors.",
								"tr.actions.fan_set_speed",
								setSpeed);
	}
} // namespace UI::FanActions
