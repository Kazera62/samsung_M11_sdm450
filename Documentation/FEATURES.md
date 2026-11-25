# Kernel features

## Dynamic Fsync

- Kconfig: `CONFIG_DYNAMIC_FSYNC=y`
- Defconfig: `arch/arm64/configs/a11q_open_defconfig`
- Runtime control of filesystem sync behavior.

## Zygote CPU-to-DDR bandwidth boost

Files:
- `fs/exec.c` — identifies Android Zygote 32-bit and 64-bit processes.
- `kernel/fork.c` — requests a temporary maximum CPU-to-DDR bandwidth vote during Zygote fork.
- `drivers/devfreq/devfreq_devbw.c` — registers the boost device only for the `qcom,cpubw` client.
- `drivers/devfreq/devfreq_boost.c` — applies and removes the temporary vote.
- `include/linux/devfreq_boost.h` — boost interface.
- Defconfig: `CONFIG_DEVFREQ_BOOST=y`.

The fork path calls `devfreq_boost_kick_max(50)`. The boost sets `boost_min_freq` to the highest frequency in the CPU bandwidth device's table, then clears it after 50 ms. The normal Devfreq governor controls the device after the boost expires.

The `qcom,mincpubw` client is not registered for this boost. This changes an interconnect bandwidth request; it does not change DDR frequency, timings, or memory capacity.

## CPUFreq and Devfreq

Relevant defconfig options:

- `CONFIG_CPU_FREQ=y`
- `CONFIG_CPU_BOOST=y`
- `CONFIG_CPU_FREQ_GOV_PERFORMANCE=y` (available for manual selection)
- `CONFIG_CPU_FREQ_GOV_SCHEDUTIL=y` (default governor)
- `CONFIG_CPU_FREQ_DEFAULT_GOV_SCHEDUTIL=y`
- `CONFIG_DEVFREQ_BOOST=y`
- `CONFIG_QCOM_DEVFREQ_DEVBW=y`

`schedutil` is the kernel default. GitHub Actions can override it for a build when explicitly requested. The `performance` governor remains available for manual testing, but is not recommended as the sustained default because it requests the maximum policy frequency. See [BUILD.md](BUILD.md).

## Devfreq high-priority workqueue

File: `drivers/devfreq/devfreq.c`

The shared `devfreq_wq` is created with `WQ_HIGHPRI | WQ_UNBOUND | WQ_FREEZABLE | WQ_MEM_RECLAIM`. This gives Devfreq state-change work a high-priority worker pool while keeping the workqueue unbound, suspend-freezable, and eligible for memory-reclaim forward progress.

This changes workqueue scheduling only. It does not set a Devfreq minimum/maximum, force the CPU-to-DDR bus to its top vote, alter the DDR clock, or replace the active Devfreq governor. It applies to Devfreq clients that use the shared workqueue, including the Qualcomm CPU bandwidth and Adreno Devfreq paths where applicable.

This is complementary to the existing Zygote CPU-to-DDR bandwidth boost described above: the Zygote path still requests its temporary bandwidth vote, while the workqueue change affects how promptly Devfreq work is scheduled. No additional Kconfig option is required.

## UBWC format lookup optimization

File: `drivers/video/fbdev/msm/mdss_mdp_util.c`

`mdss_mdp_get_format_params()` now searches the smaller `mdss_mdp_format_ubwc_map` before scanning the general `mdss_mdp_format_map`. Non-UBWC formats still fall back to the general format table.

This is a lookup-order optimization only. It does not add or remove pixel formats, change UBWC capability checks, alter buffer layout, or modify display bandwidth votes. It is independent of Devfreq boost and workqueue scheduling.

## Debugging

- `CONFIG_DEBUG_KERNEL=y` remains enabled.
- Removing optional audit features does not disable kernel debugging or `dmesg`.

## Build-time optional integrations

Selected by the CI workflow; these are not unconditional kernel features:

- BakaSU, optionally with SUSFS.
- NetHunter Wi-Fi injection and USB HID gadget patches.
- CPU governor, I/O scheduler, zRAM compressor, TCP congestion control, and `CONFIG_HZ`.

See [BUILD.md](BUILD.md) for selection and patch order.
