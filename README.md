# NOCTURNE // SDM450
### SAMSUNG KERNEL PROJECT

**ENGINEERED FOR CONTROL. BUILT FOR PERFORMANCE.**

NOCTURNE is an independent kernel development project for Samsung devices based on the Qualcomm Snapdragon 450 platform. This repository contains the Linux kernel source and device-specific build configuration used for testing, maintenance, and performance development.

> Source first. Deliberate changes. No unnecessary baggage.

---

## Platform

| Item | Details |
|---|---|
| Project | NOCTURNE // SDM450 |
| Device family | Samsung Galaxy M11 / A11 |
| SoC | Qualcomm Snapdragon 450 |
| Architecture | ARM64 |
| Kernel base | Linux 4.9 |
| Build system | GitHub Actions |

Device compatibility depends on the selected defconfig, model variant, device tree, and the resulting build. A successful compilation alone does not confirm that an image is safe to flash on every variant.

## Build with GitHub Actions

The recommended automated entry point is **[BUILD-NOCTURNE](.github/workflows/BUILD-NOCTURNE.yml)**.

1. Open the repository's **Actions** tab.
2. Select **NOCTURNE // SDM450 Kernel Build**.
3. Choose **Run workflow** on the `NOCTURNE-LIMITED` branch.
4. Select a defconfig, Clang revision, and root integration.
5. Start the run and download the generated artifact when the job finishes.

### Build options

| Option | Available choices | Default |
|---|---|---|
| Defconfig | `a11q_open_defconfig`, `a11q_usa_aio_defconfig`, `sdm450_sec_a11q_eur_open_defconfig` | `a11q_open_defconfig` |
| Clang | AOSP prebuilt `r407598` or `r416183b` | `r407598` |
| Root integration | `none` or `kernelsu-next-legacy` | `none` |
| Build user | Kernel version identity | `NOCTURNE` |
| Build host | Kernel version identity | `GitHub-Actions` |

The workflow downloads the selected Clang revision and the AOSP GCC 4.9 cross-compilers required by this source tree. It also uses the repository's vendor device-tree compiler and enables the required ARM64 DT overlay build setting.

**Root integration is disabled by default.** Choosing `kernelsu-next-legacy` adds KernelSU Next to the temporary GitHub Actions runner workspace for that build only; it does not commit those files or modify this repository's tracked source. Legacy integration on Linux 4.9 is an experimental build option and must be tested on the target device.

## Build artifacts

The workflow checks that the kernel image and Samsung device-tree binaries were generated before uploading the artifact. Available outputs include:

- `Image`
- `Image.gz`
- `Image.gz-dtb`
- Samsung DTB files
- `kernel.config`
- `System.map`, when generated
- `build-info.txt`

This workflow **does not create a recovery-flashable ZIP**. AnyKernel3 is not included in this source repository. Package or install the resulting image only with a method suitable for the exact device model and partition layout.

## Development principles

- **Clean source:** optional integrations are not kept in the repository by default.
- **Reproducible inputs:** compiler, defconfig, build identity, and root option are recorded with each build.
- **Device-tree validation:** builds fail when the expected Samsung DTB output or combined image is missing.
- **Traceable changes:** keep modifications and testing results associated with their source revision.
- **Performance with evidence:** frequency or tuning changes should be reported as tested only after verification on the target hardware.

These principles describe the project's build approach; they are not a claim that every possible device variant or tuning profile has been validated.

## Disclaimer

Custom kernels can cause boot loops, instability, thermal issues, data loss, or device malfunction. Confirm the model and selected configuration before testing, preserve a recovery path, and keep a known-good boot image available. No universal compatibility or stability guarantee is provided.

---

### NOCTURNE // SDM450
*Independent development. Deliberate modifications. Device-focused engineering.*
