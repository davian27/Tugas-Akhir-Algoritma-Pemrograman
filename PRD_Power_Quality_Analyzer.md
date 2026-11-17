# Product Requirements Document (PRD)
## Power Quality Analyzer Berbasis C++

**Versi:** 1.0 — Draft implementasi  
**Platform:** Desktop / Console Application  
**Bahasa pemrograman:** C++11 atau lebih baru  
**Pengguna utama:** Mahasiswa, praktikan, dan pengguna yang menganalisis data listrik  
**Status:** Spesifikasi awal untuk coding agent

---

## 1. Ringkasan Produk

Power Quality Analyzer adalah aplikasi desktop berbasis C++ untuk membaca data hasil pengukuran tegangan dan arus dari file CSV, menghitung parameter kualitas daya, mengidentifikasi indikasi gangguan, serta menyimpan hasil analisis ke dalam file laporan/log.

Aplikasi ditujukan sebagai proyek tugas akhir pemrograman C++ yang menerapkan materi dasar pemrograman sekaligus menyelesaikan permasalahan teknik elektro.

### 1.1 Masalah yang Diselesaikan

- Analisis data pengukuran tegangan dan arus secara manual membutuhkan waktu dan rawan kesalahan.
- Pengguna membutuhkan ringkasan parameter listrik dalam satu proses.
- Perubahan nilai tegangan dan distorsi harmonisa perlu diberi indikasi awal agar data lebih mudah dievaluasi.
- Hasil analisis perlu disimpan agar dapat ditinjau kembali dan digunakan dalam laporan praktikum/proyek.

### 1.2 Tujuan Produk

- Menyediakan proses impor data pengukuran melalui CSV yang sederhana.
- Menghitung parameter listrik secara konsisten dan transparan.
- Memberikan diagnosis awal berdasarkan ambang batas yang dapat dikonfigurasi.
- Menyimpan hasil analisis beserta informasi file dan waktu proses.
- Menghasilkan program yang modular, mudah diuji, dan dapat dikembangkan.

### 1.3 Bukan Tujuan (Out of Scope)

- Bukan alat ukur listrik mandiri dan tidak membaca sensor/hardware secara langsung pada versi pertama.
- Bukan sistem proteksi listrik yang mengendalikan pemutus sirkuit atau peralatan daya.
- Bukan perangkat tersertifikasi untuk audit kepatuhan standar atau keputusan keselamatan.
- Tidak menggunakan AI atau machine learning pada versi pertama.
- Tidak memerlukan database server, akun pengguna, atau koneksi internet.

---

## 2. Target Pengguna dan Skenario

| Pengguna | Kebutuhan | Skenario |
|---|---|---|
| Mahasiswa Teknik Elektro | Mengolah data praktikum/proyek | Memilih CSV, menjalankan analisis, lalu menyalin hasil ke laporan. |
| Praktikan/laboran | Mengevaluasi data pengukuran | Memeriksa RMS, frekuensi, daya, THD, dan indikasi gangguan. |
| Pengembang/penguji | Memastikan perhitungan benar | Menjalankan data uji dan memeriksa hasil terhadap nilai teoritis. |

### 2.1 Alur Pengguna Utama

1. Pengguna menjalankan aplikasi.
2. Pengguna memilih menu analisis data CSV.
3. Pengguna memasukkan nama/path file CSV.
4. Aplikasi memvalidasi struktur, jumlah, dan nilai data.
5. Aplikasi menghitung parameter kualitas daya.
6. Aplikasi mengevaluasi ambang batas gangguan.
7. Aplikasi menampilkan hasil dan status diagnosis.
8. Pengguna memilih menyimpan hasil; aplikasi menambahkan catatan ke file log/laporan.

---

## 3. Ruang Lingkup Fitur

