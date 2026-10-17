# NOCTURNE SDM450

Source kernel untuk Samsung Galaxy M11 / A11 berbasis Qualcomm Snapdragon 450, Linux 4.9.

## Build melalui GitHub Actions

Buka **Actions → NOCTURNE SDM450 Build → Run workflow**.

| Menu | Opsi |
|---|---|
| Kernel name | Nama kernel kustom |
| Clang | Clang r416183b, Proton Clang, ZyC Clang 12.0.1, ZyC Clang 16.0.6 (Experimental) |
| Root | Non-root, BakaSU + SUSFS |
| Enable NetHunter | `false` (default) / `true` |
| CPU Governor default | performance, powersave, userspace, ondemand, conservative, schedutil |
| I/O scheduler | cfq, noop |
| zRAM compression | lz4, lzo |
| TCP congestion | bic, cubic, htcp, westwood, reno |
| CONFIG_HZ | 100, 250, 300, 1000 |

Defconfig utama: `a11q_open_defconfig`. Workflow menerapkan dan memvalidasi pilihan governor serta `CONFIG_HZ` pada konfigurasi akhir sebelum kompilasi.

## Kali NetHunter kernel features

Aktifkan **Enable NetHunter = true** saat menjalankan workflow untuk mengaktifkan konfigurasi mac80211 injection, USB HID/configfs, dan driver Wi-Fi USB yang tersedia di source 4.9. Jika toggle `false`, fitur mac80211/injection, USB HID configfs, dan driver Wi-Fi USB eksternal opsional dinonaktifkan, tetapi konfigurasi USB Android yang dibutuhkan untuk fungsi ponsel tetap dipertahankan.

Ketika opsi itu aktif, workflow:
1. Menerapkan `patches/nethunter/kali-wifi-injection-4.9.patch` menggunakan `git apply --check` terlebih dahulu. Build berhenti jika patch tidak cocok dengan source; patch tidak dipaksakan.
2. Mengaktifkan konfigurasi yang diperlukan dengan `scripts/config`.
3. Menjalankan `make olddefconfig` dan memvalidasi hasil akhirnya sebelum kompilasi.

Opsi mass-storage yang benar pada source ini adalah `CONFIG_USB_CONFIGFS_MASS_STORAGE` (bukan `CONFIG_USB_CONFIGFS_F_MASS_STORAGE`). `CONFIG_MAC80211_HAS_RC` adalah opsi internal yang dipilih oleh Kconfig; workflow memvalidasi hasilnya daripada memaksanya secara langsung.

### Patch source kernel 4.9

- **Wi-Fi injection:** patch disesuaikan dari [Kali Linux Wi-Fi injection patch for 4.9](https://gitlab.com/kalilinux/build-scripts/kali-arm/-/blob/main/patches/kali-wifi-injection-4.9.patch). Workflow menguji patch dengan `git apply --check` lalu menerapkannya. Untuk pemeriksaan manual dari root source:
  ```sh
  git apply --check patches/nethunter/kali-wifi-injection-4.9.patch
  git apply patches/nethunter/kali-wifi-injection-4.9.patch
  ```
  Jangan mencampurkannya dengan patch mac80211 lama untuk kernel 3.x. Jika source berubah dan pemeriksaan gagal, periksa setiap hunk dan porting secara manual; jangan memaksa hunk masuk.
- **HID keyboard/mouse:** source ini sudah memiliki `drivers/usb/gadget/function/f_hid.c`, `CONFIG_USB_CONFIGFS_F_HID`, dan konfigurasi configfs. Untuk kernel 4.x, jalur HID-4 menggunakan konfigurasi ini; patch HID lama untuk kernel 3.x tidak diperlukan pada source ini.
- **USB Wi-Fi:** konfigurasi mencakup `ATH9K_HTC`, `RTL8187`, `RTL8192CU`, dan `RT2800USB`. Adapter tetap membutuhkan chipset yang didukung, firmware yang sesuai tersedia di Android, dukungan USB host/OTG, serta daya OTG yang cukup.

**Catatan:** ini mengaktifkan fitur sisi kernel, bukan memasang Kali chroot, aplikasi NetHunter, atau seluruh userspace secara otomatis. Dukungan injection berbeda antar-adapter dan perlu diuji dengan perangkat yang kompatibel.

## Hasil build dan Telegram

ZIP kernel dikemas dengan [Kazera62/AnyKernel3](https://github.com/Kazera62/AnyKernel3) dan tersedia di **GitHub Actions → Artifacts**. Jika secrets Telegram tersedia, ZIP dikirim dengan laporan build sebagai caption—tidak ada pesan laporan terpisah.

Tambahkan secrets melalui **Settings → Secrets and variables → Actions**:
- `TELEGRAM_BOT_TOKEN`
- `TELEGRAM_CHAT_ID`

Workflow menggunakan [NonGKI Kernel Build 2nd](https://github.com/JackA1ltman/NonGKI_Kernel_Build_2nd).

Nama build di `/proc/version` diatur sebagai `Zenith@GitHub` melalui `scripts/mkcompile_h`. Uji kernel sebelum dipakai sehari-hari dan simpan boot image yang berfungsi serta metode pemulihan sebelum flashing.
