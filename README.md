# NOCTURNE SDM450

Kernel project for Samsung Galaxy M11 / A11 (Snapdragon 450).

## Build

Buka **Actions → NOCTURNE SDM450 Build → Run workflow**.

Tersedia tiga pilihan:

| Menu | Pilihan |
|---|---|
| Kernel name | Nama kernel kustom |
| Clang | Clang r416183b / Proton Clang |
| Root | ReSukiSU + SUSFS / SukiSU-Ultra + SUSFS |

SUSFS selalu aktif. Defconfig tetap `a11q_open_defconfig`. Nama kernel dipakai pada versi kernel dan nama ZIP.

Workflow memakai [NonGKI Kernel Build](https://github.com/JackA1ltman/NonGKI_Kernel_Build_2nd) dan mengambil installer langsung dari [Kazera62/AnyKernel3](https://github.com/Kazera62/AnyKernel3).

## Telegram

Tambahkan secrets berikut di **Settings → Secrets and variables → Actions**:

- `TELEGRAM_BOT_TOKEN`
- `TELEGRAM_CHAT_ID`

ZIP diunggah sebagai artifact GitHub Actions sebelum dikirim ke Telegram.

Uji hasil build pada perangkat sebelum dipakai sehari-hari. Simpan boot image yang berfungsi dan metode pemulihan sebelum flashing.
