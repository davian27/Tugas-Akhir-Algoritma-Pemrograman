# TASK: REDESIGN GUI POWER QUALITY ANALYZER

Lakukan redesign GUI aplikasi Power Quality Analyzer berdasarkan desain dashboard Dark Navy + Cyan.

**Teknologi wajib:**
- C++ Win32 API.
- Windows GDI/GDI+ untuk rendering.
- Tidak perlu migrasi ke Qt atau framework GUI lain.

## 1. Desain
Buat tampilan modern dengan:
- Sidebar navigasi di sebelah kiri.
- Header dashboard.
- Panel upload CSV.
- Empat kartu statistik.
- Grafik gelombang tegangan dan arus.
- Panel diagnosis kualitas daya.
- Panel harmonisa THD.
- Tabel riwayat analisis.

## 2. Warna
- Background: #081426
- Sidebar: #0D1B32
- Card: #10233F
- Primary cyan: #00C8F0
- Text utama: #F1F5F9
- Text sekunder: #94A3B8
- Normal: hijau
- Warning: kuning/oranye
- Error: merah

## 3. Fungsionalitas
Pertahankan seluruh fungsi analisis yang sudah ada pada gui_main.cpp.

Jangan mengubah kontrak fungsi:
- DataLoader::loadCsv()
- SignalAnalysis::calculateVoltageCurrent()
- SignalAnalysis::calculateFrequency()
- PowerAnalysis::calculate()
- SignalAnalysis::calculateVoltageThd()
- DisturbanceDetector::evaluate()
- ReportManager::appendLog()

## 4. Grafik
Buat grafik tegangan dan arus berdasarkan data CSV yang benar-benar dibaca.

Jangan menggunakan data dummy untuk menampilkan hasil pengukuran.

## 5. Interaksi
- Tombol Pilih CSV membuka file dialog.
- Tombol Analisis menjalankan seluruh proses analisis.
- Tombol Simpan Laporan menyimpan laporan.
- Navigasi sidebar berpindah halaman.
- Riwayat menampilkan hasil analisis sebelumnya.

## 6. Struktur kode
Pisahkan kode rendering GUI, event handler, dan logika analisis jika memungkinkan.

Gunakan double buffering agar tidak terjadi flickering saat jendela diperbarui.

Pastikan semua komponen responsif ketika ukuran jendela berubah.

## 7. Ketentuan penting
- Jangan menghapus fitur yang sudah bekerja.
- Jangan mengganti algoritma perhitungan tanpa alasan.
- Jangan membuat angka statistik palsu.
- Pastikan seluruh fungsi dapat dikompilasi.
- Sertakan instruksi build dan library yang dibutuhkan.

Mulai dengan memeriksa seluruh source code dan header terkait sebelum melakukan perubahan.