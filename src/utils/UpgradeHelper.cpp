/*
 * UpgradeHelper.cpp
 *
 *  Created on: 2025-03-04
 *      Author: Andy Everitt
 */

#include "UpgradeHelper.h"
#include "Comm/Connection.h"
#include "Configuration.h"
#include "Debug.h"
#include "Hardware/Reset.h"
#include "Hardware/Usb.h"
#include "UI/Core/Model.h"
#include "utils/StorageHelper.h"
#include "utils/SystemHelper.h"
#include "version.h"
#include <sys/stat.h>

#include <hv/requests.h>
#include <nlohmann/json.hpp>
#include <zlib.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <optional>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define UPGRADE_EXT ".tar.gz"
static constexpr size_t UPGRADE_EXT_SIZE = sizeof(UPGRADE_EXT) - 1;
#define UPGRADE_FILE UPGRADE_FILE_NAME UPGRADE_EXT
#define TMP_FILEPATH "/tmp/" UPGRADE_FILE
#define BOOT_FILEPATH "/boot/update.tar.gz"

namespace UpgradeHelper
{
	static std::string readFileToString(const char* path);
	static void postUpdateResult(UpgradeResult result, const std::string& updateBuildrootVersion);

	namespace
	{
		enum class VersionSuffixKind : uint8_t
		{
			Alpha = 0,
			Beta,
			Rc,
			Other,
			Release,
			Numeric,
		};

		struct ParsedVersion
		{
			std::vector<int> core;
			VersionSuffixKind suffixKind = VersionSuffixKind::Release;
			int suffixNumber = 0;
		};

		struct GithubReleaseInfo
		{
			std::string tag;
			std::string downloadUrl;
		};

		static std::optional<ParsedVersion> parseVersionComponents(std::string_view version)
		{
			if (!version.empty() && (version.front() == 'v' || version.front() == 'V'))
			{
				version.remove_prefix(1);
			}

			ParsedVersion parsed;
			parsed.core.reserve(4);

			int value = 0;
			bool hasDigit = false;
			size_t suffixStart = version.size();
			for (size_t i = 0; i < version.size(); ++i)
			{
				const char c = version[i];
				if (std::isdigit(static_cast<unsigned char>(c)) != 0)
				{
					hasDigit = true;
					value = value * 10 + (c - '0');
					continue;
				}

				if (c == '.')
				{
					if (!hasDigit)
					{
						return std::nullopt;
					}
					parsed.core.push_back(value);
					value = 0;
					hasDigit = false;
					continue;
				}

				if (c == '-')
				{
					suffixStart = i + 1;
					break;
				}

				break;
			}

			if (hasDigit)
			{
				parsed.core.push_back(value);
			}

			if (parsed.core.empty())
			{
				return std::nullopt;
			}

			if (suffixStart < version.size())
			{
				const size_t suffixEnd = version.find('-', suffixStart);
				std::string_view suffixToken = version.substr(
					suffixStart,
					suffixEnd == std::string_view::npos ? std::string_view::npos : suffixEnd - suffixStart);

				if (!suffixToken.empty())
				{
					auto parseTrailingNumber = [](std::string_view token, int defaultValue) -> int
					{
						size_t digitStart = token.size();
						while (digitStart > 0 && std::isdigit(static_cast<unsigned char>(token[digitStart - 1])) != 0)
						{
							--digitStart;
						}

						if (digitStart == token.size())
						{
							return defaultValue;
						}

						int parsedValue = 0;
						for (size_t i = digitStart; i < token.size(); ++i)
						{
							parsedValue = parsedValue * 10 + (token[i] - '0');
						}
						return parsedValue;
					};

					const bool tokenNumericOnly =
						std::all_of(suffixToken.begin(),
									suffixToken.end(),
									[](const char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });

					if (tokenNumericOnly)
					{
						parsed.suffixKind = VersionSuffixKind::Numeric;
						parsed.suffixNumber = parseTrailingNumber(suffixToken, 0);
					}
					else if (suffixToken.rfind("alpha", 0) == 0)
					{
						parsed.suffixKind = VersionSuffixKind::Alpha;
						parsed.suffixNumber = parseTrailingNumber(suffixToken, 0);
					}
					else if (suffixToken.rfind("beta", 0) == 0)
					{
						parsed.suffixKind = VersionSuffixKind::Beta;
						parsed.suffixNumber = parseTrailingNumber(suffixToken, 0);
					}
					else if (suffixToken.rfind("rc", 0) == 0)
					{
						parsed.suffixKind = VersionSuffixKind::Rc;
						parsed.suffixNumber = parseTrailingNumber(suffixToken, 0);
					}
					else
					{
						parsed.suffixKind = VersionSuffixKind::Other;
						parsed.suffixNumber = parseTrailingNumber(suffixToken, 0);
					}
				}
			}

			return parsed;
		}

