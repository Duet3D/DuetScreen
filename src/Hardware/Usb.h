/*
 * Usb.h
 *
 * USB drives plugged into the screen. The system mounts each at /media/usb0 to /media/usb7; UsbMonitor looks for them
 * every half second and tells whoever registered (the UI, as EventType::UsbDrive) when one comes or goes. In the
 * simulation, folders named usb0 to usb7 in ./var/media stand in for them.
 *
 *  Created on: 6 Feb 2024
 *      Author: andy
 */

#ifndef JNI_HARDWARE_USB_H_
#define JNI_HARDWARE_USB_H_

#include "Debug.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace USB
{
	/** Read the whole of the file at `filePath` into `contents`; false, logged, if it can't be */
	bool ReadFileContents(const std::string& filePath, std::string& contents);

	/** Told that the drive at `path` was mounted (`connected`), or taken out */
	using UsbDriveCallback = std::function<void(const std::string& path, bool connected)>;

	class UsbMonitor
	{
	  public:
		/** The one monitor */
		static UsbMonitor& getInstance()
		{
			static UsbMonitor instance;
			return instance;
		}

		UsbMonitor(const UsbMonitor&) = delete;
		UsbMonitor& operator=(const UsbMonitor&) = delete;

		/** Look for drives every half second, on a thread of its own */
		void startMonitoring();

		/** Stop looking for drives */
		void stopMonitoring();

		/** Tell `callback` whenever a drive comes or goes; with `initialNotify`, of the drives mounted now too */
		void registerCallback(UsbDriveCallback callback, bool initialNotify = false);

		/** The drives mounted, e.g. "/media/usb0", in order */
		std::vector<std::string> getMountedDrives() const;

		/**
		 * Look for drives in `root` from now on, as folders usb0 to usb7 in it, each counting as a drive while it
		 * exists, as in the simulation; tests give a folder of their own. The drives already known are forgotten,
		 * each told as taken out.
		 */
		void setRoot(std::filesystem::path root);

		/** Look for drives now, telling the callbacks of any that came or went */
		void scan();

	  private:
		UsbMonitor();
		~UsbMonitor();

		/** Look for drives every half second until stopped */
		void monitorThread();

		/** Tell every callback that the drive at `path` came (`connected`) or went */
		void notifyCallbacks(const std::string& path, bool connected);

		/** The drives mounted now, in order */
		std::vector<std::string> findDrives() const;

		std::thread m_thread;									// Looks for drives, while monitoring
		std::atomic<bool> m_running = false;					// Monitoring
		mutable TracyLockable(std::mutex, m_mutex);				// Guards everything below
		std::vector<UsbDriveCallback> m_callbacks;				// Told of drives coming and going
		std::vector<std::string> m_mounts;						// The drives last found
		std::filesystem::path m_root;							// Where drives are looked for
		bool m_mountPointsOnly;									// Only mount points count, as on the screen
		TracyLockable(std::mutex, m_scanMutex);					// One scan at a time
	};

} // namespace USB

#endif /* JNI_HARDWARE_USB_H_ */
