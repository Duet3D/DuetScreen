/*
 * LayoutLibrary.cpp
 */

#include "LayoutLibrary.h"
#include "LayoutStore.h"
#include <algorithm>
#include <cctype>
#include <fmt/format.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace UI::Xml::LayoutLibrary
{
	namespace
	{
		std::filesystem::path s_folder; // Where saved layouts are kept; empty for none

		/** The file of the layout saved as `name` */
		std::filesystem::path fileOf(std::string_view name)
		{
			return s_folder / (std::string(name) + ".json");
		}
	} // namespace

	void setFolder(std::filesystem::path dir)
	{
		s_folder = std::move(dir);
	}

	bool canSave()
	{
		return !s_folder.empty();
	}

	bool isValidName(std::string_view name)
	{
		const auto allowed = [](char c)
		{ return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == ' ' || c == '-' || c == '_'; };
		return !name.empty() && name.size() <= MAX_NAME && std::all_of(name.begin(), name.end(), allowed) &&
			   name.find_first_not_of(' ') != std::string_view::npos;
	}

	std::vector<std::string> names()
	{
		std::vector<std::string> result;
		std::error_code ec;
		if (s_folder.empty() || !std::filesystem::is_directory(s_folder, ec))
		{
			return result;
		}
		for (const auto& entry : std::filesystem::directory_iterator(s_folder, ec))
		{
			const std::string stem = entry.path().stem().string();
			if (entry.is_regular_file() && entry.path().extension() == ".json" && isValidName(stem))
			{
				result.push_back(stem);
			}
		}
		std::sort(result.begin(), result.end());
		return result;
	}

	bool save(std::string_view name, const std::vector<Layout>& layouts, std::string& error)
	{
		error.clear();
		if (!canSave())
		{
			error = "layouts can't be saved (safe mode, or no user folder)";
			return false;
		}
		if (!isValidName(name))
		{
			error = fmt::format("\"{:s}\" can't name a layout", name);
			return false;
		}
		nlohmann::json json = nlohmann::json::object();
		json["format"] = FORMAT;
		json["name"] = name;
		json["layouts"] = nlohmann::json::array();
		for (const auto& layout : layouts)
		{
			json["layouts"].push_back(layout.toJson());
		}
		return Layouts::writeSafely(fileOf(name), json.dump(2) + "\n", error);
	}

	std::optional<std::vector<Layout>> load(std::string_view name, std::string& error)
	{
		error.clear();
		if (!isValidName(name))
		{
			error = fmt::format("\"{:s}\" can't name a layout", name);
			return std::nullopt;
		}
		std::ifstream file(fileOf(name), std::ios::binary);
		if (!file)
		{
			error = fmt::format("there is no layout \"{:s}\"", name);
			return std::nullopt;
		}
		std::stringstream text;
		text << file.rdbuf();
		const nlohmann::json json = nlohmann::json::parse(text.str(), nullptr, false, true);
		if (json.is_discarded() || !json.is_object() || json.value("format", 0) != FORMAT ||
			!json.contains("layouts") || !json["layouts"].is_array())
		{
			error = fmt::format("layout \"{:s}\" isn't a saved layout this firmware reads", name);
			return std::nullopt;
		}
		std::vector<Layout> layouts;
		for (const auto& entry : json["layouts"])
		{
			std::string problem;
			if (auto layout = Layout::fromJson(entry, problem))
			{
				layouts.push_back(std::move(*layout));
			}
			else
			{
				error = fmt::format("layout \"{:s}\": {:s}", name, problem);
				return std::nullopt;
			}
		}
		return layouts;
	}

	bool remove(std::string_view name)
	{
		if (!isValidName(name) || s_folder.empty())
		{
			return false;
		}
		std::error_code ec;
		const bool removed = std::filesystem::remove(fileOf(name), ec);
		std::filesystem::remove(std::filesystem::path(fileOf(name)) += ".bak", ec);
		return removed;
	}
} // namespace UI::Xml::LayoutLibrary
