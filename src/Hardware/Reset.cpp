/*
 * Reset.cpp
 *
 * Created: 19/02/2023
 * Author: Loïc G.
 *
 */

#include "Reset.h"
#include "Comm/Connection.h"
#include "Debug.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "tracy/Tracy.hpp"
#include "utils/NetworkHelper.h"
#include "utils/SystemHelper.h"

#include <cstdlib>
#include <filesystem>
#include <sys/reboot.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Restart the program
	 */
	void Restart() noexcept
	{
		ZoneScoped;
		Comm::Connection::get().stop(); // the port closed properly, everything waiting told
#if SIMULATION
		exit(EXIT_SUCCESS);
#else
	SystemHelper::restartService(SystemHelper::Services::DUETSCREEN_MONITOR);
#endif
	}

	/**
	 * @brief Reboot the hardware
	 */
	void Reboot() noexcept
	{
		ZoneScoped;
		Comm::Connection::get().stop(); // the port closed properly, everything waiting told
#if !SIMULATION
		reboot(RB_AUTOBOOT);
#endif
	}

	/**
	 * @brief Erase every setting, the WiFi networks the screen has joined and the layouts made or saved on it, then
	 * restart
	 */
	void EraseAndRestart() noexcept
	{
		ZoneScoped;
#if SIMULATION
		if (!std::filesystem::remove("config.json"))
		{
			LOG_ERROR("Failed to remove config.json file");
		}
#else
	if (!std::filesystem::remove("/etc/duetscreen.json"))
	{
		LOG_ERROR("Failed to remove /etc/duetscreen.json file");
	}
#endif
		NetworkHelper::forgetAllNetworks();
		UI::Xml::eraseUserLayouts();

		Restart();
	}

#ifdef __cplusplus
}
#endif