| ID | Fitur | Prioritas | Deskripsi |
|---|---|---|---|
| F-01 | Menu utama | Must | Menu analisis CSV, pengujian, pengaturan ambang, dan keluar. |
| F-02 | Impor CSV | Must | Membaca kolom waktu, tegangan, arus; mendukung header. |
| F-03 | Validasi data | Must | Memeriksa kolom, angka, urutan waktu, jumlah sampel, dan nilai tidak valid. |
| F-04 | Parameter tegangan/arus | Must | Menghitung Vmax, Vmin, Vpp, Vrms, dan Irms. |
| F-05 | Parameter daya | Must | Menghitung daya aktif rata-rata, daya semu, dan faktor daya. |
| F-06 | Frekuensi | Must | Estimasi frekuensi dengan zero-crossing dan interpolasi. |
| F-07 | Harmonisa/THD | Should | Mengestimasi fundamental dan harmonisa hingga orde yang ditentukan. |
| F-08 | Deteksi gangguan | Must | Memberi indikasi normal, sag, swell, interruption, harmonisa, atau multi-gangguan. |
| F-09 | Laporan/log | Must | Menyimpan ringkasan hasil ke file lokal. |
| F-10 | Pengaturan | Should | Mengubah tegangan nominal, frekuensi nominal, batas sag/swell, dan batas THD. |
| F-11 | Pengujian otomatis | Must | Menguji fungsi perhitungan menggunakan data uji dengan nilai yang diketahui. |
| F-12 | Ekspor hasil CSV | Could | Menyimpan hasil ringkas dalam format CSV terpisah. |

---

## 4. Persyaratan Fungsional

### 4.1 Impor dan Validasi CSV

Format minimum yang diterima:

```csv
waktu,tegangan,arus
0.0000,0.0,0.0
0.0001,48.0,0.5
```

Ketentuan:

- Kolom wajib: waktu (detik), tegangan (volt), arus (ampere).
- Baris header boleh ada atau tidak ada.
- Delimiter versi pertama adalah koma (`,`).
- Minimal 2 sampel untuk membaca data; untuk estimasi frekuensi dan THD diperlukan sampel yang memadai.
- Waktu harus numerik, meningkat, dan tidak duplikat. Interval sampel harus dapat ditentukan.
- Baris kosong boleh dilewati; baris data rusak harus dilaporkan dengan nomor baris, bukan diam-diam dianggap data valid.
- Aplikasi harus menolak file kosong, file tidak ditemukan, jumlah kolom salah, atau data non-finite (NaN/Inf).

### 4.2 Perhitungan Parameter

| Parameter | Definisi/rumus | Satuan |
|---|---|---|
| Vmax | Nilai maksimum sampel tegangan | V |
| Vmin | Nilai minimum sampel tegangan | V |
| Vpp | Vmax − Vmin | V |
| Vrms | `sqrt(Σ(v[i]²) / N)` | V |
| Irms | `sqrt(Σ(i[i]²) / N)` | A |
| Daya aktif P | `Σ(v[i] × i[i]) / N` | W |
| Daya semu S | `Vrms × Irms` | VA |
| Faktor daya | `P / S`, jika S > 0 | Tanpa satuan |
| Frekuensi | Dari periode antar zero-crossing positif | Hz |
| THD | `sqrt(Σ(Vh_rms²), h≥2) / V1_rms × 100%` | % |

**Catatan:** Rumus di atas mengasumsikan sampel mewakili jendela pengukuran yang sesuai. Untuk daya aktif, tegangan dan arus harus tersinkronisasi dan menggunakan skala/satuan yang benar.

### 4.3 Deteksi Gangguan

Ambang batas awal harus dapat diubah melalui konfigurasi. Nilai default berikut hanya untuk demonstrasi, bukan klaim kepatuhan standar:

