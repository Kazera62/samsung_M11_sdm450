# NOCTURNE SDM450

Linux 4.9 kernel for Samsung Galaxy M11 (m11q), based on Qualcomm SDM450.

## Target

| Item | Configuration |
|---|---|
| Device | Samsung Galaxy M11 / m11q |
| SoC | Qualcomm Snapdragon 450 (SDM450) |
| Architecture | ARM64 |
| Kernel base | Linux 4.9 |
| Defconfig | `a11q_open_defconfig` |

## Clock and voltage configuration

This branch, `NOCTURNE-EXTREME-3GHZ`, contains an experimental CPU overclock configuration.

| Domain | Reference | Configured maximum |
|---|---:|---:|
| CPU Cortex-A53 | 1.8048 GHz | 3.0144 GHz |
| GPU Adreno 506 | 600 MHz | 824 MHz |
| CCI | 721.92 MHz | 1,205.76 MHz at the 3.0144 GHz CPU rate |

- CPU OPP entries added for this branch: 2.7072, 2.8032, 2.8992, and 3.0144 GHz.
- CPU HFPLL maximum rate and VDD-MX TURBO FMAX are configured to 3,014,400,000 Hz. SVS/NOM FMAX remain at 2,448,000,000 Hz.
- CPR logical corners are extended through corner 15. The configured software voltage ceiling at corner 15 is 1,280,000 µV (1,280 mV); this is a ceiling, not a fixed operating voltage.
- GPU and CCI values above describe their existing source configuration/derived relationship; this branch's new overclock entries are for the CPU.

These are source-level limits and mappings, not a guarantee of stability or proof of successful operation on hardware. Build, device-tree validation, and runtime/boot testing have not been established by this documentation update.

## Documentation

- [Overclock implementation](Documentation/OVERCLOCK.md) — CPU/GPU OPP, PLL, CPR, regulator, CCI, and bandwidth mapping.
- [Kernel features](Documentation/FEATURES.md) — implemented kernel features and source locations.
- [Build workflow](Documentation/BUILD.md) — inputs, toolchains, build stages, and artifacts.

## Build

Run **NOCTURNE SDM450 Build** from [GitHub Actions](.github/workflows/BUILD-NOCTURNE.yml) and select the source branch/ref. The workflow uploads an AnyKernel3 ZIP artifact; Telegram delivery is optional.

## Source layout

- `arch/arm64/boot/dts/samsung/` — device tree and CPU/GPU OPP.
- `drivers/clk/msm/` — CPU/GPU clock configuration.
- `drivers/devfreq/` — devfreq and bandwidth boost.
- `arch/arm64/configs/a11q_open_defconfig` — kernel configuration.
- `.github/workflows/` — build workflow and action steps.
