# NOCTURNE SDM450 — Kernel Overclock Notes

Dokumen ini mencatat konfigurasi overclock (OC) CPU dan GPU yang saat ini ada pada branch `NOCTURNE-LIMITED-OC`. Tujuannya agar pengembang lain dapat menemukan lokasi konfigurasi, memahami hubungan antar-file, dan mengetahui bagian yang perlu diperiksa ketika menambah atau mengubah frekuensi.

> **Penting:** nilai frekuensi di bawah adalah konfigurasi yang tertulis di source. Nilai tersebut belum tentu dapat dicapai oleh setiap unit perangkat. Hasil aktual bergantung pada silicon binning, firmware, regulator/PMIC, suhu, dan batas thermal. Konfigurasi ini tidak boleh dianggap sebagai jaminan stabilitas.

## Ringkasan frekuensi

| Komponen | Frekuensi referensi/bawaan | Frekuensi maksimum pada source OC | Kenaikan maksimum |
|---|---:|---:|---:|
| CPU Qualcomm SDM450 (Cortex-A53) | 1,8048 GHz (umumnya dibulatkan menjadi 1,8 GHz) | 2,400 GHz | sekitar 33,0% |
| GPU Qualcomm Adreno 506 | 600 MHz | 747 MHz | sekitar 24,5% |
| DDR/RAM | Tidak di-overclock oleh konfigurasi ini | Tidak ada target OC DDR di source ini | — |

CPU reference frequencies yang dipertahankan:
- 652,8 MHz
- 1.036,8 MHz
- 1.401,6 MHz
- 1.689,6 MHz
- 1.804,8 MHz

Tambahan CPU OC pada tabel saat ini:
- 1.958,4 MHz
- 2.016 MHz
- 2.150,4 MHz
- 2.208 MHz
- 2.400 MHz

Tabel frekuensi CPU di device tree menggunakan satuan **kHz**. Contoh: `2400000` berarti 2.400.000 kHz atau 2,4 GHz.

GPU reference max 600 MHz berasal dari tabel clock SDM450 dasar. Pada konfigurasi OC branch ini, OPP GPU tertinggi yang dipasang adalah 747 MHz. OPP yang tersedia di tabel kustom: 747, 700, 650, 560, 510, 400, 320, 216, 133,3, dan 19,2 MHz.

## Peta file: CPU OC

### 1. Tabel frekuensi CPU

**File:** `arch/arm64/boot/dts/samsung/msm8953.dtsi`

Cari node:

```dts
msm_cpufreq: qcom,msm-cpufreq {
    ...
    qcom,cpufreq-table = ...;
};
```

Tabel saat ini memuat frekuensi reference sampai 1.804.800 kHz dan tambahan OC sampai 2.400.000 kHz. Ini adalah daftar frekuensi yang diekspos oleh konfigurasi cpufreq.

**Saat menambah/menghapus OPP CPU:**
1. Perbarui `qcom,cpufreq-table` pada node tersebut.
2. Pastikan clock driver mengizinkan frekuensi maksimum itu.
3. Periksa pemetaan bandwidth CPU pada file yang sama (lihat bagian berikut).
4. Periksa energy/power model dan thermal cooling table agar data model tidak tertinggal dari OPP baru.
5. Uji transisi frekuensi, beban semua core, suspend/resume, dan thermal throttling.

Menambah angka ke tabel cpufreq saja tidak otomatis membuat hardware mampu menjalankannya.

### 2. Batas PLL CPU dan kelas regulator

**File:** `drivers/clk/msm/clock-cpu-8953.c`

Cari definisi `apcs_hf_pll`. Pada konfigurasi saat ini terdapat:

```c
.max_rate = 2400000000UL,
...
VDD_MX_HF_FMAX_MAP1(SVS, 2400000000UL),
```

- `.max_rate` membatasi rate maksimum HFPLL yang diterima driver.
- `VDD_MX_HF_FMAX_MAP1` mengaitkan batas rate PLL dengan kelas VDD-MX yang dipakai driver.
- Clock cluster CPU juga menggunakan kelas supply `vdd_pwrcl`.

**Jangan menganggap `SVS` sebagai angka tegangan CPU tertentu.** Source ini tidak memberikan tabel tegangan inti CPU dalam mV untuk setiap OPP. Tegangan aktual diatur melalui mekanisme clock/regulator Qualcomm dan konfigurasi PMIC/RPM perangkat. Jangan menulis angka tegangan CPU seperti 1,2 V atau 1,3 V kecuali sudah diverifikasi dari konfigurasi PMIC yang tepat atau pengukuran perangkat.