		static int compareVersions(const ParsedVersion& lhs, const ParsedVersion& rhs)
		{
			const size_t maxSize = std::max(lhs.core.size(), rhs.core.size());
			for (size_t i = 0; i < maxSize; ++i)
			{
				const int left = i < lhs.core.size() ? lhs.core[i] : 0;
				const int right = i < rhs.core.size() ? rhs.core[i] : 0;
				if (left < right)
				{
					return -1;
				}
				if (left > right)
				{
					return 1;
				}
			}

			if (lhs.suffixKind != rhs.suffixKind)
			{
				return lhs.suffixKind < rhs.suffixKind ? -1 : 1;
			}

			if (lhs.suffixNumber != rhs.suffixNumber)
			{
				return lhs.suffixNumber < rhs.suffixNumber ? -1 : 1;
			}

			return 0;
		}

		static std::optional<GithubReleaseInfo> getNewestGithubReleaseWithUpgradeAsset()
		{
			HttpRequest req;
			req.method = HTTP_GET;
			req.scheme = "https";
			req.host = "api.github.com";
			req.path = "/repos/Duet3D/DuetScreen/releases"; // includes releases and prereleases, newest first
			req.headers["Accept"] = "application/vnd.github.v3+json";
			req.headers["User-Agent"] = "DuetScreenUpgradeHelper";
			req.timeout = HTTP_TIMEOUT;

			req.DumpUrl();

			hv::HttpClient cli;
			HttpResponse response;
			cli.send(&req, &response);
			if (response.status_code != HTTP_STATUS_OK)
			{
				LOG_WARN("Failed to fetch GitHub releases list: HTTP {:d}", (int)response.status_code);
				return std::nullopt;
			}

			auto payload = nlohmann::json::parse(response.body, nullptr, false);
			if (payload.is_discarded() || !payload.is_array())
			{
				LOG_WARN("Failed to parse GitHub releases response or unexpected format");
				return std::nullopt;
			}

			for (const auto& release : payload)
			{
				if (!release.is_object())
				{
					continue;
				}

				if (!release.contains("tag_name") || !release["tag_name"].is_string())
				{
					continue;
				}

				if (!release.contains("assets") || !release["assets"].is_array())
				{
					continue;
				}

				for (const auto& asset : release["assets"])
				{
					if (!asset.is_object() || !asset.contains("name") || !asset["name"].is_string())
					{
						continue;
					}

					if (asset["name"].get<std::string>() != UPGRADE_FILE)
					{
						continue;
					}

					if (!asset.contains("browser_download_url") || !asset["browser_download_url"].is_string())
					{
						continue;
					}

					GithubReleaseInfo info;
					info.tag = release["tag_name"].get<std::string>();
					info.downloadUrl = asset["browser_download_url"].get<std::string>();
					return info;
				}
			}

			LOG_WARN("No release asset named {:s} found in GitHub releases", UPGRADE_FILE);
			return std::nullopt;
		}

