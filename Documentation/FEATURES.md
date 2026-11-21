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

## Debugging

- `CONFIG_DEBUG_KERNEL=y` remains enabled.
- Removing optional audit features does not disable kernel debugging or `dmesg`.

## Build-time optional integrations

Selected by the CI workflow; these are not unconditional kernel features:

- BakaSU, optionally with SUSFS.
- NetHunter Wi-Fi injection and USB HID gadget patches.
- CPU governor, I/O scheduler, zRAM compressor, TCP congestion control, and `CONFIG_HZ`.

See [BUILD.md](BUILD.md) for selection and patch order.
