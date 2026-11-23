/*
 * Usb.cpp
 *
 *  Created on: 6 Feb 2024
 *      Author: andy
 */

#include "Usb.h"
#include "tracy/Tracy.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>

namespace USB
{
	namespace
	{
		/** How many drives the system mounts: /media/usb0 to /media/usb7 */
		constexpr int maxDrives = 8;

		/** Where the system mounts drives, or the folder standing in for it in the simulation */
		std::filesystem::path defaultRoot()
		{
#if SIMULATION
			return "./var/media";
#else
			return "/media";
#endif
		}

		/** The mount points in /proc/mounts */
		std::vector<std::string> mountPoints()
		{
			std::vector<std::string> points;
			std::ifstream mounts("/proc/mounts");
			std::string line;
			while (std::getline(mounts, line))
			{
				// "<device> <mount point> <type> <options> 0 0"
				std::istringstream fields(line);
				std::string device;
				std::string point;
				if (fields >> device >> point)
				{
					points.push_back(point);
				}
			}
			return points;
		}
	} // namespace

	bool ReadFileContents(const std::string& filePath, std::string& contents)
	{
		ZoneScoped;
		LOG_INFO("Reading file {:s}", filePath.c_str());
		std::ifstream file(filePath.c_str(), std::ios::in | std::ios::ate);

		if (!file.is_open())
		{
			LOG_ERROR("Unable to open file {:s}", filePath.c_str());
			return false;
		}

		const std::streampos endPos = file.tellg();
		if (endPos == std::streampos(-1))
		{
			LOG_ERROR("Failed to get file size for {:s}", filePath.c_str());
			return false;
		}
		file.seekg(0, std::ios::beg);
		const auto sizeOff = static_cast<std::streamoff>(endPos);
		contents.resize(static_cast<std::size_t>(sizeOff));
		LOG_DBG("Reading {:d} bytes", static_cast<int>(sizeOff));
		if (!file.read(contents.data(), static_cast<std::streamsize>(sizeOff)))
		{
			LOG_ERROR("Failed to read file {:s}", filePath.c_str());
			return false;
		}
		LOG_DBG("Read {:d} bytes", contents.size());
		return true;
	}

	UsbMonitor::UsbMonitor()
		: m_root(defaultRoot())
		, m_mountPointsOnly(!SIMULATION)
	{
	}

	UsbMonitor::~UsbMonitor()
	{
		ZoneScoped;
		stopMonitoring();
	}

	void UsbMonitor::startMonitoring()
	{
		ZoneScoped;
		if (!m_running.exchange(true))
		{
			m_thread = std::thread(&UsbMonitor::monitorThread, this);
		}
	}

	void UsbMonitor::stopMonitoring()
	{
		ZoneScoped;
		if (m_running.exchange(false) && m_thread.joinable())
		{
			m_thread.join();
		}
	}

	void UsbMonitor::registerCallback(UsbDriveCallback callback, bool initialNotify)
	{
		ZoneScoped;
		std::vector<std::string> mounts;
		{
			std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
			m_callbacks.push_back(callback);
			mounts = m_mounts;
		}
		if (initialNotify)
		{
			for (const auto& mount : mounts)
			{
				callback(mount, true);
			}
		}
	}

	std::vector<std::string> UsbMonitor::getMountedDrives() const
	{
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		return m_mounts;
	}

	void UsbMonitor::setRoot(std::filesystem::path root)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> scanning(m_scanMutex);
		std::vector<std::string> forgotten;
		{
			std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
			m_root = std::move(root);
			m_mountPointsOnly = false;
			forgotten = std::move(m_mounts);
			m_mounts.clear();
		}
		for (const auto& mount : forgotten)
		{
			notifyCallbacks(mount, false);
		}
	}

	std::vector<std::string> UsbMonitor::findDrives() const
	{
		ZoneScoped;
		std::filesystem::path root;
		bool mountPointsOnly = true;
		{
			std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
			root = m_root;
			mountPointsOnly = m_mountPointsOnly;
		}
		const std::vector<std::string> points = mountPointsOnly ? mountPoints() : std::vector<std::string>();
		std::vector<std::string> drives;
		for (int i = 0; i < maxDrives; i++)
		{
			const std::string drive = (root / fmt::format("usb{:d}", i)).string();
			std::error_code error;
			const bool found = mountPointsOnly ? std::ranges::find(points, drive) != points.end()
											   : std::filesystem::is_directory(drive, error);
			if (found)
			{
				drives.push_back(drive);
			}
		}
		return drives;
	}

	void UsbMonitor::scan()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> scanning(m_scanMutex);
		const std::vector<std::string> found = findDrives();
		std::vector<std::string> known;
		{
			std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
			known = m_mounts;
			m_mounts = found;
		}
		for (const auto& mount : found)
		{
			if (std::ranges::find(known, mount) == known.end())
			{
				LOG_INFO("USB drive mounted at: {:s}", mount);
				notifyCallbacks(mount, true);
			}
		}
		for (const auto& mount : known)
		{
			if (std::ranges::find(found, mount) == found.end())
			{
				LOG_INFO("USB drive unmounted from: {:s}", mount);
				notifyCallbacks(mount, false);
			}
		}
	}

	void UsbMonitor::notifyCallbacks(const std::string& path, bool connected)
	{
		ZoneScoped;
		std::vector<UsbDriveCallback> callbacks;
		{
			std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
			callbacks = m_callbacks;
		}
		for (const auto& callback : callbacks)
		{
			callback(path, connected);
		}
	}

	void UsbMonitor::monitorThread()
	{
		tracy::SetThreadName("USB Monitor Thread");
		LOG_VERBOSE("USB monitor thread started");
		while (m_running)
		{
			scan();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		LOG_VERBOSE("USB monitor thread stopped");
	}
} // namespace USB
