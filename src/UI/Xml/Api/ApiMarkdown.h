/*
 * ApiMarkdown.h
 *
 * Renders the UI API reference (see ApiReference.h) as Markdown: docs/XML_UI_API.md, and the built-in themes,
 * docs/XML_UI_THEMES.md.
 */

#pragma once

#include "ApiImages.h"
#include "ApiReference.h"
#include <string>
#include <string_view>

namespace UI::Xml::Api
{
	/**
	 * `reference` as a Markdown document (docs/XML_UI_API.md), with `images` (see ApiImages.h) where they belong: the
	 * active theme's pictures, and the views
	 * @param imageRoot The path from the document to the reference images the pictures are in, e.g.
	 * "../tests/ref_imgs/"; other relative paths are linked from the docs/ directory
	 */
	std::string toMarkdown(const Reference& reference, const Images& images, std::string_view imageRoot);

	/** Every built-in theme, as a Markdown document (docs/XML_UI_THEMES.md), with its pictures; see toMarkdown() */
	std::string toThemesMarkdown(const Reference& reference, const Images& images, std::string_view imageRoot);
} // namespace UI::Xml::Api
