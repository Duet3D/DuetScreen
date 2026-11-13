/*
 * LayoutStore.cpp
 */

#include "LayoutStore.h"
#include "Debug.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <sstream>
#include <unistd.h>

namespace UI::Xml::Layouts
{
	namespace
	{
		/** The layouts of one slot */
		struct SlotLayouts
		{
			std::optional<Layout> locks;	// The built-in layout, giving the locked items
			std::optional<Layout> fallback; // The default: the last bundle's layout but the user's own
			std::optional<Layout> saved;	// The user's own, or the default
			std::optional<Layout> shown;	// What is shown: the saved layout, or one being previewed
		};

		std::map<std::string, SlotLayouts, std::less<>> s_slots; // Every slot's layouts, by slot
		std::filesystem::path s_localDir;						 // Where save() writes
		std::vector<LayoutListener*> s_listeners;				 // Told about changes

		/** Tell every listener `slot` has changed */
		void notify(std::string_view slot)
		{
			// A listener may remove itself (its widget deleted) while being told
			const std::vector<LayoutListener*> listeners = s_listeners;
			for (LayoutListener* listener : listeners)
			{
				if (std::find(s_listeners.begin(), s_listeners.end(), listener) != s_listeners.end())
				{
					listener->onLayoutChanged(slot);
				}
			}
		}

		/** Repair `layout` with `locks`, reporting each change against `file` */
		void repairInto(Layout& layout,
						const Layout* locks,
						const std::filesystem::path& file,
						std::vector<LoadIssue>& issues)
		{
			for (auto& change : repair(layout, locks))
			{
				LOG_WARN("UI layout {:s}: {:s}", file.string(), change);
				issues.push_back({file, std::move(change)});
			}
		}

		/** Write all of `text` to `path` and flush it to the card */
		bool writeAndSync(const std::filesystem::path& path, std::string_view text, std::string& error)
		{
			const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd < 0)
			{
				error = fmt::format("can't write {:s}: {:s}", path.string(), std::strerror(errno));
				return false;
			}
			size_t written = 0;
			while (written < text.size())
			{
				const ssize_t n = ::write(fd, text.data() + written, text.size() - written);
				if (n < 0)
				{
					if (errno == EINTR)
					{
						continue;
					}
					error = fmt::format("can't write {:s}: {:s}", path.string(), std::strerror(errno));
					::close(fd);
					return false;
				}
				written += static_cast<size_t>(n);
			}
			const bool synced = ::fsync(fd) == 0;
			if (::close(fd) != 0 || !synced)
			{
				error = fmt::format("can't save {:s}: {:s}", path.string(), std::strerror(errno));
				return false;
			}
			return true;
		}

		/** Flush the directory `dir`'s entries (a rename in it) to the card */
		void syncDirectory(const std::filesystem::path& dir)
		{
			const int fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
			if (fd >= 0)
			{
				::fsync(fd);
				::close(fd);
			}
		}

		/** Write `text` to a temporary file beside `file`, then rename it over `file` */
		bool replace(const std::filesystem::path& file, std::string_view text, std::string& error)
		{
			std::filesystem::path temporary = file;
			temporary += ".tmp";
			if (!writeAndSync(temporary, text, error))
			{
				return false;
			}
			std::error_code ec;
			std::filesystem::rename(temporary, file, ec);
			if (ec)
			{
				error = fmt::format("can't replace {:s}: {:s}", file.string(), ec.message());
				return false;
			}
			syncDirectory(file.parent_path());
			return true;
		}

		/** The contents of `path`, or nullopt if it can't be read */
		std::optional<std::string> readFile(const std::filesystem::path& path)
		{
			std::ifstream file(path, std::ios::binary);
			if (!file)
			{
				return std::nullopt;
			}
			std::stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		}

