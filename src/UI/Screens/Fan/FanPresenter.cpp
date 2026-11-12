/*
 * FanPresenter.cpp
 */

#include "FanPresenter.h"
#include "Debug.h"
#include "ObjectModel/Fan.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/FanSource.h"
#include "UI/Xml/Runtime/Services.h"
#include <algorithm>
#include <cmath>

namespace UI
{
	FanPresenter::FanPresenter()
		: XmlPresenter("fans")
	{
		action("fans.set_speed", [this](const Xml::ActionArgs& args) { setSpeed(args); });
	}

	void FanPresenter::setSpeed(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		const Xml::FanRow* row = args.row<Xml::FanRow>();
		if (row == nullptr)
		{
			LOG_WARN("fans.set_speed must come from a row of the fans list");
			return;
		}

		const auto value = args.value();
		if (!value)
		{
			LOG_WARN("fans.set_speed needs a value");
			return;
		}

		const int32_t percent = std::clamp(static_cast<int32_t>(std::lround(*value)), 0, 100);
		const int32_t number = row->number.get();

		// Don't send what the fan is already set to
		{
			MODEL_LOCK();
			if (auto fan = OM::GetFan(static_cast<size_t>(number));
				fan && static_cast<int32_t>(std::lround(100 * fan->requestedValue)) == percent)
			{
				return;
			}
		}

		Xml::Services::sendGcode(fmt::format("M106 P{:d} S{:d}", number, 255 * percent / 100));
	}
} // namespace UI
