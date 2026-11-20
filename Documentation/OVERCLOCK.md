# Overclock implementation

## Frequency table

| Domain | Reference | Configured maximum |
|---|---:|---:|
| CPU Cortex-A53 | 1,804.8 MHz | 2,611.2 MHz |
| Adreno 506 | 600 MHz | 824 MHz |
| CCI | 721.92 MHz | 1,044.48 MHz |

CPU entries in `qcom,cpufreq-table` use kHz. Example: `2611200` = 2.6112 GHz.

## CPU

| OPP | Source |
|---:|---|
| CPU frequency table | `arch/arm64/boot/dts/samsung/msm8953.dtsi` |
| HFPLL max rate / VDD-MX FMAX | `drivers/clk/msm/clock-cpu-8953.c` |
| CPR and regulator limits | `arch/arm64/boot/dts/samsung/msm8953-regulator.dtsi` |
| Energy model | `arch/arm64/boot/dts/samsung/msm8953-cpu.dtsi` |

Configured CPU OPPs above 1.8048 GHz: 1.9584, 2.016, 2.1504, 2.208, 2.448, and 2.6112 GHz.

HFPLL `.max_rate` is 2,611,200,000 Hz. The VDD-MX FMAX map permits 2.448 GHz at SVS/NOM and 2.6112 GHz at TURBO. The 2.6112 GHz OPP uses CPR corner 11, extending the table beyond corner 10. Its software voltage ceiling is 1,140,000 µV; this is a ceiling, not a fixed operating voltage.

## CCI

CCI is derived from CPU frequency in `clock-cpu-8953.c`:

```c
#define CCI_RATE(rate) (div_u64((rate * 10ULL), 25))
```

Ratio: CPU / 2.5 (0.4).

| CPU | CCI |
|---:|---:|
| 1,804.8 MHz | 721.92 MHz |
| 2,448 MHz | 979.2 MHz |
| 2,611.2 MHz | 1,044.48 MHz |

CCI is not an independent DDR clock or DDR overclock control.

## CPU bandwidth mapping

File: `arch/arm64/boot/dts/samsung/msm8953.dtsi`

Relevant nodes: `cpubw`, `mincpubw`, `devfreq-cpufreq`, `cpubw_compute`, and `mincpubw_compute`.

The 2.448 and 2.6112 GHz CPU OPPs map to the maximum CPUBW/minCPUBW levels (7104/5859). These are interconnect bandwidth requests, not DDR clock OPPs.

## GPU Adreno 506

### OPP and bus votes

File: `arch/arm64/boot/dts/samsung/sdm450.dtsi`

Nodes: `&clock_gcc_gfx` (`qcom,gfxfreq-corner`) and `&msm_gpu` (`qcom,gpu-pwrlevels`).

| GPU OPP | Logical corner | RPM rail request | Bus vote | Bus min–max |
|---:|---:|---|---:|---:|
| 824 MHz | 10 | TURBO (384) | 10 | 10–10 |
| 747 MHz | 9 | TURBO (384) | 9 | 9–10 |
| 700 MHz | 8 | TURBO (384) | 9 | 8–10 |
| 650 MHz | 7 | TURBO (384) | 8 | 8–9 |
| 560 MHz | 6 | NOM_PLUS | 8 | 7–9 |
| 510 MHz | 5 | NOM | 7 | 6–8 |
| 400 MHz | 4 | SVS_PLUS | 5 | 4–7 |
| 320 MHz | 3 | SVS | 4 | 3–5 |
| 216 MHz | 2 | LOW_SVS | 1 | 1–4 |
| 133.3 MHz | 1 | LOW_SVS | 1 | 1–4 |
| 19.2 MHz | 0 | NONE | 0 | 0–0 |

Bus votes are interconnect bandwidth levels, not DDR frequencies. The bus vote and its min/max range are separate values in the device-tree node.

### Clock source

File: `drivers/clk/msm/clock-gcc-8953.c`

Table: `ftbl_gfx3d_clk_src_sdm450`.

| GPU output | GPLL3 source rate |
|---:|---:|
| 650 MHz | 1,300 MHz |
| 700 MHz | 1,400 MHz |
| 747 MHz | 1,494 MHz |
| 824 MHz | 1,648 MHz |

### Regulator mapping

File: `arch/arm64/boot/dts/samsung/msm8953-regulator.dtsi`

`gfx_vreg_corner` maps logical GPU corners to RPM voltage levels. Corners 7–10 request `RPM_SMD_REGULATOR_LEVEL_TURBO` (384). The regulator S2 maximum of BINNING (512) is a regulator limit, not the active GPU request.

RPM values are level identifiers, not millivolts. GPU LDO corner selectors are distinct from the physical VDD_CX rail voltage.

## Change checklist

When changing an OPP, audit all dependent tables:

1. CPU/GPU OPP table in device tree.
2. PLL and clock-source rate/divider support.
3. CPR corner, fuse mapping, and regulator limits.
4. CPU/GPU bandwidth votes.
5. Energy model and thermal cooling tables.
6. Boot, frequency transitions, sustained load, suspend/resume, and kernel logs.

Do not infer physical voltage from RPM level IDs or bandwidth level numbers. Validate on hardware; one device's stability does not establish silicon-wide stability.