| Kondisi | Aturan default | Keterangan |
|---|---|---|
| Voltage sag | `0,10 × Vnom ≤ Vrms < 0,90 × Vnom` | Tegangan turun di bawah 90% nominal. |
| Voltage swell | `Vrms > 1,10 × Vnom` | Tegangan naik di atas 110% nominal. |
| Interruption | `Vrms < 0,10 × Vnom` | Tegangan sangat rendah/hampir hilang. |
| Harmonic distortion | `THD > 5%` | Indikasi THD melewati ambang konfigurasi. |
| Multiple disturbance | Lebih dari satu kondisi aktif | Beberapa indikator terdeteksi bersamaan. |
| Normal | Tidak ada kondisi di atas | Tidak ditemukan indikasi berdasarkan aturan versi ini. |

> **Penting:** Deteksi berbasis satu nilai RMS untuk seluruh file hanya memberi klasifikasi agregat. Versi lanjutan sebaiknya menggunakan sliding window/cycle window agar durasi dan waktu mulai/selesai gangguan dapat ditentukan. Jangan menyebutnya deteksi sesuai IEC/IEEE sebelum algoritma, jendela, agregasi, dan ambang batas benar-benar diimplementasikan serta diverifikasi.

---

## 5. Persyaratan Antarmuka

Versi pertama menggunakan antarmuka console agar fokus pada algoritma C++ dan materi perkuliahan.

| Layar/menu | Isi minimum |
|---|---|
| Menu utama | 1) Analisis CSV  2) Pengujian  3) Pengaturan ambang (opsional)  4) Keluar |
| Input file | Prompt path file CSV dan pesan jika file tidak dapat dibuka. |
| Hasil analisis | Kelompok hasil tegangan, arus, daya, frekuensi, THD, dan diagnosis. |
| Validasi/error | Pesan jelas, misalnya nomor baris bermasalah dan alasan penolakan. |
| Penyimpanan | Konfirmasi lokasi file log/laporan yang berhasil dibuat. |

### 5.1 Contoh Tampilan Hasil

```text
POWER QUALITY ANALYZER
--------------------------------
File              : data_listrik.csv
Jumlah sampel     : 5000

TEGANGAN
Vmax              : ... V
Vmin              : ... V
Vpp               : ... V
Vrms              : ... V

ARUS DAN DAYA
Irms              : ... A
Frekuensi         : ... Hz
Daya aktif        : ... W
Daya semu         : ... VA
Faktor daya       : ...

HARMONISA
THD               : ... %

DIAGNOSIS
Status            : VOLTAGE SAG
--------------------------------
```

---

## 6. Arsitektur Teknis

Gunakan pemisahan tanggung jawab agar program mudah dipahami, diuji, dan dikembangkan. Hindari menempatkan seluruh logika di `main()` atau menggunakan variabel global untuk seluruh data.

| Modul/file | Tanggung jawab |
|---|---|
| `main.cpp` | Menampilkan menu dan mengatur alur aplikasi. |
| `DataLoader.h/.cpp` | Membaca CSV dan memvalidasi data. |
| `SignalAnalysis.h/.cpp` | RMS, puncak, zero-crossing, frekuensi, dan THD. |
| `PowerAnalysis.h/.cpp` | Daya aktif, daya semu, dan faktor daya. |
| `DisturbanceDetector.h/.cpp` | Menerapkan aturan diagnosis gangguan. |
| `ReportManager.h/.cpp` | Menampilkan, menyimpan log, dan ekspor ringkasan. |
| `Config.h/.cpp` | Menyimpan nilai nominal dan ambang batas. |
| `Tests.cpp` | Pengujian unit dan kasus batas. |

### 6.1 Model Data yang Disarankan

- `Sample`: `timeSeconds`, `voltage`, `current`.
- `MeasurementDataset`: `vector<Sample>` dan metadata nama file.
- `AnalysisResult`: nilai puncak, RMS, frekuensi, daya, THD, serta status validitas.
- `DisturbanceReport`: daftar jenis indikasi, nilai pemicu, ambang yang dipakai, dan rentang waktu jika tersedia.

### 6.2 Ketentuan Implementasi C++