		constexpr const char* kBuildrootVersionFile = "/etc/buildroot_version";
		constexpr std::string_view kUpgradeStatusFileName = "upgrade_status.json";
#if SIMULATION
		constexpr const char* kUpgradeStatusSocketPath = "/tmp/duetscreen_upgrade.sock";
#else
		constexpr const char* kUpgradeStatusSocketPath = "/run/duetscreen/upgrade.sock";
#endif
		std::atomic<bool> g_monitoring{false};
		std::thread g_monitorThread;
		std::atomic<uint64_t> g_lastProcessedSequence{0};

		struct UpgradeStatusMessage
		{
			UpgradeResult result;
			std::string updateBuildrootVersion;
			uint64_t sequence;
		};

		static bool parseUpgradeStatusMessage(const nlohmann::json& payload, UpgradeStatusMessage& msg)
		{
			if (!payload.is_object())
			{
				return false;
			}

			const std::string result = payload.value("result", "");
			if (result == "success")
			{
				msg.result = UpgradeResult::Success;
			}
			else if (result == "warning")
			{
				msg.result = UpgradeResult::BuildrootVersionWarning;
			}
			else if (result == "error")
			{
				msg.result = UpgradeResult::BuildrootVersionError;
			}
			else
			{
				LOG_ERROR("Invalid upgrade status result '{:s}'", result);
				return false;
			}

			msg.sequence = payload.value("sequence", 0ULL);
			msg.updateBuildrootVersion =
				payload.value("target_buildroot", payload.value("update_buildroot_version", std::string{}));
			return true;
		}

		static void postUpdateResultFromMessage(const UpgradeStatusMessage& msg)
		{
			if (msg.sequence != 0)
			{
				const uint64_t lastSeq = g_lastProcessedSequence.load();
				if (msg.sequence <= lastSeq)
				{
					LOG_DBG("Ignoring stale upgrade status message, sequence {:d} <= {:d}", msg.sequence, lastSeq);
					return;
				}
				g_lastProcessedSequence.store(msg.sequence);
			}

			postUpdateResult(msg.result, msg.updateBuildrootVersion);
		}

		static void checkAndPostExistingUpgradeStatus()
		{
			ZoneScoped;
			const std::filesystem::path statusPath = std::filesystem::path(NVS_FOLDER) / kUpgradeStatusFileName;
			std::ifstream statusFile(statusPath);
			if (!statusFile.is_open())
			{
				return;
			}

			const std::string content{std::istreambuf_iterator<char>(statusFile), std::istreambuf_iterator<char>()};
			nlohmann::json statusJson = nlohmann::json::parse(content, nullptr, false);
			std::filesystem::remove(statusPath);
			if (statusJson.is_discarded())
			{
				LOG_ERROR("Failed to parse upgrade status file: {:s}", statusPath.string());
				return;
			}

			UpgradeStatusMessage msg{};
			if (!parseUpgradeStatusMessage(statusJson, msg))
			{
				LOG_ERROR("Invalid upgrade status file: {:s}", statusPath.string());
				return;
			}

			postUpdateResultFromMessage(msg);
		}

		/** What scanning an update archive found */
		struct ArchiveContents
		{
			bool readable = false;						 // It is a whole, readable tar.gz archive
			bool hooks = false;							 // It holds an update's scripts (pre-update, post-update)
			std::optional<std::string> buildrootVersion; // Its buildroot_version, if it has one
		};

		/** A tar header's number at `field`, `length` bytes: octal, or GNU's base-256 for a large one */
		std::optional<uint64_t> tarNumber(const char* field, size_t length)
		{
			if ((static_cast<unsigned char>(field[0]) & 0x80) != 0)
			{
				uint64_t value = static_cast<unsigned char>(field[0]) & 0x7F;
				for (size_t i = 1; i < length; i++)
				{
					value = (value << 8) | static_cast<unsigned char>(field[i]);
				}
				return value;
			}
			uint64_t value = 0;
			size_t i = 0;
			while (i < length && field[i] == ' ')
			{
				i++;
			}
			if (i == length || field[i] < '0' || field[i] > '7')
			{
				return std::nullopt;
			}
			for (; i < length && field[i] >= '0' && field[i] <= '7'; i++)
			{
				value = value * 8 + static_cast<uint64_t>(field[i] - '0');
			}
			return value;
		}

