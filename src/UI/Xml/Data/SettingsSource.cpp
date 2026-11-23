/*
 * SettingsSource.cpp
 */

#include "SettingsSource.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include <algorithm>
#include <cmath>

namespace UI::Xml
{
	namespace
	{
		/** Every number setting added; a deque, so pointers to them stay good */
		std::deque<Setting>& numbers()
		{
			static std::deque<Setting> s_numbers;
			return s_numbers;
		}

		/** Every text setting added */
		std::deque<TextSetting>& texts()
		{
			static std::deque<TextSetting> s_texts;
			return s_texts;
		}
	} // namespace

	namespace Settings
	{
		void add(Setting setting)
		{
			numbers().push_back(std::move(setting));
		}

		void add(TextSetting setting)
		{
			texts().push_back(std::move(setting));
		}

		std::string lines(const std::vector<std::string>& items)
		{
			std::string result;
			for (const auto& item : items)
			{
				result += result.empty() ? "" : "\n";
				result += item;
			}
			return result;
		}
	} // namespace Settings

	SettingsSource::SettingsSource()
		: DataSource("settings")
	{
		for (const Setting& setting : numbers())
		{
			m_numbers.emplace_back().setting = &setting;
		}
		for (const TextSetting& setting : texts())
		{
			m_texts.emplace_back().setting = &setting;
		}
	}

	void SettingsSource::fields(FieldVisitor& f)
	{
		for (Number& number : m_numbers)
		{
			const Setting& setting = *number.setting;
			const std::string changed = setting.write ? " (actions.settings.set)" : "";
			f.field(setting.name, number.value, setting.description + changed);
			if (setting.options)
			{
				f.field(setting.name + "_options",
						number.options,
						"What `source.settings." + setting.name + "` can be, one per line, translated");
			}
		}
		for (Text& text : m_texts)
		{
			const std::string asked = text.setting->askHeader ? ", actions.settings.ask" : "";
			f.field(text.setting->name, text.value, text.setting->description + " (actions.settings.set" + asked + ")");
		}
	}

	void SettingsSource::onInit()
	{
		registerEventListener<EventType::ThemeChanged>(
			[this]()
			{
				UI_LOCK();
				update();
			});
	}

	void SettingsSource::update()
	{
		ZoneScoped;
		for (Number& number : m_numbers)
		{
			number.value.set(number.setting->read());
			if (number.setting->options)
			{
				number.options.set(number.setting->options());
			}
		}
		for (Text& text : m_texts)
		{
			text.value.set(text.setting->read());
		}
	}

	bool SettingsSource::set(std::string_view name, float edited)
	{
		const auto it = std::find_if(
			m_numbers.begin(), m_numbers.end(), [name](const Number& n) { return n.setting->name == name; });
		if (it == m_numbers.end() || !it->setting->write)
		{
			return false;
		}
		const auto value = static_cast<int32_t>(std::lround(edited * static_cast<float>(it->setting->scale)));
		LOG_INFO("Setting {:s} to {:d}", name, value);
		it->setting->write(value);
		update();
		return true;
	}

	bool SettingsSource::set(std::string_view name, std::string_view value)
	{
		const auto it =
			std::find_if(m_texts.begin(), m_texts.end(), [name](const Text& t) { return t.setting->name == name; });
		if (it == m_texts.end())
		{
			return false;
		}
		it->setting->write(value);
		update();
		return true;
	}

	const TextSetting* SettingsSource::textSetting(std::string_view name) const
	{
		const auto it =
			std::find_if(m_texts.begin(), m_texts.end(), [name](const Text& t) { return t.setting->name == name; });
		return it == m_texts.end() ? nullptr : it->setting;
	}
} // namespace UI::Xml
