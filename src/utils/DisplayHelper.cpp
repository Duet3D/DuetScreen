/*
 * DisplayHelper.cpp
 *
 *  Created on: 2025-02-26
 *      Author: Andy Everitt
 */

#include "DisplayHelper.h"
#include "Debug.h"
#include "UI/Components/LVGL/LvObj.h" // for UI_LOCK
#include "utils/StorageHelper.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

#if T113
// Defined in buildroot-duetscreen/output/build/linux-.../include/video/sunxi_display2.h
#  define DISP_LCD_SET_BRIGHTNESS 0x102
#  define DISP_LCD_GET_BRIGHTNESS 0x103
#  define DISP_LCD_BACKLIGHT_ENABLE 0x104
#  define DISP_LCD_BACKLIGHT_DISABLE 0x105
#endif

// Define a structure to hold parameters for brightness operations.
// The actual structure may differ based on the driver's header files.
struct BrightnessParam
{
	unsigned int screen;	 // Which screen (0 or 1)
	unsigned int brightness; // Brightness value [0-255]
};

// Constructor: Opens the display device.
DisplayHelper::DisplayHelper(const char* device, unsigned int screen)
	: m_screen(screen)
{
	ZoneScoped;
#if T113
	m_fd = open(device, O_RDWR);
	if (m_fd < 0)
	{
		throw std::runtime_error(std::string("Failed to open device: ") + strerror(errno));
	}
#else
	UNUSED(device);
	UNUSED(screen);
#endif
}

// Destructor: Closes the device.
DisplayHelper::~DisplayHelper()
{
	ZoneScoped;
	if (m_fd >= 0)
	{
		close(m_fd);
	}
}

DisplayHelper& DisplayHelper::instance()
{
	ZoneScoped;
	static DisplayHelper instance;
	return instance;
}

// Sets the display brightness.
// brightness should be in the range [0, 100]
bool DisplayHelper::setBrightness(unsigned int percentage)
{
	ZoneScoped;
	auto& disp = instance();
	disp.m_percentage = percentage;
	disp.setBrightnessInner(percentage);
	StorageHelper::setData(ID_SYS_BRIGHTNESS_KEY, percentage);
	return true;
}

bool DisplayHelper::setScreenSaverBrightness(unsigned int percentage)
{
	ZoneScoped;
	auto& disp = instance();
	disp.m_screensaverPercentage = percentage;
	return true;
}

void DisplayHelper::enableScreenSaver(bool enable)
{
	ZoneScoped;
	auto& disp = instance();
	disp.setBrightnessInner(enable ? disp.m_screensaverPercentage : disp.m_percentage);
}

bool DisplayHelper::setBrightnessInner(unsigned int percentage)
{
	ZoneScoped;
	auto& disp = instance();
	BrightnessParam param;
	percentage = std::clamp(percentage, 0u, 100u);

	// Scale brightness from 0-100 to 0-255 for the hardware
	unsigned int brightness = (percentage * 255) / 100;
	param.screen = disp.m_screen;
	param.brightness = brightness;

	if (brightness == disp.m_currentBrightness)
	{
		return true;
	}
	m_currentBrightness = brightness;
#if T113
	if (ioctl(disp.m_fd, DISP_LCD_SET_BRIGHTNESS, &param) < 0)
	{
		LOG_ERROR("ioctl setBrightness failed");
		return false;
	}
#else
	UNUSED(param);
#endif
	return true;
}

// Gets the current display brightness.
// Returns a brightness value in the range [0, 100].
unsigned int DisplayHelper::getBrightness()
{
	ZoneScoped;
	auto& disp = instance();
	return disp.m_percentage;
}

static lv_display_rotation_t toLvRotation(DisplayRotation rotation)
{
	switch (rotation)
	{
	case DisplayRotation::ROTATION_90:
		return LV_DISPLAY_ROTATION_90;
	case DisplayRotation::ROTATION_180:
		return LV_DISPLAY_ROTATION_180;
	case DisplayRotation::ROTATION_270:
		return LV_DISPLAY_ROTATION_270;
	case DisplayRotation::ROTATION_0:
	default:
		return LV_DISPLAY_ROTATION_0;
	}
}

static bool updateSplashScreenRotation(DisplayRotation rotation)
{
	ZoneScoped;
#if T113
	const int32_t rotationValue = rotation == DisplayRotation::ROTATION_0	  ? 0
								  : rotation == DisplayRotation::ROTATION_90  ? 90
								  : rotation == DisplayRotation::ROTATION_180 ? 180
								  : rotation == DisplayRotation::ROTATION_270 ? 270
																			  : 0;
	std::string splashFilePath = fmt::format("/etc/splash/splash_{:d}.png", rotationValue);
	if (!std::filesystem::exists(splashFilePath))
	{
		LOG_WARN("Splash screen file does not exist: {:s}", splashFilePath);
		return false;
	}

	return std::filesystem::copy_file(
		splashFilePath, "/etc/splash.png", std::filesystem::copy_options::overwrite_existing);
#else
	UNUSED(rotation);
#endif
	return true;
}

// Rotates the display and persists the setting.
void DisplayHelper::setRotation(DisplayRotation rotation)
{
	ZoneScoped;
	UI_LOCK();
	lv_display_set_rotation(lv_display_get_default(), toLvRotation(rotation));
	updateSplashScreenRotation(rotation);
	StorageHelper::setData(ID_DISPLAY_ROTATION, rotation);
}

// Gets the currently stored display rotation.
DisplayRotation DisplayHelper::getRotation()
{
	ZoneScoped;
	return StorageHelper::getData(ID_DISPLAY_ROTATION);
}