		/** Whether the 512-byte tar header `header` is whole: its checksum matches */
		bool tarHeaderValid(const std::array<char, 512>& header)
		{
			const std::optional<uint64_t> stated = tarNumber(header.data() + 148, 8);
			uint64_t sum = 0;
			for (size_t i = 0; i < header.size(); i++)
			{
				sum += i >= 148 && i < 156 ? ' ' : static_cast<unsigned char>(header[i]);
			}
			return stated && *stated == sum;
		}

		/** A tar header's text field at `field`, up to `length` bytes or its terminating zero */
		std::string tarText(const char* field, size_t length)
		{
			return std::string(field, strnlen(field, length));
		}

		/** Read the next `length` bytes of `archive`, or skip them (`into` null); false if it ends first */
		bool readArchive(gzFile archive, uint64_t length, std::string* into)
		{
			std::array<char, 64 * 1024> buffer{};
			while (length > 0)
			{
				const auto piece = static_cast<unsigned>(std::min<uint64_t>(length, buffer.size()));
				if (gzread(archive, buffer.data(), piece) != static_cast<int>(piece))
				{
					return false;
				}
				if (into != nullptr)
				{
					into->append(buffer.data(), piece);
				}
				length -= piece;
			}
			return true;
		}

		/** What the tar.gz archive at `path` holds of an update; read all through, as its end shows it is whole */
		ArchiveContents scanArchive(const std::filesystem::path& path)
		{
			ZoneScoped;
			ArchiveContents contents;
			gzFile archive = gzopen(path.c_str(), "rb");
			if (archive == nullptr)
			{
				return contents;
			}
			gzbuffer(archive, 128 * 1024);
			std::array<char, 512> header{};
			std::string longName; // The next entry's name, from a GNU long name entry before it
			while (gzread(archive, header.data(), header.size()) == static_cast<int>(header.size()))
			{
				if (std::all_of(header.begin(), header.end(), [](char c) { return c == 0; }))
				{
					contents.readable = true; // the end
					break;
				}
				const std::optional<uint64_t> size = tarNumber(header.data() + 124, 12);
				if (!tarHeaderValid(header) || !size)
				{
					break;
				}
				const uint64_t padded = (*size + 511) / 512 * 512;
				const char type = header[156];
				if (type == 'L')
				{
					std::string name;
					if (!readArchive(archive, padded, &name))
					{
						break;
					}
					longName = tarText(name.data(), name.size());
					continue;
				}

				std::string name = longName;
				longName.clear();
				if (name.empty())
				{
					const std::string prefix = tarText(header.data() + 345, 155);
					name = tarText(header.data(), 100);
					name = prefix.empty() ? name : prefix + "/" + name;
				}
				if (name.starts_with("./"))
				{
					name.erase(0, 2);
				}
				contents.hooks |= name == "pre-update" || name == "post-update";

				const bool regular = type == '0' || type == '\0';
				if (name == "buildroot_version" && regular)
				{
					std::string data;
					if (!readArchive(archive, padded, &data))
					{
						break;
					}
					data.resize(static_cast<size_t>(*size));
					std::erase_if(data, [](char c) { return c == '\r' || c == '\n'; });
					contents.buildrootVersion = data;
				}
				else if (!readArchive(archive, padded, nullptr))
				{
					break;
				}
			}
			gzclose(archive);
			return contents;
		}