Ketika mengubah batas CPU di atas 2,4 GHz, jangan hanya mengubah `.max_rate`. Periksa juga tabel cpufreq, kemampuan PLL, kelas regulator, dan semua tabel terkait. Angka di atas 2,4 GHz belum divalidasi untuk perangkat ini.

### 3. Bandwidth yang mengikuti frekuensi CPU

**File:** `arch/arm64/boot/dts/samsung/msm8953.dtsi`

Node yang perlu diperiksa:
- `cpubw: qcom,cpubw`
- `mincpubw: qcom,mincpubw`
- `devfreq-cpufreq`
- `cpubw_compute`
- `mincpubw_compute`

Pemetaan `cpu-to-dev-map` dan `qcom,core-dev-table` menghubungkan frekuensi CPU dengan level permintaan bandwidth. Pada source saat ini, frekuensi OC CPU yang tinggi menggunakan level bandwidth yang tersedia pada tabel tersebut.

**Catatan penting tentang RAM:** nilai `qcom,bw-tbl` adalah tabel permintaan bandwidth bus. Komentar pada level tertingginya menuliskan 931,2 MHz, tetapi tabel tersebut bukan tabel OPP DDR dan bukan perintah langsung untuk mengubah clock RAM. Jangan menaikkan nilai bandwidth lalu menyebutnya sebagai RAM overclock. Konfigurasi ini tidak menambahkan DDR OPP atau mengubah timing/training RAM.

### 4. Energy/power model CPU

**File pendukung:** `arch/arm64/boot/dts/samsung/msm8953-cpu.dtsi`

File ini berisi data model biaya/beban cluster CPU. Jika daftar OPP diubah lagi, audit data model ini agar representasi frekuensi dan biaya daya tetap konsisten. Angka pada tabel energy model bukan nilai tegangan CPU.

## Peta file: GPU Adreno 506 OC

### 1. OPP GPU dan peta frekuensi ke corner

**File:** `arch/arm64/boot/dts/samsung/sdm450.dtsi`

Cari dua node berikut:
- `&clock_gcc_gfx` → `qcom,gfxfreq-corner`
- `&msm_gpu` → `qcom,gpu-pwrlevels`

Tabel OPP aktif pada branch ini:

| Urutan OPP | GPU frequency | GPU corner | Rail request |
|---:|---:|---:|---|
| 0 (tertinggi) | 747 MHz | 9 | BINNING |
| 1 | 700 MHz | 8 | TURBO_NO_CPR |
| 2 | 650 MHz | 7 | TURBO |
| 3 | 560 MHz | 6 | NOM_PLUS |
| 4 | 510 MHz | 5 | NOM |
| 5 | 400 MHz | 4 | SVS_PLUS |
| 6 | 320 MHz | 3 | SVS |
| 7 | 216 MHz | 2 | LOW_SVS |
| 8 | 133,3 MHz | 1 | LOW_SVS |
| 9 (idle/XO) | 19,2 MHz | 0 | NONE |

Pada tiga OPP tertinggi, `qcom,bus-freq`, `qcom,bus-min`, dan `qcom,bus-max` disetel ke level 10. Itu adalah permintaan bus untuk OPP GPU tersebut, bukan pengaturan frekuensi DDR secara langsung.

Ketika menambah OPP GPU:
1. Tambahkan frekuensi dan sumber/divider yang sesuai pada clock table GPU.
2. Tambahkan pasangan frekuensi/corner pada `qcom,gfxfreq-corner`.
3. Tambahkan OPP dengan urutan dan nomor corner yang konsisten pada `qcom,gpu-pwrlevels`.
4. Pastikan kelas regulator mampu menerima corner yang diminta.
5. Tentukan bus vote berdasarkan kebutuhan OPP dan validasi hasilnya; jangan asal menaikkan bus level.

### 2. Sumber clock GPU

**File:** `drivers/clk/msm/clock-gcc-8953.c`

Cari tabel:

```c
static struct clk_freq_tbl ftbl_gfx3d_clk_src_sdm450[] = {
    ...
};
```

Tabel khusus SDM450 dipilih ketika node clock memakai compatible `qcom,gcc-gfx-sdm450`. Entri OC saat ini:

| GPU output | GPLL3 source rate | Divider |
|---:|---:|---:|
| 650 MHz | 1.300 GHz | 1 |
| 700 MHz | 1.400 GHz | 1 |
| 747 MHz | 1.494 GHz | 1 |

Angka source rate dan divider harus cocok dengan kemampuan PLL serta konfigurasi clock hardware. Menambah OPP di device tree tanpa menambah entri clock source yang valid dapat membuat frekuensi tidak tersedia atau gagal disetel.

### 3. Corner VDD_CX dan batas regulator

**File:** `arch/arm64/boot/dts/samsung/msm8953-regulator.dtsi`

