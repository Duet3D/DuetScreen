#include "Debug.h"

#include "ObjectModel/Tool.h"
#include "ToolSubscribers.h"
#include "UI/Core/Model.h"

bool ToolSubscribers::nullTool(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(data);
	if (OM::RemoveTool(indices[0], false) > 0)
	{
		Model::get().post<EventType::ToolData>();
	}
	return true;
}

bool ToolSubscribers::toolHeater(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolHeater(indices[0], indices[1], (uint8_t)data))
	{
		LOG_ERROR("Failed to update tool {:d} heater {:d}", indices[0], indices[1]);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolExtruder(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolExtruder(indices[0], indices[1], (uint8_t)data))
	{
		LOG_ERROR("Failed to update tool {:d} extruder {:d}", indices[0], indices[1]);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolFan(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolFan(indices[0], indices[1], (uint8_t)data))
	{
		LOG_ERROR("Failed to update tool {:d} fan {:d}", indices[0], indices[1]);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolFilamentExtruder(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolFilamentExtruder(indices[0], (int8_t)data))
	{
		LOG_ERROR("Failed to update tool {:d} filamentExtruder to {:d}", indices[0], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolActiveTemp(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolTemp(indices[0], indices[1], data, true))
	{
		LOG_ERROR("Failed to update tool {:d} active temperature[{:d}] to {:d}", indices[0], indices[1], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolStandbyTemp(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolTemp(indices[0], indices[1], data, false))
	{
		LOG_ERROR("Failed to update tool {:d} standby temperature[{:d}] to {:d}", indices[0], indices[1], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolSpindle(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolSpindle(indices[0], static_cast<int8_t>(data)))
	{
		LOG_ERROR("Failed to update tool {:d} spindle to {:d}", indices[0], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolSpindleRpm(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolSpindleRpm(indices[0], static_cast<int8_t>(data)))
	{
		LOG_ERROR("Failed to update tool {:d} spindleRpm to {:d}", indices[0], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolName(indices[0], data))
	{
		LOG_ERROR("Failed to update tool {:d} name to {:s}", indices[0], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolMix(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolMix(indices[0], indices[1], data))
	{
		LOG_ERROR("Failed to update tool {:d} mix {:d} to {:g}", indices[0], indices[1], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolOffset(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolOffset(indices[0], indices[1], data))
	{
		LOG_ERROR("Failed to update tool {:d} offset {:d} to {:g}", indices[0], indices[1], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolState(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::UpdateToolStatus(indices[0], data))
	{
		LOG_ERROR("Failed to update tool {:d} name to {:s}", indices[0], data);
		return false;
	}
	return true;
}

bool ToolSubscribers::toolArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (OM::RemoveTool(indices[0], true))
	{
	}
	Model::get().post<EventType::ToolData>();
	return true;
}

bool ToolSubscribers::toolHeaterArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (OM::RemoveToolHeaters(indices[0], indices[1]))
	{
	}
	Model::get().post<EventType::ToolHeaterData>(indices[0]);
	return true;
}

bool ToolSubscribers::toolExtruderArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (OM::RemoveToolExtruders(indices[0], indices[1]))
	{
	}
	return true;
}

bool ToolSubscribers::toolFanArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (OM::RemoveToolFans(indices[0], indices[1]))
	{
	}
	return true;
}

bool ToolSubscribers::toolOffsetArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	for (size_t axisIndex = indices[1]; axisIndex < MAX_TOTAL_AXES; ++axisIndex)
	{
		if (!OM::UpdateToolOffset(indices[0], axisIndex, 0.0f))
		{
			LOG_ERROR("Failed to reset tool {:d} offset {:d} to 0", indices[0], axisIndex);
		}
	}
	Model::get().post<EventType::AxesData>();
	return true;
}
