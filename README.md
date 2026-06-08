<div align="center">
  <img src="https://img.icons8.com/color/96/000000/internet-of-things.png" alt="IoT Logo"/>
  <h1>🌟 Smart Logistics Container Monitoring 🌟</h1>
  <p><strong>Sistem Pemantauan Cerdas Kontainer Ekspedisi Berbasis Internet of Things (IoT)</strong></p>
  
  <p>
    <img src="https://img.shields.io/badge/Framework-Next.js_14-black?style=for-the-badge&logo=next.js" alt="Next.js" />
    <img src="https://img.shields.io/badge/Hardware-ESP32_WROOM-E53935?style=for-the-badge&logo=espressif" alt="ESP32" />
    <img src="https://img.shields.io/badge/Database-Firebase_RTDB-FFCA28?style=for-the-badge&logo=firebase" alt="Firebase" />
    <img src="https://img.shields.io/badge/UI-Glassmorphism-00E5FF?style=for-the-badge" alt="UI" />
    <img src="https://img.shields.io/badge/Code-C++_&_JS-00599C?style=for-the-badge&logo=c%2B%2B" alt="Code" />
  </p>
</div>

---

## 📖 Tentang Proyek Ini
Di era logistik modern, menjaga kualitas barang di dalam kontainer kargo adalah prioritas utama. Proyek **Smart Logistics Container Monitoring** adalah solusi komprehensif dua arah yang menggabungkan perangkat keras (*hardware*) mikrokontroler dengan perangkat lunak (*software*) website berkelas premium.

Sistem ini tidak hanya berfungsi sebagai "kamera pemantau" suhu, tetapi bertindak sebagai **Otak Otomatis (Smart Logic)** yang mampu mengambil tindakan saat terjadi anomali (suhu ekstrem atau pintu dibongkar), serta memberikan kendali penuh kepada admin dari jarak jauh!

---

## 🏗️ Arsitektur Sistem & Topologi

Sistem ini berdiri di atas 3 pilar utama yang saling terhubung secara *real-time*:

1. **Titik Sensor (ESP32)**: Berada di dalam kontainer. Membaca kondisi fisik setiap detik dan mengeksekusi perintah dari server.
2. **Jembatan Awan (Firebase)**: Bertindak sebagai peladen data ultra-cepat (*Realtime Database*).
3. **Pusat Komando (Web Dashboard)**: Antarmuka *Glassmorphism* super modern yang digunakan oleh manusia untuk membaca data grafik dan menekan tombol kendali.

---

## 🔥 Fitur Super (Smart Logic)

Bukan sekadar sistem monitoring biasa, proyek ini dilengkapi dengan algoritma cerdas:
- **🎛️ Algoritma Hysteresis**: Kipas tidak akan menyala-mati secara berulang-ulang (*spamming*) saat suhu berada di ambang batas. Sistem memiliki *cooldown* cerdas.
- **💓 Detak Jantung (Heartbeat Indicator)**: ESP32 mengirim sinyal "Saya Hidup" setiap saat. Jika tidak ada kabar selama 2 menit, website akan mendeteksi status kontainer menjadi **OFFLINE**.
- **🧠 Sistem Hibrida (Auto & Manual)**: Kipas dapat menyala otomatis saat kepanasan, TETAPI admin jarak jauh tetap bisa menyalakannya secara paksa lewat website.
- **🚨 Deteksi Pembobolan (Smart Alarm)**: Jika pintu dibuka paksa, *buzzer* peringatan akan berbunyi di kontainer, dan website akan menampilkan peringatan bahaya yang di-log ke dalam sistem!

---

## 📂 Struktur Direktori Proyek

