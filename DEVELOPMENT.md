# Development

## Get started

Clone the project and the related sub modules:

```bash
git clone --recursive https://github.com/Duet3D/DuetScreen.git
```

note if you are a member of the Duet3D organisation andd want to close with ssh use:
```bash
git clone --recursive git@github.com:Duet3D/DuetScreen.git
```

The following steps are required to setup VSCode as the development environment for the project.
- Copy `.vscode/settings.json.default` to `.vscode/settings.json`
- If you want to debug code running on the physical screen then you will need to:
  - Clone the [buildroot-duetscreen](https://github.com/Duet3D/buildroot-duetscreen) repository.
  - Build the [buildroot-duetscreen](https://github.com/Duet3D/buildroot-duetscreen) project.
  - Set the `buildroot_path` and `duetscreen_ip` settings in `.vscode/settings.json` to the correct values.

## Simulating
It is possible to simulate the GUI on PC without access to the physical hardware. This can be beneficial for testing and development purposes as it allows for debugging using gdb. 

The simulation is only setup to run on Linux or WSL2 on Windows.

The following steps are required to run the GUI on PC:

### Install the required dependencies:
```bash
sudo apt-get update && sudo apt-get install -y build-essential libsdl2-dev cmake ninja-build libusb-1.0-0-dev pkg-config clang-format libavcodec-dev libavformat-dev libswscale-dev libavutil-dev libpng-dev libspdlog-dev
```

### Setup udev rules for USB communications
```bash
sudo bash -c 'cat ./config/99-usb.rules > /etc/udev/rules.d/99-usb.rules'
sudo service udev restart
sudo usermod -aG plugdev $USER
```

### *WSL2 only* Setup USBIPD:
- Attach the Duet as a USB device using usbipd


### Configure the project:
> [!NOTE]
> Configuring and building the project can be skipped if you intend to debug the code since it can be done by running the `Debug DuetScreen` debug configuration in VSCode.

```bash
cmake --preset Simulation
```

### Build the project:
```bash
cmake --build --preset Simulation
```

## Building for the Duet3D screen
1. Clone the [buildroot-duetscreen](https://github.com/Duet3D/buildroot-duetscreen) project
2. Checkout the `dev` branch
3. Enable SSH on the Duet3D screen
  - This can be done in the GUI by going to `Settings` > `Developer` > `Enable SSH`
  - Alternatively, you can enable SSH by adding a file called `ssh` to the root of the microSD card.
4. In vscode, run the `Push DuetScreen - SSH - Release` task.
  - This will build the project and push the binary to the Duet3D screen.
  - Use the `Push DuetScreen - SSH - Debug` task to push the debug version of the binary.
5. The code will not automatically start running on the Duet3D screen. You can run the `Start DuetScreen on remote` task to start the code.
6. Alternatively, you can start a remote debug session using the `Remote Debug DuetScreen` configuration. This will start the code and attach gdb to it.

## Debugging
The program can be debugged using gdb when running as a simulation or on the physical hardware.

VSCode has been configured for both of these scenarios.
- To debug the simulation, select the `Debug DuetScreen` configuration in VSCode and start debugging.
- To debug the code running on the physical hardware, select the `Remote Debug DuetScreen` configuration in VSCode and start debugging.
  - This will start the code and attach gdb to it.
  - If the code is already running, you need to kill it first. This can be done by pushing a new build to the Duet3D screen with the `Push DuetScreen - SSH - Debug` task.


## Testing

The project is setup to use Google Test (gtest) for unit testing.

### UI testing (image comparison)
The UI has image-based regression tests that render components/views and compare them against validated reference images.

- Test sources: `tests/src/test_cases/UI/`
- Reference images: `tests/ref_imgs/`
- On mismatch, a new image is created next to the reference with the suffix `_err`.

A test suite would look like this:
```cpp
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

class MyTestSuite : public UiTestSuite
{
  public:
	MyTestSuite() // run before each test in suite
		: btn("test_button", lv_screen_active())
	{
	}

  ~MyTestSuite() = default; // run after each test in suite

	UI::Button btn;
};

TEST_F(MyTestSuite, Basic) {
    EXPECT_EQUAL_SCREENSHOT("button_basic.png");
}

TEST_F(MyTestSuite, WithText) {
    btn.setText("Click Me");
    EXPECT_EQUAL_SCREENSHOT("button_with_text.png");
}
```

#### How to run
You can run the tests via VS Code or from the terminal.

- VS Code task: Terminal > Run Task… > `Run Tests`
- CLI:
```bash
python3 scripts/run_tests.py
```

The script will:
1) Delete any existing `*_err.*` files in `tests/ref_imgs/`
2) Configure CMake if needed (default preset: `Simulation`)
3) Build the test binary (`DuetScreen.tests`)
4) Run the tests (via `ctest`)
5) If any `*_err` images are produced, open a full-screen review window where you can update references

