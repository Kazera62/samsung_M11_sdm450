# NOCTURNE SDM450

Linux 4.9 kernel for Samsung Galaxy M11 (`m11q`, Qualcomm SDM450).

## Platform

| Item | Value |
|---|---|
| Device | Samsung Galaxy M11 (`m11q`) |
| SoC | Qualcomm Snapdragon 450 (SDM450) |
| Architecture | ARM64 |
| Kernel base | Linux 4.9 |
| Defconfig | `arch/arm64/configs/a11q_open_defconfig` |

## Configured clock limits

| Domain | Reference | Configured maximum |
|---|---:|---:|
| CPU | 1.8048 GHz | 3.0144 GHz |
| GPU (Adreno 506) | 600 MHz | 824 MHz |
| CCI | 721.92 MHz | 1,205.76 MHz |

These are source-configured limits, not validated stability targets. The CCI target follows CPU / 2.5 and depends on matching CPU and CCI frequency-to-corner maps.

## Build

Run [NOCTURNE SDM450 Build](.github/workflows/BUILD-NOCTURNE.yml) through GitHub Actions.

1. Select the source branch/ref.
2. Set the toolchain, root mode, optional integrations, and kernel defaults.
3. Download the AnyKernel3 ZIP from the workflow artifacts.

Telegram delivery is optional. See [Build documentation](Documentation/BUILD.md) for all inputs and pipeline details.

## Documentation

- [Build](Documentation/BUILD.md) — workflow inputs, build stages, and artifacts.
- [Overclock implementation](Documentation/OVERCLOCK.md) — CPU/GPU OPPs, PLL, CPR, regulators, CCI, and bandwidth mapping.
- [Kernel features](Documentation/FEATURES.md) — enabled features, Kconfig options, and source locations.

## Key source paths

| Path | Purpose |
|---|---|
| `arch/arm64/boot/dts/samsung/` | Device tree and CPU/GPU OPP tables |
| `drivers/clk/msm/` | Qualcomm clock drivers |
| `drivers/devfreq/` | Devfreq and bandwidth boost |
| `arch/arm64/configs/a11q_open_defconfig` | Kernel configuration |
| `.github/workflows/` | GitHub Actions build workflow |
