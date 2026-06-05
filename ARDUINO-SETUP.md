# Arduino Compile Guide for `monitoring.ino`

Panduan ini menjelaskan langkah lengkap untuk mengompilasi dan mengunggah sketch `monitoring.ino` pada board ESP32.

## 1. Target board

Rekomendasi board:
- `ESP32 Dev Module`
- Jika Anda memakai modul spesifik: pilih `ESP32 Wrover Module` atau `ESP32 Pico Kit` sesuai hardware.

### Pengaturan board di Arduino IDE
- Board: `ESP32 Dev Module`
- Flash Frequency: `80 MHz`
- CPU Frequency: `240 MHz`
- Upload Speed: `115200`
- Flash Mode: `DIO`
- Partition Scheme: `Default 4MB with spiffs`
- PSRAM: `Disabled` (kecuali papan Anda mendukung PSRAM)

## 2. Board package ESP32

Instalasi board ESP32:
1. Buka `File` → `Preferences`
2. Tambahkan URL berikut ke `Additional Boards Manager URLs`:
   - `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Buka `Tools` → `Board` → `Boards Manager...`
4. Cari `esp32` dan install versi:
   - **`2.0.11`** atau **`2.0.12`**

> Versi ini stabil untuk sebagian besar library ESP32 dan mendukung `WiFiClientSecure`, `Wire`, serta `time.h`.

## 3. Library yang diperlukan

Gunakan Arduino Library Manager untuk menginstal versi-versi ini:

- `Firebase ESP32 Client` oleh **Mobizt**
  - Rekomendasi versi: **`3.1.0`**
  - Jika tidak ada, gunakan versi stabil sekitar **`3.0.3`** atau **`3.0.2`**
- `DHT sensor library` oleh **Adafruit**
  - Rekomendasi versi: **`1.4.3`**
- `Adafruit GFX Library` oleh **Adafruit**
  - Rekomendasi versi: **`1.11.5`**
- `Adafruit SSD1306` oleh **Adafruit**
  - Rekomendasi versi: **`2.5.7`**
- `RTClib` oleh **Adafruit**
  - Rekomendasi versi: **`1.13.0`**

### Catatan penting
- `WiFi.h`, `Wire.h`, `WiFiClientSecure.h`, dan `time.h` sudah disertakan oleh board ESP32, jadi tidak perlu diinstal secara terpisah.
- Pastikan header `FirebaseClient.h` tersedia dari library Mobizt yang Anda pilih.

## 4. Instalasi library melalui Arduino IDE

1. Buka Arduino IDE
2. Pilih menu `Sketch` → `Include Library` → `Manage Libraries...`
3. Cari setiap nama library di atas dan install versi yang direkomendasikan
4. Restart Arduino IDE setelah installasi selesai

## 5. Konfigurasi yang harus disesuaikan

Sebelum kompilasi, ganti nilai berikut di `monitoring.ino`:
- `WIFI_SSID` → nama WiFi Anda
- `WIFI_PASSWORD` → password WiFi Anda
- `API_KEY` → API Key Firebase Anda
- `DATABASE_URL` → URL Realtime Database Firebase Anda

Jangan masukkan kredensial publik ke repo jika kode akan dibagikan.

## 6. Struktur hardware / pin mapping

Sketch `monitoring.ino` menggunakan pin:
- `DHTPIN = 4`
- `REED_PIN = 34`
- `BUZZER_PIN = 25`
- `RELAY_KIPAS = 26`
- `RELAY_SOLENOID = 27`
- `Wire.begin(21, 22)` untuk I2C SDA/SCL
- OLED I2C address: `0x3C`

## 7. Jika menggunakan PlatformIO

Buat file `platformio.ini` di root project jika ingin build otomatis:

```ini
[env:esp32dev]
platform = espressif32@~4.4.0
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags =
  -D CORE_DEBUG_LEVEL=0

lib_deps =
  Mobizt/Firebase ESP32 Client@^3.1.0
  adafruit/DHT sensor library@^1.4.3
  adafruit/Adafruit GFX Library@^1.11.5
  adafruit/Adafruit SSD1306@^2.5.7
  adafruit/RTClib@^1.13.0

upload_speed = 115200

board_build.flash_mode = dio
board_build.partitions = default
```

## 8. Troubleshooting compile errors

- Jika error `FirebaseClient.h` tidak ditemukan:
  - Pastikan `Firebase ESP32 Client` benar terinstall
  - Alternatif: pilih versi `3.0.3` jika versi terbaru tidak cocok
- Jika error `Adafruit_SSD1306.h`:
  - Pastikan `Adafruit GFX Library` dan `Adafruit SSD1306` sudah terinstal
- Jika error `RTClib.h`:
  - Pastikan `RTClib` versi `1.13.0`
- Jika board tidak muncul:
  - Verifikasi installasi board package ESP32 dan restart Arduino IDE

## 9. Cara compile dan upload

1. Buka `monitoring.ino` di Arduino IDE
2. Pilih board: `Tools` → `Board` → `ESP32 Dev Module`
3. Pilih port yang benar pada `Tools` → `Port`
4. Tekan tombol `Verify` (checkmark) untuk compile
5. Tekan tombol `Upload` (panah kanan) untuk upload ke ESP32

## 10. Verifikasi setelah upload

- Buka `Tools` → `Serial Monitor`
- Set baud rate ke `115200`
- Reset board, lalu cek log startup
- Pastikan WiFi berhasil terhubung dan Firebase siap

---

Jika Anda ingin, saya juga dapat membuat file `platformio.ini` secara otomatis dengan dependensi yang sudah disesuaikan untuk project ini.