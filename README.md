# NOCTURNE // SDM450

Kernel project for Samsung Galaxy M11 / A11 (Snapdragon 450).

## Build

Open **Actions → NOCTURNE // SDM450 Kernel Build → Run workflow**.

The workflow has only three inputs:

| Input | Options | Default |
|---|---|---|
| Kernel name | Custom name, 1–32 letters/numbers/periods/underscores/hyphens | `NOCTURNE` |
| Clang | LineageOS Clang r416183b, Proton Clang | LineageOS Clang r416183b |
| Root mode | `root-no-susfs`, `root-susfs` | `root-susfs` |

The kernel name is applied as the Linux kernel release suffix and ZIP filename label. The kernel defconfig is fixed to `a11q_open_defconfig).

Both root modes use the upstream BakaSU integration (ReSukiSU lineage). `root-no-susfs` builds root support without SUSFS; `root-susfs` enables the upstream SUSFS patch and configuration. SUSFS patch conflicts stop the build rather than being silently ignored.

The workflow uses [NonGKI_Kernel_Build_2nd](https://github.com/JackA1ltman/NonGKI_Kernel_Build_2nd) and gets the packaging files from [Kazera62/AnyKernel3](https://github.com/Kazera62/AnyKernel3). The device-specific AnyKernel3 installer is maintained in that repository and is packaged without modifying it in this workflow.

## Telegram ZIP delivery

Add these repository secrets under **Settings → Secrets and variables → Actions**:

- `TELEGRAM_BOT_TOKEN`: your bot token.
- `TELEGRAM_CHAT_ID`: destination chat or channel ID. The bot needs permission to send documents.

The built ZIP is uploaded as a GitHub Actions artifact before the workflow attempts to send it to Telegram.

## Testing notice

A successful compilation does not guarantee that every M11/A11 variant will boot. Keep a known-good boot image and recovery method available before flashing a custom kernel.