```text
📦 iot-monitoring
 ┣ 📂 monitoring_final      ➜ (Bagian Hardware/C++)
 ┃ ┗ 📜 monitoring_final.ino    # Kode ESP32 Utama (Logika Sensor & Aktuator)
 ┣ 📂 web                   ➜ (Bagian Software/Next.js)
 ┃ ┣ 📂 src
 ┃ ┃ ┣ 📂 app               # Halaman Dashboard, Riwayat, Login (App Router)
 ┃ ┃ ┣ 📂 components        # Komponen UI (Sidebar, Layout, Auth)
 ┃ ┃ ┗ 📂 lib               # Konfigurasi koneksi Firebase
 ┃ ┣ 📜 next.config.mjs     # Konfigurasi Turbopack Next.js
 ┃ ┣ 📜 package.json        # Dependencies (Chart.js, Firebase, React)
 ┃ ┗ 📜 globals.css         # Gaya Desain Premium (Dark Mode & Glassmorphism)
 ┗ 📜 README.md             # Dokumentasi Proyek Ini
```

---

## 🔌 Spesifikasi Hardware & Panduan Kabel (Wiring)

Berikut adalah skema perakitan untuk bagian kontainer (ESP32):

| Komponen (Hardware) | Fungsi Utama | Pin ESP32 |
| :--- | :--- | :--- |
| **DHT22** | Sensor pembaca Suhu & Kelembapan berakurasi tinggi | `Pin 4` |
| **Buzzer** | Sirine peringatan pembobolan | `Pin 25` |
| **Reed Switch** | Sensor magnetik pembaca status Pintu | `Pin 34` |
| **Relay 1 (Kipas)** | Aktuator pendingin otomatis / manual | `Pin 27` |
| **Relay 2 (Solenoid)** | Kunci pintu elektrik | `Pin 26` |
| **OLED SSD1306** | Layar indikator lokal di luar kontainer | `SDA: 21, SCL: 22` |

*(Catatan: ESP32 harus selalu terhubung ke jaringan WiFi atau Hotspot portabel di dalam kendaraan untuk mengirim data)*.

---

## 🚀 Panduan Instalasi (Getting Started)

Ingin mencoba menjalankan sistem ini di komputer Anda? Ikuti langkah mudah berikut:

### 1️⃣ Konfigurasi Web Dashboard
1. Pastikan Anda telah menginstal [Node.js](https://nodejs.org/).
2. Buka Terminal, arahkan ke folder web: `cd web`
3. Ketik perintah: `npm install`
4. Buat file bernama `.env` di dalam folder `web`, lalu isi kuncinya:
   ```env
   NEXT_PUBLIC_FIREBASE_API_KEY="xxx"
   NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN="xxx"
   NEXT_PUBLIC_FIREBASE_PROJECT_ID="xxx"
   NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET="xxx"
   NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID="xxx"
   NEXT_PUBLIC_FIREBASE_APP_ID="xxx"
   NEXT_PUBLIC_FIREBASE_DATABASE_URL="xxx"
   ```
5. Ketik `npm run dev` dan buka `http://localhost:3000` di *browser*.

### 2️⃣ Konfigurasi ESP32 (Arduino IDE)
1. Buka file `monitoring_final/monitoring_final.ino` menggunakan **Arduino IDE**.
2. Pastikan Anda sudah menginstal *library* yang diperlukan:
   - `Firebase ESP32 Client` oleh Mobizt
   - `DHT sensor library` oleh Adafruit
   - `Adafruit SSD1306` & `RTClib`
3. Ubah nama WiFi (`WIFI_SSID`) dan kata sandinya (`WIFI_PASSWORD`) pada baris ke-26 sesuai dengan jaringan Anda.
4. Hubungkan ESP32 ke laptop, lalu klik tombol **Upload**.

---

## 🌍 Siap Meluncur ke Publik (Deployment)

Karena aplikasi Web ini dikembangkan menggunakan **Next.js**, cara paling elegan untuk meng-*online*-kannya adalah melalui **[Vercel](https://vercel.com/)**.
- Masukkan *repository* Anda ke **GitHub**.
- Impor ke Vercel dan pastikan Anda mengisi **Root Directory** ke opsi `web`.
- Masukkan semua variabel rahasia (`.env`) Anda ke panel *Environment Variables* di pengaturan Vercel.
- Klik Deploy. *Voila!* Puskom (Pusat Komando) Anda kini online 24/7 di internet!

---

<div align="center">
  <p>Dikembangkan dengan ❤️ untuk masa depan logistik pintar.</p>
</div>
