# NOCTURNE // SDM450
### SAMSUNG KERNEL PROJECT

**ENGINEERED FOR CONTROL. BUILT FOR PERFORMANCE.**

NOCTURNE is an independent kernel development project for Samsung devices based on the Qualcomm Snapdragon 450 platform. This repository contains the kernel source and device-specific configuration used for development, testing, and performance work.

> Source first. Deliberate changes. No unnecessary baggage.

---

## Platform

| Item | Details |
|---|---|
| Project | NOCTURNE // SDM450 |
| Target family | Samsung Galaxy M11 / A11 |
| SoC | Qualcomm Snapdragon 450 |
| Architecture | ARM64 |
| Kernel base | Linux 4.9 |
| Defconfig | `a11q_open_defconfig` |
| Build system | GitHub Actions |

Compatibility depends on the exact model, firmware, partition layout, and testing on the target device. A successful compile alone does not prove that an image is safe for every variant.

## Build with GitHub Actions

Open **[BUILD-NOCTURNE](.github/workflows/BUILD-NOCTURNE.yml)** in the repository's **Actions** tab and select **Run workflow**.

The workflow has configurable inputs:

| Menu | Options | Default |
|---|---|---|
| Clang toolchain | LineageOS Clang r416183b; Proton Clang | LineageOS Clang r416183b |
| SUSFS patch diagnostics | Enabled; disabled | Enabled |
| Telegram delivery | Send; skip | Send |

The defconfig is intentionally fixed to `a11q_open_defconfig`. Root integration is fixed to the upstream builder's SUSFS-capable `bakasu` route. Build identity overrides have been removed.

Each run checks out the latest `mainline` branch of [NonGKI_Kernel_Build_2nd](https://github.com/JackA1ltman/NonGKI_Kernel_Build_2nd), then clones this kernel repository's selected branch into the runner workspace. Upstream framework changes are therefore picked up on the next manual run; upstream changes do not automatically trigger a build.

## Root integration: ReSukiSU lineage + SUSFS

The workflow uses the upstream builder's automatic integration settings:

- `KERNELSU_AUTO_GET=true`
- `KERNELSU_AUTO_FORK=bakasu`
- `SUSFS_ENABLE=true`

The upstream builder currently exposes its SUSFS-capable automatic route as `bakasu` (BakaSU / ReSukiSU lineage). Root and SUSFS sources are obtained at build time rather than committed to this kernel source tree.

The workflow initially uses the builder's generic Linux 4.9 SUSFS patch. Patch diagnostics are enabled by default. If the generic patch does not match this exact M11/SDM450 source, the run may stop with rejected hunks; a compatible patch will then be needed. This is preferable to silently treating a partial patch as validated.

## AnyKernel3 packaging

The workflow fetches the packaging template from **[Kazera62/AnyKernel3](https://github.com/Kazera62/AnyKernel3)** at build time. The repository's tracked kernel source therefore does not need to vendor the installer.

Before packaging, the workflow adjusts the temporary `anykernel.sh` for Samsung M11/A11:

- Sets `BLOCK=boot;` rather than the inherited Galaxy Nexus partition path.
- Retains the device checks for `m11q`, `m11`, `sdm450`, and `a11q`.
- Removes inherited Galaxy Nexus/tuna-specific ramdisk edits.

The ZIP is assembled by the upstream build framework and is uploaded as a GitHub Actions artifact.

## Telegram delivery

To send the built ZIP, add these repository secrets:

| Secret | Value |
|---|---|
| `TELEGRAM_BOT_TOKEN` | Token from your Telegram bot |
| `TELEGRAM_CHAT_ID` | Destination chat or channel ID; the bot must have permission to send documents there |

Set them under **Settings → Secrets and variables → Actions**. Do not put bot tokens directly in the workflow YAML. Telegram delivery is enabled by default and can be skipped from the workflow menu. The GitHub artifact is uploaded before the Telegram step, so it remains available even if Telegram delivery fails.

## Build output

The expected output is an AnyKernel3 ZIP named from the device codename, project label, timestamp, and pack method. The upstream packer uploads it as the build artifact and the Telegram step sends the same ZIP when enabled.

The ZIP has not been validated until it has been installed and tested on the intended device. Verify the exact model, keep a known-good boot image and recovery method available, and check the boot result before daily use.

## Development principles

- **Clean source:** root integration and packaging tools stay outside the tracked kernel source by default.
- **Consistent configuration:** one defconfig is used for repeatable testing.
- **Traceable builds:** compiler preset, upstream framework, source revision, and patch diagnostics are visible in the Actions run.
- **Evidence-based tuning:** report performance changes only after verification on target hardware.

## Disclaimer

Custom kernels can cause boot loops, instability, thermal issues, data loss, or device malfunction. Test carefully and maintain a recovery path. No universal compatibility or stability guarantee is provided.

---

### NOCTURNE // SDM450
*Independent development. Deliberate modifications. Device-focused engineering.*
