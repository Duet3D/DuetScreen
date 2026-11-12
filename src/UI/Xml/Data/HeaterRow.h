/*
 * HeaterRow.h
 *
 * The fields every heater row has, whether the heater belongs to a tool, a bed or a chamber:
 *
 *     heater          heater number
 *     name            display name, e.g. "Heater 1" or "Bed"
 *     state           status key: active, standby, off, fault, tuning, offline or unknown
 *     status          status, translated for display
 *     current         current temperature as text, one decimal place, e.g. "21.2"
 *     active          active target temperature, °C
 *     standby         standby target temperature, °C
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
			std::string_view state; // Status key, e.g. "active"
			float current;		   // Current temperature, °C
			int32_t active;		   // Active target, °C
			int32_t standby;	   // Standby target, °C
		};

		/** Declare the heater fields */
		void fields(FieldVisitor& f) override;

		/** Show `values` */
		void update(const Values& values);

		IntSubject heater;			 // Heater number
		StringSubject<32> name;		 // Display name
		StringSubject<16> state;	 // Status key
		StringSubject<32> status;	 // Translated status
		StringSubject<16> current;	 // Current temperature, one decimal place
		IntSubject active;			 // Active target, °C
		IntSubject standby;			 // Standby target, °C
	};
} // namespace UI::Xml
