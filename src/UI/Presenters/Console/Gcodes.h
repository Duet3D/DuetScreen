/*
 * Gcodes.h
 *
 *  Created on: 2 Jan 2024
 *      Author: Andy Everitt
 */

#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

/** A G-code the console offers */
struct gcode
{
	std::string_view gcode;	  // e.g. "G28"
	std::string_view helpKey; // Translation key of what it does; translate it when it is shown (see i18n.h)
};

class Gcodes
{
  public:
	static size_t getGcodeCount();
	static const gcode* getGcode(size_t index);

  private:
	Gcodes();
	static Gcodes& get()
	{
		static Gcodes instance;
		return instance;
	}
	const std::vector<gcode> m_gcodes;
};
