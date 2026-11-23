/*
 * UnitSystem.cpp
 *
 *  Created on: 2025-10-14
 *      Author: Andy Everitt
 */

#include "UnitSystem.h"
#include "Debug.h"
#include "LockWrapper.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tree/Tree.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <mutex>

namespace Units
{
	/**
	 * @brief Get the current unit system used by the Duet as set with `G20` or `G21`: the distance unit of the input
	 * channel the screen uses (inputs[channel].distanceUnit), metric until the printer says otherwise
	 * @return the current unit system used by the Duet
	 */
	UnitSystem getCurrentDuetUnitSystem()
	{
		ZoneScoped;
		const size_t channel = OM::GetChannelIndex();
		std::lock_guard lock(mutexModel);
		const auto unit = OM::Tree::get().root()["inputs"][channel]["distanceUnit"].text();
		return unit == "in" ? UnitSystem::Imperial : UnitSystem::Metric;
	}

	/**
	 * @brief Get the current displayed unit system
	 * @return the current displayed unit system
	 */
	UnitSystem getCurrentDisplayedUnitSystem()
	{
		ZoneScoped;
		auto id = StorageHelper::getData(ID_UNIT_SYSTEM);
		return id;
	}

	/**
	 * @brief Get the translated distance unit string for the currently displayed unit system
	 * @return the translated distance unit string
	 */
	const std::string& getDisplayedDistanceUnit()
	{
		ZoneScoped;
		switch (getCurrentDisplayedUnitSystem())
		{
		case UnitSystem::Metric:
			return _("units.mm");
		case UnitSystem::Imperial:
			return _("units.inches");
		default:
			LOG_FATAL_THROW("Unsupported unit system");
		}
	}

	/**
	 * @brief Get the translated speed unit string for the currently displayed unit system
	 * @return the translated speed unit string
	 */
	const std::string& getDisplayedSpeedUnit()
	{
		ZoneScoped;
		switch (getCurrentDisplayedUnitSystem())
		{
		case UnitSystem::Metric:
			return _("units.mm_speed");
		case UnitSystem::Imperial:
			return _("units.inches_speed");
		default:
			LOG_FATAL_THROW("Unsupported unit system");
		}
	}

	/**
	 * @brief Convert a metric position (mm) to the currently displayed units
	 * @param distance_mm distance/position in mm
	 * @return the distance/position in the currently displayed units
	 */
	float convertDistanceToCurrentDisplayedUnit(float distance_mm)
	{
		ZoneScoped;
		switch (getCurrentDisplayedUnitSystem())
		{
		case UnitSystem::Metric:
			return distance_mm;
		case UnitSystem::Imperial:
			return mmToInches(distance_mm);
		default:
			LOG_FATAL_THROW("Unsupported unit system");
		}
	}

	/**
	 * @brief Convert a metric speed (mm/s) to the currently displayed units
	 * @param speed_mm_s speed in mm/s
	 * @return the speed in the currently displayed units
	 */
	float convertSpeedToCurrentDisplayedUnit(float speed_mm_s)
	{
		ZoneScoped;
		switch (getCurrentDisplayedUnitSystem())
		{
		case UnitSystem::Metric:
			return speed_mm_s;
		case UnitSystem::Imperial:
			return 60 * mmToInches(speed_mm_s);
		default:
			LOG_FATAL_THROW("Unsupported unit system");
		}
	}

	/**
	 * @brief Convert a distance in the currently displayed units to the units used by the Duet
	 * @param distance value displayed in the UI, as calculated by `convertDistanceToCurrentDisplayedUnit()`
	 * @return the distance in `units` used by the Duet
	 */
	float convertDisplayedDistanceToDuetUnits(float distance)
	{
		ZoneScoped;
		UnitSystem display_units = getCurrentDisplayedUnitSystem();
		UnitSystem duet_units = getCurrentDuetUnitSystem();

		if (display_units == duet_units)
		{
			return distance;
		}

		switch (duet_units)
		{
		case UnitSystem::Metric:
			return inchesToMm(distance);
		case UnitSystem::Imperial:
			return mmToInches(distance);
		default:
			LOG_FATAL_THROW("Unsupported unit system");
		}
	}

	/**
	 * @brief Convert a speed in the currently displayed units to the units used by the Duet
	 * @param speed value displayed in the UI, as calculated by `convertSpeedToCurrentDisplayedUnit()`
	 * @return the speed in `units/min` used by the Duet
	 */
	float convertDisplayedSpeedToDuetUnits(float speed)
	{
		ZoneScoped;
		UnitSystem display_units = getCurrentDisplayedUnitSystem();
		UnitSystem duet_units = getCurrentDuetUnitSystem();

		if (display_units == UnitSystem::Metric)
		{
			/* metric is mm/s which needs to be converted to mm/min */
			speed *= 60;
		}

		if (display_units == duet_units)
		{
			return speed;
		}
		switch (duet_units)
		{
		case UnitSystem::Metric:
			return inchesToMm(speed); // convert inches/min to mm/min
		case UnitSystem::Imperial:
			return mmToInches(speed); // convert mm/min to inches/min
		default:
			LOG_FATAL_THROW("Unsupported unit system");
		}
	}

	float inchesToMm(float inches)
	{
		ZoneScoped;
		return inches * 25.4f;
	}

	float mmToInches(float mm)
	{
		ZoneScoped;
		return mm / 25.4f;
	}

	std::string formatBytes(std::uintmax_t bytes)
	{
		ZoneScoped;
		const char* sizes[] = {"B", "KB", "MB", "GB", "TB"};
		int order = 0;

		if (bytes == 0)
			return "0 B";

		double len = static_cast<double>(bytes);
		while (len >= 1024 && order < 4)
		{
			order++;
			len /= 1024;
		}

		return fmt::format("{:.2f} {:s}", len, sizes[order]);
	}
} // namespace Units
