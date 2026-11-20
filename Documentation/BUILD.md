# GitHub Actions build

Workflow: `.github/workflows/BUILD-NOCTURNE.yml`  
Name: **NOCTURNE SDM450 Build**  
Trigger: `workflow_dispatch` only. Select the source branch/ref before starting the run.

## Inputs

| Input | Options / default |
|---|---|
| `kernel_name` | String; default `NOCTURNE-LIMITED-OC`; 1–32 chars, letters/digits/dot/underscore/hyphen |
| `clang_profile` | Clang r416183b (default), Neutron Clang 24.0.0git, Neutron Clang 19.0.0git, Neutron Clang 14.0.0, ZyC Clang 14.0.6, ZyC Clang 12.0.1, Proton Clang |
| `root` | Non-root (default), BakaSU without SUSFS, BakaSU + SUSFS |
| `enable_nethunter` | Boolean; default false |
| `send_zip_to_telegram` | Boolean; default true |
| `cpu_default_governor` | performance (default), schedutil, ondemand, conservative, userspace, powersave |
| `io_default_scheduler` | noop (default), deadline, cfq |
| `compression_algorithm` | lz4 (default), lzo, zstd, lz4hc |
| `tcp_default_congestion` | bbr (default), cubic, westwood, htcp, hybla, veno, vegas, cdg, dctcp, bic, reno |
| `config_hz` | 1000 (default), 100, 250, 300 |

## Build environment

| Setting | Value |
|---|---|
| Runner | `ubuntu-24.04` |
| Timeout | 180 minutes |
| Device | `samsung_m11` |
| Codename | `m11q` |
| Architecture | ARM64 |
| Defconfig | `a11q_open_defconfig` |
| Kernel source | Current repository; selected workflow ref |
| Build framework | `JackA1ltman/NonGKI_Kernel_Build_2nd`, ref `mainline` |
| Packaging | Kazera62 AnyKernel3, branch `master` |
| DTB | Rebuilt Samsung DTB included in boot image payload and `Image.gz-dtb` |

The workflow uses the selected Git ref as `KERNEL_BRANCH`. The kernel name is validated before build.

## Pipeline

1. Validate kernel name and selected ref.
2. Check out the NonGKI build framework.
3. Patch framework actions for Samsung Linux 4.9, selected root mode, and optional NetHunter/SUSFS.
4. Prepare build environment and fetch toolchains/source.
5. Apply selected kernel defaults and compatibility adjustments.
6. Compile the kernel.
7. Verify selected NetHunter options, CPU governor, and timer tick configuration.
8. Validate `Image.gz`, `Image.gz-dtb`, and rebuilt external DTB.
9. Package and test the AnyKernel3 ZIP.
10. Collect build metadata and upload the ZIP artifact.
11. Optionally send the ZIP and build report to Telegram.

## Optional integrations

### Root

- **Non-root** skips KernelSU setup and KSU defconfig changes.
- **BakaSU without SUSFS** enables BakaSU integration.
- **BakaSU + SUSFS** enables SUSFS and the inline Samsung Linux 4.9 compatibility patch.

BakaSU is selected from its upstream `main` branch. SUSFS version is configured as 2.3.0 in the workflow.

### NetHunter

When enabled, the workflow applies:
- `patches/nethunter/kali-wifi-injection-4.9.patch`
- `patches/nethunter/usb-hidg-linux-4.9.patch`

The patches are checked with `git apply --check`. Already-applied patches are accepted; mismatched patches fail the build instead of being forced.

### Telegram

When `send_zip_to_telegram=true`, the workflow requires repository secrets:
- `TELEGRAM_BOT_TOKEN`
- `TELEGRAM_CHANNEL_ID`

If either secret is absent, the GitHub Actions artifact remains available and Telegram delivery is skipped. Telegram API errors do not fail the build.

## Output and validation

The successful build uploads the AnyKernel3 ZIP as a GitHub Actions artifact. The workflow validates the ZIP archive and checks that it contains `Image.gz-dtb` and the rebuilt external `dtb` payload.

The workflow does not publish a GitHub Release. Telegram delivery is optional.

## Reproducibility

The build framework follows upstream `mainline`, AnyKernel3 follows `master`, and some toolchain profiles use moving branches/tags or downloaded release assets. Rebuilding the same kernel commit later may therefore use different external inputs. For reproducible builds, pin external repositories and toolchain assets to immutable commit IDs or checksums.
