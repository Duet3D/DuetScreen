/*
 * FanSource.cpp
 */

#include "FanSource.h"
#include "Debug.h"
#include "ObjectModel/Fan.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <cmath>

namespace UI::Xml
{
	/** A 0-1 fraction as a whole percentage */
	static int32_t toPercent(float fraction)
	{
		return static_cast<int32_t>(std::lround(100.0f * fraction));
	}

	void FanRow::fields(FieldVisitor& f)
	{
		f.field("number", number, "Fan number, as used by M106 P");
		f.field("name", name, "Display name, e.g. \"Fan 1\"");
		f.field("requested", requested, "Requested speed, percent 0-100");
		f.field("actual", actual, "Measured speed, percent 0-100");
		f.field("rpm", rpm, "Tachometer reading, -1 if none");
		f.field("thermostatic", thermostatic, "1 if the fan is temperature controlled, else 0");
	}

	void FanRow::update(const OM::Fan& fan)
	{
		number.set(static_cast<int32_t>(fan.index));
		name.set(_("fine_tune.fan", fan.index));
		requested.set(toPercent(fan.requestedValue));
		actual.set(toPercent(fan.actualValue));
		rpm.set(fan.rpm);
		thermostatic.set(fan.thermostatic ? 1 : 0);
	}

	FanSource::FanSource()
		: DataSource("fans")
	{
	}

	void FanSource::fields(FieldVisitor& f)
	{
		f.list("fans", m_fans, "One row per fan, in slot order");
	}

	void FanSource::onInit()
	{
		registerEventListener<EventType::FanData>(this, &FanSource::update);
	}

	void FanSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		m_fans.sync(OM::GetFanCount(),
					[](FanRow& row, size_t slot)
					{
						if (auto fan = OM::GetFanBySlot(slot))
						{
							row.update(*fan);
						}
					});
	}

	void FanSource::clear()
	{
		m_fans.clear();
	}
} // namespace UI::Xml
