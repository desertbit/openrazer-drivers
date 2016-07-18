# Razer Drivers

**This is a complete rewrite and cleanup based on the [razer-drivers-legacy](https://github.com/openrazer/razer-drivers-legacy) project. If you are missing a specific device support, don't hesitate to contribute to this project.**

This repository holds the source code for the linux kernel drivers to support various Razer devices.
All notable changes to this project will be documented in the **[CHANGELOG.md](CHANGELOG.md)** file.


## Supported Devices

| NAME                     | TYPE     | DRIVER SUPPORT                                                                        |
|:-------------------------|:---------|:--------------------------------------------------------------------------------------|
| Razer Blade Stealth 2016 | Keyboard | brightness control, lid logo control, FN mode control, partial lighting modes support |
| Razer Blade 14 2016      | Keyboard | brightness control, lid logo control, FN mode control, partial lighting modes support |


## Installation

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

## Authors

A list of all authors contributed to this project can be found in the **[AUTHORS.md](AUTHORS.md)** file.
