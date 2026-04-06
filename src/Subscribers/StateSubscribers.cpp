#include "Debug.h"

#include "Configuration.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Alert.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tool.h"
#include "StateSubscribers.h"
#include "UI/Core/Model.h"
#include "utils/TimeHelper.h"

bool StateSubscribers::networkName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetPrinterName(data);
	return true;
}

bool StateSubscribers::networkActualIP(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	Model::get().post<EventType::IpAddress>(std::string(data));
	return true;
}

bool StateSubscribers::status(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetStatus(data);
	return true;
}

bool StateSubscribers::currentTool(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetCurrentTool(data);
	Model::get().post<EventType::CurrentTool>();
	return true;
}

bool StateSubscribers::nullMessageBox(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	if (data != nullptr)
		return true;
	OM::g_currentAlert.Reset();

	Model::get().post<EventType::Alert>(OM::g_currentAlert);
	return true;
}

bool StateSubscribers::messageBoxAxisControls(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::g_currentAlert.controls = data;
	OM::g_currentAlert.flags.SetBit(OM::Alert::GotControls);
	return true;
}

bool StateSubscribers::messageBoxMessage(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::g_currentAlert.text.copy(data);
	OM::g_currentAlert.flags.SetBit(OM::Alert::GotText);
	return true;
}

bool StateSubscribers::messageBoxMode(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::g_currentAlert.mode = static_cast<OM::Alert::Mode>(data);
	OM::g_currentAlert.flags.SetBit(OM::Alert::GotMode);
	return true;
}

bool StateSubscribers::messageBoxSeq(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::g_currentAlert.seq = data;
	OM::g_currentAlert.flags.SetBit(OM::Alert::GotSeq);
	return true;
}

bool StateSubscribers::messageBoxTimeout(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::g_currentAlert.timeout = std::chrono::milliseconds(data);
	OM::g_currentAlert.flags.SetBit(OM::Alert::GotTimeout);
	return true;
}

bool StateSubscribers::messageBoxTitle(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::g_currentAlert.title.copy(data);
	OM::g_currentAlert.flags.SetBit(OM::Alert::GotTitle);

	if (OM::g_currentAlert.seq != OM::g_lastAlertSeq)
	{
		LOG_DBG("New message box alert: '{}', seq={}", OM::g_currentAlert.title.c_str(), OM::g_currentAlert.seq);
		OM::g_lastAlertSeq = OM::g_currentAlert.seq;
		Model::get().post<EventType::Alert>(OM::g_currentAlert);
	}
	return true;
}

bool StateSubscribers::messageBoxMin(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	if (data == nullptr)
	{
		OM::g_currentAlert.limits.numberInt.min = std::numeric_limits<int32_t>::min();
		OM::g_currentAlert.limits.numberFloat.min = std::numeric_limits<float>::lowest();
		OM::g_currentAlert.limits.text.min = 0;
		return true;
	}
	Comm::GetInteger(data, OM::g_currentAlert.limits.numberInt.min);
	Comm::GetFloat(data, OM::g_currentAlert.limits.numberFloat.min);
	Comm::GetInteger(data, OM::g_currentAlert.limits.text.min);
	return true;
}

bool StateSubscribers::messageBoxMax(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	if (data == nullptr)
	{
		OM::g_currentAlert.limits.numberInt.max = std::numeric_limits<int32_t>::max();
		OM::g_currentAlert.limits.numberFloat.max = std::numeric_limits<float>::max();
		OM::g_currentAlert.limits.text.max = std::numeric_limits<int32_t>::max();
		return true;
	}
	Comm::GetInteger(data, OM::g_currentAlert.limits.numberInt.max);
	Comm::GetFloat(data, OM::g_currentAlert.limits.numberFloat.max);
	Comm::GetInteger(data, OM::g_currentAlert.limits.text.max);
	return true;
}

bool StateSubscribers::messageBoxDefault(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	if (data == nullptr)
	{
		OM::g_currentAlert.limits.numberInt.valueDefault = 0;
		OM::g_currentAlert.limits.numberFloat.valueDefault = 0.0;
		OM::g_currentAlert.limits.text.valueDefault.Clear();
		return true;
	}
	Comm::GetInteger(data, OM::g_currentAlert.limits.numberInt.valueDefault);
	Comm::GetFloat(data, OM::g_currentAlert.limits.numberFloat.valueDefault);
	OM::g_currentAlert.limits.text.valueDefault.copy(data);
	return true;
}

bool StateSubscribers::messageBoxCancelButton(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::g_currentAlert.cancelButton = data;
	return true;
}

bool StateSubscribers::messageBoxChoices(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (indices[0] >= ALERT_MAX_CHOICES)
	{
		LOG_ERROR("Too many choices in message box");
		return false;
	}
	OM::g_currentAlert.choices[indices[0]].copy(data);
	OM::g_currentAlert.choices_count = indices[0] + 1;
	return true;
}

bool StateSubscribers::time(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	static std::chrono::milliseconds lastUpdated = 0ms;

	if (data == nullptr)
	{
		return true;
	}

	const auto now = TimeHelper::getRunningTime();
	if (now - lastUpdated < TIME_SYNC_INTERVAL)
	{
		return true;
	}
	LOG_DBG("Setting system time to {:s}", data);
	TimeHelper::setDateTime(data);
	lastUpdated = now;
	Model::get().post<EventType::Time>();
	return true;
}

bool StateSubscribers::inputChannel(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	/* This will not be triggered by a `rr_model` HTTP request,
	 * it needs to be requested explicitly with `M409 K"state" F"vn"` */
	OM::SetChannelIndex(data);
	return true;
}
