# NOCTURNE SDM450 — Kernel Overclock Notes

Dokumen ini mencatat konfigurasi overclock (OC) CPU dan GPU yang saat ini ada pada branch `NOCTURNE-LIMITED-OC`. Tujuannya agar pengembang lain dapat menemukan lokasi konfigurasi, memahami hubungan antar-file, dan mengetahui bagian yang perlu diperiksa ketika menambah atau mengubah frekuensi.

> **Penting:** nilai frekuensi di bawah adalah konfigurasi yang tertulis di source. Nilai tersebut belum tentu dapat dicapai oleh setiap unit perangkat. Hasil aktual bergantung pada silicon binning, firmware, regulator/PMIC, suhu, dan batas thermal. Konfigurasi ini tidak boleh dianggap sebagai jaminan stabilitas.

## Ringkasan frekuensi

| Komponen | Frekuensi referensi/bawaan | Frekuensi maksimum pada source OC | Kenaikan maksimum |
|---|---:|---:|---:|
| CPU Qualcomm SDM450 (Cortex-A53) | 1,8048 GHz (umumnya dibulatkan menjadi 1,8 GHz) | 2,6112 GHz | sekitar 44,7% |
| GPU Qualcomm Adreno 506 | 600 MHz | 824 MHz | sekitar 37,3% |
| CCI / coherent interconnect | 721,92 MHz (pada CPU 1,8048 GHz) | 1.044,48 MHz | sekitar 44,7% |

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
- 2.448 MHz
- 2.6112 MHz

Tabel frekuensi CPU di device tree menggunakan satuan **kHz**. Contoh: `2448000` berarti 2.448 GHz dan `2611200` berarti 2.6112 GHz.

GPU reference max 600 MHz berasal dari tabel clock SDM450 dasar. Pada konfigurasi OC branch `NOCTURNE-LIMITED-OC`, OPP GPU tertinggi yang dipasang adalah 824 MHz. OPP yang tersedia di tabel kustom: 824, 747, 700, 650, 560, 510, 400, 320, 216, 133,3, dan 19,2 MHz.

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

