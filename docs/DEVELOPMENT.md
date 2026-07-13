# Development

## Get started

This project is developed and tested in the included devcontainer on Ubuntu 24.04 LTS. The supported setup is VS Code + Docker via the files in `.devcontainer/`.

It might be possible to develop on other Linux distributions or operating systems, or to work outside the devcontainer, but that is not the primary workflow documented here.

Clone the project and the related sub modules:

```bash
git clone --recursive https://github.com/Duet3D/DuetScreen.git
```
or
```bash
git clone --recursive git@github.com:Duet3D/DuetScreen.git
```

> [!WARNING]
> The project uses Git Submodules. When cloning the project or checking out a branch/commit, make sure to run `git submodule update --init --recursive` to ensure that the submodules are checked out to the correct commit.

The devcontainer installs the required build tools, Python dependencies, and `gcc-15` automatically. You only need to install `gcc-15` yourself if you deliberately build the simulation outside the devcontainer.

## Setting up VSCode

The project is set up to use VS Code as the development environment. The project uses CMake and can be built entirely from a CLI, but the intended workflow is to do that from inside the devcontainer.

The following host tools are required:
- Docker Engine or Docker Desktop
- VS Code

The following VS Code extensions are required on the host:
- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
- [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

These extensions are recommended but not required:
- [Smart File Templates](https://marketplace.visualstudio.com/items?itemName=TrevorNesbitt.smart-file-templates)
- [LLDB](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb)

If you want to upload or debug code running on the physical screen, clone [buildroot-duetscreen](https://github.com/Duet3D/buildroot-duetscreen) as a sibling of this repository before opening the devcontainer:

```text
parent-folder/
├── DuetScreen/
└── buildroot-duetscreen/
```

When the folder layout matches this structure, the devcontainer automatically bind mounts `../buildroot-duetscreen` to `/workspaces/buildroot-duetscreen` and the existing T113 tasks work without extra VS Code settings.

## Devcontainer

This repository includes a devcontainer under `.devcontainer/` for Linux development in Docker.

1. Clone this repository with submodules.
2. Optionally clone `buildroot-duetscreen` next to it if you need T113 build, deploy, or remote debug tasks.
3. Open the `DuetScreen` folder in VS Code.
4. Run `Dev Containers: Reopen in Container`.
5. Wait for the post-create setup to finish.

The post-create step creates `env/` if needed and installs the Python requirements automatically.

If a folder named `buildroot-duetscreen` exists in the parent directory of this repository, the devcontainer setup automatically bind mounts it at `/workspaces/buildroot-duetscreen`.

This allows the existing T113 tasks that use `${config:buildroot_path}` to work from inside the container with `buildroot_path` set to `/workspaces/buildroot-duetscreen`.

You do not need to run `scripts/install_prerequisites.sh` or `scripts/install_gcc15.sh` when using the devcontainer.

## Simulating

It is possible to simulate the GUI on PC without access to the physical hardware. This can be beneficial for testing and development purposes as it allows for debugging using gdb. 

The easiest way to set up the simulation environment is to use VS Code inside the devcontainer with the provided configurations.

The simulation is only setup to run on Linux or WSL2 on Windows.

After reopening in the devcontainer, the following steps are required to run the GUI on PC:

### Setup udev rules for USB communications
> [!NOTE]
> This step is only required if you want to communicate between the PC and the Duet3D mainboard via USB. This is not required for simulating the GUI on PC.
> If you want to skip this step, you can communicate with the Duet3D mainboard via WiFi instead.

```bash
sudo bash -c 'cat ./config/99-usb.rules > /etc/udev/rules.d/99-usb.rules'
sudo service udev restart
sudo usermod -aG plugdev $USER
```

#### *WSL2 only* Setup USBIPD:
- Attach the Duet as a USB device using usbipd

> [!NOTE]
> The https://marketplace.visualstudio.com/items?itemName=thecreativedodo.usbip-connect extension for VSCode can be used to make this easier.

### Configure the project:
> [!NOTE]
> Configuring and building the project can be skipped if you intend to debug the code since it can be done by running the `Debug DuetScreen` debug configuration in VSCode.

A number of CMake presets are provided for different configurations. The 2 main ones for simulation are: `Simulation` and `Simulation-Release`. If you want to have breakpoint debugging and performance/memory overlays enabled, use the `Simulation` preset, otherwise use the `Simulation-Release` preset.

```bash
cmake --preset Simulation
```
or use the release configuration to remove performance and memory overlays (breakpoint debugging won't work):
```bash
cmake --preset Simulation-Release
```

### Build the project:
Use the same preset name as used for configuration:
```bash
cmake --build --preset Simulation
```
or
```bash
cmake --build --preset Simulation-Release
```

### Run the simulation:
The built binary can be found in the `out/build/<preset_name>/` directory. Run it using:
```bash
./out/build/Simulation/DuetScreen
```
or
```bash
./out/build/Simulation-Release/DuetScreen
```

If you are intentionally building outside the devcontainer, install the native dependencies first:

```bash
./scripts/install_prerequisites.sh
./scripts/install_gcc15.sh
```

## Building for the Duet3D screen
1. Clone the [buildroot-duetscreen](https://github.com/Duet3D/buildroot-duetscreen) project next to this repository.
2. Checkout the `master` branch.
3. Reopen `DuetScreen` in the devcontainer so the sibling buildroot checkout is mounted automatically.
4. Enable SSH on the Duet3D screen.
  - You can enable SSH by adding a file called `ssh` to the root of the microSD card on first boot and setting a password or `authorized_keys` file. https://github.com/Duet3D/buildroot-duetscreen/blob/master/BOOT.md#ssh
5. In vscode, run the `Push DuetScreen - SSH` task.
  - This will prompt for the build type (`Release`, `Release_with_profiling`, or `Debug`) and the screen's IP address, then build the project and push the binary to the Duet3D screen.
6. The code will not automatically start running on the Duet3D screen. You can run the `Start DuetScreen on remote` task to start the code.
7. Alternatively, you can start a remote debug session using the `Remote Debug DuetScreen` configuration. This will start the code and attach gdb to it.

## Debugging / Running Simulation
> [!NOTE]
> Debugging can be used as an easy way to configure and build the program even if you don't want to use the breakpoint debugging.

The program can be debugged using gdb when running as a simulation or on the physical hardware. lldb can also be used instead of gdb for simulation debugging if preferred.

VSCode has been configured for both of these scenarios.
- To debug the simulation, select the `Debug DuetScreen` configuration in VSCode and start debugging.
- To debug the code running on the physical hardware, select the `Remote Debug DuetScreen` configuration in VSCode and start debugging.
  - This will start the code and attach gdb to it.
  - If the code is already running, you need to kill it first. This can be done by pushing a new build to the Duet3D screen with the `Push DuetScreen - SSH` task (select the `Debug` build type).

## Commit messages
Commit messages should be in the following format:
```
<type>(<scope>): <short description>
```

Where:
- `<type>` is the type of change being made. Valid types are:
  - `feat`: A new feature
  - `fix`: A bug fix
  - `docs`: Documentation changes
  - `refactor`: Code changes that neither fix a bug nor add a feature
  - `test`: Adding or updating tests
  - `chore`: Changes that are neither a bug fix nor a feature (eg. build scripts, CI/CD, etc.)
- `<scope>` is the area of the code being changed. This can be a specific module
  - Typically this would be the name of a directory in `src/` or `assets/` (eg. `UI`, `Network`, `i18n`, etc.)
  - If the change affects multiple areas of the code, use the scope that is most relevant
- `<short description>` is a brief description of the change being made. This should be written in the imperative mood (eg. "Add feature" not "Added feature" or "Adds feature").
  - Multi-line descriptions are allowed but require a blank line between the short description and the long description.

> [!WARNING]
> The commit message format is used to automatically generate the changelog for the project. Pull Requests that do not follow this format may be rejected.

## Assets

### Adding a new language (i18n)
- Language files are located in the `assets/i18n/` directory.
- Each language file is a JSON file with the following structure:
```json
{
  "readable": "English (UK)",
  "translations": {
    "key": "translation",
    "parent_key": {
      "child_key": "child_translation",
      "child_key_2": "child_translation_2"
    }
    ...
}
```
- The `readable` field is the name of the language as it will appear in the language selection dropdown in the GUI.
- The `translations` field is a dictionary of key-value pairs where the key is the identifier used in the code and the value is the translation.
- The language file must be named using the [IETF language tag](https://en.wikipedia.org/wiki/IETF_language_tag) format. For example, `en-GB.json` for English (UK) or `fr-FR.json` for French (France).
- To add a new language, create a new JSON file in the `assets/i18n/` directory with the appropriate structure and name.
- Translation keys support nesting using objects of arbitrary depth. For example, the key `parent_key.child_key` can be used to access the translation for `child_key` under `parent_key`.
- Some translations may include formatting placeholders. The [fmt](https://github.com/fmtlib/fmt) library is used for string formatting. A good reference for the formatting syntax can be found [here](https://hackingcpp.com/cpp/libs/fmt.html).
  - The [en-GB.json](../assets/i18n/en-GB.json) file contains the reference implementation for all formatting placeholders. If the order of the placeholders needs to be changed for a specific language, this can be done by changing the order of the placeholders in the translation string and adding the appropriate index to the placeholder.
  ```json
  {
    "example": "This is an example of a placeholder: {:s}, and another one: {:d}",
    "example_reordered": "This is an example of a placeholder: {1:d}, and another one: {0:s}"
  }
  ```

> [!NOTE]
> The language files are loaded at runtime without need to be compiled into the binary. When simulating on PC, the language files are loaded from the `assets/i18n/` directory in the project. When running on the Duet3D screen, the language files are loaded from the `/etc/assets/i18n/` directory.

Running `python scripts/manage_translations.py` will check for missing translation keys in the language files and prompt the user to enter translations for any missing keys. This can be used to easily keep the language files up to date with the code.
Use `python scripts/manage_translations.py --help` for more information on how to use the script.

### Adding a new icon set
- Icon sets are located in the `assets/icons/` directory.
- Each icon set is a subdirectory in the `assets/icons/` directory.
- To add a new icon set, create a new subdirectory in the `assets/icons/` directory and add the icons to it.
- Icons should be PNG format with transparent backgrounds
- Icon colour does not matter as the icons are recoloured at runtime to match the current theme.
- The icon set can be selected in the GUI settings under `Settings > Display > Icons`
- The filenames of the icons should match the filenames used in the code. If in doubt look at the existing icon sets for reference.
- A translation key is used to provide a human readable name for the icon set in the GUI. This key should be added to the `i18n` language files under the `theme.icon_sets.{icon_set_folder_name}` key. For example, for an icon set called `example`, the translation key would be `theme.icon_sets.example`.
- https://fonts.google.com/icons is a good source for icons.

### Adding a new font
- Fonts are located in the `assets/fonts/` directory.
- A font can be a `.ttf` or a compiled `.bin` lvgl font file.
- Some themes set different weights for different UI elements. For this to be supported, the font must be a variable weight `.ttf` font.
  - These fonts contain multiple weights in a single file and allow the weight to be changed at runtime.
  - https://fonts.google.com/?categoryFilters=Technology:%2FTechnology%2FVariable is a good source for variable weight fonts.
- To add a new font, add the font file to the `assets/fonts/` directory.
- The font can be selected in the GUI settings under `Settings > Display > Font`

### Adding assets to the DuetScreen
The above sections describe how to add assets to the project which will then be included in the `DuetScreen.tar.gz` file in the next release or manual build. If you want to add or update assets at runtime without needing to rebuild the project, you can do any of the following:
1. Remove the SD card from the DuetScreen and insert it into a PC that is capable of reading ext4 file systems. Then copy the new/updated assets to the appropriate directories under `/etc/assets/` on the SD card. Reinsert the SD card into the DuetScreen and reboot.
2. Use SCP to copy the assets to the screen over the network.
    - This requires SSH to be enabled on the DuetScreen.
    - Example command to copy a new language file:
      ```bash
      scp path/to/new_language.json root@<duetscreen_ip>:/etc/assets/i18n/
      ```
3. Create a `DuetScreen.tar.gz` file with `./scripts/create_upgrade.sh --skip-binary`, and use it to update the screen via the GUI upgrade process.
    - This will only update the assets and not the binary.
    - Note that this process will first delete all existing assets on the screen before copying the new ones.

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

![Example of a failed UI test](images/DEVELOPMENT/ui_test_example.png)

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


### Hardware Testing

Hardware testing is used to verify that the PCB hardware is functioning correctly. The following tests are conducted:
- Touchscreen calibration
  - Comparison of touch input coordinates with expected values
- Dead/stuck pixel test
  - Shows Red, Green, Blue, and White screens to check for dead/stuck pixels
- Memory test
  - Validates the internal NAND flash
- WiFi test
  - Validates the internal WiFi module is recognised
- USB-A test
  - Writes and reads data to a flash drive via USB-A port
- Buzzer test
  - Plays a sound through the buzzer to verify functionality
- Speaker test
  - Plays a sound through the speaker to verify functionality

The hardware tests can be started from the developer settings screen.

#### Building

A version of the UI which boots straight into the hardware tests can be created by setting the cmake cache variable `HARDWARE_TEST` to `ON`.

> [!NOTE]
> `ccmake <path_to_build_dir>` can be used to change the CMake cache variables.
> Or `cmake -DHARDWARE_TEST=ON --preset <preset_name>` can be used to set the variable.

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
If you want to view the logs with a GUI, you can follow the instructions in [Tracing](#tracing) to use the tracy profiler. The logs are available under the "Messages" tab in tracy. This works for both simulation and code running on the Duet3D screen (if `DUETSCREEN_ENABLE_PROFILING` is `ON`). Comma separated filters can be applied to the logs in tracy.

If you want to filter the logs and have the output as text, you can use the `scripts/filter_logs.py` script.

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

### Tracing
Tracing support has been incorporated into the program using [tracy](https://github.com/wolfpld/tracy.git).

This allows the performance and timing of the program to be analysed in real time without using breakpoints or pausing the program. The tracy profiler adds minimal overhead to the DuetScreen program and is only active when a tracy server is connected.

All log messages are also sent to tracy for easy viewing and filtering.

![alt text](images/DEVELOPMENT/tracy.png)

Tracy can be used when simulating on PC or when running on the physical Duet3D screen.

> [!NOTE]
> The screen must be connected to the same network as the machine running the tracy server.

> [!NOTE]
> Tracy support is only enabled in `Debug` builds by default. For remote deployment, use the `Release_with_profiling` build type in the `Push DuetScreen - SSH` task — this uses the `T113-Release_with_profiling` preset which enables both `DUETSCREEN_ENABLE_PROFILING` and `DUETSCREEN_ENABLE_LV_PROFILING`. To enable profiling manually in any other build, set those cmake cache variables to `ON`.

#### Building tracy server
A prebuilt tracy server binary for Windows can be downloaded from the [tracy releases page](https://github.com/wolfpld/tracy/releases), the version must match the tracy version used in this project. For simplicity a copy of the compatible tracy server binary for Windows is included in the [tools/win32/](../tools/win32/tracy-profiler.exe) directory.

The tracy server can be built from source for Linux and MacOS in the [libraries/tracy/](../libraries/tracy/) directory. Instructions for building the tracy server can be found in the [tracy repository](../libraries/tracy/manual/tracy.md#buildingserver)

To build on Ubuntu 24.04 LTS the following commands worked for me but your mileage may vary:
```bash
cd libraries/tracy
cmake -B ./profiler/build -S ./profiler -DCMAKE_BUILD_TYPE=Release -DLEGACY=1
cmake --build ./profiler/build --config Release --parallel
```

Then run the tracy server using:
```bash
./libraries/tracy/profiler/build/tracy-profiler
```