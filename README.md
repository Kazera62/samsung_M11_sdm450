# android_kernel_samsung_sdm450

Kernel source for Samsung Galaxy M11 (SM-M115F, m11q, SDM450).

## Building a flashable zip

This repo does not commit a built kernel image — `Image.gz-dtb` is a build
output, not source. After building the kernel from this source tree:

1. Build the kernel as usual for this target; you'll end up with an
   `Image.gz-dtb` (or `Image.gz`/`dtb` depending on your build config).
2. Copy that file into `AnyKernel3/` at the root of this repo, alongside
   `anykernel.sh`.
3. Zip the *contents* of `AnyKernel3/` (not the folder itself) into a
   `.zip` file.
4. Flash that zip via a custom recovery (TWRP).

`AnyKernel3/` contains the [AnyKernel3](https://github.com/osm0sis/AnyKernel3)
packaging template (`anykernel.sh`, `META-INF/`, `tools/`) already
configured for this device, including the recovery device-codename
whitelist (`device.name1`-`device.name9` in `anykernel.sh`) needed for
TWRP's device-check to pass across the various TWRP builds seen in the
wild for this device.
