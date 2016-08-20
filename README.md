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

- [ArchLinux AUR Package](https://aur.archlinux.org/packages/openrazer-drivers-dkms/)

### Automatic installation with DKMS

This requires DKMS to be installed.

```
    make DESTDIR=/ install_udev install_dkms
```

### Manual installation

This requires recompilation on every kernel update.

```
    make all
    make DESTDIR=/ install_udev install
```


## Driver Device Files

Device files created by the driver in `/sys/bus/hid/drivers/hid-razer/${ID}`


| FILE                 | ACCESS | OPTIONAL | DESCRIPTION                                                                         |
|:---------------------|:------:|:--------:|:------------------------------------------------------------------------------------|
| device_type          |   r    |          | Returns a friendly device type string.                                              |
| get_serial           |   r    |          | Gets the serial number from the device.                                             |
| get_firmware_version |   r    |          | Gets the firmware version from the device.                                          |
| brightness           |   rw   |    x     | Returns the brightness value from 0-255 or sets the brightness to the ASCII number. |
| fn_mode              |   rw   |    x     | Returns the current fn mode or sets the FN mode to the ASCII number.                |
| set_logo             |   w    |    x     | Sets the logo lighting state to the ASCII number written to this file.              |
| set_key_colors       |   w    |    x     | Writes the color rows on the keyboard. Takes in all the colors for the keyboard.    |
| get_key_rows         |   r    |    x     | Returns the amount of key rows.                                                     |
| get_key_columns      |   r    |    x     | Returns the amount of key columns.                                                  |
| mode_none            |   w    |    x     |                                                                                     |
| mode_static          |   w    |    x     |                                                                                     |
| mode_custom          |   w    |    x     |                                                                                     |
| mode_wave            |   w    |    x     |                                                                                     |
| mode_spectrum        |   w    |    x     |                                                                                     |
| mode_reactive        |   w    |    x     |                                                                                     |
| mode_starlight       |   w    |    x     |                                                                                     |
| mode_breath          |   w    |    x     |                                                                                     |


## TODO

- Add an extra install option for the udev rules.
- Create a debian package.
- BlackWidow Chroma: add support for game mode
- Add information from https://github.com/pez2001/razer_chroma_drivers/wiki


## Authors

A list of all authors contributed to this project can be found in the **[AUTHORS.md](AUTHORS.md)** file.
