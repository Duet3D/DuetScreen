#include "Debug.h"

#include "Alert.h"
#include "Tree/Node.h"
#include "utils/utils.h"
#include <algorithm>
#include <string>

namespace OM
{
	void Alert::Reset()
	{
		LOG_VERBOSE("Resetting alert");
		mode = Mode::None;
		flags.Clear();
		for (size_t i = 0; i < ARRAY_SIZE(choices); i++)
		{
			choices[i].Clear();
		}
		timeout = std::chrono::milliseconds(0);
		limits.numberInt.min = std::numeric_limits<int32_t>::min();
		limits.numberInt.max = std::numeric_limits<int32_t>::max();
		limits.numberFloat.min = std::numeric_limits<float>::lowest();
		limits.numberFloat.max = std::numeric_limits<float>::max();
		limits.text.min = 0;
		limits.text.max = std::numeric_limits<int32_t>::max();
	}

	Alert Alert::fromObjectModel(const Node& box)
	{
		Alert alert;
		if (!box.isObject())
		{
			return alert;
		}
		// Each field the box has is noted in `flags`, as the alert dialog asks which arrived
		const auto got = [&](const char* field, uint8_t flag)
		{
			if (box[field].exists())
			{
				alert.flags.SetBit(flag);
			}
		};
		alert.mode = static_cast<Mode>(box["mode"].integer(static_cast<int64_t>(Mode::None)));
		got("mode", GotMode);
		alert.seq = static_cast<uint32_t>(box["seq"].integer(0));
		got("seq", GotSeq);
		alert.timeout = std::chrono::milliseconds(box["timeout"].integer(0));
		got("timeout", GotTimeout);
		alert.title.copy(std::string(box["title"].text()).c_str());
		got("title", GotTitle);
		alert.text.copy(std::string(box["message"].text()).c_str());
		got("message", GotText);
		alert.controls = static_cast<uint32_t>(box["axisControls"].integer(0));
		got("axisControls", GotControls);
		alert.cancelButton = box["cancelButton"].boolean(false);

		// min, max and default mean what the mode makes of them; null leaves the widest limits
		if (const Node& min = box["min"]; min.exists())
		{
			alert.limits.numberInt.min = static_cast<int32_t>(min.integer(0));
			alert.limits.numberFloat.min = static_cast<float>(min.number(0));
			alert.limits.text.min = static_cast<int32_t>(min.integer(0));
		}
		if (const Node& max = box["max"]; max.exists())
		{
			alert.limits.numberInt.max = static_cast<int32_t>(max.integer(0));
			alert.limits.numberFloat.max = static_cast<float>(max.number(0));
			alert.limits.text.max = static_cast<int32_t>(max.integer(0));
		}
		alert.limits.numberInt.valueDefault = 0;
		alert.limits.numberFloat.valueDefault = 0.0f;
		alert.limits.text.valueDefault.Clear();
		if (const Node& value = box["default"]; value.exists())
		{
			alert.limits.numberInt.valueDefault = static_cast<int32_t>(value.integer(0));
			alert.limits.numberFloat.valueDefault = static_cast<float>(value.number(0));
			alert.limits.text.valueDefault.copy(std::string(value.text()).c_str());
		}

		const Node& choices = box["choices"];
		alert.choices_count = std::min(choices.size(), static_cast<size_t>(ALERT_MAX_CHOICES));
		for (size_t i = 0; i < alert.choices_count; ++i)
		{
			alert.choices[i].copy(std::string(choices[i].text()).c_str());
		}
		return alert;
	}
} // namespace OM
