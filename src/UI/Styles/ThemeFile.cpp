/*
 * ThemeFile.cpp
 */

#include "ThemeFile.h"
#include "src/libs/expat/expat.h"
#include <charconv>
#include <fmt/format.h>
#include <sstream>

namespace UI::Themes
{
	namespace
	{
		/** expat callbacks build a ThemeFile here */
		struct ParseState
		{
			XML_Parser parser = nullptr;   // The parser, for line numbers and stopping
			ThemeFile theme;			   // What has been read
			std::vector<std::string> path; // Open elements, outermost first
			std::string error;			   // First error, if any
		};

		/** The value of attribute `name`, or nullptr */
		const char* attr(const XML_Char** attrs, std::string_view name)
		{
			for (size_t i = 0; attrs[i]; i += 2)
			{
				if (name == attrs[i])
				{
					return attrs[i + 1];
				}
			}
			return nullptr;
		}

		/** `text` as an unsigned integer, if all of it is one */
		std::optional<uint32_t> toUint(const char* text)
		{
			if (text == nullptr)
			{
				return std::nullopt;
			}
			uint32_t value = 0;
			const std::string_view s(text);
			auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
			if (s.empty() || ec != std::errc() || ptr != s.data() + s.size())
			{
				return std::nullopt;
			}
			return value;
		}

		/** Record the first error and stop parsing */
		void fail(ParseState& state, const std::string& message)
		{
			if (state.error.empty())
			{
				state.error = fmt::format("line {:d}: {:s}", XML_GetCurrentLineNumber(state.parser), message);
				XML_StopParser(state.parser, XML_FALSE);
			}
		}

		/** `<theme>` */
		void readTheme(ParseState& state, const XML_Char** attrs)
		{
			const char* name = attr(attrs, "name");
			if (name == nullptr || name[0] == '\0')
			{
				fail(state, "<theme> needs a name");
				return;
			}
			state.theme.name = name;
			if (const char* extends = attr(attrs, "extends"))
			{
				state.theme.extends = extends;
			}
			if (const char* abstract = attr(attrs, "abstract"))
			{
				state.theme.abstract = std::string_view(abstract) == "true";
			}
		}

		/** `<color>` inside `<palette>` */
		void readColor(ParseState& state, const XML_Char** attrs)
		{
			ColorAdjustment adjustment;
			const char* name = attr(attrs, "name");
			if (name == nullptr)
			{
				fail(state, "<color> needs a name");
				return;
			}
			adjustment.color = name;
			if (const char* from = attr(attrs, "from"))
			{
				adjustment.from = from;
			}
			for (const char* field : {"lightness", "chroma"})
			{
				const char* value = attr(attrs, field);
				if (value == nullptr)
				{
					continue;
				}
				auto change = ColorAdjustment::Change::parse(value);
				if (!change)
				{
					fail(state,
						 fmt::format(
							 "<color name=\"{:s}\"> {:s}=\"{:s}\" isn't +x, -x, *x or a number", name, field, value));
					return;
				}
				(std::string_view(field) == "lightness" ? adjustment.lightness : adjustment.chroma) = change;
			}
			if (const char* when = attr(attrs, "when"))
			{
				const std::string_view w(when);
				if (w == "dark")
				{
					adjustment.when = ColorAdjustment::When::Dark;
				}
				else if (w == "light")
				{
					adjustment.when = ColorAdjustment::When::Light;
				}
				else
				{
					fail(state, fmt::format("when=\"{:s}\" must be dark or light", when));
					return;
				}
			}
			state.theme.colors.push_back(std::move(adjustment));
		}

		/** `<font>` inside `<fonts>` */
		void readFont(ParseState& state, const XML_Char** attrs)
		{
			const char* name = attr(attrs, "name");
			if (name == nullptr)
			{
				fail(state, "<font> needs a name");
				return;
			}
			FontSpec& font = state.theme.fonts[name];
			if (const char* size = attr(attrs, "size"))
			{
				font.size = toUint(size);
				if (!font.size)
				{
					fail(state, fmt::format("<font name=\"{:s}\"> size=\"{:s}\" isn't a number", name, size));
					return;
				}
			}
			if (const char* weight = attr(attrs, "weight"))
			{
				font.weight = std::string_view(weight) == "none" ? std::optional<uint32_t>(0) : toUint(weight);
				if (!font.weight)
				{
					fail(state,
						 fmt::format("<font name=\"{:s}\"> weight=\"{:s}\" isn't a number or none", name, weight));
					return;
				}
			}
			if (const char* style = attr(attrs, "style"))
			{
				font.style = style;
			}
		}