- Gunakan C++11 atau lebih baru; disarankan C++17 bila lingkungan compiler mendukung.
- Gunakan `std::vector`, `std::string`, `std::ifstream`, `std::ofstream`, dan fungsi dari `<cmath>`.
- Hindari array global berukuran tetap jika jumlah sampel dapat ditampung secara dinamis.
- Pisahkan header (`.h`) dan implementasi (`.cpp`) untuk modul utama.
- Tangani exception parsing secara terarah dan berikan pesan yang informatif.
- Gunakan `double` untuk perhitungan numerik.
- Jangan menganggap faktor daya selalu berada pada rentang 0–1 tanpa memeriksa kualitas data dan definisi perhitungannya.

---

## 7. Algoritma dan Catatan Akurasi

### 7.1 RMS

RMS dihitung dari seluruh sampel dalam jendela: akar dari rata-rata kuadrat sampel. Untuk sinyal AC dengan offset DC, tentukan apakah RMS total atau RMS komponen AC yang ingin dilaporkan; jangan menghilangkan offset secara diam-diam.

### 7.2 Frekuensi Zero-Crossing

Deteksi crossing positif (sampel sebelumnya ≤ 0 dan sampel saat ini > 0), lalu interpolasi waktu crossing. Frekuensi dihitung dari jumlah periode dibagi selang waktu crossing pertama dan terakhir. Tolak hasil jika crossing tidak cukup atau selang waktu tidak valid.

### 7.3 THD

Untuk versi awal, gunakan DFT langsung atau algoritma spektral yang dijelaskan dengan jelas. Pastikan frekuensi sampling, jumlah siklus, kebocoran spektrum (*spectral leakage*), sinkronisasi, dan pemilihan bin harmonisa diperhatikan.

Alternatif yang disarankan adalah FFT jika pustaka tambahan diizinkan; jika tidak, implementasikan DFT dengan batasan ukuran data yang terdokumentasi.

### 7.4 Jendela Analisis

Rancangan versi 1 dapat menganalisis satu file sebagai satu jendela agregat. Untuk versi lanjutan, analisis per siklus atau sliding window lebih tepat untuk menemukan waktu kejadian, durasi, dan perubahan gangguan. Tampilkan metode yang dipakai pada laporan.

---

## 8. Persyaratan Non-Fungsional

| ID | Persyaratan | Kriteria |
|---|---|---|
| NF-01 | Portabilitas | Dapat dikompilasi pada compiler C++ modern di Windows; kode tidak bergantung pada path absolut. |
| NF-02 | Kinerja | Mampu memproses setidaknya 100.000 sampel dalam waktu yang wajar pada komputer kuliah. |
| NF-03 | Keandalan | Input rusak tidak menyebabkan crash; kesalahan dilaporkan. |
| NF-04 | Keterbacaan | Nama fungsi/variabel deskriptif dan kode modular. |
| NF-05 | Reproducibility | File dan konfigurasi yang sama menghasilkan hasil yang sama. |
| NF-06 | Keamanan penggunaan | Aplikasi hanya menganalisis file; tidak mengendalikan perangkat listrik. |

---

## 9. Pengujian dan Kriteria Penerimaan

| ID uji | Kasus | Hasil yang diharapkan |
|---|---|---|
| T-01 | CSV sinus 50 Hz dengan amplitudo dan skala diketahui | Vrms dan frekuensi mendekati nilai teoritis dalam toleransi yang ditetapkan. |
| T-02 | Tegangan konstan 0 V | Vrms = 0; tidak terjadi pembagian dengan nol; diagnosis interruption jika konfigurasi mengaktifkannya. |
| T-03 | Arus nol | Irms = 0; daya semu = 0; faktor daya ditampilkan sebagai tidak terdefinisi/—. |
| T-04 | File tidak ditemukan | Pesan error jelas dan aplikasi tetap berjalan. |
| T-05 | CSV kolom kurang atau nilai bukan angka | File ditolak atau baris invalid dilaporkan dengan nomor baris. |
| T-06 | Waktu duplikat/tidak berurutan | Validasi gagal dengan pesan yang jelas. |
| T-07 | Vrms di bawah 90% nominal | Indikasi sag sesuai aturan konfigurasi. |
| T-08 | Vrms di atas 110% nominal | Indikasi swell sesuai aturan konfigurasi. |
| T-09 | Sinyal dengan harmonisa terukur | THD mendekati nilai referensi pada kondisi sampling yang memenuhi asumsi. |
| T-10 | Simpan hasil | Log berhasil dibuat dan berisi parameter serta diagnosis. |