#### Review UI
- Layout: Top row shows Reference (left) and New (_err) (right). Bottom row shows the visual Difference (RGB-only) centered.
- Images auto-scale to fit the window/display while keeping aspect ratio.
- Controls: Previous [←], Next [→], Update [Y], Skip [N], Update All [A], Quit [Q/Esc]
- “Update” replaces the reference image with the `_err` image and removes the `_err` file.

![Example of a failed UI test](docs/ui_test_example.png)

#### Prerequisites
- Build tools: cmake, ninja, SDL2, etc. (see Simulating section above)
- Python packages for the GUI reviewer:
  - Pillow (for image loading and scaling)
  - Tkinter (for the GUI)

On Ubuntu/Debian you can install these with:
```bash
sudo apt-get install -y python3-pil python3-tk
```
Alternatively, install Pillow via pip:
```bash
pip install pillow
```

If Pillow/Tkinter are unavailable, the script falls back to a CLI prompt without image previews.

#### Environment variables (optional)
- `DUETSCREEN_CMAKE_PRESET` — CMake preset to configure (default: `Simulation`)
- `DUETSCREEN_BUILD_DIR` — Use a specific build directory (otherwise auto-detected under `out/build`)
- `DUETSCREEN_SCREEN_WIDTH` / `DUETSCREEN_SCREEN_HEIGHT` — Override detected screen size for scaling (useful in headless/remote sessions)

#### Troubleshooting
- If no build directory is found, the script runs `cmake --preset <preset>` automatically.
- If tests fail to run, check that `DuetScreen.tests` exists in `out/build/<preset>/tests/` and that `ctest` is available in PATH.
- If the review window doesn’t appear (headless), set the screen width/height env vars or run with CLI fallback.


## Logs
The code generates logs that are output to 3 places:
1. The console
2. A log file:
    - The log file is located in the `/var/log/` directory on the Duet3D screen.
    - The log file is located in the working directory when starting the simulation on PC.
    - The log file is called `DuetScreen.log`.
    - The log file is rotated after it exceeds a certain size (~5-10MB).
3. The GUI:
  - This needs to be enabled in the GUI settings.
  - Only `WARN` and `ERROR` messages are shown in the GUI to prevent lag.

### Log levels
The log levels are as follows:
- `VERBOSE`: All messages are shown. This is useful for very fine grained debugging but should generally not be needed.
- `DEBUG`: Debug messages are shown. This is useful for general debugging.
- `INFO`: Informational messages are shown. This is useful for general information.
- `WARN`: Warning messages are shown. These represent issues that may require attention.
- `ERROR`: Error messages are shown. These represent when something has failed unexpectedly (ie a network request).
- `FATAL`: Fatal messages are shown. These represent events that the program cannot recover from.

All messages more severe than the selected log level are shown. For example, if the log level is set to `WARN`, then all `WARN` and `ERROR` messages are shown.


### Filtering logs
When using `DEBUG` and `VERBOSE` log levels, the logs are generated too fast to be useful. Since you generally only want to see the logs from a part of the code, you can use the `scripts/filter_logs.py` script to filter the logs.

This script works for both the simulation and code running on the Duet3D screen.

Arguments:
- `--follow`, `-f`: Follow the log file. This will show new log messages as they are added to the file. The code will not exit until the user presses `Ctrl+C`.
- `--remote`, `-r`: The IP address of a Duet3D screen to get the log file from. This will use SSH to copy the log file to the local machine and then filter it. If `--follow` is used the log file is monitored without copying it to the local machine.
- `--output`, `-o`: An optional output file. If not specified, the output will be printed to the console.
- `filters`: all remaining arguments are used as filters. Multiple filters can be used. The filters are case insensitive and space separated. To use a filter with spaces, use quotes. For example: `"My Filter"`.

Examples:
```bash
# Follow the log file from a Duet3D screen with IP address 192.168.0.20, filter the logs for "My Filter" and "Another Filter", and output to a file called filtered.log
python scripts/filter_logs.py --follow --remote 192.168.0.20 --output filtered.log "My Filter" "Another Filter"

# Filter the local log files for "My Filter" and "Another Filter", output just to the console
python scripts/filter_logs.py "My Filter" "Another Filter"
```

### Download logs from the Duet3D screen
1. Enable SSH on the Duet3D screen
2. Run `scp root@<ip_address>:/var/log/DuetScreen.log .` to download the log file to the current directory.
