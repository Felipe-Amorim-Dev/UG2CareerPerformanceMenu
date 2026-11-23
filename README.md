# UG2CareerPerformanceMenu

**Career Mode Performance Package Menu for Need for Speed: Underground 2**

![Release](https://img.shields.io/badge/Release-v1.0-blue)
![Source](https://img.shields.io/badge/Source-v19.1-green)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![Architecture](https://img.shields.io/badge/Architecture-x86-orange)
![Game](https://img.shields.io/badge/Game-NFS%20Underground%202-yellow)

## Overview

**UG2CareerPerformanceMenu** is an ASI mod for **Need for Speed: Underground 2 (PC)** that enhances the Career Mode performance upgrade interface.

It adds a stage-based performance package menu (Stage 1, Stage 2, Stage 3, and Extreme) and allows players to view their installed Unique Performance Upgrades directly from the garage. The mod aims to preserve the game's original interface and normal upgrade behavior.

## Features

- Separate **Stage 1**, **Stage 2**, and **Stage 3** performance packages.
- Dedicated **Extreme** package for displaying Unique Performance Upgrades.
- Automatic detection of Unique Performance Upgrades for the selected Career Mode vehicle.
- Category-specific names and **Installed** status.
- Read-only Extreme package: its presentation is not intended to buy or install conventional parts.
- Integration with the native performance menu, preserving standard package navigation.

### Unique upgrade categories

| Category | Display name |
| --- | --- |
| Engine | Engine Unique |
| ECU | ECU Unique |
| Turbo | Turbo Unique |
| Transmission | Transmission Unique |
| Suspension | Suspension Unique |
| Brakes | Brakes Unique |

## Compatibility

| Requirement | Details |
| --- | --- |
| Game | Need for Speed: Underground 2 |
| Game version | PC v1.2 |
| Operating system | Windows |
| Architecture | 32-bit (x86) |
| Mod release | v1.0 |
| Source version | 19.1 |
| Loader | Compatible ASI Loader required |

Compatibility with other executable versions or frontend/performance-menu mods is not guaranteed.

## Download

**[Download the v1.0 release](https://github.com/Felipe-Amorim-Dev/UG2CareerPerformanceMenu/releases/tag/v1.0)**

| Release asset | Description |
| --- | --- |
| `UG2CareerPerformanceMenu.asi` | Main mod plugin |
| `UG2CareerPerformanceMenu.ini` | Configuration file |
| `GENERIC_LEVEL4.dds` | Optional Stage 4 icon texture |

## Installation

1. Install **Need for Speed: Underground 2 v1.2 (PC)**.
2. Make sure a compatible **ASI Loader** is installed.
3. Download `UG2CareerPerformanceMenu.asi` and `UG2CareerPerformanceMenu.ini` from [Releases](https://github.com/Felipe-Amorim-Dev/UG2CareerPerformanceMenu/releases).
4. Copy both files into the folder containing `SPEED2.EXE`.
5. Start the game, enter **Career Mode**, and open the garage's **Performance Upgrade** menu.

Example folder structure:

```text
Need for Speed Underground 2/
├── SPEED2.EXE
├── UG2CareerPerformanceMenu.asi
├── UG2CareerPerformanceMenu.ini
└── ...
```

> **Important:** Back up your save files and any modified game resources before installing mods.

## Optional Stage 4 icon (`GENERIC_LEVEL4.dds`)

The optional texture features a metallic package icon with the number **4**.

**The DDS file is not loaded automatically** by copying it to the game folder. To use it, import the texture into the appropriate FrontB resources with a compatible editing tool, and ensure the frontend references the imported texture. Back up the original FrontB files first.

The v19.1 implementation is designed to work with the game's original FrontB resources; importing a custom icon is a separate customization step.

## Configuration

The mod reads `UG2CareerPerformanceMenu.ini`. Options control its main behavior, including the stage-based menu, Extreme package, Unique detection, category names, and diagnostic logging.

For normal play, start with the **INI included in the release**. Do not enable experimental diagnostic settings unless troubleshooting.

## Building from source

### Requirements

- Visual Studio 2022
- Desktop development with C++ workload
- MSVC toolchain and Windows SDK
- Project's MinHook sources/dependencies

### Build

1. Clone the repository:

   ```bash
   git clone https://github.com/Felipe-Amorim-Dev/UG2CareerPerformanceMenu.git
   ```

2. Open the solution (`.sln`) in Visual Studio 2022.
3. Select **Release | Win32 (x86)**.
4. Run **Build → Build Solution**.
5. Find the generated `.asi` in the project's configured output directory.

The plugin must target **x86**, because `SPEED2.EXE` is a 32-bit executable.

## Technical overview

- C++ ASI plugin for the game frontend.
- MinHook-based runtime function interception.
- Integration with native performance package UI structures.
- Career Mode vehicle data used to determine Unique upgrade status.
- Read-only presentation of Unique upgrades without intentional save modifications.

## Troubleshooting

### The mod does not load

- Check the game executable version (PC v1.2).
- Confirm the ASI Loader is installed.
- Verify the `.asi` and `.ini` are next to `SPEED2.EXE`.
- Review diagnostic logs if enabled.

### The Extreme package is missing

- Check the configuration file.
- Test without other frontend/performance-menu modifications.
- Verify the game version.

### The custom icon does not appear

- The DDS must be imported and referenced within the frontend resources.
- Simply copying `GENERIC_LEVEL4.dds` into the game directory is insufficient.

## Issues and contributions

Bug reports, suggestions, and pull requests are welcome. For bug reports, please provide the game version, mod version, steps to reproduce, and relevant logs or screenshots. Avoid including personal data or private save files.

## Credits

**Developer:** Felipe Amorim

Created for the Need for Speed: Underground 2 modding community.

## Disclaimer

This is an unofficial community modification and is not affiliated with or endorsed by Electronic Arts, EA Black Box, or the original developers of Need for Speed: Underground 2. The game and its trademarks belong to their respective owners. No original game executable is distributed here.
