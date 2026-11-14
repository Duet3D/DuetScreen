/*
 * ObjectModel.cpp
 *
 *  Created on: 7 Sep 2020
 *      Author: manuel
 */
#include "Debug.h"
#include "ObjectModel/Tree/Tree.h"

// #include "UI/UserInterface.h"

#include "Alert.h"
#include "Axis.h"
#include "BedOrChamber.h"
#include "Configuration.h"
#include "DebugCommands.h"
#include "Directories.h"
#include "Files.h"
#include "Heightmap.h"
#include "Job.h"
#include "ListHelpers.h"
#include "PrinterStatus.h"
#include "Spindle.h"
#include "Tool.h"
#include "UI/Core/Model.h"
#include "Utils.h"
#include "nameof.hpp"
#include "utils/utils.h"

namespace OM
{
	void RemoveAll()
	{
		MODEL_LOCK();
		g_currentAlert.Reset();
		g_lastAlertSeq = 0;
		Move::Reset();
		RemoveBed(0, true);
		RemoveChamber(0, true);
		RemoveFan(0, true);
		Heat::RemoveHeater(0, true);
		RemoveAnalogSensor(0, true);
		RemoveEndstop(0, true);
		RemoveSpindle(0, true);
		RemoveTool(0, true);
		ClearCurrentHeightmap();
		ClearHeightmapCache();
		FileSystem::ClearFileSystem();
		SetPrinterName("");
		SetPrinterUniqueId("");
		Tree::get().clear();
	}

