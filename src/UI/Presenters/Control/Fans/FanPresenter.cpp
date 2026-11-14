/*
 * FanPresenter.cpp
 */

#include "FanPresenter.h"
#include "Debug.h"
#include "ObjectModel/Tree/Tree.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Om/OmList.h"
#include "UI/Xml/Runtime/Services.h"
#include <algorithm>
#include <cmath>

namespace UI
{
	FanPresenter::FanPresenter()
		: XmlPresenter("fans")
	{
		action("fans.set_speed",
			   "Set the fan's speed in percent: the edited value (`value_confirmed`), else `arg`. From a row of "
			   "`om.fans`.",
			   [this](const Xml::ActionArgs& args) { setSpeed(args); });
	}

	void FanPresenter::setSpeed(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		const Xml::OmRow* row = args.row<Xml::OmRow>();
		if (row == nullptr)
		{
			LOG_WARN("fans.set_speed must come from a row of the om.fans list");
			return;
		}

		const auto value = args.value();
		if (!value)
		{
			LOG_WARN("fans.set_speed needs a value");
			return;
		}

		setFanSpeed(row->index.get(), *value);
	}

	void setFanSpeed(int32_t number, float value)
	{
		const int32_t percent = std::clamp(static_cast<int32_t>(std::lround(value)), 0, 100);

		// Don't send what the fan is already set to
		{
			MODEL_LOCK();
			const OM::Node& fan = OM::Tree::get().root()["fans"][static_cast<size_t>(std::max(number, 0))];
			if (fan.exists() && static_cast<int32_t>(std::lround(100 * fan["requestedValue"].number())) == percent)
			{
				return;
			}
		}

		Xml::Services::sendGcode(fmt::format("M106 P{:d} S{:d}", number, 255 * percent / 100));
	}
} // namespace UI
