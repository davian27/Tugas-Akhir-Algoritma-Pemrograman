# Panduan Presentasi

Panduan ini berisi ringkasan proyek, urutan demo, dan contoh narasi. Sesuaikan bagian demo dengan file CSV yang akan digunakan.

## Ringkasan proyek

**Power Quality Analyzer** adalah aplikasi C++ yang membantu membaca dan merangkum data pengukuran tegangan serta arus. Pengguna memasukkan file CSV, aplikasi menghitung parameter listrik, menampilkan grafik, dan memberi indikasi kondisi berdasarkan ambang yang digunakan.

### Masalah yang diangkat

Data tegangan dan arus sering kali perlu diperiksa satu per satu. Menghitung nilai efektif, frekuensi, daya, dan distorsi secara manual memerlukan waktu. Aplikasi ini menyatukan pembacaan data dan perhitungan tersebut dalam satu alur.

### Tujuan

- Mempermudah input data pengukuran melalui CSV.
- Menyajikan ringkasan parameter listrik dan grafik gelombang.
- Memberikan indikasi awal jika nilai melewati ambang konfigurasi.
- Menyimpan ringkasan analisis sebagai log atau PDF.

## Narasi pembuka

> "Proyek saya adalah Power Quality Analyzer, aplikasi C++ untuk membantu menganalisis data tegangan dan arus dari file CSV. Aplikasi memvalidasi data, menghitung parameter listrik seperti Vrms, frekuensi, daya, dan THD, kemudian menampilkan ringkasan dan indikasi gangguan. Hasil analisis juga bisa disimpan sebagai laporan."

## Urutan demo aplikasi

1. **Tunjukkan halaman utama.** Kenalkan area pemilihan CSV, ringkasan hasil, grafik, dan panel diagnosis.
2. **Pilih file CSV.** Jelaskan bahwa susunan datanya adalah waktu dalam detik, tegangan dalam volt, dan arus dalam ampere.
3. **Jalankan analisis.** Tunjukkan bahwa aplikasi memeriksa format, waktu yang meningkat, dan interval sampel yang seragam.
4. **Baca hasil.** Jelaskan beberapa metrik utama menggunakan tabel di bawah.
5. **Buka halaman Analisis dan Riwayat.** Tunjukkan daftar analisis yang tersimpan, lalu klik salah satu baris untuk memuat kembali hasil dan grafiknya.
6. **Simpan hasil.** Gunakan **Ekspor PDF** untuk laporan yang dapat dibagikan, atau **Simpan laporan** untuk menambahkan hasil ke log.

## Penjelasan metrik dengan bahasa sederhana

| Metrik | Cara menjelaskannya saat presentasi |
|---|---|
| Vrms | Nilai efektif tegangan; berguna untuk membandingkan tingkat tegangan dengan nilai nominal. |
| Irms | Nilai efektif arus pada data yang dianalisis. |
| Vmax, Vmin, Vpp | Menunjukkan batas atas, batas bawah, dan rentang perubahan tegangan. |
| Frekuensi | Perkiraan banyaknya siklus gelombang per detik, dihitung dari lintasan tegangan melewati nol. |
| Daya aktif | Rata-rata daya yang dihitung dari pasangan sampel tegangan dan arus. |
| Daya semu | Perkalian nilai efektif tegangan dan arus. |
| Faktor daya (PF) | Perbandingan daya aktif terhadap daya semu; pada tampilan dan laporan diberi label PF. |
| THD | Perkiraan besarnya komponen harmonisa tegangan dibanding komponen fundamental. Nilai lebih tinggi menunjukkan distorsi harmonisa yang lebih besar menurut perhitungan ini. |

## Cara menjelaskan diagnosis

Diagnosis membandingkan nilai hasil analisis dengan konfigurasi aplikasi. Nilai awalnya adalah 220 Vrms untuk tegangan nominal, 50 Hz untuk frekuensi nominal, dan 5% untuk batas THD. Aplikasi menandai interruption jika Vrms kurang dari 10% nilai nominal, sag pada rentang 10% hingga kurang dari 90%, dan swell jika lebih dari 110%.