		/** The first `fields` dot-separated fields of `version`, as `cut -d. -f1,2` gives them for 2 */
		std::string_view versionFields(std::string_view version, size_t fields)
		{
			size_t end = 0;
			for (size_t i = 0; i < fields; i++)
			{
				end = version.find('.', i == 0 ? 0 : end + 1);
				if (end == std::string_view::npos)
				{
					return version;
				}
			}
			return version.substr(0, end);
		}

		/** The third dot-separated field of `version` (its patch), or empty */
		std::string_view versionPatch(std::string_view version)
		{
			const std::string_view majorMinor = versionFields(version, 2);
			if (majorMinor.size() >= version.size())
			{
				return {};
			}
			const std::string_view rest = version.substr(majorMinor.size() + 1);
			return rest.substr(0, rest.find('.'));
		}

		/** Whether `path` is on a USB drive that is plugged in */
		bool onUsbDrive(const std::string& path)
		{
			const std::vector<std::string> drives = USB::UsbMonitor::getInstance().getMountedDrives();
			return std::any_of(drives.begin(),
							   drives.end(),
							   [&path](const std::string& drive) { return path.starts_with(drive + "/"); });
		}
	} // namespace

	void startMonitoringUpgradeStatus()
	{
		if (g_monitoring.exchange(true))
		{
			// Already running
			return;
		}

		checkAndPostExistingUpgradeStatus();

#if T113
		g_monitorThread = std::thread(
			[]
			{
				tracy::SetThreadName("Upgrade Result Monitor");

				std::filesystem::path socketPath = kUpgradeStatusSocketPath;
				std::error_code ec;
				std::filesystem::create_directories(socketPath.parent_path(), ec);
				if (ec)
				{
					LOG_ERROR("Failed to create upgrade status socket directory {:s}: {:s}",
							  socketPath.parent_path().string(),
							  ec.message());
					g_monitoring.store(false);
					return;
				}

				std::filesystem::remove(socketPath, ec);

				const int socketFd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
				if (socketFd < 0)
				{
					LOG_ERROR(
						"Failed to create upgrade status socket {:s}: {:s}", kUpgradeStatusSocketPath, strerror(errno));
					g_monitoring.store(false);
					return;
				}

				sockaddr_un addr{};
				addr.sun_family = AF_UNIX;
				if (socketPath.string().size() >= sizeof(addr.sun_path))
				{
					LOG_ERROR("Upgrade status socket path too long: {:s}", socketPath.string());
					close(socketFd);
					g_monitoring.store(false);
					return;
				}
				std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

				if (bind(socketFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
				{
					LOG_ERROR("Failed to bind upgrade status socket {:s}: {:s}", socketPath.string(), strerror(errno));
					close(socketFd);
					g_monitoring.store(false);
					return;
				}

				constexpr size_t bufLen = 2048;
				char buf[bufLen]{};
				while (g_monitoring.load())
				{
					pollfd pfd{};
					pfd.fd = socketFd;
					pfd.events = POLLIN;
					const int pollResult = poll(&pfd, 1, 250);
					if (pollResult <= 0 || (pfd.revents & POLLIN) == 0)
					{
						continue;
					}

					const ssize_t len = recv(socketFd, buf, bufLen - 1, 0);
					if (len <= 0)
					{
						continue;
					}

					buf[len] = '\0';
					nlohmann::json statusJson = nlohmann::json::parse(buf, nullptr, false);
					if (statusJson.is_discarded())
					{
						LOG_ERROR("Failed to parse upgrade status socket payload: {:s}", buf);
						continue;
					}

					UpgradeStatusMessage msg{};
					if (!parseUpgradeStatusMessage(statusJson, msg))
					{
						continue;
					}

					ZoneScopedN("Upgrade Result Monitor - status message");
					postUpdateResultFromMessage(msg);
				}

				close(socketFd);
				std::filesystem::remove(socketPath, ec);
				if (ec)
				{
					LOG_ERROR("Failed to remove upgrade status socket {:s}: {:s}", socketPath.string(), ec.message());
				}
				g_monitoring.store(false);
			});
		g_monitorThread.detach();

#endif
	}

	std::string_view getBuildrootVersion()
	{
#if SIMULATION
		return "Simulation";
#endif

		static std::string version;
		if (!version.empty())
		{
			return version;
		}

		version = readFileToString(kBuildrootVersionFile);
		return version;
	}

	static std::string readFileToString(const char* path)
	{
		std::ifstream f(path);
		if (!f.is_open())
			return {};
		std::string s;
		std::getline(f, s);
		return s;
	}

	static void postUpdateResult(UpgradeResult result, const std::string& updateBuildrootVersion)
	{
		UpgradeInfo info;
		info.result = result;
		info.currentVersion = FIRMWARE_VERSION;
		info.currentBuildrootVersion = getBuildrootVersion();
		if (!updateBuildrootVersion.empty())
		{
			info.updateBuildrootVersion = updateBuildrootVersion;
		}
		LOG_DBG("Upgrade result: '{:s}', current version: '{:s}', current buildroot version: '{:s}', update buildroot "
				"version: '{:s}'",
				nameof::nameof_enum(result),
				info.currentVersion,
				info.currentBuildrootVersion,
				info.updateBuildrootVersion);
		Model::get().post<EventType::UpdateResult>(std::move(info));
	}

	static bool removeTmpFile()
	{
		ZoneScoped;
		std::error_code ec;
		std::filesystem::remove(TMP_FILEPATH, ec);
		if (ec)
		{
			LOG_ERROR("Failed to remove temporary upgrade file " TMP_FILEPATH ": {:s}", ec.message());
			return false;
		}
		return true;
	}

	static bool copyFileFromUsb(const std::string& filePath)
	{
		ZoneScoped;
		if (!onUsbDrive(filePath))
		{
			LOG_ERROR("File path {:s} is not on USB", filePath.c_str());
			return false;
		}

		if (!looksLikeUpdate(filePath))
		{
			LOG_ERROR("File {:s} does not have the required " UPGRADE_EXT " extension", filePath);
			return false;
		}

		struct stat sb;
		if (stat(filePath.c_str(), &sb) == -1)
		{
			LOG_ERROR("Failed to get file stats for {:s}", filePath);
			return false;
		}

		removeTmpFile(); // Remove any previous upgrade file
		if (!std::filesystem::copy_file(filePath, TMP_FILEPATH))
		{
			LOG_ERROR("Failed to copy file \"{:s}\" to /tmp", filePath);
			return false;
		}
		LOG_DBG("File \"{:s}\" copied to " TMP_FILEPATH, filePath);
		return true;
	}

	static bool moveTmpFileToBoot()
	{
		ZoneScoped;
		std::filesystem::remove(BOOT_FILEPATH);
		if (!std::filesystem::exists(TMP_FILEPATH))
		{
			LOG_ERROR("Temporary upgrade file " TMP_FILEPATH " does not exist");
			return false;
		}
		std::filesystem::copy_file(TMP_FILEPATH, BOOT_FILEPATH);
		std::filesystem::remove(TMP_FILEPATH);
		LOG_DBG("File moved from " TMP_FILEPATH " to " BOOT_FILEPATH);
		return true;
	}

	static bool createUpgradeCompleteFile(const std::string& usbPath)
	{
		ZoneScoped;
		if (!onUsbDrive(usbPath + "/"))
		{
			LOG_ERROR("File path {:s} is not on USB", usbPath.c_str());
			return false;
		}

		FILE* file = fopen((usbPath + "/upgraded").c_str(), "w");
		if (file == nullptr)
		{
			LOG_ERROR("Failed to create upgrade complete file");
			return false;
		}
		fclose(file);
		return true;
	}

	static bool upgradeFromTmp()
	{
		ZoneScoped;
#if SIMULATION
		LOG_INFO("Simulating upgrade...");
		std::this_thread::sleep_for(std::chrono::seconds(2));
		postUpdateResult(UpgradeResult::Success, {});
		return true;
#endif
		if (!moveTmpFileToBoot())
		{
			LOG_ERROR("Failed to move file to boot partition");
			return false;
		}

		return SystemHelper::restartService(SystemHelper::Services::UPGRADE);
	}

	bool looksLikeUpdate(std::string_view name)
	{
		return name.size() > UPGRADE_EXT_SIZE && name.ends_with(UPGRADE_EXT);
	}

	UpdateCheck checkUpdate(const std::filesystem::path& path, std::string_view installed)
	{
		ZoneScoped;
		UpdateCheck check;
		check.installedBuildrootVersion = installed;
		const ArchiveContents contents = scanArchive(path);
		if (!contents.readable || !contents.hooks)
		{
			LOG_INFO("{:s} isn't a screen update", path.string());
			return check;
		}
		check.updateBuildrootVersion = contents.buildrootVersion.value_or("");
		const std::string_view update = check.updateBuildrootVersion;
		if (update.empty() || installed.empty())
		{
			check.verdict = UpdateCheck::Verdict::Installable; // as the update service: nothing to compare
		}
		else if (versionFields(update, 2) != versionFields(installed, 2))
		{
			check.verdict = UpdateCheck::Verdict::WrongSystem;
		}
		else
		{
			const std::string_view updatePatch = versionPatch(update);
			const std::string_view installedPatch = versionPatch(installed);
			check.verdict = !updatePatch.empty() && !installedPatch.empty() && updatePatch != installedPatch
								? UpdateCheck::Verdict::PatchDiffers
								: UpdateCheck::Verdict::Installable;
		}
		LOG_INFO("{:s} is a screen update for system {:s} (this screen's: {:s}): {:s}",
				 path.string(),
				 update,
				 installed,
				 nameof::nameof_enum(check.verdict));
		return check;
	}

	UpdateCheck checkUpdate(const std::filesystem::path& path)
	{
#if SIMULATION
		return checkUpdate(path, "");
#else
		return checkUpdate(path, getBuildrootVersion());
#endif
	}

	void installFromUsb(std::string path, std::function<void(std::string why)> failed)
	{
		ZoneScoped;
		std::thread(
			[path = std::move(path), failed = std::move(failed)]
			{
				tracy::SetThreadName("Update install");
				LOG_INFO("Attempting upgrade from USB file {:s}", path);
				const auto fail = [&failed](std::string why)
				{
					LOG_ERROR("Couldn't install the update: {:s}", why);
					if (failed)
					{
						failed(std::move(why));
					}
				};
				struct stat fileStat{};
				if (stat(path.c_str(), &fileStat) != 0)
				{
					fail(fmt::format("{:s} can't be read", path));
					return;
				}
				if (!copyFileFromUsb(path))
				{
					fail(fmt::format("{:s} couldn't be copied from the USB drive", path));
					return;
				}
				StorageHelper::setData(ID_UPGRADE_FILE_LAST_MODIFIED, fileStat.st_mtime);
				createUpgradeCompleteFile(path.substr(0, path.find_last_of('/')));
				if (!upgradeFromTmp())
				{
					fail("the update service couldn't be started");
				}
			})
			.detach();
	}

	void offerUpdatesOnUsbDrives()
	{
		USB::UsbMonitor::getInstance().registerCallback(
			[](const std::string& drive, bool mounted)
			{
				ZoneScopedN("Upgrade USB Callback"); // on the monitor's thread
				const std::string path = drive + "/" UPGRADE_FILE;
				struct stat fileStat{};
				if (!mounted || stat(path.c_str(), &fileStat) != 0)
				{
					return;
				}
				if (fileStat.st_mtime == StorageHelper::getData(ID_UPGRADE_FILE_LAST_MODIFIED))
				{
					return; // installed already
				}
				if (!checkUpdate(path).installable())
				{
					return;
				}
				Model::get().post<EventType::UpdateAvailable>(path);
			});
	}

	bool upgradeFromDuet()
	{
		ZoneScoped;
		std::string filePath = "/firmware/" UPGRADE_FILE;

		LOG_INFO("Attempting upgrade from Duet file {:s}", filePath.c_str());
		removeTmpFile(); // Remove any previous upgrade file

		Comm::Connection::get().readFile(
			filePath,
			[filePath](std::expected<std::string, Comm::ReplyError> contents)
			{
				if (!contents)
				{
					LOG_ERROR("Failed to download file \"{:s}\" from Duet: {:s}", filePath, contents.error().message);
					return;
				}
				std::ofstream file(TMP_FILEPATH, std::ios::binary);
				if (!file.is_open())
				{
					LOG_ERROR("Failed to create file \"" TMP_FILEPATH "\"");
					return;
				}
				file.write(contents->data(), static_cast<std::streamsize>(contents->size()));
				file.close();
				upgradeFromTmp();
			});
		return true;
	}

	bool upgradeFromGithubLatest()
	{
		ZoneScoped;
		LOG_INFO("Attempting upgrade from GitHub latest release");

		removeTmpFile();

		auto releaseInfo = getNewestGithubReleaseWithUpgradeAsset();
		if (!releaseInfo.has_value())
		{
			LOG_ERROR("Unable to determine latest GitHub release asset");
			return false;
		}

		std::string downloadUrl = releaseInfo->downloadUrl;

		// Parse download URL (expecting https://host/path)
		const std::string httpsPrefix = "https://";
		if (downloadUrl.rfind(httpsPrefix, 0) == 0)
			downloadUrl = downloadUrl.substr(httpsPrefix.size());
		auto slashPos = downloadUrl.find('/');
		if (slashPos == std::string::npos)
		{
			LOG_ERROR("Invalid download URL: {:s}", downloadUrl);
			return false;
		}
		std::string host = downloadUrl.substr(0, slashPos);
		std::string path = downloadUrl.substr(slashPos);

		HttpRequest req2;
		req2.method = HTTP_GET;
		req2.scheme = "https";
		req2.host = host;
		req2.path = path;
		req2.headers["User-Agent"] = "DuetScreenUpgradeHelper";
		req2.timeout = HTTP_TIMEOUT * 6; // give more time for download

		req2.DumpUrl();

		HttpResponse r2;
		hv::HttpClient cli2;
		cli2.send(&req2, &r2);
		if (r2.status_code != HTTP_STATUS_OK)
		{
			LOG_ERROR("Failed to download asset: HTTP {:d}", (int)r2.status_code);
			return false;
		}

		std::ofstream file(TMP_FILEPATH, std::ios::binary);
		if (!file.is_open())
		{
			LOG_ERROR("Failed to create file \"" TMP_FILEPATH "\"");
			return false;
		}
		file.write(r2.body.data(), r2.body.size());
		file.close();
		LOG_DBG("Downloaded asset to " TMP_FILEPATH);

		return upgradeFromTmp();
	}

	std::optional<std::string> checkForUpdate()
	{
		ZoneScoped;

		auto releaseInfo = getNewestGithubReleaseWithUpgradeAsset();
		if (!releaseInfo.has_value())
		{
			return std::nullopt;
		}

		const std::string& latestTag = releaseInfo->tag;
		auto installedVersion = parseVersionComponents(FIRMWARE_VERSION);
		auto latestVersion = parseVersionComponents(latestTag);
		if (!installedVersion.has_value() || !latestVersion.has_value())
		{
			LOG_WARN("Unable to compare versions, installed='{:s}', latest='{:s}'", FIRMWARE_VERSION, latestTag);
			return std::nullopt;
		}

		if (compareVersions(*latestVersion, *installedVersion) > 0)
		{
			return latestTag;
		}

		return std::nullopt;
	}
} // namespace UpgradeHelper
