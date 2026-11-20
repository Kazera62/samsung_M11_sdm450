# Kernel features

Only features with an implementation in this source tree are listed here.

## Dynamic Fsync

- Kconfig: `CONFIG_DYNAMIC_FSYNC=y`
- Defconfig: `arch/arm64/configs/a11q_open_defconfig`
- Purpose: runtime control of filesystem sync behavior through the Dynamic Fsync implementation.

## Zygote CPU-to-DDR bandwidth boost

Files:
- `fs/exec.c` — identifies Android Zygote 32-bit and 64-bit processes.
- `kernel/fork.c` — requests a temporary maximum CPU-to-DDR bandwidth vote during Zygote fork.
- `drivers/devfreq/devfreq_devbw.c` — registers the boost device only for the `qcom,cpubw` client.
- `drivers/devfreq/devfreq_boost.c` — applies and removes the temporary vote.
- `include/linux/devfreq_boost.h` — boost interface.
- Defconfig: `CONFIG_DEVFREQ_BOOST=y`.

The fork path calls `devfreq_boost_kick_max(50)`. The boost sets `boost_min_freq` to the highest frequency in the CPU bandwidth device's table, then clears it after 50 ms. The normal Devfreq governor controls the device after the boost expires.

The minimum-bandwidth client (`qcom,mincpubw`) is not registered for this boost. This changes an interconnect bandwidth request; it does not change DDR frequency, timings, or memory capacity.

## CPU frequency and Devfreq

Relevant defconfig options include:

- `CONFIG_CPU_FREQ=y`
- `CONFIG_CPU_BOOST=y`
- `CONFIG_CPU_FREQ_GOV_PERFORMANCE=y`
- `CONFIG_CPU_FREQ_GOV_SCHEDUTIL=y`
- `CONFIG_DEVFREQ_BOOST=y`
- `CONFIG_QCOM_DEVFREQ_DEVBW=y`

Available governors and defaults can be overridden by the GitHub Actions build inputs. See [BUILD.md](BUILD.md).

## Debugging

- `CONFIG_DEBUG_KERNEL=y`
- Kernel debugging support remains enabled.
- Do not treat the removal of optional audit features as disabling kernel debugging or dmesg.

## Build-time optional integrations

These are selected by the CI workflow and are not unconditional kernel features:

- BakaSU, optionally with SUSFS.
- NetHunter Wi-Fi injection and USB HID gadget patches.
- CPU governor, I/O scheduler, zRAM compressor, TCP congestion control, and `CONFIG_HZ`.

See [BUILD.md](BUILD.md) for selection and patch order.
