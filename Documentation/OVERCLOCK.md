# Overclock implementation

This document describes the clock, OPP, CPR, regulator, and interconnect configuration in the `NOCTURNE-EXTREME-3GHZ` branch. Values below describe source configuration; they are not measurements from a running device.

## Frequency overview

| Domain | Reference | Configured maximum in this branch |
|---|---:|---:|
| CPU Cortex-A53 | 1,804.8 MHz | 3,014.4 MHz |
| GPU Adreno 506 | 600 MHz | 824 MHz |
| CCI | 721.92 MHz | 1,205.76 MHz* |

*The 1,205.76 MHz CCI maximum follows CPU / 2.5 and is backed by the CCI-specific frequency-to-corner map in `arch/arm64/boot/dts/samsung/msm8953.dtsi`. It is not a DDR frequency.

## CPU

### Source files

| Function | Source |
|---|---|
| CPU frequency/OPP table and CPR frequency mapping | `arch/arm64/boot/dts/samsung/msm8953.dtsi` |
| HFPLL maximum rate and VDD-MX FMAX | `drivers/clk/msm/clock-cpu-8953.c` |
| CPR corners, voltage ceilings/floors, and regulator constraints | `arch/arm64/boot/dts/samsung/msm8953-regulator.dtsi` |
| CPU energy model | `arch/arm64/boot/dts/samsung/msm8953-cpu.dtsi` |

### CPU OPP table

The CPU entries in `qcom,cpufreq-table` use kHz. The added high-frequency entries are:

| Device-tree value | Frequency |
|---:|---:|
| `2707200` | 2,707.2 MHz |
| `2803200` | 2,803.2 MHz |
| `2899200` | 2,899.2 MHz |
| `3014400` | 3,014.4 MHz |

The preceding configured high OPPs include 1,958.4, 2,016, 2,150.4, 2,208, and 2,448 MHz, with 2,611.2 MHz as the previous upper step. The four entries above extend the table through 3.0144 GHz.

### HFPLL and VDD-MX limit

File: `drivers/clk/msm/clock-cpu-8953.c`

The `apcs_hf_pll.max_rate` is configured to `3014400000UL` (3,014,400,000 Hz). The VDD-MX FMAX map is:

| VDD-MX level | FMAX |
|---|---:|
| SVS | 2,448,000,000 Hz |
| NOM | 2,448,000,000 Hz |
| TURBO | 3,014,400,000 Hz |

SVS and NOM remain at 2.448 GHz; only TURBO is extended to 3.0144 GHz. The driver derives CPU OPP voltage entries from the clock's VDD levels and the CPR/regulator data. The PLL maximum is a software rate limit; it does not by itself prove that a particular chip can operate reliably at that rate.

### CPR corner extension

File: `arch/arm64/boot/dts/samsung/msm8953-regulator.dtsi`

The APC CPR regulator is configured with logical corner maximum 15. The four new CPU frequency-to-corner mappings are:

| CPU frequency | CPR corner |
|---:|---:|
| 2,707.2 MHz | 12 |
| 2,803.2 MHz | 13 |
| 2,899.2 MHz | 14 |
| 3,014.4 MHz | 15 |

The active CPR speed bins are 0, 2, 6, and 7. Their `qcom,cpr-speed-bin-corners` entries are set to 15. Each active `qcom,cpr-corners` speed-bin row retains eight cells, with the active entries set to 15. The active `qcom,cpr-corner-fmax-map` rows retain four cells and end at corner 15. Inactive speed bins remain zeroed.

The configured CPR voltage ceiling values for the new corners are:

| CPR corner | CPU frequency | `qcom,cpr-voltage-ceiling` |
|---:|---:|---:|
| 12 | 2,707.2 MHz | 1,170,000 µV |
| 13 | 2,803.2 MHz | 1,205,000 µV |
| 14 | 2,899.2 MHz | 1,240,000 µV |
| 15 | 3,014.4 MHz | 1,280,000 µV |

These values are software ceilings (1.170, 1.205, 1.240, and 1.280 V), not fixed voltages that CPR necessarily applies at each frequency. The configured voltage-floor entries for these added corners are 500,000 µV. The floor-to-ceiling maximum-range arrays are extended to 15 entries; the CPR revision 0 row uses zero range values, while the other revision rows use 50,000 µV entries.

The CPR open-loop-voltage-as-ceiling options are disabled in this node so that the explicit DT ceiling arrays are used rather than the scaled open-loop value being substituted as the ceiling. Fuse-derived CPR data and silicon binning still participate in the actual runtime voltage calculation.

### PMIC regulator constraints

The PM8953 S5 CPU supply and its S5 limit constraint are extended to 1,280,000 µV in the device tree. The APC CPR logical regulator maximum is set to corner 15.

