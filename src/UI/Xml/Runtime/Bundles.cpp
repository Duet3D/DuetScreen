/*
 * Bundles.cpp
 */

#include "Bundles.h"
#include "Debug.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "XmlUi.h"
#include "helix_xml.h"
#include "src/libs/expat/expat.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>

namespace UI::Xml::Bundles
{
	namespace
	{
		/** What preparse() reads from a component */
		struct Preparsed
		{
			std::string presenter; // `presenter` attribute of the root element
			bool sawRoot = false;  // Whether the root element has been seen
		};

		/** expat start-element handler: read the root element's `presenter` attribute */
		void onStartElement(void* user, const XML_Char* name, const XML_Char** attrs)
		{
			auto* p = static_cast<Preparsed*>(user);
			if (p->sawRoot)
			{
				return;
			}
			p->sawRoot = true;
			if (std::string_view(name) != "component")
			{
				return;
			}
			for (size_t i = 0; attrs[i]; i += 2)
			{
				if (std::string_view(attrs[i]) == "presenter")
				{
					p->presenter = attrs[i + 1];
				}
			}
		}

		/**
		 * Check `text` is well-formed and read its root element's `presenter`, before the engine sees it, so errors
		 * can be reported with a line number
		 * @return an error message, or empty
		 */
		std::string preparse(const std::string& text, Preparsed& out)
		{
			XML_Parser parser = XML_ParserCreate(nullptr);
			XML_SetUserData(parser, &out);
			XML_SetElementHandler(parser, onStartElement, nullptr);

			std::string error;
			if (XML_Parse(parser, text.data(), static_cast<int>(text.size()), XML_TRUE) == XML_STATUS_ERROR)
			{
				error = fmt::format(
					"line {:d}: {:s}", XML_GetCurrentLineNumber(parser), XML_ErrorString(XML_GetErrorCode(parser)));
			}
			XML_ParserFree(parser);
			return error;
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

		/** Whether `entry` is a file whose name is `prefix`, anything, then `suffix` */
		bool matches(const std::filesystem::directory_entry& entry,
					 const std::string& prefix,
					 const std::string& suffix)
		{
			const std::string name = entry.path().filename().string();
			return entry.is_regular_file() && name.size() >= prefix.size() + suffix.size() &&
				   name.starts_with(prefix) && name.ends_with(suffix);
		}

		/**
		 * Expand a manifest entry: a file, or a file name with a single `*` wildcard. If the folder holding the file
		 * name is `**`, the wildcard is matched in the folder above it and every folder below that:
		 * "components", "**" and "*.xml" joined by slashes is every XML file anywhere under components/. Sorted by
		 * path.
		 */
		std::vector<std::filesystem::path> expand(const std::filesystem::path& root, const std::string& pattern)
		{
			const std::filesystem::path full = root / pattern;
			const std::string filename = full.filename().string();
			const size_t star = filename.find('*');
			if (star == std::string::npos)
			{
				return {full};
			}

			const std::string prefix = filename.substr(0, star);
			const std::string suffix = filename.substr(star + 1);
			const std::filesystem::path dir = full.parent_path();
			std::vector<std::filesystem::path> files;
			std::error_code ec;
			if (dir.filename() == "**")
			{
				for (const auto& entry : std::filesystem::recursive_directory_iterator(dir.parent_path(), ec))
				{
					if (matches(entry, prefix, suffix))
					{
						files.push_back(entry.path());
					}
				}
			}
			else
			{
				for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
				{
					if (matches(entry, prefix, suffix))
					{
						files.push_back(entry.path());
					}
				}
			}
			std::sort(files.begin(), files.end());
			return files;
		}

		/** Loads bundles into a Result */
		class Loader
		{
		  public:
			/** Load every bundle under `paths` */
			Result load(const Paths& paths)
			{
				loadBundle(paths.builtin);
				for (const auto& dir : userBundles(paths.user))
				{
					loadBundle(dir);
				}
				return std::move(m_result);
			}

		  private:
			/** Record a problem */
			void addIssue(const std::filesystem::path& file, std::string message)
			{
				LOG_ERROR("UI bundle: {:s}: {:s}", file.string(), message);
				m_result.issues.push_back({file, std::move(message)});
			}

			/** The bundle directories in `dir` (those with a manifest), in name order */
			static std::vector<std::filesystem::path> userBundles(const std::filesystem::path& dir)
			{
				std::vector<std::filesystem::path> bundles;
				std::error_code ec;
				if (dir.empty() || !std::filesystem::is_directory(dir, ec))
				{
					return bundles;
				}
				for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
				{
					if (entry.is_directory() && std::filesystem::exists(entry.path() / "manifest.json"))
					{
						bundles.push_back(entry.path());
					}
				}
				std::sort(bundles.begin(), bundles.end());
				return bundles;
			}

			/** Load one bundle directory */
			void loadBundle(const std::filesystem::path& dir)
			{
				ZoneScoped;
				const std::filesystem::path manifestPath = dir / "manifest.json";
				m_result.files.push_back(manifestPath);

				auto text = readFile(manifestPath);
				if (!text)
				{
					addIssue(manifestPath, "missing");
					return;
				}

				const nlohmann::json manifest = nlohmann::json::parse(*text, nullptr, false, true);
				if (manifest.is_discarded() || !manifest.is_object())
				{
					addIssue(manifestPath, "isn't valid JSON");
					return;
				}

				const std::string id = manifest.value("id", dir.filename().string());
				const std::string api = manifest.value("ui_api", "");
				int major = -1;
				std::sscanf(api.c_str(), "%d", &major);
				if (major != UI_API_VERSION_MAJOR)
				{
					addIssue(manifestPath,
							 fmt::format("needs UI API {:s}, this firmware provides {:d}.{:d}",
										 api.empty() ? "?" : api,
										 UI_API_VERSION_MAJOR,
										 UI_API_VERSION_MINOR));
					return;
				}

				LOG_INFO("Loading UI bundle '{:s}' from {:s}", id, dir.string());
				m_bundleFiles.clear();
				for (const auto& pattern : manifest.value("themes", std::vector<std::string>{}))
				{
					for (const auto& file : expand(dir, pattern))
					{
						loadTheme(file);
					}
				}
				for (const auto& pattern : manifest.value("components", std::vector<std::string>{}))
				{
					for (const auto& file : expand(dir, pattern))
					{
						loadComponent(file, id);
					}
				}
			}

			/** Read one theme file; Themes builds it once every bundle is loaded */
			void loadTheme(const std::filesystem::path& file)
			{
				ZoneScoped;
				m_result.files.push_back(file);
				auto text = readFile(file);
				if (!text)
				{
					addIssue(file, "can't be read");
					return;
				}
				std::string error;
				if (auto theme = Themes::parseThemeFile(*text, file, error))
				{
					m_result.themes.push_back(std::move(*theme));
				}
				else
				{
					addIssue(file, error);
				}
			}

			/** Check one component file and register it with the engine */
			void loadComponent(const std::filesystem::path& file, const std::string& bundle)
			{
				ZoneScoped;
				m_result.files.push_back(file);
				const std::string name = file.stem().string();

				// Names are global, so two files of one name in different folders of a bundle would clash
				if (auto [it, added] = m_bundleFiles.try_emplace(name, file); !added)
				{
					addIssue(file, fmt::format("has the same name as {:s}", it->second.string()));
					return;
				}

				auto text = readFile(file);
				if (!text)
				{
					addIssue(file, "can't be read");
					return;
				}

				Preparsed pre;
				if (std::string error = preparse(*text, pre); !error.empty())
				{
					addIssue(file, error);
					return;
				}
				if (!pre.presenter.empty() && !Presenters::exists(pre.presenter))
				{
					addIssue(file, fmt::format("unknown presenter '{:s}'", pre.presenter));
					return;
				}

				// The engine keeps the earlier definition if this one fails
				if (lv_xml_register_component_from_data(name.c_str(), text->c_str()) != LV_RESULT_OK)
				{
					addIssue(file, "isn't a valid component");
					return;
				}

				m_result.components[name] = ComponentInfo{name, pre.presenter, file, bundle};
			}

			Result m_result;											// What has been loaded so far
			std::map<std::string, std::filesystem::path> m_bundleFiles; // The bundle being loaded's components by name
		};
	} // namespace

	Result loadAll(const Paths& paths)
	{
		ZoneScoped;
		return Loader().load(paths);
	}
} // namespace UI::Xml::Bundles
