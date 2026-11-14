/*
 * HeaterRow.h
 *
 * The fields every heater row has, whether the heater belongs to a tool, a bed or a chamber: what needs working out
 * (its name, the scale of a bar for it, and its targets, which are the tool's own for a tool's heater). Its state and
 * temperature are the object model's, `om.heat.heaters[{heater}]`. The fields are declared in fields(), and listed in
 * the UI API reference (docs/XML_UI_API.md) under each list of heaters.
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include <string_view>

namespace UI::Xml
{
	class HeaterRow : public Row
	{
	  public:
		/** What a heater row shows */
		struct Values
		{
			int32_t heater;		   // Heater number
			std::string_view name; // Display name
			int32_t active;		   // Active target, °C
			int32_t standby;	   // Standby target, °C
			float min;			   // Lowest temperature the heater allows, °C
			float max;			   // Highest temperature the heater allows, °C
		};

		/** Declare the heater fields */
		void fields(FieldVisitor& f) override;

		/** Show `values` */
		void update(const Values& values);

		IntSubject heater;			 // Heater number
		StringSubject<32> name;		 // Display name
		IntSubject scaleMin;		 // Bottom of a temperature scale, °C
		IntSubject scaleMax;		 // Top of a temperature scale, °C
		IntSubject active;			 // Active target, °C
		IntSubject standby;			 // Standby target, °C
	};
} // namespace UI::Xml
