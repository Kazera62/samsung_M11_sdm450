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

### Per-bin CPU frequency-to-corner mapping

The CPU clock driver reads the speed-bin value from eFuse bits 10:8 and loads the matching `qcom,speedX-bin-v0-cl` property. The active CPR speed bins in this device tree are 0, 2, 6, and 7. Each active CPU map must contain the same upper OPP range; the driver does not substitute the bin-0 table when another bin has its own property.

All four active CPU maps now cover 2.7072, 2.8032, 2.8992, and 3.0144 GHz at corners 12–15. The bin-0 map already contained these rows; the missing rows in bins 2, 6, and 7 are now added. Before this correction, those three bins ended at 2.6112 GHz / corner 11, so devices using them could not register the higher CPU OPPs even though the global cpufreq table, HFPLL limit, and CPR voltage data supported them.

The `CPU_COST_0`, `CLUSTER_COST_0`, and `CLUSTER_COST_1` scheduler energy tables now cover the CPU OPP range through 3.0144 GHz. The four new power-cost entries are estimates, not silicon measurements. They are extrapolated from the 2.6112 GHz / corner 11 entries using the dynamic-power approximation `P ~ frequency * voltage^2`, with the CPR voltage ceilings for corners 12–15 (1.170, 1.205, 1.240, and 1.280 V) as the voltage proxy. The resulting relative scheduler costs are:

| CPU frequency | Core cost | Cluster 0 cost | Cluster 1 cost |
|---:|---:|---:|---:|
| 2,707.2 MHz | 84 | 220 | 214 |
| 2,803.2 MHz | 92 | 241 | 235 |
| 2,899.2 MHz | 101 | 264 | 257 |
| 3,014.4 MHz | 112 | 293 | 285 |

These values complete the scheduler model's frequency coverage; they do not define regulator voltage or guarantee accurate energy ranking. Replace them with measured or platform-calibrated power costs when such data is available.

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

The `qcom,speedX-bin-v0-cci` entries are CCI-specific frequency-to-corner maps consumed by `clock_cpu_probe()`. They must cover the CCI rate associated with every enabled CPU OPP. The active speed-bin maps (0, 2, 6, and 7) include the four added CCI rows for corners 12–15. Keep these CCI maps paired with the CPU `-cl` maps above; extending CCI alone is not sufficient to register higher CPU OPPs.

CCI rate is not a DDR clock or a memory-frequency OPP. Raising the CCI rate does not itself raise DDR frequency; bandwidth requests remain controlled by the separate CPUBW/minCPUBW devfreq maps below.

## CPU bandwidth mapping

File: `arch/arm64/boot/dts/samsung/msm8953.dtsi`

The `cpubw` and `mincpubw` bandwidth tables now end at the existing 7104 level. The unsupported 7324 / 960 MHz request entry has been removed. The `cpubw-cpufreq` and `cpubw_compute` maps cap all enabled CPU OPPs, including 2.7072–3.0144 GHz, at 7104. The `mincpubw-cpufreq` and `mincpubw_compute` maps also do not request above 7104.

The value 7104 is a devfreq bandwidth request, not a DRAM timing profile. `devfreq_devbw` scales it by 1,048,576, and the MSM Bus aggregation path uses the 8-byte CPU-to-EBI path width. This produces a calculated request of about 931.14 MHz. The phone's `bimc_clk/measure` result near 931.2 MHz is consistent with that ceiling. The CPU frequency OPP table is unchanged by this memory-bandwidth cap.

## GPU Adreno 506

The GPU clock OPP table is unchanged. The GPUBW table and matching MSM Bus vectors are capped at index 10, the existing 931.2 MHz DDR / 465.6 MHz BIMC request level. The unsupported index 11 request for 960 MHz has been removed. The 824 MHz GPU OPP now has its bus maximum fixed at index 10, so KGSL cannot select a higher GPUBW entry.

### OPP and bus votes

Files:
- `arch/arm64/boot/dts/samsung/msm8953-gpu.dtsi`: GPUBW bandwidth table and MSM Bus vectors.
- `arch/arm64/boot/dts/samsung/sdm450.dtsi`: GPU OPP bus vote/minimum/maximum indices.

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

### GPUBW bandwidth levels

File: `arch/arm64/boot/dts/samsung/msm8953-gpu.dtsi`

The `gpubw` table, `qcom,msm-bus,num-cases`, and `vectors-KBps` now agree on 11 cases (indices 0–10). The index is a bus-vote case, not a clock frequency.

