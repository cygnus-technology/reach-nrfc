# nRF Connect Reach Demo

## Project Information
This project is provided by the team at i3 Product Development to serve as a simple example of a nRF Connect-based IoT device enabled with Cygnus Reach support.  It uses the limited I/O on the Nordic nRF52840 Dongle to demonstrate the major services Reach offers, including OTA updates through the file service.

##  Using the Pre-Built Demo
To run the demo without building the project in its entirity, all that is necessary is an [nRF52840 Dongle](https://www.nordicsemi.com/Products/Development-hardware/nRF52840-Dongle), a phone with the Cygnus Reach app installed, and a computer with a USB-A port.  To program the nRF52840 Dongle, install [nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop) and its programmer tool.  Download the `reach-nrfc-v<version>.hex` file from this repository's latest release.  Press the side-facing button on the dongle to get it into programming mode, add the hex file, and hit "Write".  Once this finishes successfully, the green LED should turn on, and pressing the larger upwards-facing button on the dongle should cause a second green LED to blink once per second.

## Demo Features

### Physical UI
The user interface provided by the dongle is very basic.  There is an RGB LED, which should be green when not connected to BLE, and blue when connected to BLE.  The upwards-facing button enables or disables the "identify" feature, which blinks the secondary green LED.

### Virtual COM Port
The demo supports a virtual COM port connection via USB, which appears with the name `USB Serial Device`.  This serial port runs at 115200 baud, with 8-bit data, no parity, and 1 stop bit.  Using a program such as [Tera Term](https://teratermproject.github.io/index-en.html), connect to this port to see debug printouts and access the CLI.  Type `help` or `?` and hit enter to see the available commands.

### Reach Features

#### CLI Service
The CLI service through Reach mirrors what is available through the virtual COM port.

#### Parameter Repository Service
The parameters implemented in the demo are intended to demonstrate the common data types defined by Reach (booleans, integers, floating-point values, bitfields, enumerations, strings, and bytearrays), as well as the metadata associated with parameters.  Most of the parameters should be self-explanatory, but a few are slightly more complicated.

The `User Device Name` parameter can be used to modify the advertised name of the dongle.  When blank, the board will advertise as `Reacher nRF52840`.  Writing a new string to this will make the device advertise this new name in future.  Rewriting this to be blank will make the board revert to advertising as `Reacher nRF52840`.

The `Timezone Enabled` and `Timezone Offset` parameters both relate to the Time service, and are covered in that section.

The other parameters reflect some basic system information, as well as allowing the user to change the color of the RGB LEDs, remotely enable the identification LED, or change the rate at which the identification LED blinks.  Of these settings, only the `Identify Interval` persists across reboots.  The two RGB LED parameters show the state of the RGB LED in two different forms.  The state shows exactly which LEDs are turned on, and the color translates this into more user-friendly descriptions.  Writing either parameter will change the LED color and both parameters.  The LED color will be reset to green after disconnecting from BLE, and to blue after reconnecting.

By default, the dongle does not watch for changes to parameters so that they can be updated in the app or web portal.  The refresh button in both will re-read all of the parameters.  To get live updates on a parameter, notifications must be set up for it.  Here, there are options for minimum and maximum notification intervals, as well as a value change trigger.  The minimum notification interval determines how much time must elapse between two notifications of the parameter changing, even if the parameter is changing more quickly than this.  Enabling the maximum notification interval will require a notification to be generated after that time elapses, even if the value has not changed.  The value change trigger determines how much the parameter value must change compared to the last notification to generate a new notification.  To see how notifications behave in action, the `Button Pressed` and `Uptime` parameters are good test cases.

#### File Service
The file service includes simple examples of read-only, read/write, and write-only files.  The `ota.bin` file is used for OTA updates, which is covered in its own section.  `cygnus-reach-logo.png` is a hardcoded image of the Reach logo.  `io.txt` is stored in persistent memory, and can be any file up to 2048 bytes.  By default, it contains the lyrics to "The Well" by The Crane Wives.

#### Commands Service
The `Reset Defaults` comand will reset all user-controlled parameters to their default values.  Additionally, it will reset `io.txt` to its default contents.  The `Reboot` and `Invalidate OTA Image` commands are mostly relevant to the OTA process, which is covered in its own section.

#### Time Service
The time service allows the dongle to report its internal time, and for the app/web portal to align the dongle to the correct time.  The time service is designed to support both devices which keep track of only the time and date, and devices which also keep track of their timezone (often relevant for devices with battery-backed real-time clocks).  Typically, a device would be one or the other, but the `Timezone Enabled` parameter has been provided to show how the app and web portal behave with either mode.  The `Timezone Offset` parameter shows the current timezone offset, which can be set manually as well as through the time service.  Setting it manually and then getting the time from the device should show the device time with the new offset.

The nRF52840 Dongle does not have a real-time clock, just a real-time counter which does not persist between reboots.  When the demo starts, its UTC time will be initialized to January 1st, 1970 (a Unix timestamp of 0).
	
#### OTA Updates
This demo shows one possible method of implementing OTA updates on a Reach-enabled device, in this case leveraging Nordic's integration of [MCUboot](https://docs.mcuboot.com/).  The following steps must be followed to do an OTA update:
1) Connect to the device through the app or web portal
2) Go to the "Files" service, and upload a new `ota.bin` file.  Each release includes an example file (`reach-nrfc-v<version>-ota.bin`) for testing the OTA update process.  If you have built the project yourself, the `zephyr/app_update.bin` file found within the build folder should be used.  Uploading the file will take around a minute, and it may look like no progress is occurring for the first few seconds, as the processor erases the memory the update will be stored in.
3) Once the file has been uploaded, use the `Reboot` command to initiate MCUboot's update process.  The reboot is required because MCUboot checks for valid firmware updates and begins its internal firmware update process on startup.  To cancel the OTA update after uploading the `ota.bin` file but before rebooting, use the `Invalidate OTA Image` command.
4) After rebooting, the dongle should keep its LEDs off while processing the update,which usually takes 20-30 seconds.  Once the dongle updates successfully, the RGB LED should turn green again, and the app or web portal may automatically reconnect to the dongle.

