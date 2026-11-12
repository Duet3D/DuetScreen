/*
 * HeaterRow.cpp
 */

#include "HeaterRow.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI::Xml
{
	void HeaterRow::fields(FieldVisitor& f)
	{
		f.field("heater", heater, "Heater number");
		f.field("name", name, "Display name, e.g. \"Heater 1\" or \"Bed\"");
		f.field("state", state, "Status key: active, standby, off, fault, tuning, offline or unknown");
		f.field("status", status, "Status, translated for display");
		f.field("current", current, "Current temperature as text, one decimal place");
		f.field("active", active, "Active target temperature, °C");
		f.field("standby", standby, "Standby target temperature, °C");
	}

	void HeaterRow::update(const Values& values)
	{
		heater.set(values.heater);
		name.set(values.name);
		state.set(values.state);
		status.set(_(fmt::format("temperature.status.{:s}", values.state)));
		current.set(fmt::format("{:.1f}", values.current));
		active.set(values.active);
		standby.set(values.standby);
	}
} // namespace UI::Xml
