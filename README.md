# NOCTURNE SDM450

Source kernel untuk Samsung Galaxy M11 / A11 berbasis Qualcomm Snapdragon 450.

## Build melalui GitHub Actions

Buka **Actions → NOCTURNE SDM450 Build → Run workflow**. Pilihan yang tersedia:

| Menu | Opsi |
|---|---|
| Kernel name | Nama kernel kustom |
| Clang | Clang r416183b, Proton Clang, ZyC Clang 12.0.1, ZyC Clang 16.0.6 (Experimental) |
| Root | Non-root, BakaSU + SUSFS |
| CPU Governor default | performance, powersave, userspace, ondemand, conservative, schedutil |
| I/O scheduler | cfq, noop |
| zRAM compression | lz4, lzo |
| TCP congestion | bic, cubic, htcp, westwood, reno |
| CONFIG_HZ | 100, 250, 300, 1000 |

Defconfig yang digunakan: `a11q_open_defconfig`.

## Hasil build dan Telegram

ZIP kernel dikemas menggunakan [Kazera62/AnyKernel3](https://github.com/Kazera62/AnyKernel3) dan tersedia di **GitHub Actions → Artifacts**. Jika secret Telegram tersedia, ZIP dikirim ke Telegram dengan laporan build sebagai caption—tidak ada pesan laporan terpisah.

Tambahkan secrets di **Settings → Secrets and variables → Actions**:
- `TELEGRAM_BOT_TOKEN`
- `TELEGRAM_CHAT_ID`

Workflow menggunakan [NonGKI Kernel Build 2nd](https://github.com/JackA1ltman/NonGKI_Kernel_Build_2nd).

Uji kernel sebelum digunakan sehari-hari. Simpan boot image yang berfungsi dan siapkan metode pemulihan sebelum flashing.