The 1,280,000 µV figure is the configured software ceiling for the top CPR corner. It must not be confused with a measured rail voltage, a guaranteed delivered voltage, or a recommendation to force that voltage manually. Actual voltage selection depends on CPR, fuse data, regulator behavior, and runtime conditions.

## CCI

Source files:
- Clock relationship and rate propagation: `drivers/clk/msm/clock-cpu-8953.c`
- CCI frequency-to-corner maps: `arch/arm64/boot/dts/samsung/msm8953.dtsi`
- CCI regulator corner limits: `arch/arm64/boot/dts/samsung/msm8953-regulator.dtsi`

The driver defines:

```c
#define CCI_RATE(rate) (div_u64((rate * 10ULL), 25))
```

This gives a CPU-to-CCI relationship of CPU / 2.5 (40%). The CCI mux uses the same HFPLL source and supports half-step dividers, so a 2.5 divider produces the rates below. The CPU clock transition path also updates the cached `cci_clk.c.rate` using this formula.

| CPU rate | CCI rate | CCI corner |
|---:|---:|---:|
| 1,804.8 MHz | 721.92 MHz | 5 |
| 2,448 MHz | 979.2 MHz | 10 |
| 2,611.2 MHz | 1,044.48 MHz | 11 |
| 2,707.2 MHz | 1,082.88 MHz | 12 |
| 2,803.2 MHz | 1,121.28 MHz | 13 |
| 2,899.2 MHz | 1,159.68 MHz | 14 |
| 3,014.4 MHz | 1,205.76 MHz | 15 |

The `qcom,speedX-bin-v0-cci` entries are CCI-specific frequency-to-corner maps consumed by `clock_cpu_probe()`. They must cover the CCI rate associated with every enabled CPU OPP. The active speed-bin maps (0, 2, 6, and 7) now include the four added CCI rows for corners 12–15. Before this correction, the maps stopped at 1.04448 GHz (corner 11), even though the CPU OPP table and the arithmetic CPU-to-CCI relationship had been extended further.

CCI rate is not a DDR clock or a memory-frequency OPP. Raising the CCI rate does not itself raise DDR frequency; bandwidth requests remain controlled by the separate CPUBW/minCPUBW devfreq maps below.

## CPU bandwidth mapping

File: `arch/arm64/boot/dts/samsung/msm8953.dtsi`

Relevant nodes include `cpubw`, `mincpubw`, `devfreq-cpufreq`, `cpubw_compute`, and `mincpubw_compute`.

The existing 2.448 and 2.6112 GHz CPU OPP entries map to the maximum CPUBW/minCPUBW levels (7104/5859). Those values are interconnect bandwidth requests, not DDR clock OPPs. Do not assume the new CPU OPPs imply a higher DDR frequency; verify the actual mapping tables and runtime devfreq votes separately.

## GPU Adreno 506

The GPU configuration is documented here for comparison. The 3 GHz branch changes described above add CPU OPPs; they do not add a new GPU OPP.

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

### GPU clock source

File: `drivers/clk/msm/clock-gcc-8953.c`

Table: `ftbl_gfx3d_clk_src_sdm450`.

| GPU output | GPLL3 source rate |
|---:|---:|
| 650 MHz | 1,300 MHz |
| 700 MHz | 1,400 MHz |
| 747 MHz | 1,494 MHz |
| 824 MHz | 1,648 MHz |

### GPU regulator mapping

File: `arch/arm64/boot/dts/samsung/msm8953-regulator.dtsi`

`gfx_vreg_corner` maps logical GPU corners to RPM voltage levels. Corners 7–10 request `RPM_SMD_REGULATOR_LEVEL_TURBO` (384). The regulator S2 maximum of BINNING (512) is a regulator limit, not the active GPU request.

RPM values are level identifiers, not millivolts. GPU LDO corner selectors are distinct from the physical VDD_CX rail voltage.

## Validation status and change checklist

The values documented here were checked against the branch's source configuration. This documentation does not establish that the kernel has compiled, that the device tree passes `dtc` validation, or that the device boots and sustains these frequencies. No runtime voltage, frequency, thermal, or stability measurements are implied.

When changing an OPP, audit all dependent tables:

1. CPU/GPU OPP table in device tree.
2. PLL and clock-source rate/divider support.
3. CPR corner count, speed-bin/fuse mapping, voltage ceiling/floor arrays, and regulator limits.
4. CPU/GPU bandwidth votes.
5. Energy model and thermal cooling tables.
6. Boot, frequency transitions, sustained load, suspend/resume, and kernel logs.

Do not infer physical voltage from RPM level IDs or bandwidth level numbers. A configured maximum is a source limit, not proof of silicon-wide stability.
