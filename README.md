# Power Quality Analyzer

Power Quality Analyzer adalah aplikasi C++ untuk membaca data tegangan dan arus dari CSV, menghitung beberapa parameter listrik, lalu memberi indikasi awal kondisi kualitas daya. Aplikasi menyediakan antarmuka desktop Windows dan versi console.

Proyek ini membantu pengguna memahami data hasil pengukuran dengan lebih cepat. Aplikasi tidak membaca sensor secara langsung, tidak mengendalikan peralatan listrik, dan tidak menyatakan kepatuhan terhadap standar tertentu.

## Cara kerja singkat

1. Pilih file CSV yang berisi waktu, tegangan, dan arus.
2. Aplikasi memeriksa format serta urutan dan jarak waktu sampel.
3. Aplikasi menghitung nilai listrik dan menganalisis gelombang.
4. Hasil dibandingkan dengan ambang konfigurasi untuk membuat diagnosis awal.
5. Hasil dapat dilihat di aplikasi, disimpan ke log, atau diekspor sebagai PDF.

## Parameter yang ditampilkan

| Parameter | Arti singkat |
|---|---|
| Vmax, Vmin | Tegangan tertinggi dan terendah di antara sampel. |
| Vpp | Selisih tegangan tertinggi dan terendah. |
| Vrms, Irms | Nilai efektif tegangan dan arus pada seluruh data. |
| Frekuensi | Perkiraan frekuensi tegangan dari titik lintas nol. |
| Daya aktif | Rata-rata hasil kali tegangan dan arus pada tiap sampel. |
| Daya semu | Hasil kali Vrms dan Irms. |
| Faktor daya (PF) | Perbandingan daya aktif dengan daya semu; ditampilkan dengan label PF. |
| THD tegangan | Perkiraan distorsi harmonisa tegangan hingga orde yang dikonfigurasi. |

## Format file CSV

Gunakan tiga kolom dengan urutan **waktu (detik), tegangan (volt), arus (ampere)**. Pemisah kolom adalah koma. Header boleh dihilangkan; jika digunakan, nama kolom harus dikenali seperti `waktu/time/timestamp`, `tegangan/voltage`, dan `arus/current`. Nilai desimal harus menggunakan titik.

```csv
waktu,tegangan,arus
0.000000,0.000000,-3.082207
0.000200,19.535825,-2.676529
0.000400,38.994551,-2.260287
```

Waktu sampel harus meningkat dan berjarak seragam (toleransi 0,1%). Minimal dua sampel diperlukan. Baris kosong dilewati, sedangkan data yang tidak valid akan ditolak dengan keterangan nomor baris.

## Menjalankan aplikasi desktop di Windows

Kompilasi dengan g++ MinGW-w64 yang mendukung C++17 dari folder proyek:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -municode -mwindows src/AnalysisDatabase.cpp src/DataLoader.cpp src/SignalAnalysis.cpp src/PowerAnalysis.cpp src/DisturbanceDetector.cpp src/ReportManager.cpp src/gui_main.cpp -lcomdlg32 '-Wl,-Bstatic' -lsqlite3 '-Wl,-Bdynamic' -o PowerQualityAnalyzerGUI.exe
```

Lalu jalankan:

```powershell
./PowerQualityAnalyzerGUI.exe
```

Pilih CSV dari panel aplikasi, tekan **Analisis**, lalu lihat ringkasan dan grafik. Riwayat disimpan ke database lokal `output/analysis_history.sqlite3` dan tetap tersedia setelah aplikasi ditutup. Klik baris riwayat untuk membuka kembali hasil dan grafik; sampel, ringkasan, diagnosis, waktu analisis, dan konfigurasi disimpan bersama. Tombol **Simpan laporan** menambahkan laporan teks ke `output/analysis.log`; **Ekspor PDF** membuka dialog untuk menentukan lokasi PDF.

## Menjalankan versi console

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/AnalysisDatabase.cpp src/DataLoader.cpp src/SignalAnalysis.cpp src/PowerAnalysis.cpp src/DisturbanceDetector.cpp src/ReportManager.cpp src/Config.cpp src/Tests.cpp src/main.cpp '-Wl,-Bstatic' -lsqlite3 '-Wl,-Bdynamic' -o PowerQualityAnalyzer.exe
./PowerQualityAnalyzer.exe
```

Menu console menyediakan analisis CSV, self-test, pengaturan ambang, dan keluar. Kompilasi manual memerlukan header dan library SQLite; perintah di atas menautkan SQLite secara statis. Untuk build CMake, SQLite development library perlu tersedia di sistem. Build Windows menghasilkan aplikasi desktop; pada platform lain, konfigurasi proyek menghasilkan versi console.

## Cara membaca diagnosis

Nilai awal aplikasi adalah tegangan nominal 220 Vrms, frekuensi nominal 50 Hz, dan batas THD 5%. Pada versi console, nilai konfigurasi dapat diubah selama sesi. Aturan diagnosis default menandai interruption di bawah 10% tegangan nominal, sag dari 10% sampai kurang dari 90%, dan swell di atas 110%.

Diagnosis saat ini memakai satu nilai Vrms dan THD yang dihitung dari seluruh file. Karena itu, hasil menunjukkan indikasi agregat dan tidak menunjukkan waktu mulai, durasi, atau lokasi kejadian. Ambang tersebut adalah pengaturan awal aplikasi, bukan klaim kesesuaian IEC/IEEE.

THD dihitung dengan proyeksi DFT langsung hingga orde harmonisa yang dikonfigurasi (default orde 10), selama frekuensi Nyquist memungkinkan. Estimasi lebih dapat diandalkan jika data memuat jumlah siklus bulat; kebocoran spektrum dapat memengaruhi hasil.

## Struktur kode

| Berkas | Tanggung jawab |
|---|---|
| `src/gui_main.cpp` | Antarmuka desktop, interaksi, grafik, dan riwayat. |
| `src/AnalysisDatabase.cpp` | Menyimpan dan memuat riwayat serta sampel dari SQLite. |
| `src/main.cpp` | Menu dan alur versi console. |
| `src/DataLoader.cpp` | Membaca serta memvalidasi CSV. |
| `src/SignalAnalysis.cpp` | RMS, nilai puncak, frekuensi, dan THD. |
| `src/PowerAnalysis.cpp` | Daya aktif, daya semu, dan faktor daya. |
| `src/DisturbanceDetector.cpp` | Diagnosis berdasarkan ambang konfigurasi. |
| `src/ReportManager.cpp` | Tampilan laporan, log teks, dan ekspor PDF. |
| `src/Config.cpp` | Pengaturan ambang pada versi console. |
| `src/Tests.cpp` | Self-test perhitungan dasar. |
| `Model.h` | Struktur data dan nilai konfigurasi. |

Untuk urutan demo dan catatan penjelasan saat presentasi, lihat [Panduan Presentasi](PANDUAN_PRESENTASI.md). Spesifikasi kebutuhan awal tersedia di [PRD](PRD_Power_Quality_Analyzer.md), sedangkan catatan rancangan GUI ada di [gui.md](gui.md).
