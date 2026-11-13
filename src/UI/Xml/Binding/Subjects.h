/*
 * Subjects.h
 *
 * Subjects are the only way data reaches an XML view: a widget binds to a subject by name (`bind_text="name"`) and
 * redraws when the subject changes. These wrappers own their storage, including string buffers, and only notify
 * observers when the value really changes, so code that writes every poll doesn't cause redraws.
 */

#pragma once

#include "UI/Lvgl/LvSubject.h"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace UI::Xml
{
	/** Base of every subject type, so scopes and field lists can hold any of them */
	class Subject
	{
	  public:
		virtual ~Subject() = default;

		/** The LVGL subject that widgets bind to */
		virtual lv_subject_t* raw() = 0;

		/** What the subject holds, as the UI API reference names it, e.g. "int" or "text" */
		virtual std::string_view typeName() const = 0;
	};

	/** A whole number, e.g. a fan number, a percentage or a 0/1 flag */
	class IntSubject : public Subject
	{
	  public:
		/** Create the subject holding `initial` */
		explicit IntSubject(int32_t initial = 0) { m_subject.initInt(initial); }

		/** Set the value; observers are only notified if it changed */
		void set(int32_t value)
		{
			if (m_subject.getInt() != value)
			{
				m_subject.setInt(value);
			}
		}

		/** The current value */
		int32_t get() const { return m_subject.getInt(); }

		/** The LVGL subject that widgets bind to */
		lv_subject_t* raw() override { return m_subject; }

		/** "int" */
		std::string_view typeName() const override { return "int"; }

	  private:
		LvSubject m_subject; // The LVGL subject
	};

	/** A colour, e.g. for `<bind_style_prop prop="bg_color" subject="…"/>` */
	class ColorSubject : public Subject
	{
	  public:
		/** Create the subject holding black */
		ColorSubject() { m_subject.initColor(lv_color_black()); }

		/** Set the colour; observers are only notified if it changed */
		void set(lv_color_t color)
		{
			if (!lv_color_eq(m_subject.getColor(), color))
			{
				m_subject.setColor(color);
			}
		}

		/** The current colour */
		lv_color_t get() const { return m_subject.getColor(); }

		/** The LVGL subject that widgets bind to */
		lv_subject_t* raw() override { return m_subject; }

		/** "colour" */
		std::string_view typeName() const override { return "colour"; }

	  private:
		LvSubject m_subject; // The LVGL subject
	};

	/**
	 * Text, e.g. a name or a preformatted temperature
	 * @tparam N Buffer size including the terminator. Longer values are truncated.
	 */
	template <size_t N = 64>
	class StringSubject : public Subject
	{
		static_assert(N > 1);

	  public:
		/** Create the subject holding `initial` */
		explicit StringSubject(std::string_view initial = "")
		{
			const std::string value(truncate(initial));
			m_subject.initString(m_buf.data(), m_prev.data(), N, value.c_str());
		}

		/** Set the value; observers are only notified if it changed */
		void set(std::string_view value)
		{
			value = truncate(value);
			if (get() != value)
			{
				const std::string terminated(value);
				m_subject.copyString(terminated.c_str());
			}
		}

		/** The current value */
		std::string_view get() const { return m_subject.getString(); }

		/** The LVGL subject that widgets bind to */
		lv_subject_t* raw() override { return m_subject; }

		/** "text" */
		std::string_view typeName() const override { return "text"; }

	  private:
		/** Cut `value` to what fits in the buffer */
		static std::string_view truncate(std::string_view value) { return value.substr(0, N - 1); }

		std::array<char, N> m_buf{};  // Current value; declared before the subject so it outlives it
		std::array<char, N> m_prev{}; // Previous value, required by LVGL
		LvSubject m_subject;		  // The LVGL subject
	};
} // namespace UI::Xml
