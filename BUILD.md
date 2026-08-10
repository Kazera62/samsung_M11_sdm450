# Build notes

Always build via `build_kernel.sh`, or replicate its flags exactly if invoking
`make` by hand. The two that matter most:

```
DTC_EXT=$(pwd)/tools/dtc
CONFIG_BUILD_ARM64_DT_OVERLAY=y
```

## Why

`tools/dtc` is a vendor-forked device tree compiler (Android build,
`1.4.4-Android-build`) that understands this tree's DT overlay format.
The system `dtc` (e.g. `1.7.x` from a stock distro) does not.

If `DTC_EXT`/`CONFIG_BUILD_ARM64_DT_OVERLAY` are omitted, `make` still
reports success end-to-end -- `vmlinux`, `Image`, and `Image.gz` all
build cleanly, and the final `CAT arch/arm64/boot/Image.gz-dtb` step
still runs and "succeeds". But the per-board `.dtb` files under
`arch/arm64/boot/dts/samsung/` are silently never generated, so that
CAT step concatenates nothing: `Image.gz-dtb` ends up byte-identical
to `Image.gz`. The build produces a boot image with no device tree in
it and gives no error indicating this.

Symptom if you hit this: `md5sum Image.gz Image.gz-dtb` match, and
`find arch/arm64/boot/dts/samsung -name '*.dtb'` under the output
directory is empty. Rebuild with the correct flags rather than
assuming the artifact is valid.
