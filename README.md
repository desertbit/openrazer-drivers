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


## TODO

- Create a debian package.
- BlackWidow Chroma: add support for game mode
- Add information from https://github.com/pez2001/razer_chroma_drivers/wiki


## Authors

A list of all authors contributed to this project can be found in the **[AUTHORS.md](AUTHORS.md)** file.