Gunakan istilah **indikasi** atau **hasil berdasarkan ambang aplikasi**. Perhitungan memakai satu jendela yang mencakup seluruh file, sehingga diagnosis tidak menunjukkan kapan gangguan dimulai atau berapa lama berlangsung. Nilai ambang ini juga bukan pernyataan bahwa aplikasi telah memenuhi standar IEC atau IEEE.

## Alur program dan pembagian modul

```text
CSV
  |
  v
DataLoader: baca dan validasi sampel
  |
  v
SignalAnalysis: RMS, puncak, frekuensi, THD
  |
  v
PowerAnalysis: daya aktif, daya semu, faktor daya
  |
  v
DisturbanceDetector: bandingkan hasil dengan ambang
  |
  v
GUI dan ReportManager: tampilkan, simpan log, ekspor PDF
  |
  v
AnalysisDatabase: simpan hasil dan sampel ke SQLite
```

Pemisahan ini membuat setiap bagian memiliki tugas yang jelas: pembacaan data, analisis sinyal, perhitungan daya, diagnosis, antarmuka, dan laporan.

## Materi perkuliahan yang diterapkan

Bagian ini menghubungkan materi algoritma dan C++ dengan contoh yang benar-benar ada di source code. Saat presentasi, pilih contoh yang terlihat pada demo dan jelaskan urutan **input, proses, lalu output**.

| Modul | Materi yang tampak di codebase | Contoh untuk dijelaskan |
|---|---|---|
| **1. Dasar algoritma dan C++** | Struktur program, variabel, tipe data, fungsi, input/output, dan pembagian algoritma menjadi langkah-langkah. | Versi console membaca pilihan dengan `cin`, menampilkan hasil dengan `cout`; struktur `Sample`, `MeasurementDataset`, dan `AnalysisResult` menyimpan data. Lihat `src/main.cpp` dan `Model.h`. |
| **2. Operator dan pustaka** | Operator aritmatika, relasi, logika, penugasan, fungsi matematika, dan format output. | Operasi RMS dan daya menggunakan aritmatika; `sqrt`, `sin`, `cos`, `hypot`, serta `fabs` mendukung perhitungan; `fixed` dan `setprecision` merapikan angka. Lihat `src/SignalAnalysis.cpp`, `src/PowerAnalysis.cpp`, dan `src/ReportManager.cpp`. |
| **3. Variabel dan kontrol keputusan** | Deklarasi dan inisialisasi variabel, `const`, kondisi boolean, `if/else`, dan `switch-case`. | Validasi CSV dan diagnosis memilih cabang berdasarkan nilai. GUI memakai `switch` untuk menangani pesan Windows. Kode memakai `#include` dan `#pragma once`, tetapi konstanta tidak didefinisikan dengan `#define`. Lihat `src/DataLoader.cpp`, `src/DisturbanceDetector.cpp`, dan `src/gui_main.cpp`. |
| **4. Perulangan** | `for`, `while`, counter, akumulator, `break`, `continue`, dan perulangan bersarang. | Loop mengumpulkan sampel dan menghitung jumlah kuadrat; loop bersarang menghitung proyeksi harmonisa. `break` dipakai pada menu console dan `continue` saat melewati baris kosong/header. Lihat `src/SignalAnalysis.cpp`, `src/DataLoader.cpp`, dan `src/main.cpp`. |
| **5. Fungsi dan array** | Fungsi modular, `std::vector` sebagai daftar sampel, Vpp, dan pencarian nilai maksimum-minimum. | `DataLoader`, `SignalAnalysis`, `PowerAnalysis`, dan `DisturbanceDetector` punya tanggung jawab terpisah. Vmax/Vmin dicari dengan memeriksa setiap sampel, lalu Vpp dihitung sebagai Vmax dikurangi Vmin. Lihat `src/SignalAnalysis.cpp` dan `Model.h`. |
| **6. String dan pola IPO** | `std::string`, parsing CSV, pencarian dan pemotongan string, parameter fungsi, nilai balik, serta alur input-proses-output. | `DataLoader` memisahkan kolom CSV dan mengubahnya menjadi angka; laporan menggunakan operasi string untuk memformat isi. Lihat `src/DataLoader.cpp` dan `src/ReportManager.cpp`. |
| **7. Algoritma efisien dan proyek mandiri** | Logging hasil, penyimpanan database, dan pola input-proses-output. | `ReportManager` menambahkan laporan ke `output/analysis.log`; `AnalysisDatabase` menyimpan ringkasan, konfigurasi, diagnosis, dan sampel ke `output/analysis_history.sqlite3`. Riwayat dapat dibuka lagi setelah aplikasi dijalankan ulang. Proyek belum mengolah data 24 jam, menghitung jam puncak, atau load factor. |
| **8. Pengujian dan fungsi matematika** | Kode modular, self-test, dan fungsi transenden selain `exp()`. | `Tests.cpp` memeriksa nilai teoritis sinyal sintetis untuk Vrms, frekuensi, faktor daya, THD, dan interruption. Pemeriksaan memakai kondisi dan exception, bukan `assert()`. `exp()` tidak digunakan; fungsi seperti `sin`, `cos`, dan `sqrt` digunakan untuk analisis. |