**Kriteria selesai:** seluruh fitur Must berjalan, semua kasus uji utama lulus, tidak ada crash pada input salah, rumus dan asumsi didokumentasikan, serta contoh data uji disertakan.

---

## 10. Struktur Folder Proyek

```text
PowerQualityAnalyzer/
├── README.md
├── PRD_Power_Quality_Analyzer.md
├── data/
│   ├── sample_normal.csv
│   ├── sample_sag.csv
│   ├── sample_swell.csv
│   └── sample_harmonic.csv
├── include/
│   ├── DataLoader.h
│   ├── SignalAnalysis.h
│   ├── PowerAnalysis.h
│   ├── DisturbanceDetector.h
│   ├── ReportManager.h
│   └── Config.h
├── src/
│   ├── main.cpp
│   ├── DataLoader.cpp
│   ├── SignalAnalysis.cpp
│   ├── PowerAnalysis.cpp
│   ├── DisturbanceDetector.cpp
│   ├── ReportManager.cpp
│   └── Config.cpp
├── tests/
│   └── Tests.cpp
└── output/
    └── (file laporan/log yang dihasilkan)
```

---

## 11. Tahapan Pengerjaan yang Diminta kepada Agent

| Tahap | Pekerjaan agent | Output yang harus diperiksa |
|---|---|---|
| 1. Analisis | Baca PRD, jelaskan asumsi, usulkan struktur file. | Rencana implementasi dan daftar asumsi. |
| 2. Fondasi | Buat model data, menu, dan parser CSV. | Program dapat membaca dan memvalidasi CSV. |
| 3. Perhitungan | Implementasikan RMS, puncak, daya, dan frekuensi. | Unit test dengan data teoritis. |
| 4. Harmonisa | Implementasikan DFT/FFT dan THD dengan batasan yang terdokumentasi. | Uji sinyal fundamental dan harmonisa. |
| 5. Diagnosis | Implementasikan aturan gangguan berbasis konfigurasi. | Uji setiap kondisi dan multi-gangguan. |
| 6. Laporan | Implementasikan tampilan dan penyimpanan log/CSV. | File keluaran dapat dibuka dan dibaca. |
| 7. Finalisasi | Perbaiki error, README, contoh dataset, dan instruksi compile. | Proyek dapat dibangun dari awal. |

### 11.1 Instruksi Kerja untuk Coding Agent

Salin instruksi berikut bersama PRD ini ke agent:

> Anda bertindak sebagai software engineer C++ yang mengimplementasikan aplikasi sesuai PRD ini.
>
> 1. Jangan langsung membuat seluruh kode dalam satu file. Kerjakan bertahap sesuai tahapan pada bagian 11.
> 2. Sebelum coding, ringkas pemahaman kebutuhan dan tampilkan struktur folder yang akan dibuat.
> 3. Gunakan C++11 atau lebih baru, kode modular, validasi input yang ketat, dan unit test.
> 4. Jangan mengarang hasil pengukuran. Sertakan dataset sintetis dengan parameter yang diketahui untuk pengujian.
> 5. Untuk setiap rumus, jelaskan asumsi, satuan, dan batasannya.
> 6. Jika ada kebutuhan yang ambigu, ajukan pertanyaan sebelum mengambil keputusan yang memengaruhi algoritma.
> 7. Setelah setiap tahap, tampilkan file yang dibuat/diubah, cara compile, cara menjalankan, dan hasil pengujian.
> 8. Jangan mengklaim kesesuaian standar IEC/IEEE tanpa implementasi dan verifikasi yang mendukungnya.
> 9. Prioritaskan kebenaran perhitungan dan keterlacakan hasil dibanding banyaknya fitur.

