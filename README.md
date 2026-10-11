# NOCTURNE // SDM450

Kernel project for Samsung Galaxy M11 / A11 (Snapdragon 450).

## Build

Buka **Actions → NOCTURNE // SDM450 Kernel Build → Run workflow**.

Menu build hanya berisi tiga pilihan:

| Menu | Pilihan | Default |
|---|---|---|
| Kernel name | Nama kernel kustom | `NOCTURNE` |
| Clang | Clang r416183b, Proton Clang | Clang r416183b |
| Root | ReSukiSU + SUSFS, SukiSU-Ultra + SUSFS | ReSukiSU + SUSFS |

SUSFS selalu aktif. Pilihan root menentukan root yang dipasang; tidak ada pilihan root tanpa SUSFS. Defconfig tetap `a11q_open_defconfig`.

Pilihan ReSukiSU menggunakan repo yang sekarang bernama BakaSU. Pilihan SukiSU-Ultra menggunakan integrasi SukiSU-Ultra dari build framework.

Workflow mengambil [NonGKI_Kernel_Build_2nd](https://github.com/JackA1ltman/NonGKI_Kernel_Build_2nd) untuk proses build dan menggunakan [Kazera62/AnyKernel3](https://github.com/Kazera62/AnyKernel3) langsung untuk packaging.

## Telegram

Atur secret repository berikut di **Settings → Secrets and variables → Actions**:

- `TELEGRAM_BOT_TOKEN`
- `TELEGRAM_CHAT_ID`

ZIP diunggah sebagai GitHub Actions artifact sebelum dikirim ke Telegram.

## Catatan

Build baru tetap perlu diuji pada perangkat sebelum digunakan sehari-hari. Simpan boot image yang diketahui berfungsi dan metode pemulihan sebelum flashing.