### Topik yang belum diterapkan secara langsung

- Pseudocode dan flowchart formal belum menjadi artefak proyek; diagram alur di atas hanya ringkasan proses.
- Function overloading, array 2D/3D, `do-while`, `exit()`, dan `assert()` tidak digunakan.
- GUI memakai halaman dan event loop, tetapi belum memakai finite state machine formal.
- Bitwise hanya muncul untuk memeriksa flag API Windows dan byte encoding; bukan bagian utama algoritma listrik.
- Belum ada analisis beban 24 jam, arus rata-rata/puncak per waktu, jam puncak, ataupun load factor.

### Narasi singkat untuk menjelaskan materi

> "Materi yang paling jelas terlihat adalah fungsi modular, validasi dan parsing string CSV, percabangan, perulangan, penggunaan vector, serta operasi matematika. Contohnya, data dibaca dan divalidasi terlebih dahulu, kemudian loop digunakan untuk menghitung RMS dan mencari nilai maksimum-minimum. Hasilnya diteruskan ke modul daya dan diagnosis, lalu ditampilkan atau disimpan sebagai laporan. Self-test memeriksa beberapa perhitungan dengan sinyal sintetis."

## Batasan yang sebaiknya disampaikan

- Data dimasukkan melalui file; aplikasi tidak terhubung langsung ke sensor.
- Waktu sampel perlu meningkat dengan interval yang seragam.
- Frekuensi dihitung menggunakan zero-crossing dan THD menggunakan proyeksi DFT langsung; hasil bergantung pada kualitas serta panjang data.
- Analisis saat ini merangkum satu file secara keseluruhan, bukan melacak waktu kejadian.
- Diagnosis merupakan indikasi berbasis ambang yang dapat dikonfigurasi, bukan sertifikasi atau sistem proteksi.

## Narasi penutup

> "Aplikasi ini menunjukkan bagaimana C++ dapat digunakan untuk mengolah data pengukuran secara modular, mulai dari validasi CSV sampai pelaporan. Pengembangan berikutnya dapat mencakup analisis per jendela waktu agar lokasi dan durasi gangguan dapat diketahui, serta pengujian dengan dataset yang lebih beragam."

## Pertanyaan yang mungkin muncul

**Mengapa CSV harus memiliki interval waktu seragam?**

Karena estimasi frekuensi dan analisis harmonisa mengandalkan jarak waktu antarsampel yang konsisten.

**Mengapa nilai THD dapat tidak tersedia atau berubah?**

THD memerlukan frekuensi fundamental yang terdeteksi dan jumlah sampel yang memadai. Hasil proyeksi DFT juga dipengaruhi panjang jendela dan jumlah siklus di dalam data.

**Apakah diagnosis menyatakan data pasti memenuhi standar?**

Tidak. Diagnosis hanya menunjukkan hasil perbandingan terhadap ambang yang sedang digunakan aplikasi.
