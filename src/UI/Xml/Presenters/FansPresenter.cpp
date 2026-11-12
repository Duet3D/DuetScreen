/*
 * FansPresenter.cpp
 */

#include "FansPresenter.h"
#include "Debug.h"
#include "ObjectModel/Fan.h"
#include "UI/Xml/OmPublisher.h"
#include "UI/Xml/XmlUi.h"
#include <algorithm>
#include <cmath>

namespace UI::Xml
{
	FansPresenter::FansPresenter()
		: XmlPresenter("fans")
	{
		action("fans.set_speed", [this](const ActionArgs& args) { setSpeed(args); });
	}

	void FansPresenter::setSpeed(const ActionArgs& args)
	{
		ZoneScoped;
		OmPublisher* publisher = OmPublisher::get();
		if (!args.item || publisher == nullptr || *args.item >= publisher->fans().size())
		{
			LOG_WARN("fans.set_speed needs to come from an om.fans list item");
			return;
		}

		const auto value = args.value();
		if (!value)
		{
			LOG_WARN("fans.set_speed needs a value");
			return;
		}

		const int32_t percent = std::clamp(static_cast<int32_t>(std::lround(*value)), 0, 100);
		const auto number = static_cast<size_t>(publisher->fans()[*args.item].number.get());

		// Don't send what the fan is already set to
		if (auto fan = OM::GetFan(number);
			fan && static_cast<int32_t>(std::lround(100 * fan->requestedValue)) == percent)
		{
			return;
		}

		sendGcode(fmt::format("M106 P{:d} S{:d}", number, 255 * percent / 100));
	}
} // namespace UI::Xml
