# NOCTURNE SDM450

Kernel Linux 4.9 untuk Samsung Galaxy M11 (m11q), berbasis Qualcomm SDM450.

## Target

| Item | Konfigurasi |
|---|---|
| Device | Samsung Galaxy M11 / m11q |
| SoC | Qualcomm Snapdragon 450 (SDM450) |
| Architecture | ARM64 |
| Kernel base | Linux 4.9 |
| Defconfig | `a11q_open_defconfig` |
| Build | GitHub Actions |

## Clock configuration

| Domain | Reference | Maximum configured |
|---|---:|---:|
| CPU | 1.8048 GHz | 2.6112 GHz |
| GPU Adreno 506 | 600 MHz | 824 MHz |
| CCI | 721.92 MHz | 1,044.48 MHz |

Nilai maksimum adalah konfigurasi source, bukan jaminan stabilitas pada setiap unit.

## Documentation

- [Overclock implementation](Documentation/OVERCLOCK.md) — CPU/GPU OPP, PLL, CPR, regulator, CCI, dan bandwidth mapping.
- [Kernel features](Documentation/FEATURES.md) — fitur kernel yang ditambahkan dan lokasi implementasinya.
- [Build workflow](Documentation/BUILD.md) — parameter, toolchain, build stages, dan artifact GitHub Actions.

## Build

Jalankan workflow **NOCTURNE SDM450 Build** melalui GitHub Actions pada branch yang ingin dibangun. Workflow menggunakan `.github/workflows/BUILD-NOCTURNE.yml`.

Defconfig: `a11q_open_defconfig`.

## Source layout

- `arch/arm64/boot/dts/samsung/` — device tree dan CPU/GPU OPP.
- `drivers/clk/msm/` — CPU/GPU clock configuration.
- `drivers/devfreq/` — devfreq dan bandwidth boost.
- `arch/arm64/configs/a11q_open_defconfig` — kernel configuration.
- `.github/workflows/` — build workflow dan action steps.