Cari node GPU regulator `gfx_vreg_corner` dan properti:
- `qcom,vdd-cx-corner-map`
- `qcom,num-corners`
- `qcom,ldo-enable-corner-map`

Pemetaan tiga corner OC teratas saat ini:
- Corner 7 / 650 MHz → `RPM_SMD_REGULATOR_LEVEL_TURBO` (384)
- Corner 8 / 700 MHz → `RPM_SMD_REGULATOR_LEVEL_TURBO_NO_CPR` (416)
- Corner 9 / 747 MHz → `RPM_SMD_REGULATOR_LEVEL_BINNING` (512)

Supply aktif PM8953 S2 / VDD_CX juga memiliki batas maksimum `RPM_SMD_REGULATOR_LEVEL_BINNING` pada node `pm8953_s2_level`.

**Penting: 384, 416, dan 512 bukan nilai volt dan bukan mV.** File regulator menggunakan `qcom,use-voltage-level`; angka tersebut adalah kode level request Qualcomm RPM. Tegangan fisik yang diterapkan bergantung pada implementasi RPM/PMIC dan firmware perangkat. Source ini tidak membuktikan tegangan aktual dalam mV pada GPU 650, 700, atau 747 MHz.

Konfigurasi LDO internal GPU juga terpisah dari pemetaan VDD_CX. Pada file regulator, `qcom,num-ldo-corners = <3>` dan `qcom,ldo-enable-corner-map = <1 1 1 0 0 0 0 0 0>`; artinya LDO internal hanya diaktifkan pada tiga corner pertama. Nilai `qcom,ldo-voltage-ceiling` dan `qcom,ldo-voltage-floor` adalah pengaturan LDO tersebut, bukan tegangan yang boleh langsung diklaim sebagai tegangan OPP 650/700/747 MHz.

### 4. Definisi nama level RPM

**File referensi:** `include/dt-bindings/regulator/qcom,rpm-smd-regulator.h`

Definisi yang digunakan:
- `RPM_SMD_REGULATOR_LEVEL_TURBO` = 384
- `RPM_SMD_REGULATOR_LEVEL_TURBO_NO_CPR` = 416
- `RPM_SMD_REGULATOR_LEVEL_BINNING` = 512

File ini menyediakan nama/konstanta level RPM. Jangan mengubah konstanta tersebut menjadi angka mV atau menganggapnya sebagai skala tegangan linear.

## Pengujian pada perangkat

Source menunjukkan konfigurasi yang diminta kernel; perangkat harus diuji untuk memastikan clock benar-benar tercapai dan stabil.

Contoh pemeriksaan melalui Termux/root:

```sh
# CPU: daftar frekuensi yang diekspos cpufreq
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies

# CPU: frekuensi maksimum yang dikonfigurasi
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq

# GPU: frekuensi maksimum dan frekuensi saat ini
cat /sys/class/kgsl/kgsl-3d0/max_gpuclk
cat /sys/class/kgsl/kgsl-3d0/gpuclk
```

Tidak semua ROM/kernel mengekspos semua node sysfs. Jika suatu path tidak tersedia, periksa node devfreq/KGSL yang tersedia pada perangkat, jangan menganggap fitur gagal hanya dari satu path.

Lakukan pengujian bertahap:
- Boot, idle, dan suspend/resume.
- Beban CPU single-core lalu multi-core.
- Beban GPU bertahap dan pengujian grafis/game.
- Pantau suhu CPU/GPU, throttling, konsumsi daya, artefak visual, hang, reboot, dan kernel log.
- Hentikan pengujian jika muncul artefak, crash, suhu tidak wajar, atau ketidakstabilan.
- Simpan kernel/boot image yang diketahui stabil agar dapat melakukan rollback.

Jangan menyebut konfigurasi stabil sebelum diuji pada perangkat fisik. Keberhasilan compile saja tidak membuktikan stabilitas.

## Catatan untuk pengembang

- Referensi perubahan OC pada dokumen ini mengacu pada branch `NOCTURNE-LIMITED-OC`.
- Frekuensi bawaan/reference dan frekuensi OC dipisahkan agar kenaikan mudah dilihat.
- Jangan memasukkan angka tegangan mV yang tidak didukung data PMIC/RPM atau pengukuran aktual.
- Jangan mencampur level bus bandwidth dengan clock DDR.
- Jika OPP diubah, audit semua tabel yang terkait: clock source, cpufreq/GPU OPP, voltage corner, regulator limit, bandwidth vote, energy model, dan thermal cooling.
- Setelah perubahan, periksa device tree hasil build serta uji pada perangkat. README ini adalah dokumentasi source, bukan sertifikasi stabilitas atau panduan menaikkan tegangan secara membabi buta.
