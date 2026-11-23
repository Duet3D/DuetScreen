/*
 * HotReload.h
 *
 * Reloads the UI when a bundle file changes, so XML can be edited while the simulator runs.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace UI::Xml::HotReload
{
	/** Check the watched files every `periodMs` and call Xml::reload() when one changes. 0 stops checking. */
	void setPeriod(uint32_t periodMs);

	/** Watch `files` from now on, as they are now */
	void watch(const std::vector<std::filesystem::path>& files);
} // namespace UI::Xml::HotReload