| GPU bus index | GPUBW request | DDR target label | BIMC target label |
|---:|---:|---:|---:|
| 0 | 0 | Off | Off |
| 1 | 1611 | 211.2 MHz | 105.6 MHz |
| 2 | 2124 | 278.4 MHz | 139.2 MHz |
| 3 | 2929 | 384.0 MHz | 192.0 MHz |
| 4 | 3222 | 422.4 MHz | 211.2 MHz |
| 5 | 4248 | 556.8 MHz | 278.4 MHz |
| 6 | 5126 | 672.0 MHz | 336.0 MHz |
| 7 | 5859 | 768.0 MHz | 384.0 MHz |
| 8 | 6152 | 806.4 MHz | 403.2 MHz |
| 9 | 6445 | 844.8 MHz | 422.4 MHz |
| 10 | 7104 | 931.2 MHz | 465.6 MHz |

GPUBW and CPUBW are separate clients whose requests are aggregated by MSM Bus. Both paths are now constrained to their pre-existing top request level; neither table adds a new physical DRAM operating point.

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

## Measuring the actual BIMC clock on the phone

The MSM8953 clock driver includes a hardware frequency-measurement mux. In this tree, the mux has a selectable `bimc_clk` source and the `qcom,cc-debug-8953` device-tree node registers the measurement interface. This is a hardware counter measurement, not the cached rate printed by a generic clock summary.

After booting this kernel, run as root:

```sh
mount -t debugfs debugfs /sys/kernel/debug 2>/dev/null || true
cat /sys/kernel/debug/clk/bimc_clk/measure
```

The file returns a measured rate in Hz. Sample it during a sustained CPU/memory workload when checking that the existing top bandwidth level is reached. A value near 931.2 MHz is consistent with the current 7104 request ceiling. An idle reading may be lower because the bus is dynamically scaled.

For the current 8-byte CPU-to-EBI path, the top request calculation is:

| Bandwidth table value | Calculated BIMC clock request |
|---:|---:|
| 7104 (top supported request level) | 931,135,488 Hz (about 931.14 MHz) |

This is the Linux-side requested rate calculated from `devfreq_devbw`'s MiB scaling and the MSM Bus width. It does not independently prove the DRAM data rate or replace validation on the phone. Do not force the RPM BIMC clock manually or hold the memory bus at its maximum continuously as a substitute for a supported DRAM operating point.

## Physical LPDDR3 ceiling and overclock feasibility

The SDM450 platform specification identifies system memory as non-PoP LPDDR3 SDRAM on a 32-bit EBI interface, with support up to 933 MHz. A hardware BIMC measurement of 931,203,698 Hz is therefore consistent with the platform's nominal 933 MHz ceiling; the small difference is within the distinction between the programmed target and the measured counter result.

The Linux path in this tree is a bandwidth-vote path, not a DRAM timing/voltage programming path:

1. MSM Bus aggregates client bandwidth requests and computes a requested fabric clock.
2. The BIMC fabric applies that request through `clk_set_rate()`.
3. `bimc_clk` is an RPM SMD clock using `RPM_MEM_CLK_TYPE` and `BIMC_ID`; the RPM clock driver sends the rate request to remote RPM firmware.
4. DRAM initialization, PHY training, timing parameters, and supported memory operating points are not described by the CPUBW/GPUBW tables in this kernel source.

The GCC probe also sends an initial BIMC RPM vote using `INT_MAX`. This is not a new LPDDR3 OPP or a DRAM overclock implementation. Raising a Linux bandwidth-table request cannot, by itself, add a supported DDR operating point. This branch now keeps CPU and GPU memory-bandwidth requests at the existing 931.2 MHz ceiling.

**Physical LPDDR3 overclock above 933 MHz is not implemented or validated by this kernel branch.** A real attempt would require, at minimum, the exact DRAM component identification and datasheet, a compatible RPM/DDR firmware implementation with a new trained timing/voltage operating point, and hardware validation for data integrity, suspend/resume, thermal behavior, and sustained load. No such RPM/DDR firmware source or DRAM training table is present in this kernel repository. Do not describe a higher bandwidth vote as a physical RAM overclock.

## Validation status and change checklist

The values documented here were checked against the branch's source configuration. This documentation does not establish that the kernel has compiled, that the device tree passes `dtc` validation, or that the device boots and sustains these frequencies. No runtime voltage, frequency, thermal, or stability measurements are implied.

When changing an OPP, audit all dependent tables:

1. CPU/GPU OPP table in device tree.
2. PLL and clock-source rate/divider support.
3. CPR corner count, speed-bin/fuse mapping, voltage ceiling/floor arrays, and regulator limits.
4. CPU/GPU bandwidth votes, including CPUBW/minCPUBW devfreq maps, GPUBW `qcom,bw-tbl`, KGSL `vectors-KBps`, matching `num-cases`, and GPU `bus-min`/`bus-max` indices.
5. Energy model and thermal cooling tables.
6. Boot, frequency transitions, sustained load, suspend/resume, and kernel logs.

Do not infer physical voltage from RPM level IDs or bandwidth level numbers. A configured maximum is a source limit, not proof of silicon-wide stability.
