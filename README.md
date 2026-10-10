# NOCTURNE // SDM450
### SAMSUNG KERNEL PROJECT

**ENGINEERED FOR CONTROL. BUILT FOR PERFORMANCE.**

NOCTURNE is an independent kernel development project for Samsung Galaxy M11/A11 devices based on Qualcomm Snapdragon 450. The source is maintained for kernel development, device-specific integration, and performance testing.

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

Compatibility depends on exact model, firmware, and partition layout. A successful compile does not prove that a build is safe for every variant.

## Run a build

Open **[BUILD-NOCTURNE](.github/workflows/BUILD-NOCTURNE.yml)** in the repository's **Actions** tab and choose **Run workflow**. The workflow exposes 19 configurable inputs for compiler selection, root/SUSFS integration, optional patches, packaging, and Telegram delivery.

### Workflow menu reference

| Menu | Options | Default |
|---|---|---|
| Clang toolchain | `lineage-r416183b`, `proton-clang` | `lineage-r416183b` |
| KernelSU fork | `bakasu`, `sukisu`, `next`, `rsuntk`, `xxksu` | `bakasu` |
| KernelSU integration method | `shell`, `manual` | `shell` |
| SUSFS | Enabled / disabled | Enabled |
| SUSFS patch diagnostics | Enabled / disabled | Enabled |
| Continue with rejected SUSFS hunks | Disabled / enabled | Disabled |
| Re:Kernel | Disabled / enabled | Disabled |
| Droidspaces | Disabled / enabled | Disabled |
| NoMount | Disabled / enabled | Disabled |
| KPM | Disabled / enabled | Disabled |
| Inline KPM | Disabled / enabled | Disabled |
| KPatch-Next | Disabled / enabled | Disabled |
| Baseband Guard | Disabled / enabled | Disabled |
| Baseband Guard boot protection | Disabled / enabled | Disabled |
| Generate a standalone DTB | Disabled / enabled | Disabled |
| Package kernel modules | Disabled / enabled | Disabled |
| Merge config files | Comma-separated config filenames, or blank | Blank |
| Free workspace space | Enabled / disabled | Enabled |
| Telegram delivery | Send / skip | Send |

The defconfig remains fixed to `a11q_open_defconfig`; no build-identity overrides are applied. The branch selected in the Actions UI is the kernel branch built by the workflow.

### Important option dependencies

- With SUSFS enabled, the upstream builder supports the `bakasu` (BakaSU / ReSukiSU lineage) and `sukisu` (SukiSU-Ultra) routes. Selecting `next`, `rsuntk`, or `xxksu` requires SUSFS to be disabled; otherwise the validation step stops the run.
- KPM must be paired with Inline KPM or KPatch-Next. Inline KPM requires KPM enabled and `sukisu` selected. KPatch-Next requires KPM enabled.
- Baseband Guard boot protection requires Baseband Guard enabled.
- Enabling Droidspaces while SUSFS is enabled may produce an upstream compatibility warning. Re:Kernel, Droidspaces, NoMount, KPM, Baseband Guard, DTB/DTBO features, and custom config merges are optional and are not enabled by default.
- Enabling “continue with rejected SUSFS hunks” can result in a partially patched kernel. Review the rejection logs and artifacts before using that build.

Each run checks out the latest `mainline` branch of [NonGKI_Kernel_Build_2nd](https://github.com/JackA1ltman/NonGKI_Kernel_Build_2nd). That upstream framework is downloaded at build time; its changes are not pinned to a fixed commit.

## Root integration: ReSukiSU lineage + SUSFS

The default root route is the upstream builder's `bakasu` selector (BakaSU / ReSukiSU lineage), with SUSFS enabled. The generic Linux 4.9 SUSFS patch is used by default and patch diagnostics are enabled. If it does not cleanly match this exact M11/SDM450 source, the build should stop and provide rejected-hunk diagnostics rather than treating a partial patch as validated.

Root and SUSFS sources are fetched during the build; they are not vendored into the kernel source tree.

## AnyKernel3 packaging

The workflow clones **[Kazera62/AnyKernel3](https://github.com/Kazera62/AnyKernel3)** and packages its tracked installer **as-is**. Device-specific AnyKernel3 configuration is maintained in that repository, not patched in the kernel build workflow.

The installer is configured for the M11/A11 identifiers `m11q`, `m11`, and `a11q`, uses `BLOCK=boot;` with `IS_SLOT_DEVICE=0`, and does not apply device-specific ramdisk edits. Kernel-module packaging remains disabled by default; when enabled, the installer can process modules included in the ZIP.

The workflow uploads the resulting ZIP as a GitHub Actions artifact before attempting Telegram delivery.

## Telegram delivery

To send the ZIP, add these repository secrets under **Settings → Secrets and variables → Actions**:

| Secret | Value |
|---|---|
| `TELEGRAM_BOT_TOKEN` | Token from your Telegram bot |
| `TELEGRAM_CHAT_ID` | Destination chat/channel ID; the bot must be allowed to send documents |

Telegram delivery defaults to **Send** and can be changed to **Skip** in the workflow menu. Do not place a bot token directly in workflow YAML.

## Build output and testing

The expected output is a ZIP produced by the upstream AnyKernel3 packer, named with the device codename, project label, timestamp, and pack method. A ZIP is not validated until it has been flashed and tested on the intended device. Keep a known-good boot image and recovery method available.

## Development principles

- **Clean source:** root integration and packaging stay outside the tracked kernel source where possible.
- **Repeatable builds:** one defconfig is used for device-focused testing.
- **Traceability:** compiler preset, selected features, upstream framework revision, and patch diagnostics are visible in the Actions run.
- **Evidence-based tuning:** performance changes are reported only after verification on target hardware.

## Disclaimer

Custom kernels can cause boot loops, instability, thermal problems, data loss, or device malfunction. Test carefully and maintain a recovery path. No universal compatibility or stability guarantee is provided.

---

### NOCTURNE // SDM450
*Independent development. Deliberate modifications. Device-focused engineering.*
