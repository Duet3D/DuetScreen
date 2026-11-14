/*
 * Constants.cpp
 */

#include "Constants.h"
#include "Subjects.h"
#include "UI/Xml/Runtime/Translations.h"
#include <charconv>
#include <map>
#include <memory>
#include <string>

namespace UI::Xml::Constants
{
	namespace
	{
		/** The longest text a text or translation constant holds, in bytes */
		constexpr size_t TEXT_SIZE = 256;

		/** A text or translation constant's subject */
		using TextSubject = StringSubject<TEXT_SIZE>;

		/** Every constant made, by name as written */
		std::map<std::string, std::unique_ptr<Subject>, std::less<>>& made()
		{
			static std::map<std::string, std::unique_ptr<Subject>, std::less<>> s_made;
			return s_made;
		}

		/** `name` as a whole number, if it is one */
		std::optional<int32_t> numberOf(std::string_view name)
		{
			int32_t value = 0;
			const char* end = name.data() + name.size();
			const auto [ptr, error] = std::from_chars(name.data(), end, value);
			if (name.empty() || error != std::errc() || ptr != end)
			{
				return std::nullopt;
			}
			return value;
		}

		/** A new subject for the constant `name`, of kind `kind` */
		std::unique_ptr<Subject> make(std::string_view name, Kind kind)
		{
			switch (kind)
			{
			case Kind::number:
				return std::make_unique<IntSubject>(*numberOf(name));
			case Kind::text:
				return std::make_unique<TextSubject>(name.substr(1, name.size() - 2));
			case Kind::translation:
				return std::make_unique<TextSubject>(Translations::translate(name));
			}
			return nullptr;
		}
	} // namespace

	std::optional<Kind> kindOf(std::string_view name)
	{
		if (numberOf(name))
		{
			return Kind::number;
		}
		if (name.size() >= 2 && name.front() == '\'' && name.back() == '\'')
		{
			return Kind::text;
		}
		if (const auto key = Translations::key(name); key && !key->empty())
		{
			return Kind::translation;
		}
		return std::nullopt;
	}

	Subject* find(std::string_view name)
	{
		auto it = made().find(name);
		if (it != made().end())
		{
			return it->second.get();
		}
		const auto kind = kindOf(name);
		if (!kind)
		{
			return nullptr;
		}
		return made().emplace(std::string(name), make(name, *kind)).first->second.get();
	}

	void retranslate()
	{
		for (auto& [name, subject] : made())
		{
			if (kindOf(name) == Kind::translation)
			{
				static_cast<TextSubject&>(*subject).set(Translations::translate(name));
			}
		}
	}

	void clear()
	{
		made().clear();
	}
} // namespace UI::Xml::Constants
