# Kernel features

## Dynamic Fsync

- Kconfig: `CONFIG_DYNAMIC_FSYNC=y`
- Defconfig: `arch/arm64/configs/a11q_open_defconfig`
- Runtime control of filesystem sync behavior.
- Control: `/sys/kernel/dyn_fsync/Dyn_fsync_active` (`0` = normal, default; `1` = enabled). The setting does not persist across reboots.

### Behavior when enabled

- While the primary display (fb0) is on, sync requests that go through `vfs_fsync_range()` return success without writing anything.
- A full filesystem sync runs when fb0 blanks (including low-power blank modes) or when its last user releases it. It runs synchronously, after the panel is off and, for blank events, after other framebuffer notifiers.
- Deferral stops from `PM_SUSPEND_PREPARE` until `PM_POST_SUSPEND`, so the suspend path's own filesystem sync covers deferred data.
- Data is also flushed when the feature is disabled, and on restart, halt or power off.
- After a release-path power off, deferral stays off until the next framebuffer unblank event.

### Data loss risk

Enabling this feature breaks the durability guarantee that applications rely on.

- Skipped while the display is on: `fsync`, `fdatasync`, `msync(MS_SYNC)`, `O_SYNC`/`O_DSYNC` writes, and in-kernel callers such as loop device flushes.
- A kernel panic, hang, watchdog reset, forced power-key reset or sudden power loss while the display is on can lose data that applications were told was saved. Databases can be corrupted, and filesystem images on loop devices can be damaged.
- Errors that `fsync` would have reported are not returned to the caller.
- The screen-off flush can delay the blank request, and therefore the next screen-on, when much data is dirty or storage is slow.

Leave it disabled if data integrity matters.

## KCAL color control

- Source: `drivers/video/fbdev/msm/mdss_mdp_kcal_ctrl.c`. Kconfig: `CONFIG_FB_MSM_MDSS_KCAL_CTRL=y`.
- Based on [savoca/kcal](https://github.com/savoca/kcal) and the Linux 4.4 variant in [PrimoDev23/kernel_xiaomi_whyred](https://github.com/PrimoDev23/kernel_xiaomi_whyred), reworked for MDP v1.7 post-processing.
- Sysfs directory: `/sys/devices/platform/kcal_ctrl.0/`.

| Node | Values | Default | Effect |
|---|---|---|---|
| `kcal` | `R G B`, each 0-256 | `256 256 256` | Channel gain; raised to `kcal_min` if lower |
| `kcal_min` | 0-256 | 35 | Lower limit for the channel gains |
| `kcal_enable` | 0 or 1 | 1 | `0` disables the blocks KCAL has configured |
| `kcal_sat` | 224-383, or 128 | 255 | Global saturation |
| `kcal_hue` | 0-1536 | 0 | Global hue |
| `kcal_val` | 128-383 | 255 | Global value |
| `kcal_cont` | 128-383 | 255 | Global contrast |

```sh
cd /sys/devices/platform/kcal_ctrl.0
echo "256 240 220" > kcal
```

### Behavior

- Nothing is programmed at boot. A color block is touched only after a related node is written, and only the picture adjustment fields that were written are enabled.
- A change takes effect with the next displayed frame. Writes made before the display pipeline is ready are applied on the next unblank.
- Settings are not saved across reboots.
- KCAL shares the PCC and picture adjustment blocks with the ROM (for example night light or display color modes). Whichever writes last wins, and `kcal_enable=0` disables the block rather than restoring the ROM setting.
- `kcal_invert` is not provided.
- Value ranges follow the upstream KCAL interface. The visual result of the picture adjustment values has not been verified on this panel.

## Maple I/O scheduler

- Source: `block/maple-iosched.c`. Kconfig: `CONFIG_IOSCHED_MAPLE=y`.
- Built in and selectable; the defconfig default stays `noop`. Choose `maple` in the workflow input `io_default_scheduler` to make it the default.
- Authors: Joe Maples, based on Zen and SIO by Brandon Berhent and Miguel Boton. Ported from [xellDart/Maple-IO-Scheduler](https://github.com/xellDart/Maple-IO-Scheduler), commit `2efe369`.

### Behavior

Four FIFO queues (sync/async, read/write). Reads are preferred until writes have been passed over `writes_starved` times. After `fifo_batch` requests, the oldest expired request is served, reads first. Within the normal path, async requests go before sync requests of the same direction. While fb0 is blanked, expiry times are multiplied by `sleep_latency_multiple` and the write starvation limit becomes 1.

### Use

```sh
cat /sys/block/mmcblk0/queue/scheduler
echo maple > /sys/block/mmcblk0/queue/scheduler
```

ROM init scripts can override the default at boot. Tunables are in `/sys/block/mmcblk0/queue/iosched/` while Maple is active:

| Tunable | Default | Range |
|---|---|---|
| `sync_read_expire` (ms) | 100 | 0-60000 |
| `sync_write_expire` (ms) | 350 | 0-60000 |
| `async_read_expire` (ms) | 200 | 0-60000 |
| `async_write_expire` (ms) | 500 | 0-60000 |
| `fifo_batch` | 16 | 1-1024 |
| `writes_starved` | 3 | 1-1024 |
| `sleep_latency_multiple` | 5 | 1-100 |

Expiry values are rounded up to whole ticks (10 ms at `CONFIG_HZ=100`).

### Changes from the imported source

- Display state uses the framebuffer notifier instead of the HiSilicon-specific panel variable.
- A zero expiry no longer drops the request.
- An expired sync request is no longer skipped when an older sync and a newer async request are both expired.
- Neighbor lookup recognizes all four list heads, because merging can move a request between lists.
- `sleep_latency_multiple` set through sysfs now takes effect.
- Default expiry values are converted from milliseconds to jiffies, so they match their units at any `CONFIG_HZ`.
- Sysfs input is validated and bounded; registration errors are returned.

No performance gain has been measured on this device.

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
