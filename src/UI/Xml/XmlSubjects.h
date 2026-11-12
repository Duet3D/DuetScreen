/*
 * XmlSubjects.h
 *
 * Subjects are the only way data reaches an XML view. These wrappers own their storage (including string buffers) and
 * only notify observers when the value actually changes, so publishers can write every poll without redrawing.
 */

#pragma once

#include "UI/Components/LVGL/LvSubject.h"
#include "helix_xml.h"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace UI::Xml
{
	class IntSubject
	{
	  public:
		explicit IntSubject(int32_t initial = 0) { m_subject.initInt(initial); }

		/** Set the value; observers are only notified if it changed */
		void set(int32_t value)
		{
			if (m_subject.getInt() != value)
			{
				m_subject.setInt(value);
			}
		}

		int32_t get() const { return m_subject.getInt(); }
		lv_subject_t* raw() { return m_subject; }

	  private:
		LvSubject m_subject;
	};

	/**
	 * @tparam N Buffer size including the terminator. Longer values are truncated.
	 */
	template <size_t N = 64>
	class StringSubject
	{
		static_assert(N > 1);

	  public:
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

		std::string_view get() const { return m_subject.getString(); }
		lv_subject_t* raw() { return m_subject; }

	  private:
		static std::string_view truncate(std::string_view value) { return value.substr(0, N - 1); }

		// Buffers are declared before the subject so they outlive it
		std::array<char, N> m_buf{};
		std::array<char, N> m_prev{};
		LvSubject m_subject;
	};

	/**
	 * A set of named subjects that XML bindings resolve against while a view is being built. Contexts chain: a list
	 * item's context has the screen's context as its parent. See libraries/helix-xml/src/xml/lv_xml_subject_ctx.h.
	 */
	class SubjectContext
	{
	  public:
		explicit SubjectContext(lv_xml_subject_ctx_t* parent = nullptr)
			: m_ctx(lv_xml_subject_ctx_create(parent))
		{
		}

		~SubjectContext() { lv_xml_subject_ctx_delete(m_ctx); }

		SubjectContext(const SubjectContext&) = delete;
		SubjectContext& operator=(const SubjectContext&) = delete;

		void add(const std::string& name, lv_subject_t* subject) { lv_xml_subject_ctx_add(m_ctx, name.c_str(), subject); }

		template <typename S>
			requires requires(S& s) {
				{ s.raw() } -> std::same_as<lv_subject_t*>;
			}
		void add(const std::string& name, S& subject)
		{
			add(name, subject.raw());
		}

		lv_xml_subject_ctx_t* get() const { return m_ctx; }

	  private:
		lv_xml_subject_ctx_t* m_ctx;
	};
} // namespace UI::Xml
