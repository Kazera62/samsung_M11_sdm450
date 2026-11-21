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

## Clock configuration

| Domain | Reference | Configured maximum |
|---|---:|---:|
| CPU | 1.8048 GHz | 3.0144 GHz |
| GPU Adreno 506 | 600 MHz | 824 MHz |
| CCI | 721.92 MHz | 1,205.76 MHz |

Configured maxima are source limits, not a guarantee of stability on every device. See [Overclock implementation](Documentation/OVERCLOCK.md) for the detailed CPU OPP, PLL, CPR, regulator, CCI, and bandwidth mapping.

The CCI maximum follows CPU / 2.5 and requires matching frequency-to-corner entries in the CCI map in `arch/arm64/boot/dts/samsung/msm8953.dtsi`.

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