		/** Make sure the local bundle has a manifest listing its layouts */
		bool ensureLocalManifest(std::string& error)
		{
			const std::filesystem::path manifest = s_localDir / "manifest.json";
			if (std::filesystem::exists(manifest))
			{
				return true;
			}
			const std::string text = fmt::format(R"({{
  "id": "{:s}",
  "name": "Changes made on the screen",
  "ui_api": "{:d}.{:d}",
  "layouts": ["layouts/*.json"]
}}
)",
												 LOCAL_BUNDLE,
												 UI_API_VERSION_MAJOR,
												 UI_API_VERSION_MINOR);
			return writeSafely(manifest, text, error);
		}
	} // namespace

	void set(std::vector<LayoutFile> files, std::filesystem::path localDir, std::vector<LoadIssue>& issues)
	{
		s_slots.clear();
		s_localDir = std::move(localDir);

		// The built-in layouts (the first of each slot) give the locks; each later one replaces the one before
		std::map<std::string, const LayoutFile*> defaults;
		std::map<std::string, const LayoutFile*> local;
		for (const auto& file : files)
		{
			SlotLayouts& slot = s_slots[file.layout.slot];
			if (!slot.locks)
			{
				slot.locks = file.layout;
			}
			(file.local ? local : defaults)[file.layout.slot] = &file;
		}

		for (auto& [name, slot] : s_slots)
		{
			const Layout* locks = &*slot.locks;
			if (auto it = defaults.find(name); it != defaults.end())
			{
				slot.fallback = it->second->layout;
				repairInto(*slot.fallback, locks, it->second->file, issues);
			}
			if (auto it = local.find(name); it != local.end())
			{
				slot.saved = it->second->layout;
				repairInto(*slot.saved, locks, it->second->file, issues);
			}
			else
			{
				slot.saved = slot.fallback;
			}
			slot.shown = slot.saved;
		}
		for (const auto& [name, slot] : s_slots)
		{
			notify(name);
		}
	}

	void clear()
	{
		s_slots.clear();
		s_localDir.clear();
	}

	const Layout* find(std::string_view slot)
	{
		const auto it = s_slots.find(slot);
		return it == s_slots.end() || !it->second.shown ? nullptr : &*it->second.shown;
	}

	const Layout* defaultFor(std::string_view slot)
	{
		const auto it = s_slots.find(slot);
		return it == s_slots.end() || !it->second.fallback ? nullptr : &*it->second.fallback;
	}

	const Layout* locksFor(std::string_view slot)
	{
		const auto it = s_slots.find(slot);
		return it == s_slots.end() || !it->second.locks ? nullptr : &*it->second.locks;
	}

	std::vector<std::string> slots()
	{
		std::vector<std::string> names;
		for (const auto& [name, slot] : s_slots)
		{
			names.push_back(name);
		}
		return names;
	}

	bool save(Layout layout, std::string& error)
	{
		error.clear();
		SlotLayouts& slot = s_slots[layout.slot];
		repair(layout, slot.locks ? &*slot.locks : nullptr);
		const bool isDefault = slot.fallback && layout == *slot.fallback;
		slot.saved = layout;
		slot.shown = layout;
		notify(layout.slot);

		if (s_localDir.empty())
		{
			return true;
		}
		const std::filesystem::path file = s_localDir / "layouts" / (layout.slot + ".json");
		std::error_code ec;
		if (isDefault)
		{
			// Back to the default: the user's own file would only stop later changes to the default showing
			std::filesystem::remove(file, ec);
			std::filesystem::remove(std::filesystem::path(file) += ".bak", ec);
			return true;
		}

		std::filesystem::create_directories(file.parent_path(), ec);
		if (ec)
		{
			error = fmt::format("can't create {:s}: {:s}", file.parent_path().string(), ec.message());
			return false;
		}
		return ensureLocalManifest(error) && writeSafely(file, layout.toJson().dump(2) + "\n", error);
	}

	void preview(Layout layout)
	{
		SlotLayouts& slot = s_slots[layout.slot];
		repair(layout, slot.locks ? &*slot.locks : nullptr);
		if (slot.shown && *slot.shown == layout)
		{
			return;
		}
		slot.shown = std::move(layout);
		notify(slot.shown->slot);
	}

	void revert(std::string_view name)
	{
		const auto it = s_slots.find(name);
		if (it == s_slots.end() || it->second.shown == it->second.saved)
		{
			return;
		}
		it->second.shown = it->second.saved;
		notify(name);
	}

	const Layout* saved(std::string_view name)
	{
		const auto it = s_slots.find(name);
		return it == s_slots.end() || !it->second.saved ? nullptr : &*it->second.saved;
	}

	void addListener(LayoutListener* listener)
	{
		s_listeners.push_back(listener);
	}

	void removeListener(LayoutListener* listener)
	{
		std::erase(s_listeners, listener);
	}

	bool writeSafely(const std::filesystem::path& file, std::string_view text, std::string& error)
	{
		if (std::filesystem::exists(file))
		{
			// Keep the old contents first, the same way, so there is always a whole copy of one or the other
			auto old = readFile(file);
			std::filesystem::path backup = file;
			backup += ".bak";
			if (!old || !replace(backup, *old, error))
			{
				if (error.empty())
				{
					error = fmt::format("can't read {:s}", file.string());
				}
				return false;
			}
		}
		return replace(file, text, error);
	}
} // namespace UI::Xml::Layouts