		/** `<const>` inside `<consts>` */
		void readConst(ParseState& state, const XML_Char** attrs)
		{
			const char* name = attr(attrs, "name");
			const char* value = attr(attrs, "value");
			if (name == nullptr || value == nullptr)
			{
				fail(state, "<const> needs a name and a value");
				return;
			}
			state.theme.consts.emplace_back(name, value);
		}

		/** `<style>` inside `<styles>` */
		void readStyle(ParseState& state, const XML_Char** attrs)
		{
			StyleEdit edit;
			edit.line = static_cast<int>(XML_GetCurrentLineNumber(state.parser));
			for (size_t i = 0; attrs[i]; i += 2)
			{
				const std::string_view key(attrs[i]);
				if (key == "name")
				{
					edit.name = attrs[i + 1];
				}
				else if (key == "copy")
				{
					edit.copy = attrs[i + 1];
				}
				else if (key == "merge")
				{
					std::istringstream names(attrs[i + 1]);
					for (std::string n; names >> n;)
					{
						edit.merge.push_back(n);
					}
				}
				else
				{
					edit.props.emplace_back(attrs[i], attrs[i + 1]);
				}
			}
			if (edit.name.empty())
			{
				fail(state, "<style> needs a name");
				return;
			}
			state.theme.styles.push_back(std::move(edit));
		}

		/** expat start-element handler */
		void onStart(void* user, const XML_Char* elementName, const XML_Char** attrs)
		{
			auto& state = *static_cast<ParseState*>(user);
			const std::string element(elementName);
			const std::string parent = state.path.empty() ? "" : state.path.back();
			state.path.push_back(element);

			if (parent.empty())
			{
				if (element != "theme")
				{
					fail(state, fmt::format("the root element must be <theme>, not <{:s}>", element));
					return;
				}
				readTheme(state, attrs);
			}
			else if (parent == "theme" && element == "palette")
			{
				for (size_t i = 0; attrs[i]; i += 2)
				{
					state.theme.palette[attrs[i]] = attrs[i + 1];
				}
			}
			else if (parent == "palette" && element == "color")
			{
				readColor(state, attrs);
			}
			else if (parent == "fonts" && element == "font")
			{
				readFont(state, attrs);
			}
			else if (parent == "consts" && element == "const")
			{
				readConst(state, attrs);
			}
			else if (parent == "styles" && element == "style")
			{
				readStyle(state, attrs);
			}
			else if (parent == "theme" && (element == "fonts" || element == "consts" || element == "styles"))
			{
				// Containers; their children are read above
			}
			else
			{
				fail(state, fmt::format("<{:s}> isn't allowed inside <{:s}>", element, parent));
			}
		}

		/** expat end-element handler */
		void onEnd(void* user, const XML_Char*)
		{
			static_cast<ParseState*>(user)->path.pop_back();
		}
	} // namespace

	std::optional<ThemeFile> parseThemeFile(const std::string& text,
											const std::filesystem::path& source,
											std::string& error)
	{
		ParseState state;
		state.parser = XML_ParserCreate(nullptr);
		state.theme.source = source;
		XML_SetUserData(state.parser, &state);
		XML_SetElementHandler(state.parser, onStart, onEnd);

		if (XML_Parse(state.parser, text.data(), static_cast<int>(text.size()), XML_TRUE) == XML_STATUS_ERROR &&
			state.error.empty())
		{
			state.error = fmt::format("line {:d}: {:s}",
									  XML_GetCurrentLineNumber(state.parser),
									  XML_ErrorString(XML_GetErrorCode(state.parser)));
		}
		XML_ParserFree(state.parser);

		if (!state.error.empty())
		{
			error = state.error;
			return std::nullopt;
		}
		if (state.theme.name.empty())
		{
			error = "no <theme> element";
			return std::nullopt;
		}
		return std::move(state.theme);
	}
} // namespace UI::Themes
