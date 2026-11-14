/*
 * NumberPad.cpp
 */

#include "NumberPad.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "i18n/i18n.h"
#include "utils/utils.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fmt/format.h>
#include <limits>

namespace UI::Xml
{
	namespace
	{
		/** The most characters a number pad shows */
		constexpr size_t MAX_TYPED = 20;

		/** The most decimals a number shows: a float holds no more that a user means */
		constexpr int DECIMALS = 3;

		/** Beyond this, a limit is none: anything can be typed that way (a slider that may go past its end) */
		constexpr float NO_LIMIT = 1e9f;

		/** `text` as a number, as typed: empty is 0, and it is kept within what a float holds */
		float parse(std::string_view text)
		{
			const double value = std::strtod(std::string(text).c_str(), nullptr);
			return static_cast<float>(std::clamp(value,
												 static_cast<double>(std::numeric_limits<float>::lowest()),
												 static_cast<double>(std::numeric_limits<float>::max())));
		}
	} // namespace

	NumberPad::NumberPad(XmlPresenter& owner, std::string id)
		: m_id(std::move(id))
	{
		owner.action(m_id + ".key",
					 "Type the key `arg`: a digit or a dot. The first key replaces the value the pad starts with.",
					 [this](const ActionArgs& args)
					 {
						 if (args.arg.size() == 1)
						 {
							 type(args.arg[0]);
						 }
					 });
		owner.action(m_id + ".backspace",
					 "Remove the last character shown, whether typed or the value the pad starts with",
					 [this](const ActionArgs&)
					 {
						 const std::string typed(m_typed.get());
						 setText(typed.empty() ? std::string() : typed.substr(0, typed.size() - 1));
					 });
		owner.action(m_id + ".sign",
					 "Make the number negative, or positive again, if it may be negative",
					 [this](const ActionArgs&)
					 {
						 if (m_signed.get() == 0)
						 {
							 return;
						 }
						 const std::string typed(m_typed.get());
						 setText(typed.starts_with('-') ? typed.substr(1) : "-" + typed);
					 });
		owner.action(m_id + ".clear", "Empty what is typed", [this](const ActionArgs&) { setText(""); });
	}

	void NumberPad::fields(FieldVisitor& f)
	{
		f.field(m_id + ".text", m_typed, "What has been typed");
		f.field(m_id + ".range", m_range, "The numbers that can be typed, e.g. \"0 to 280\"; empty for any");
		f.field(m_id + ".unit", m_unit, "The number's unit, shown after it, e.g. \"s\"; empty for none");
		f.field(m_id + ".confirmable",
				m_confirmable,
				"1 if what has been typed can be used: a number within the limits (nothing typed is 0), or any digits "
				"and dots");
		f.field(m_id + ".signed", m_signed, "1 if the number may be negative, so the sign key works");
		f.field(m_id + ".decimal", m_decimal, "1 if the number may have a fraction, so the dot key works");
	}

	void NumberPad::askNumber(float min, float max, std::optional<float> start, bool whole, std::string_view unit)
	{
		m_number = true;
		m_min = min;
		m_max = max;
		m_signed.set(min < 0.0f ? 1 : 0);
		m_decimal.set(whole ? 0 : 1);
		m_unit.set(unit);
		m_range.set(rangeOf(min, max, unit));
		setText(start ? shown(*start) : std::string());
		m_fresh = start.has_value();
	}

	void NumberPad::askText(std::string_view start)
	{
		m_number = false;
		m_signed.set(0);
		m_decimal.set(1);
		m_unit.set("");
		m_range.set("");
		setText(start);
		m_fresh = !start.empty();
	}

	float NumberPad::value() const
	{
		return parse(m_typed.get());
	}

	bool NumberPad::confirmable() const
	{
		if (!m_number)
		{
			return true;
		}
		const float number = value();
		return number >= m_min && number <= m_max;
	}

	std::string NumberPad::rangeOf(float min, float max, std::string_view unit)
	{
		const bool hasMin = min > -NO_LIMIT;
		const bool hasMax = max < NO_LIMIT;
		const auto limit = [unit](float value)
		{ return unit.empty() ? shown(value) : fmt::format("{:s} {:s}", shown(value), unit); };
		if (hasMin && hasMax)
		{
			return _("dialogs.range", shown(min), limit(max));
		}
		if (hasMin)
		{
			return _("dialogs.at_least", limit(min));
		}
		return hasMax ? _("dialogs.at_most", limit(max)) : std::string();
	}

	std::string NumberPad::shown(float value)
	{
		return utils::formatFloat(value, DECIMALS);
	}

	void NumberPad::type(char key)
	{
		const bool dot = key == '.';
		if (!std::isdigit(static_cast<unsigned char>(key)) && !dot)
		{
			return;
		}
		std::string typed = m_fresh ? std::string() : std::string(m_typed.get());
		// A number has one dot at most, and a whole number none; digits and dots any number of them
		if (dot && m_number && (m_decimal.get() == 0 || typed.find('.') != std::string::npos))
		{
			return;
		}
		if (typed.size() < MAX_TYPED)
		{
			typed += key;
		}
		setText(typed);
	}

	void NumberPad::setText(std::string_view text)
	{
		m_fresh = false;
		m_typed.set(text);
		m_confirmable.set(confirmable() ? 1 : 0);
		if (m_onChange)
		{
			m_onChange();
		}
	}
} // namespace UI::Xml
