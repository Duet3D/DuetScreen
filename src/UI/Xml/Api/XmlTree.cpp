/*
 * XmlTree.cpp
 */

#include "XmlTree.h"
#include "helix_xml.h"
#include "src/libs/expat/expat.h"
#include <fmt/format.h>
#include <fstream>
#include <sstream>

namespace UI::Xml::Api
{
	namespace
	{
		/** What expat builds the element tree with */
		struct ParseState
		{
			XML_Parser parser = nullptr;  // The parser, for line numbers
			Element root;				  // The root element, once started
			std::vector<Element*> open{}; // Elements started and not ended, innermost last
		};

		/** expat start handler: add the element under the innermost open one */
		void onStart(void* user, const XML_Char* name, const XML_Char** attrs)
		{
			auto& state = *static_cast<ParseState*>(user);
			Element element{.tag = name,
							.attrs = {},
							.line = static_cast<int>(XML_GetCurrentLineNumber(state.parser)),
							.children = {}};
			for (size_t i = 0; attrs[i] != nullptr; i += 2)
			{
				element.attrs.emplace_back(attrs[i], attrs[i + 1]);
			}
			// An open element's address is stable: only the innermost open element gains children
			if (state.open.empty())
			{
				state.root = std::move(element);
				state.open.push_back(&state.root);
			}
			else
			{
				auto& siblings = state.open.back()->children;
				siblings.push_back(std::move(element));
				state.open.push_back(&siblings.back());
			}
		}

		/** expat end handler: close the innermost element */
		void onEnd(void* user, const XML_Char*)
		{
			static_cast<ParseState*>(user)->open.pop_back();
		}
	} // namespace

	const std::string* Element::attr(std::string_view name) const
	{
		for (const auto& [n, value] : attrs)
		{
			if (n == name)
			{
				return &value;
			}
		}
		return nullptr;
	}

	const Element* Element::child(std::string_view name) const
	{
		for (const auto& c : children)
		{
			if (c.tag == name)
			{
				return &c;
			}
		}
		return nullptr;
	}

	std::optional<Element> parseXmlFile(const std::string& path, std::string& error)
	{
		std::ifstream file(path);
		if (!file)
		{
			error = "can't be read";
			return std::nullopt;
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		const std::string text = buffer.str();

		ParseState state;
		state.parser = XML_ParserCreate(nullptr);
		XML_SetUserData(state.parser, &state);
		XML_SetElementHandler(state.parser, onStart, onEnd);
		const bool ok =
			XML_Parse(state.parser, text.data(), static_cast<int>(text.size()), XML_TRUE) != XML_STATUS_ERROR;
		if (!ok)
		{
			error = fmt::format("line {:d}: {:s}",
								XML_GetCurrentLineNumber(state.parser),
								XML_ErrorString(XML_GetErrorCode(state.parser)));
		}
		XML_ParserFree(state.parser);
		return ok ? std::optional<Element>(std::move(state.root)) : std::nullopt;
	}
} // namespace UI::Xml::Api
