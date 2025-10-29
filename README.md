# DuetScreen

This project is a GUI for the Duet3D screen. It is based on the LittlevGL (LVGL) library. The program can be compiled to run on the Duet3D screen or on a PC. The PC version is useful for development and testing purposes.

## Getting started

### Flashing a new Duet3D screen
1. Download the latest `sdcard.img` from the release page.
2. Flash a microSD card with the image
    - use [balenaEtcher](https://www.balena.io/etcher/) on Windows
    - use `dd` on Linux
        - ```bash
            sudo dd if=sdcard.img of=/dev/sdX bs=4M
            ```
3. Insert the microSD card into the Duet3D screen and power it on.


### Connecting the Duet3D screen to a WiFi network
There are a few methods to connect the Duet3D screen to a WiFi network. The recommended method is to use the `wpa_supplicant.conf` file. This file should be placed on the microSD card after it has been flashed.

1. You can copy a file called `wpa_supplicant.conf` to the root of the microSD card. This file should contain the WiFi credentials in the following format:
    ```
    ctrl_interface=/var/run/wpa_supplicant
    update_config=1
    ap_scan=1

    network={
        ssid="your-SSID"
        psk="your-PASSWORD"
        key_mgmt=WPA-PSK
    }
    ```
    - This method is the easiest if you are setting up multiple screens, or you know the WiFi credentials in advance.
2. Alternatively, you can connect to a network using the GUI.
    - This method is useful if you are setting up a single screen and you do not know the WiFi credentials in advance.
    - The GUI is currently placeholder and has some known issues.
    - You need to power cycle the screen after selecting network mode in the GUI


## Powering the Duet3D screen
The Duet3D screen can be powered in the following ways:
- **5V_IN**: This is the recommended method. 
- **USB-C**: Relies on the connected host/device to supply sufficient power.
- **UART5**: This is a legacy method for compatibility with PanelDue wiring. It is not recommended for new installations.

> [!WARNING]
> All the power methods are **NOT** isolated. This means that if you connect the screen to a mainboard, the mainboard must be powered by the same power supply as the screen. If you do not do this, you may damage the screen or the mainboard.
> - The screen is designed to be powered by a 5V power supply. If you are using a 12V or 24V power supply, you will need to use a buck converter to step down the voltage to 5V. **You will likely damage the screen and any connected devices if you do not do this**.

## Connecting to a Duet3D mainboard
Multiple methods are available to connect the Duet3D screen to a mainboard. The recommended method is to use a USB cable. This allows for the best performance and is the easiest to set up.

### USB
1. Connect the Duet3D screen to the mainboard using a USB cable.
    - **Either** the **USB-A** and **USB-C** ports on the screen can be used.
    - If using the **USB-C** port, make sure to set the screen to USB **host mode**.
2. In the GUI, select the USB connection method.

> [!NOTE]
> When the Duet3D screen detects a USB connection to a Duet3D mainboard, it will automatically send `M575 P0 S0` to configure the mainboard for USB communication.

### WiFi
> [!NOTE]
> The screen has a built-in WiFi module, it also supports external WiFi modules with the `RTL8188FU` chipset.
> - If using the built-in WiFi module, the USB-C port must be set to USB host mode, this is done in the setting GUI.
> - If using an external WiFi module, connect it to the USB-A port on the screen, or use the USB-C port and set it to USB host mode.
> - There are multiple variants of the `RTL8188` chipset. Currently the screen only supports `RTL8188FU`. Other variants are unlikely to work.

1. Ensure the Duet3D screen is connected to the same WiFi network as the mainboard.
    - See the [Connecting the Duet3D screen to a WiFi network](#connecting-the-duet3d-screen-to-a-wifi-network) section above.
2. In the GUI, select the WiFi connection method.
3. Power cycle the screen.
4. Once rebooted, enter the IP address of the mainboard.

### UART

> [!WARNING]
> This method is for legacy support only to provide an easy upgrade path for PanelDue users. It is not recommended for new installations.

1. Connect the Duet3D screen to the mainboard using a UART cable.
    - Use connector `UART Duet` on the screen.
2. In the GUI, select the UART connection method.
3. Set the baud rate on the mainboard to `115200`. use `M575 P1 S1 B115200` in config.g, this is similar to connecting a PanelDue, other than the default baud rate is 115200


## Updating the Duet3D screen
Several methods are available to update the Duet3D screen.

1. **Using the GUI**
    - Copy the update file (`DuetScreen.tar.gz`) to the root directory of a USB flash drive.
    - Insert the USB flash drive into the Duet3D screen.
    - In the GUI you will be prompted to update the screen.
    - If the update is successful, the screen will automatically reboot. This will appear as a brief flash and the GUI will return to the home screen.
    - The update will create an empty file called `upgraded` in the root directory of the USB flash drive. This file is used to indicate that the update was successful.

2. **Force Update**
    - If the GUI is not working, you can force an update by renaming the update file to `update.tar.gz` and placing it in the **root directory of the USB flash drive OR microSD** card.
    - Insert the USB flash drive or microSD card into the Duet3D screen.
    - *(If using a microSD card)* Power on the screen and it will automatically update.
    - The update will have succeeded if the `update.tar.gz` file is removed from the root directory of the flash drive or microSD card.
3. **Fallback**
    - If the screen is still not working, you will have to reflash the microSD card with the latest image.

> [!warning]
> Occasionally, an update may require the whole microSD card to be reflashed. This will be indicated in the release notes.
> ![Buildroot Version](docs/buildroot_version.png)
>
> In this case, follow the instructions in the [Flashing a new Duet3D screen](#flashing-a-new-duet3d-screen) section above.


## USB Ports
The Duet3D screen has two USB ports:
- USB-A: This port is also a host port. 
    - It can be used to connect to a Duet3D mainboard, wifi modules, or USB flash drives.
- USB-C: This port can be a host or device port.
    - It can be used to connect to a Duet3D mainboard, wifi modules, or USB flash drives in host mode.
    - It can be used to connect to a PC in device mode for software debugging.
    - It can be used to power the screen in either mode (assuming the attached device/host is able to supply power).

> [!WARNING]
> A Duet3D mainboard **CANNOT** provide power to the screen via the USB-C. If connecting to a Duet3D mainboard this will likely damage either the screen, the mainboard, or both. Always power the screen via the `5V_IN` port when connecting to a Duet3D mainboard.

USB hubs are supported **if they are NOT smart**. A smart hub is one that requires a driver to work. This includes most USB-C hubs. If you are using a USB-C hub, make sure it is a dumb hub. A dumb hub is one that does not require a driver to work. This includes most USB-A hubs. If in doubt, use a USB-A hub.

## Building the project
Notes on how to build the project are found in [DEVELOPMENT.md](docs/DEVELOPMENT.md).
