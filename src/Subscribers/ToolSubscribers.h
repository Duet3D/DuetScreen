#pragma once

#include "Subscribers/Subscribers.h"

class ToolSubscribers : public SubscriberMap
{
  public:
	ToolSubscribers()
	{
		addSubscriber("tools^", nullTool);
		addSubscriber("tools^:heaters^", toolHeater);
		addSubscriber("tools^:extruders^", toolExtruder);
		addSubscriber("tools^:fans^", toolFan);
		addSubscriber("tools^:filamentExtruder", toolFilamentExtruder);
		addSubscriber("tools^:active^", toolActiveTemp);
		addSubscriber("tools^:standby^", toolStandbyTemp);
		addSubscriber("tools^:spindle", toolSpindle);
		addSubscriber("tools^:spindleRpm", toolSpindleRpm);
		addSubscriber("tools^:name", toolName);
		addSubscriber("tools^:mix^", toolMix);
		addSubscriber("tools^:offsets^", toolOffset);
		addSubscriber("tools^:state", toolState);

		addArrayEndSubscriber("tools^", toolArrayEnd);
		addArrayEndSubscriber("tools^:heaters^", toolHeaterArrayEnd);
		addArrayEndSubscriber("tools^:extruders^", toolExtruderArrayEnd);
		addArrayEndSubscriber("tools^:fans^", toolFanArrayEnd);
		addArrayEndSubscriber("tools^:offsets^", toolOffsetArrayEnd);
	}

  private:
	static bool nullTool(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool toolHeater(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool toolExtruder(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool toolFan(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool toolFilamentExtruder(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool toolActiveTemp(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool toolStandbyTemp(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool toolSpindle(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool toolSpindleRpm(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool toolName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool toolMix(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool toolOffset(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool toolState(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);

	static bool toolArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
	static bool toolHeaterArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
	static bool toolExtruderArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
	static bool toolFanArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
	static bool toolOffsetArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
};
