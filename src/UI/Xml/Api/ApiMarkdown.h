/*
 * ApiMarkdown.h
 *
 * Renders the UI API reference (see ApiReference.h) as Markdown: docs/XML_UI_API.md.
 */

#pragma once

#include "ApiReference.h"
#include <string>

namespace UI::Xml::Api
{
	/** `reference` as a Markdown document. Paths that are relative are linked from the docs/ directory. */
	std::string toMarkdown(const Reference& reference);
} // namespace UI::Xml::Api