Tabel saat ini memuat frekuensi reference sampai 1.804.800 kHz dan OPP OC sampai 2.611.200 kHz. OPP 2.400 MHz telah dihapus; OPP 2.448 MHz tetap dipertahankan dan 2.6112 GHz menjadi OPP tertinggi. Ini adalah daftar frekuensi yang diekspos oleh konfigurasi cpufreq.

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
.max_rate = 2611200000UL,
...
.fmax = (unsigned long[VDD_MX_NUM]) {
    [VDD_MX_SVS] = 2448000000UL,
    [VDD_MX_NOM] = 2448000000UL,
    [VDD_MX_TUR] = 2611200000UL,
},
```

- `.max_rate` membatasi rate maksimum HFPLL yang diterima driver.
- `VDD_MX_HF_FMAX_MAP1` mengaitkan batas rate PLL dengan kelas VDD-MX yang dipakai driver.
- Clock cluster CPU juga menggunakan kelas supply `vdd_pwrcl`.

**Jangan menganggap `SVS` sebagai angka tegangan CPU tertentu.** Source ini tidak memberikan tabel tegangan inti CPU dalam mV untuk setiap OPP. Tegangan aktual diatur melalui mekanisme clock/regulator Qualcomm dan konfigurasi PMIC/RPM perangkat. Jangan menulis angka tegangan CPU seperti 1,2 V atau 1,3 V kecuali sudah diverifikasi dari konfigurasi PMIC yang tepat atau pengukuran perangkat.

OPP 2.448 GHz tetap menggunakan CPR corner 10. OPP 2.6112 GHz memakai virtual CPR corner 11 yang diperluas dari anchor kalibrasi fuse terakhir (corner 10). Ceiling software corner 11 dibatasi ke 1.140.000 µV (1,14 V), mengikuti batas VDD_APC PM8953 S5 pada device tree. Nilai ini adalah **batas atas**, bukan tegangan tetap yang selalu dipasok; tegangan aktual tetap ditentukan oleh CPR, data fuse, dan kondisi perangkat.

OPP 2.6112 GHz memakai corner 11 dan memerlukan pemetaan CPR virtual yang konsisten dengan fuse anchor corner 10. Batas 1,14 V adalah ceiling, bukan target tetap. Pengujian perangkat yang berhasil pada satu unit tidak menjamin margin yang sama pada unit lain; bila muncul hang, reboot, panas tidak wajar, atau gagal boot, turunkan OPP maksimum ke tingkat yang stabil.

Jika ingin menaikkan CPU lebih jauh lagi, jangan hanya mengubah `.max_rate`. Periksa tabel cpufreq, kemampuan PLL, CPR corner/fuse tables, regulator limit, energy model, bandwidth maps, dan thermal cooling. Frekuensi di atas 2.6112 GHz belum dikonfigurasi atau divalidasi untuk perangkat ini.

### 3. Bandwidth yang mengikuti frekuensi CPU

**File:** `arch/arm64/boot/dts/samsung/msm8953.dtsi`

Node yang perlu diperiksa:
- `cpubw: qcom,cpubw`
- `mincpubw: qcom,mincpubw`
- `devfreq-cpufreq`
- `cpubw_compute`
- `mincpubw_compute`

Pemetaan `cpu-to-dev-map` dan `qcom,core-dev-table` menghubungkan frekuensi CPU dengan level permintaan bandwidth. OPP 2.448 dan 2.6112 GHz dipetakan ke level bandwidth tertinggi yang sudah ada (CPUBW 7104 dan minCPUBW 5859); tabel bandwidth itu sendiri tidak dinaikkan.

**Catatan penting tentang RAM:** nilai `qcom,bw-tbl` adalah tabel permintaan bandwidth bus. Komentar pada level tertingginya menuliskan 931,2 MHz, tetapi tabel tersebut bukan tabel OPP DDR dan bukan perintah langsung untuk mengubah clock RAM. Jangan menaikkan nilai bandwidth lalu menyebutnya sebagai RAM overclock. Konfigurasi ini tidak menambahkan DDR OPP atau mengubah timing/training RAM.

### 4. CCI / coherent interconnect

**File driver:** `drivers/clk/msm/clock-cpu-8953.c`  
**File OPP/binning:** `arch/arm64/boot/dts/samsung/msm8953.dtsi`

CCI pada driver ini tidak diatur dengan tabel frekuensi yang berdiri sendiri dari CPU. Macro `CCI_RATE(rate)` menghitung clock CCI sebesar 40% dari frekuensi CPU:

```c
#define CCI_RATE(rate) (div_u64((rate * 10ULL), 25))
```

Dengan rasio 0,4 tersebut:

| CPU clock | CCI clock |
|---:|---:|
| 1.804,8 MHz (referensi) | 721,92 MHz |
| 2.448 MHz | 979,2 MHz |
| 2.6112 MHz (OC tertinggi) | 1.044,48 MHz |

CCI mengikuti CPU melalui rasio tetap 0,4; pada OPP 2.6112 GHz, target CCI adalah 1.04448 GHz. **Rasio CCI tidak diubah** dan CCI belum di-overclock secara independen. Mapping 960 MHz yang sebelumnya dipakai oleh OPP CPU 2.400 GHz dihapus; mapping 979,2 MHz tetap pada CPR corner 10 dan 1.04448 GHz ditambahkan pada corner 11.

Saat mengubah rasio atau frekuensi CCI di masa depan, audit secara bersamaan:
1. `CCI_RATE()` pada `clock-cpu-8953.c`.
2. Semua properti `qcom,speed*-bin-v0-cci` pada `msm8953.dtsi`.
3. Batas PLL, divider/mux CCI, dan tabel corner/CPR.
4. Transisi CPU/CCI saat beban berubah, suspend/resume, dan thermal throttling.

CCI adalah coherent interconnect clock. Ini **bukan** overclock DDR, bukan pengaturan langsung clock BIMC, dan bukan bukti bahwa RAM berjalan lebih cepat. Tabel bandwidth `cpubw`/`mincpubw` tetap menggunakan level maksimum yang sudah ada.

### 5. Energy/power model CPU

**File pendukung:** `arch/arm64/boot/dts/samsung/msm8953-cpu.dtsi`

File ini berisi data model biaya/beban cluster CPU. OPP 2.448 GHz memiliki scheduler cost core 71, cluster 0 sebesar 185, dan cluster 1 sebesar 180. OPP 2.6112 GHz memakai estimasi cost core 77, cluster 0 sebesar 201, dan cluster 1 sebesar 196. Nilai OPP tertinggi merupakan estimasi scheduler, **belum dikalibrasi lewat pengukuran daya**; angka ini bukan nilai tegangan CPU.

## Peta file: GPU Adreno 506 OC

### 1. OPP GPU dan peta frekuensi ke corner

**File:** `arch/arm64/boot/dts/samsung/sdm450.dtsi`

Cari dua node berikut:
- `&clock_gcc_gfx` → `qcom,gfxfreq-corner`
- `&msm_gpu` → `qcom,gpu-pwrlevels`

Tabel OPP aktif pada branch ini:

| Urutan OPP | GPU frequency | GPU corner | Rail request |
|---:|---:|---:|---|
| 0 (tertinggi) | 824 MHz | 10 | TURBO (384) |
| 1 | 747 MHz | 9 | TURBO (384) |
| 2 | 700 MHz | 8 | TURBO (384) |
| 3 | 650 MHz | 7 | TURBO (384) |
| 4 | 560 MHz | 6 | NOM_PLUS |
| 5 | 510 MHz | 5 | NOM |
| 6 | 400 MHz | 4 | SVS_PLUS |
| 7 | 320 MHz | 3 | SVS |
| 8 | 216 MHz | 2 | LOW_SVS |
| 9 | 133,3 MHz | 1 | LOW_SVS |
| 10 (idle/XO) | 19,2 MHz | 0 | NONE |

Indeks awal GPU disetel ke 9 (`qcom,initial-pwrlevel = <9>`), yaitu OPP 133,3 MHz pada tabel efektif SDM450. Indeks dinaikkan satu karena OPP 824 MHz ditambahkan di urutan paling atas.

Permintaan bus diskalakan mengikuti OPP GPU. Nilai di bawah merupakan indeks level pada tabel bus Qualcomm (0–10), bukan frekuensi clock DDR langsung:

| GPU | bus-freq | bus-min | bus-max |
|---:|---:|---:|---:|
| 824 MHz | 10 | 10 | 10 |
| 747 MHz | 9 | 9 | 10 |
| 700 MHz | 9 | 8 | 10 |
| 650 MHz | 8 | 8 | 9 |
| 560 MHz | 8 | 7 | 9 |
| 510 MHz | 7 | 6 | 8 |
| 400 MHz | 5 | 4 | 7 |
| 320 MHz | 4 | 3 | 5 |
| 216 MHz | 1 | 1 | 4 |
| 133,3 MHz | 1 | 1 | 4 |
| 19,2 MHz (XO) | 0 | 0 | 0 |

Level 10 adalah level tertinggi pada tabel bus. Vote ini mengatur permintaan bandwidth GPU ke bus; ia tidak mengunci clock DDR ke nilai tertentu.

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
| 824 MHz | 1.648 GHz | 1 |

Angka source rate dan divider harus cocok dengan kemampuan PLL serta konfigurasi clock hardware. Menambah OPP di device tree tanpa menambah entri clock source yang valid dapat membuat frekuensi tidak tersedia atau gagal disetel.

### 3. Corner VDD_CX dan batas regulator

**File:** `arch/arm64/boot/dts/samsung/msm8953-regulator.dtsi`

Cari node GPU regulator `gfx_vreg_corner` dan properti:
- `qcom,vdd-cx-corner-map`
- `qcom,num-corners`
- `qcom,ldo-enable-corner-map`

Pemetaan GPU tertinggi pada branch `NOCTURNE-LIMITED-OC`:
- Logical corner 7 / 650 MHz → `RPM_SMD_REGULATOR_LEVEL_TURBO` (384)
- Logical corner 8 / 700 MHz → `RPM_SMD_REGULATOR_LEVEL_TURBO` (384)
- Logical corner 9 / 747 MHz → `RPM_SMD_REGULATOR_LEVEL_TURBO` (384)
- Logical corner 10 / 824 MHz → `RPM_SMD_REGULATOR_LEVEL_TURBO` (384)

Tabel `qcom,gfxfreq-corner` sekarang memilih logical corner 7/8/9/10 secara berurutan untuk 650/700/747/824 MHz. Tabel `qcom,vdd-cx-corner-map` menerjemahkan keempat logical corner tersebut ke RPM TURBO (384). Nomor logical corner tetap berbeda untuk mempertahankan tingkat corner terpisah, tetapi request RPM-nya sama.

Node `pm8953_s2_level` tetap mencantumkan BINNING sebagai batas maksimum regulator. Properti batas maksimum tersebut bukan vote aktif yang otomatis meminta BINNING.

**Penting: 384, 416, dan 512 bukan nilai volt dan bukan mV.** File regulator menggunakan `qcom,use-voltage-level`; angka tersebut adalah kode level request Qualcomm RPM. Tegangan fisik yang diterapkan bergantung pada implementasi RPM/PMIC dan firmware perangkat. Source ini tidak membuktikan tegangan aktual dalam mV pada GPU 650, 700, 747, atau 824 MHz.

Konfigurasi LDO internal GPU juga terpisah dari pemetaan VDD_CX. Pada file regulator, `qcom,num-corners = <10>`, `qcom,num-ldo-corners = <3>`, dan `qcom,ldo-enable-corner-map = <1 1 1 0 0 0 0 0 0 0>`; artinya tabel regulator menyediakan logical corner 1–10, sedangkan LDO internal hanya diaktifkan pada tiga corner pertama. Batas `regulator-min-microvolt = <1>` dan `regulator-max-microvolt = <10>` juga harus mencakup semua selector corner; nilai maksimum dinaikkan ke 10 agar logical corner 10 tidak ditolak oleh constraint regulator. Nilai `qcom,ldo-voltage-ceiling` dan `qcom,ldo-voltage-floor` adalah pengaturan LDO tersebut, bukan tegangan yang boleh langsung diklaim sebagai tegangan OPP 650/700/747/824 MHz.

### 4. Definisi nama level RPM

**File referensi:** `include/dt-bindings/regulator/qcom,rpm-smd-regulator.h`

Definisi level RPM yang masih tersedia di source:
- `RPM_SMD_REGULATOR_LEVEL_TURBO` = 384
- `RPM_SMD_REGULATOR_LEVEL_TURBO_NO_CPR` = 416
- `RPM_SMD_REGULATOR_LEVEL_BINNING` = 512

Konstanta 416 dan 512 tetap tersedia untuk regulator atau consumer lain. OPP GPU 650, 700, 747, dan 824 MHz pada branch `NOCTURNE-LIMITED-OC` semuanya diarahkan ke RPM TURBO (384). Angka-angka ini adalah kode level RPM, bukan nilai mV atau skala tegangan linear.

## Zygote DDR bandwidth boost

Fitur ini memberi dorongan bandwidth CPU-to-DDR singkat saat Android Zygote membuat proses aplikasi baru.

**Jalur implementasi:**
- `fs/exec.c` mengenali Zygote 32-bit dan 64-bit (`/system/bin/app_process32` dan `/system/bin/app_process64`) yang dimulai oleh init.
- `kernel/fork.c` memanggil `devfreq_boost_kick_max(50)` ketika proses Zygote melakukan fork.
- `drivers/devfreq/devfreq_devbw.c` mendaftarkan fitur hanya pada device `qcom,cpubw`, bukan `qcom,mincpubw`.
- `drivers/devfreq/devfreq_boost.c` menetapkan `boost_min_freq` ke level tertinggi yang tersedia selama 50 ms, lalu menghapus boost dan mengembalikan kendali ke governor Devfreq.
- `CONFIG_DEVFREQ_BOOST=y` mengaktifkan fitur melalui `a11q_open_defconfig`.

**Makna pengamatan:** ketika aplikasi dibuka, FKM dapat memperlihatkan permintaan bandwidth naik ke level tertinggi; setelah 50 ms, boost sementara dilepas dan nilai dapat turun mengikuti governor serta beban aktual. Ini sesuai dengan desain. Jika aktivitas aplikasi masih membutuhkan bandwidth tinggi, governor tetap dapat mempertahankan level yang lebih tinggi.

Fitur ini mengubah **vote bandwidth bus CPU-to-DDR**, bukan kapasitas RAM, bukan tabel OPP/clock/tegangan DDR, dan bukan overclock RAM. Angka maksimum pada aplikasi pemantau adalah indikasi level permintaan bus/devfreq; jangan langsung mengartikannya sebagai clock fisik chip RAM.
## Pengujian pada perangkat

Source menunjukkan konfigurasi yang diminta kernel. Pada perangkat yang telah diuji, kernel berhasil boot dan pengujian beban CPU/GPU mencapai OPP maksimum; FKM juga menunjukkan pola DDR boost naik ketika aplikasi dibuka lalu turun setelah boost berakhir. Hasil ini adalah pengujian pada satu unit, bukan jaminan untuk seluruh unit perangkat.

Contoh pemeriksaan melalui Termux/root:

```sh
# CPU: daftar frekuensi yang diekspos cpufreq
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies

# CPU: frekuensi maksimum yang dikonfigurasi
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq

# CPU: CCI dan PLL (bila debugfs clock summary aktif)
cat /sys/kernel/debug/clk/clk_summary | grep -E 'cci_clk|apcs_hf_pll'

# GPU: frekuensi maksimum dan frekuensi saat ini
cat /sys/class/kgsl/kgsl-3d0/max_gpuclk
cat /sys/class/kgsl/kgsl-3d0/gpuclk
```

Tidak semua ROM/kernel mengekspos semua node sysfs. Jika suatu path tidak tersedia, periksa node devfreq/KGSL yang tersedia pada perangkat, jangan menganggap fitur gagal hanya dari satu path.

Lakukan pengujian bertahap:
- Boot, idle, dan suspend/resume.
- Beban CPU single-core lalu multi-core; periksa OPP tertinggi 2.611.200 kHz dan suhu/throttling.
- Periksa CCI clock ketika CPU berada pada OPP tertinggi; target perhitungan untuk OPP 2.6112 GHz adalah 1.044.480.000 Hz.
- Beban GPU bertahap pada 650, 700, 747, lalu 824 MHz; periksa apakah frekuensi benar-benar tercapai sebelum melanjutkan ke level berikutnya. Uji grafis/game dan beban berkelanjutan.
- Pantau suhu CPU/GPU, throttling, konsumsi daya, artefak visual, hang, reboot, dan kernel log.
- Hentikan pengujian jika muncul artefak, crash, suhu tidak wajar, atau ketidakstabilan.
- Simpan kernel/boot image yang diketahui stabil agar dapat melakukan rollback.

Pengujian boot dan beban awal sudah berhasil pada perangkat yang diuji. Untuk pemakaian jangka panjang, tetap pantau suhu, thermal throttling, konsumsi daya, dan kemungkinan error/reboot; hasil satu unit tidak menjamin karakteristik silicon unit lain.

## Catatan untuk pengembang

- Referensi perubahan OC pada dokumen ini mengacu pada branch `NOCTURNE-LIMITED-OC`.
- Frekuensi bawaan/reference dan frekuensi OC dipisahkan agar kenaikan mudah dilihat.
- Jangan memasukkan angka tegangan mV yang tidak didukung data PMIC/RPM atau pengukuran aktual.
- Jangan mencampur level bus bandwidth dengan clock DDR.
- Jika OPP diubah, audit semua tabel yang terkait: clock source, cpufreq/GPU OPP, voltage corner, regulator limit, bandwidth vote, energy model, dan thermal cooling.
- Setelah perubahan, periksa device tree hasil build serta uji pada perangkat. README ini adalah dokumentasi source, bukan sertifikasi stabilitas atau panduan menaikkan tegangan secara membabi buta.


## CPU OPP 2.6112 GHz dan CPR corner 11

OPP CPU tertinggi pada source branch ini adalah **2.6112 GHz** (PLL L value 136 dengan referensi 19,2 MHz). CCI mengikuti rasio CPU / 2,5, sehingga target CCI pada OPP ini adalah **1.04448 GHz**.

- OPP 2.6112 GHz menggunakan logical CPR corner 11. Anchor kalibrasi fuse hardware tetap sampai corner 10; kode CPR memperluas virtual OPP di atas anchor terakhir dan mempertahankan ceiling eksplisit untuk corner tertinggi.
- Ceiling software corner 11 dibatasi pada **1,14 V**. Ini batas atas yang dikonfigurasi, bukan tegangan konstan atau jaminan margin untuk semua chip.
- CPU PLL memakai MX TURBO untuk OPP 2.6112 GHz; batas OPP 2.448 GHz dan di bawahnya tetap menggunakan pemetaan MX sebelumnya.
- Nilai scheduler cost pada 2.6112 GHz merupakan estimasi, bukan hasil pengukuran daya.

OPP 2.6112 GHz telah berhasil boot dan mencapai frekuensi maksimum dalam pengujian beban CPU pada perangkat yang diuji. Pertahankan pemantauan suhu CPU/PMIC, konsumsi daya, throttling, dan stabilitas jangka panjang. Hasil pengujian satu perangkat tidak menjamin hasil yang sama pada semua unit SDM450.