	static Debug::DebugCommand s_logOM(
		"dbg_log_OM",
		[]()
		{
			LOG_INFO("ObjectModel:");
			LOG_INFO("  Axes:");
			for (size_t i = 0; i < MAX_TOTAL_AXES; ++i)
			{
				auto axis = Move::GetAxis(i);
				if (axis != nullptr)
				{
					LOG_INFO("    [{:d}]: ({:c}), babystep({:g}), user({:g}), machine({:g}), homed({})",
							 axis->index,
							 axis->letter[0],
							 axis->babystep,
							 axis->userPosition,
							 axis->machinePosition,
							 axis->homed != 0);
				}
			}
			LOG_INFO("  Extruders:");
			for (size_t i = 0; i < MAX_TOTAL_AXES; ++i)
			{
				auto extruder = Move::GetExtruderAxis(i);
				if (extruder != nullptr)
				{
					LOG_INFO("    [{:d}]: pos({:g}), f({:g}), spmm({:g}), pa({:g}), fil({:g}, {:s})",
							 extruder->index,
							 extruder->position,
							 extruder->factor,
							 extruder->stepsPerMm,
							 extruder->pressureAdvance,
							 extruder->filamentDiameter,
							 extruder->filamentName.c_str());
				}
			}
			LOG_INFO("  Beds:");
			for (size_t i = 0; i < MAX_SLOTS; ++i)
			{
				auto bed = GetBed(i);
				if (bed != nullptr)
				{
					LOG_INFO("    [{:d}]: heater({:d})", bed->index, bed->heater);
				}
			}
			LOG_INFO("  Chambers:");
			for (size_t i = 0; i < MAX_SLOTS; ++i)
			{
				auto chamber = GetChamber(i);
				if (chamber != nullptr)
				{
					LOG_INFO("    [{:d}]: heater({:d})", chamber->index, chamber->heater);
				}
			}
			LOG_INFO("  Fans:");
			for (size_t i = 0; i < MAX_FANS; ++i)
			{
				auto fan = GetFan(i);
				if (fan != nullptr)
				{
					LOG_INFO("    [{:d}]: actual({:g}), requested({:g}), rpm({:d})",
							 fan->index,
							 fan->actualValue,
							 fan->requestedValue,
							 fan->rpm);
				}
			}
			LOG_INFO("  Heaters:");
			for (size_t i = 0; i < MAX_HEATERS; ++i)
			{
				auto heater = Heat::GetHeater(i);
				if (heater != nullptr)
				{
					LOG_INFO("    [{:d}]: status({:d}), active({:d}), standby({:d}), current({:g}),",
							 heater->index,
							 (int)heater->status,
							 heater->activeTemp,
							 heater->standbyTemp,
							 heater->current);
					LOG_INFO("             min({:g}), max({:g}), pwm({:g}), sensor({:d})",
							 heater->min,
							 heater->max,
							 heater->avgPwm,
							 heater->sensor == nullptr ? -1 : heater->sensor->index);
				}
			}
			LOG_INFO("  Job:");
			LOG_INFO("    name: {:s}", GetJobName().c_str());
			LOG_INFO("    Print time: {:d}", GetPrintTime());
			LOG_INFO("    Print duration: {:d}", GetPrintDuration());
			LOG_INFO("    Print remaining (slicer): {:d}", GetPrintRemaining(OM::RemainingTimeType::SLICER));
			LOG_INFO("    Current object: {:d}", GetCurrentJobObjectIndex());
			LOG_INFO("    Objects:");
			for (size_t i = 0; i < MAX_TRACKED_OBJECTS; ++i)
			{
				auto jobObject = GetJobObject(i);
				if (jobObject != nullptr)
				{
					LOG_INFO("      [{:d}]: cancelled({:d}), name({:s})",
							 jobObject->index,
							 jobObject->cancelled,
							 jobObject->name.c_str());
					LOG_INFO("            bounds({:d}, {:d}, {:d}, {:d})",
							 jobObject->bounds.x[0],
							 jobObject->bounds.x[1],
							 jobObject->bounds.y[0],
							 jobObject->bounds.y[1]);
				}
			}
			LOG_INFO("  Sensors:");
			for (size_t i = 0; i < MAX_SENSORS; ++i)
			{
				auto sensor = GetAnalogSensor(i);
				if (sensor != nullptr)
				{
					LOG_INFO("    [{:d}]: {:s}, {:g} @ time({}ms)",
							 sensor->index,
							 sensor->name.c_str(),
							 sensor->lastReading,
							 sensor->lastReadingTime);
				}
			}
			LOG_INFO("  Endstops:");
			for (size_t i = 0; i < MAX_ENDSTOPS; ++i)
			{
				auto endstop = GetEndstop(i);
				if (endstop != nullptr)
				{
					LOG_INFO("    [{:d}]: triggered({:d})", endstop->index, endstop->triggered);
				}
			}
			LOG_INFO("  Spindles:");
			for (size_t i = 0; i < MAX_SLOTS; ++i)
			{
				auto spindle = GetSpindle(i);
				if (spindle != nullptr)
				{
					LOG_INFO(
						"    [{:d}]: state({:d}), active({:d}), current({:d}), min({:d}), max({:d}), canReverse({:d})",
						spindle->index,
						(int)spindle->state,
						spindle->active,
						spindle->current,
						spindle->min,
						spindle->max,
						spindle->canReverse);
				}
			}
			LOG_INFO("  Tools:");
			for (size_t i = 0; i < MAX_SLOTS; ++i)
			{
				auto tool = GetTool(i);
				if (tool != nullptr)
				{
					LOG_INFO("    [{:d}]: {:s}, status({}), filamentExtruder({:d}), spindle({:d})",
							 tool->index,
							 tool->name.c_str(),
							 (int)tool->status,
							 tool->filamentExtruder,
							 tool->spindle == nullptr ? -1 : tool->spindle->index);
					for (size_t j = 0; j < MAX_HEATERS_PER_TOOL; ++j)
					{
						auto th = tool->GetHeater(j);
						if (th != nullptr)
						{
							LOG_INFO("      ToolHeater[{:d}]: active({:d}), standby({:d}), heater({:d})",
									 j,
									 th->activeTemp,
									 th->standbyTemp,
									 th->heater == nullptr ? -1 : th->heater->index);
						}
					}
					std::string extruders = "      Extruders: [";
					bool first = true;
					for (size_t j = 0; j < MAX_EXTRUDERS_PER_TOOL; ++j)
					{
						auto ea = tool->GetExtruder(j);
						if (ea != nullptr)
						{
							extruders += fmt::format("{:s}{:d}", first ? "" : ", ", ea->index);
							first = false;
						}
					}
					extruders += "]";
					LOG_INFO("{:s}", extruders.c_str());

					std::string fans = "      Fans: [";
					first = true;
					for (size_t j = 0; j < MAX_FANS; ++j)
					{
						auto fan = tool->GetFan(j);
						if (fan != nullptr)
						{
							fans += fmt::format("{:s}{:d}", first ? "" : ", ", fan->index);
							first = false;
						}
					}
					fans += "]";
					LOG_INFO("{:s}", fans.c_str());
				}
			}
			LOG_INFO("  Printer Name: {:s}", OM::GetPrinterName().c_str());
			LOG_INFO("  Printer State: {:s}({})", OM::GetStatusText(), nameof::nameof_enum(OM::GetStatus()));

			LOG_INFO("\n");
		});
} // namespace OM