---

## 12. Risiko dan Mitigasi

| Risiko | Dampak | Mitigasi |
|---|---|---|
| Data sampling tidak memadai | Frekuensi/THD salah atau tidak stabil | Validasi sampling dan jumlah siklus; tampilkan peringatan. |
| Data tegangan dan arus tidak sinkron | Daya aktif dan faktor daya keliru | Dokumentasikan kebutuhan sinkronisasi dan validasi timestamp. |
| Algoritma THD sederhana | Hasil spektrum bias | Gunakan DFT/FFT dengan sampling tepat dan uji sinyal sintetis. |
| Ambang gangguan dianggap standar resmi | Kesimpulan teknis menyesatkan | Labeli sebagai ambang konfigurasi/demo dan dokumentasikan sumber standar bila kelak digunakan. |
| CSV berukuran besar | Memori/waktu proses meningkat | Gunakan vector, batas ukuran yang jelas, dan optimasi setelah pengukuran. |

---

## 13. Rencana Pengembangan Lanjutan

- Analisis sliding window untuk mendeteksi waktu mulai, waktu selesai, dan durasi gangguan.
- Grafik gelombang tegangan/arus dan spektrum harmonisa.
- Konfigurasi profil sistem 1-fasa/3-fasa.
- Ekspor laporan ke HTML/PDF setelah versi console stabil.
- Dukungan format data dari perangkat akuisisi tertentu.
- Perbandingan hasil terhadap standar kualitas daya yang relevan setelah metode dan pengujian tervalidasi.

---

## 14. Definisi Selesai (Definition of Done)

- [ ] Source code terpisah secara modular dan dapat dikompilasi tanpa error.
- [ ] Program dapat membaca CSV valid dan memberi pesan yang benar untuk CSV tidak valid.
- [ ] Parameter listrik utama dihitung dengan rumus dan satuan yang terdokumentasi.
- [ ] Diagnosis gangguan menggunakan ambang yang dapat ditelusuri.
- [ ] Pengujian normal, sag, swell, interruption, harmonisa, dan input salah tersedia.
- [ ] Hasil analisis dapat disimpan dan dibuka kembali.
- [ ] README menjelaskan kebutuhan compiler, perintah compile, format CSV, dan contoh penggunaan.
- [ ] Tidak ada klaim akurasi atau kepatuhan standar yang belum dibuktikan.

---

## Lampiran A — Catatan Dataset Uji

Agent harus membuat dataset sintetis terpisah untuk tiap skenario. Gunakan waktu sampling seragam, misalnya `fs = 5 kHz` atau lebih tinggi sesuai harmonisa yang ingin diuji, dan durasi beberapa siklus.

Contoh sinyal:

`v(t) = √2 × Vrms × sin(2πft)`

dengan arus yang memiliki beda fase untuk menguji daya dan faktor daya. Untuk THD, tambahkan komponen harmonisa dengan amplitudo RMS yang diketahui. Cantumkan parameter pembangkit data agar hasil program dapat dibandingkan dengan nilai teoritis.

## Lampiran B — Keputusan Default Versi 1

| Item | Default |
|---|---|
| Tegangan nominal | 220 V (dapat diubah) |
| Frekuensi nominal | 50 Hz (dapat diubah) |
| Batas sag | Di bawah 90% nominal dan minimal 10% nominal |
| Batas swell | Di atas 110% nominal |
| Batas interruption | Di bawah 10% nominal |
| Batas THD | 5% (ambang awal yang dapat diubah) |
| Antarmuka | Console |
| Format input | CSV: waktu, tegangan, arus |
| Format output | TXT log; ekspor CSV sebagai fitur opsional |