## Building the Project

### Project Setup
Steps 1-6 cover setting up the nRF Connect development environment, so these may be skipped for those who have worked with nRF Connect previously.

1. Install [VS Code](https://code.visualstudio.com/)
2. Install [nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop) if not installed already
3. Install the Programmer and Toolchain Manager from within nRF Connect for Desktop
4. Install SDK v2.6.0 from within the Toolchain Manager
5. Install [nRF Connect Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download)
6. Open VS Code through the toolchain manager for SDK v2.6.0, and install the recommended extensions
7. To allow the BLE advertisement to be changed while not advertising (particularly when connected), a file within the SDK must be modified.
   - In the toolchain manager, select the drop-down for the 2.6.0 SDK, and hit `Open SDK directory`
   - In this window, navigate to `zephyr/subsys/bluetooth/host` and open the `adv.c` file
   - Comment out lines 857-859 of the file (which may also be found by searching for the first instance of `if (!atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {`.
   - If this is not done, attempting to set the `User Device Name` parameter will result in an error which warns that this change may not have been made.
8. Check out the repository from Bitbucket.  Make sure that there are no spaces in the path to the repository folder, as this has been known to break the build process.
9. In the nRF Connect extension, do "Open an existing application" and select the reacher-nrfc folder.  Once this has been opened, it should show up in the `APPLICATIONS` section as `reach-nrfc`.
10. Under the `reach-nrfc` application, select "Add build configuration" to set up a new build.  Import the build configuration from the `nRF52840 Dongle` CMake preset, and hit "Build Configuration".  After this, the build should appear in the `APPLICATIONS` section, under the `reach-nrfc` application.

For future builds, select the relevant build (the selected build is highlighted in blue and has an open file folder icon), and hit the `Build` button within the nRF Connect extension tab, under `ACTIONS`.  To program the board via the nRF Connect programmer, use the `zephyr/merged.hex` file in the build folder.  To do an OTA update as described above, use the `zephyr/app_update.bin` file.

### Modifying the Project
The layout of the demo is intended to be fairly basic.  `src/main.c` contains the initialization code and handles the LED and button interactions.  `include/definitions.h` and `src/definitions.c` are typically auto-generated with a Python script based on a definition .xlsx file, and define the Reach services implemented in the demo, as well as non-application-specific implementations of functions needed for these services.  Application-specific implementations of Reach service functions may be found in `src/cli.c`, `src/commands.c`, `src/files.c`, and `src/param_repo.c`.  `src/param_repo.c` also includes functions related to the Time service, and `src/cli.c` also handles the virtual COM port.

Overall, there are 3 tasks created by the application.  `src/main.c` has a task to handle the identification LED, `src/cli.c` has a task to handle CLI interactions through the virtual COM port, and `Integrations/nRFConnect/reach_nrf_connect.c` has a task which handles data coming in through the BLE.

The `pm_static.yml` file defines the allocation of flash for the bootloader, main and secondary firmware images, and flash reserved for the file system.  This has been tuned to provide enough space for the bootloader and two files on the file system.  Some trial end error is required to modify this file correctly.  Building other examples for the same board and comparing the `partitions.yml` file in the build directory to `pm_static.yml` will help with understanding how to change this file.

Code in `reach-c-stack` should not be modified unless absolutely necessary, as this is shared among all Reach device projects written in C.  See [reach-c-stack](https://github.com/cygnus-technology/reach-c-stack) for information about contributing to this repository.  Code in `Integrations/nRFConnect` is intended to be shared among multiple Nordic projects, though it is not part of a repository.

To learn how to regnerate the `definitions.c` and `definitions.h` files, see `reach-util/c-gen/README.md`.

## Contributing
To contribute, create an issue in the repository, and the team at i3 Product Development will respond as quickly as possible.

## Additional Information

### Project References

 - [Cygnus Reach Documentation](https://docs.cygnustechnology.com/)
 - [Nordic nRF52840 Dongle Product Page](https://www.nordicsemi.com/Products/Development-hardware/nRF52840-Dongle)

### Firmware Resources

 - [Nordic/Zephyr KConfig Options](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.6.0/kconfig/index.html)
 - [Cygnus Reach C SDK Repository](https://github.com/cygnus-technology/reach-c-stack)
 - [Cygnus Reach Utilities Repository](https://github.com/cygnus-technology/reach-util)
 - [Cygnus Reach Protobuf Repository](https://github.com/cygnus-technology/reach-protobuf)
