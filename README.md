# Razer Drivers

**This is a complete rewrite and cleanup based on the [openrazer-drivers-legacy](https://github.com/openrazer/openrazer-drivers-legacy) project. If you are missing a specific device support, don't hesitate to contact me.**

This repository holds the source code for the linux kernel drivers to support various Razer devices.
All notable changes to this project will be documented in the **[CHANGELOG.md](CHANGELOG.md)** file.


## Supported Devices

| NAME                     | TYPE     | DRIVER SUPPORT                                                                            |
|:-------------------------|:---------|:------------------------------------------------------------------------------------------|
| Razer Blade Stealth 2016 | Keyboard | brightness control, lid logo control, FN mode control, all lighting modes, set key colors |
| Razer Blade 14 2016      | Keyboard | brightness control, lid logo control, FN mode control, all lighting modes, set key colors |
| BlackWidow Chroma        | Keyboard | brightness control, macro keys, all lighting modes, set key colors                        |


## Installation

### Packages

- [ArchLinux AUR Package](https://aur.archlinux.org/packages/razer-drivers-dkms/)

### Automatic installation with DKMS

This requires DKMS to be installed.

```
    make DESTDIR=/ install_dkms
```

### Manual installation

This requires recompilation on every kernel update.

```
    make all
    make DESTDIR=/ install
```


## Driver Device Files

Device files created by the driver in `/sys/bus/hid/drivers/hid-razer/${ID}`


| FILE                 | ACCESS | DESCRIPTION                                                                         |
|:---------------------|:-------|:------------------------------------------------------------------------------------|
| device_type          | r      | Returns a friendly device type string.                                              |
| get_serial           | r      | Gets the serial number from the device.                                             |
| get_firmware_version | r      | Gets the firmware version from the device.                                          |
| brightness           | rw     | Returns the brightness value from 0-255 or sets the brightness to the ASCII number. |
| fn_mode              | rw     | Returns the current fn mode or sets the FN mode to the ASCII number.                |
| set_logo             | w      | Sets the logo lighting state to the ASCII number written to this file.              |
| set_key_colors       | w      | Writes the color rows on the keyboard. Takes in all the colors for the keyboard.    |
| get_key_rows         | r      | Returns the amount of key rows.                                                     |
| get_key_columns      | r      | Returns the amount of key columns.                                                  |
| mode_none            | w      |                                                                                     |
| mode_static          | w      |                                                                                     |
| mode_custom          | w      |                                                                                     |
| mode_wave            | w      |                                                                                     |
| mode_spectrum        | w      |                                                                                     |
| mode_reactive        | w      |                                                                                     |
| mode_breath          | w      |                                                                                     |
| mode_starlight       | w      |                                                                                     |


## TODO

- Create a debian package.
- BlackWidow Chroma: add support for game mode
- Add information from https://github.com/pez2001/razer_chroma_drivers/wiki


## Authors

A list of all authors contributed to this project can be found in the **[AUTHORS.md](AUTHORS.md)** file.
