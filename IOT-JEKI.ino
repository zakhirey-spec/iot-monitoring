// ==========================================
// 🏗️ SISTEM MONITORING KONTAINER LOGISTIK
// ESP32 + Firebase Realtime Database
// ==========================================
// Migrasi dari Blynk ke Firebase
// Library: FirebaseClient by Mobizt (terbaru)
// ==========================================

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <time.h>

// ==========================================
// 🔐 KONFIGURASI FIREBASE
// ==========================================
// GANTI dengan API Key dari Firebase Console → Project Settings
#define API_KEY "AIzaSyAdaq-v_4r3HeY4_lIiaN_J8FVsXgXzVTg"

// Database URL (sudah diketahui dari project Anda, HAPUS "https://")
#define DATABASE_URL "iot-monitoring-1d95d-default-rtdb.asia-southeast1.firebasedatabase.app"

// ==========================================
// 📶 KONFIGURASI WIFI
// ==========================================
#define WIFI_SSID "YANTO GAMING"
#define WIFI_PASSWORD "makesomenoise"

// ==========================================
// 📌 PIN DEFINITIONS (TIDAK BERUBAH)
// ==========================================
#define DHTPIN          4
#define DHTTYPE         DHT22
#define REED_PIN        34
#define BUZZER_PIN      25
#define RELAY_KIPAS     26
#define RELAY_SOLENOID  27

// ==========================================
// 📺 OLED CONFIGURATION
// ==========================================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// ==========================================
// ⚙️ THRESHOLD & TIMING
// ==========================================
#define SUHU_MAX          30.0
#define KELEMBAPAN_MAX    80.0
#define BUZZER_DURATION   3000    // 3 detik
#define INTERVAL_KIRIM    15000   // 15 detik kirim data
#define INTERVAL_SENSOR   2000    // 2 detik baca sensor
#define INTERVAL_PINTU    300     // 300ms cek pintu
#define INTERVAL_OLED     1000    // 1 detik update OLED
#define INTERVAL_HEARTBEAT 60000  // 1 menit heartbeat

// ==========================================
// 🔧 NTP CONFIGURATION
// ==========================================
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 25200  // GMT+7 (WIB) dalam detik
#define DST_OFFSET 0

// ==========================================
// 📦 OBJECTS
// ==========================================
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;
bool rtcValid = false;

// Firebase Objects
FirebaseData fbdo;        // Untuk kirim data rutin
FirebaseData fbdoStream;  // KHUSUS untuk menerima data (stream)
FirebaseAuth auth;
FirebaseConfig config;

// ==========================================
// 📊 STATUS SISTEM
// ==========================================
float suhu = 0, kelembapan = 0;
bool statusPintu = false;
bool manualKipas = false;
bool manualSolenoid = false;

// Alarm Flags
bool alarmSuhuAktif = false;
bool alarmHumidAktif = false;
bool alarmPintuAktif = false;
bool alarmSuhuSent = false;   // Mencegah spam alarm
bool alarmHumidSent = false;
bool alarmPintuSent = false;

// Buzzer State
bool buzzerMuted = false;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;

// Timing (non-blocking)
unsigned long lastSensorRead = 0;
unsigned long lastDataSend = 0;
unsigned long lastPintuCheck = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastHeartbeat = 0;

// Firebase state
bool firebaseReady = false;
bool streamStarted = false;
bool wifiConnected = false;

// Statistik koneksi
unsigned long dataSuccessCount = 0;
unsigned long dataFailCount = 0;

// ==========================================
// 📶 FUNGSI KONEKSI WIFI
// ==========================================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }

  Serial.println("📶 Menghubungkan ke WiFi...");
  WiFi.mode(WIFI_STA);
  // Aktifkan auto reconnect bawaan ESP32
  WiFi.setAutoReconnect(true); 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.print("✅ WiFi terhubung! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("❌ Gagal terhubung ke WiFi!");
  }
}

// ==========================================
// 🔥 INISIALISASI FIREBASE
// ==========================================
void initFirebase() {
  Serial.println("🔥 Menginisialisasi Firebase...");
  
  // Konfigurasi Database URL dan API Key
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign-up sebagai anonymous atau guest
  config.signer.test_mode = true; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  firebaseReady = true;
  Serial.println("✅ Firebase siap!");
}

// ==========================================
// 📡 STREAM CALLBACK (Menerima kontrol dari web)
// ==========================================
void streamCallback(StreamData data) {
  Serial.printf("📡 Stream event: path=%s, type=%s\n", data.dataPath().c_str(), data.dataType().c_str());

  // Handle jika data yang datang adalah JSON (update keseluruhan)
  if (data.dataType() == "json") {
    FirebaseJson *json = data.jsonObjectPtr();
    if (!json) return;

    FirebaseJsonData result;
    if (json->get(result, "kipas")) {
      int nilai = result.intValue;
      manualKipas = (nilai == 1);
      if (manualKipas) {
        digitalWrite(RELAY_KIPAS, HIGH);
        Serial.println("🌀 Kipas: MANUAL ON (dari web)");
      } else {
        if (suhu <= SUHU_MAX && kelembapan <= KELEMBAPAN_MAX) {
          digitalWrite(RELAY_KIPAS, LOW);
          Serial.println("🌀 Kipas: MATI (dari web)");
        } else {
          Serial.println("🌀 Kipas: tetap AUTO (kondisi melebihi batas)");
        }
      }
    }

    if (json->get(result, "solenoid")) {
      int nilai = result.intValue;
      manualSolenoid = (nilai == 1);
      digitalWrite(RELAY_SOLENOID, nilai ? HIGH : LOW);
      Serial.printf("🔒 Solenoid: %s (dari web)\n", manualSolenoid ? "TERBUKA" : "TERKUNCI");
    }

    if (json->get(result, "buzzerMute")) {
      int nilai = result.intValue;
      buzzerMuted = (nilai == 1);
      if (buzzerMuted) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
        Serial.println("🔕 Buzzer: MUTE AKTIF (dari web)");
      } else {
        Serial.println("🔊 Buzzer: MUTE DINONAKTIFKAN (dari web)");
      }
    }
  } 
  // Handle jika data yang datang adalah update satuan per field (contoh: /kipas = 1)
  else if (data.dataType() == "int" || data.dataType() == "boolean" || data.dataType() == "double") {
    String path = data.dataPath();
    int nilai = data.intData();

    if (path == "/kipas") {
      manualKipas = (nilai == 1);
      if (manualKipas) {
        digitalWrite(RELAY_KIPAS, HIGH);
        Serial.println("🌀 Kipas: MANUAL ON (dari web)");
      } else {
        if (suhu <= SUHU_MAX && kelembapan <= KELEMBAPAN_MAX) {
          digitalWrite(RELAY_KIPAS, LOW);
          Serial.println("🌀 Kipas: MATI (dari web)");
        } else {
          Serial.println("🌀 Kipas: tetap AUTO (kondisi melebihi batas)");
        }
      }
    } else if (path == "/solenoid") {
      manualSolenoid = (nilai == 1);
      digitalWrite(RELAY_SOLENOID, nilai ? HIGH : LOW);
      Serial.printf("🔒 Solenoid: %s (dari web)\n", manualSolenoid ? "TERBUKA" : "TERKUNCI");
    } else if (path == "/buzzerMute") {
      buzzerMuted = (nilai == 1);
      if (buzzerMuted) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
        Serial.println("🔕 Buzzer: MUTE AKTIF (dari web)");
      } else {
        Serial.println("🔊 Buzzer: MUTE DINONAKTIFKAN (dari web)");
      }
    }
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("⚠️ Firebase stream timeout!");
  }
}

// ==========================================
// 📡 START STREAM (Listen untuk kontrol)
// ==========================================
void startStream() {
  if (!firebaseReady || streamStarted) return;

  Serial.println("📡 Memulai stream kontrol...");
  Firebase.beginStream(fbdoStream, "/kontrol");
  Firebase.setStreamCallback(fbdoStream, streamCallback, streamTimeoutCallback);
  streamStarted = true;
  Serial.println("✅ Stream kontrol aktif pada /kontrol");
}

// ==========================================
// 🔊 PROCESS BUZZER (Unified Logic)
// ==========================================
void processBuzzer() {
  bool adaAlarm = alarmSuhuAktif || alarmHumidAktif || alarmPintuAktif;

  // PRIORITAS 1: Jika MUTE aktif → Paksa BUZZER MATI total
  if (buzzerMuted) {
    if (buzzerActive) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerActive = false;
    }
    return;
  }

  // PRIORITAS 2: Ada alarm & buzzer belum nyala → NYALAKAN
  if (adaAlarm && !buzzerActive) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerActive = true;
    buzzerStartTime = millis();
    Serial.println("🔔 BUZZER: ON (Alarm terdeteksi)");
  }

  // PRIORITAS 3: Timeout 3 detik → MATIKAN
  if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
    Serial.println("🔇 BUZZER: OFF (Timeout 3 detik)");
  }

  // PRIORITAS 4: Alarm hilang → MATIKAN
  if (!adaAlarm && buzzerActive) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}

// ==========================================
// 🌡️ FUNGSI BACA SENSOR DHT22
// ==========================================
void bacaSensor() {
  float bacaSuhu = NAN;
  float bacaHumid = NAN;
  int retries = 0;

  while (retries < 3 && (isnan(bacaSuhu) || isnan(bacaHumid))) {
    bacaSuhu = dht.readTemperature();
    bacaHumid = dht.readHumidity();
    if (!isnan(bacaSuhu) && !isnan(bacaHumid)) break;
    retries++;
    delay(300);
  }

  if (isnan(bacaSuhu) || isnan(bacaHumid)) {
    Serial.println("⚠️ ERROR: Gagal baca DHT22 setelah retry");
    return;
  }

  suhu = bacaSuhu;
  kelembapan = bacaHumid;

  // Kontrol Kipas Otomatis
  if (!manualKipas) {
    if (suhu > SUHU_MAX || kelembapan > KELEMBAPAN_MAX) {
      digitalWrite(RELAY_KIPAS, HIGH);
    } else {
      digitalWrite(RELAY_KIPAS, LOW);
    }
  }

  // Set Flag Alarm Suhu (Hysteresis 1 derajat)
  if (suhu >= SUHU_MAX) {
    alarmSuhuAktif = true;
  } else if (suhu < SUHU_MAX - 1.0) {
    alarmSuhuAktif = false;
    alarmSuhuSent = false;
  }

  // Set Flag Alarm Kelembapan (Hysteresis 2 persen)
  if (kelembapan >= KELEMBAPAN_MAX) {
    alarmHumidAktif = true;
  } else if (kelembapan < KELEMBAPAN_MAX - 2.0) {
    alarmHumidAktif = false;
    alarmHumidSent = false;
  }
}

// ==========================================
// 🚪 FUNGSI CEK PINTU (Reed Switch)
// ==========================================
void cekPintu() {
  bool bacaPintu = digitalRead(REED_PIN);

  if (bacaPintu == HIGH && !statusPintu) {
    statusPintu = true;
    alarmPintuAktif = true;
    Serial.println("🚪 PINTU: TERBUKA");
  }
  else if (bacaPintu == LOW && statusPintu) {
    statusPintu = false;
    alarmPintuAktif = false;
    alarmPintuSent = false;
    Serial.println("🚪 PINTU: TERTUTUP");
  }
}

// ==========================================
// 📤 KIRIM DATA KE FIREBASE
// ==========================================
void kirimDataFirebase() {
  if (!firebaseReady || !wifiConnected) return;

  Serial.println("📤 Mengirim data ke Firebase...");

  unsigned long timestamp = 0;
  int yr = 2000, mo = 1, dy = 1, hr = 0, mn = 0, sc = 0;
  time_t now_t = time(nullptr);
  
  if (now_t > 1609459200) { // Jika NTP valid (> tahun 2021)
    timestamp = (unsigned long)now_t;
    struct tm* timeinfo = localtime(&now_t);
    yr = timeinfo->tm_year + 1900;
    mo = timeinfo->tm_mon + 1;
    dy = timeinfo->tm_mday;
    hr = timeinfo->tm_hour;
    mn = timeinfo->tm_min;
    sc = timeinfo->tm_sec;
  } else if (rtcValid) { // Jika NTP mati tapi RTC hidup
    DateTime now = rtc.now();
    timestamp = now.unixtime();
    yr = now.year(); mo = now.month(); dy = now.day();
    hr = now.hour(); mn = now.minute(); sc = now.second();
  } else {
    timestamp = millis() / 1000;
  }

  char tanggal[11];
  snprintf(tanggal, sizeof(tanggal), "%04d-%02d-%02d", yr, mo, dy);

  char waktu[9];
  snprintf(waktu, sizeof(waktu), "%02d:%02d:%02d", hr, mn, sc);

  FirebaseJson rtJson;
  rtJson.set("suhu", isnan(suhu) ? 0 : suhu);
  rtJson.set("kelembapan", isnan(kelembapan) ? 0 : kelembapan);
  rtJson.set("pintu", statusPintu);
  rtJson.set("kipas", digitalRead(RELAY_KIPAS) == HIGH);
  rtJson.set("solenoid", digitalRead(RELAY_SOLENOID) == HIGH);
  rtJson.set("buzzerMuted", buzzerMuted);
  rtJson.set("buzzerActive", buzzerActive);
  rtJson.set("manualKipas", manualKipas);
  rtJson.set("timestamp", (double)timestamp);
  rtJson.set("tanggal", tanggal);
  rtJson.set("waktu", waktu);
  rtJson.set("alarmSuhu", alarmSuhuAktif);
  rtJson.set("alarmKelembapan", alarmHumidAktif);
  rtJson.set("alarmPintu", alarmPintuAktif);
  rtJson.set("rssi", WiFi.RSSI());
  rtJson.set("uptime", (double)(millis() / 1000));

  bool setOk = Firebase.setJSON(fbdo, "/realtime", rtJson);
  if (setOk) {
    dataSuccessCount++;
    Serial.println("  ✅ Realtime data terkirim");
  } else {
    dataFailCount++;
    Serial.print("  ❌ Gagal kirim realtime data: ");
    Serial.println(fbdo.errorReason());
  }

  // ---- 2. Push data historis (append) ----
  FirebaseJson logJsonObj;
  logJsonObj.set("suhu", isnan(suhu) ? 0 : suhu);
  logJsonObj.set("kelembapan", isnan(kelembapan) ? 0 : kelembapan);
  logJsonObj.set("pintu", statusPintu);
  logJsonObj.set("kipas", digitalRead(RELAY_KIPAS) == HIGH);
  logJsonObj.set("timestamp", (double)timestamp);
  logJsonObj.set("tanggal", tanggal);
  logJsonObj.set("waktu", waktu);

  bool pushOk = Firebase.pushJSON(fbdo, "/log", logJsonObj);
  if (pushOk) {
    Serial.println("  ✅ Log historis tersimpan");
  } else {
    Serial.print("  ❌ Gagal simpan log historis: ");
    Serial.println(fbdo.errorReason());
  }

  // ---- 3. Kirim alarm jika ada (hanya sekali per event) ----
  if (alarmSuhuAktif && !alarmSuhuSent) {
    FirebaseJson alarmObj;
    alarmObj.set("tipe", "suhu_tinggi");
    alarmObj.set("pesan", "PERINGATAN! Suhu: " + String(suhu, 1) + "°C");
    alarmObj.set("nilai", isnan(suhu) ? 0 : suhu);
    alarmObj.set("batas", (double)SUHU_MAX);
    alarmObj.set("timestamp", (double)timestamp);
    alarmObj.set("dibaca", false);
    
    Firebase.pushJSON(fbdo, "/alarm", alarmObj);
    alarmSuhuSent = true;
    Serial.println("  🚨 Alarm suhu tinggi terkirim!");
  }

  if (alarmHumidAktif && !alarmHumidSent) {
    FirebaseJson alarmObj2;
    alarmObj2.set("tipe", "kelembapan_tinggi");
    alarmObj2.set("pesan", "PERINGATAN! Kelembapan: " + String(kelembapan, 1) + "%");
    alarmObj2.set("nilai", isnan(kelembapan) ? 0 : kelembapan);
    alarmObj2.set("batas", (double)KELEMBAPAN_MAX);
    alarmObj2.set("timestamp", (double)timestamp);
    alarmObj2.set("dibaca", false);
    
    Firebase.pushJSON(fbdo, "/alarm", alarmObj2);
    alarmHumidSent = true;
    Serial.println("  🚨 Alarm kelembapan tinggi terkirim!");
  }

  if (alarmPintuAktif && !alarmPintuSent) {
    FirebaseJson alarmObj3;
    alarmObj3.set("tipe", "pintu_terbuka");
    alarmObj3.set("pesan", "PERINGATAN! Pintu kontainer terbuka!");
    alarmObj3.set("timestamp", (double)timestamp);
    alarmObj3.set("dibaca", false);
    
    Firebase.pushJSON(fbdo, "/alarm", alarmObj3);
    alarmPintuSent = true;
    Serial.println("  🚨 Alarm pintu terbuka terkirim!");
  }

  Serial.printf("📊 Statistik: %lu sukses, %lu gagal\n", dataSuccessCount, dataFailCount);
}

// ==========================================
// 💓 HEARTBEAT (Cek koneksi & status)
// ==========================================
void sendHeartbeat() {
  if (!firebaseReady || !wifiConnected) return;

  unsigned long timestamp = 0;
  time_t t = time(nullptr);
  if (t > 1609459200) {
    timestamp = (unsigned long)t;
  } else if (rtcValid) {
    timestamp = rtc.now().unixtime();
  } else {
    timestamp = millis() / 1000;
  }

  FirebaseJson hbJson;
  hbJson.set("online", true);
  hbJson.set("rssi", WiFi.RSSI());
  hbJson.set("uptime", (double)(millis() / 1000));
  hbJson.set("freeHeap", (double)ESP.getFreeHeap());
  hbJson.set("ip", WiFi.localIP().toString());
  hbJson.set("timestamp", (double)timestamp);

  Firebase.setJSON(fbdo, "/status", hbJson);
}

// ==========================================
// 📺 FUNGSI UPDATE OLED
// ==========================================
void updateOLED() {
  int yr = 2000, mo = 1, dy = 1, hr = 0, mn = 0, sc = 0;
  time_t now_t = time(nullptr);
  if (now_t > 1609459200) {
    struct tm* timeinfo = localtime(&now_t);
    yr = timeinfo->tm_year + 1900;
    mo = timeinfo->tm_mon + 1;
    dy = timeinfo->tm_mday;
    hr = timeinfo->tm_hour;
    mn = timeinfo->tm_min;
    sc = timeinfo->tm_sec;
  } else if (rtcValid) {
    DateTime now = rtc.now();
    yr = now.year(); mo = now.month(); dy = now.day();
    hr = now.hour(); mn = now.minute(); sc = now.second();
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Baris 1: Waktu & Tanggal
  display.setCursor(0, 0);
  display.printf("%02d:%02d:%02d %02d/%02d/%04d", hr, mn, sc, dy, mo, yr);

  // Baris 2: Suhu
  display.setCursor(0, 14);
  display.print("Suhu    : ");
  display.print(suhu, 1);
  display.print(" C");
  if (suhu > SUHU_MAX) display.print(" *");

  // Baris 3: Kelembapan
  display.setCursor(0, 26);
  display.print("Humidity: ");
  display.print(kelembapan, 1);
  display.print(" %");
  if (kelembapan > KELEMBAPAN_MAX) display.print(" *");

  // Baris 4: Status Pintu
  display.setCursor(0, 38);
  display.print("Pintu   : ");
  if (statusPintu) {
    display.setTextColor(SSD1306_BLACK);
    display.fillRect(58, 38, 50, 8, SSD1306_WHITE);
    display.setCursor(58, 38);
    display.print("BUKA!");
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.print("TERTUTUP");
  }

  // Baris 5: Status Buzzer + WiFi
  display.setCursor(0, 50);
  if (buzzerMuted) {
    display.print("Bzr:MUTE");
  } else if (buzzerActive) {
    display.print("Bzr:AKTF");
  } else {
    display.print("Bzr:Siap");
  }

  // WiFi indicator di kanan bawah
  display.setCursor(72, 50);
  if (wifiConnected) {
    display.print("WiFi:");
    display.print(WiFi.RSSI());
  } else {
    display.print("WiFi:OFF");
  }

  display.display();
}

// ==========================================
// ⚙️ SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("==========================================");
  Serial.println("  🏗️ SISTEM MONITORING KONTAINER v2.0");
  Serial.println("  Firebase Realtime Database Edition");
  Serial.println("==========================================");

  // ---- Setup Pin ----
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_KIPAS, OUTPUT);
  pinMode(RELAY_SOLENOID, OUTPUT);

  digitalWrite(RELAY_KIPAS, LOW);
  digitalWrite(RELAY_SOLENOID, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // ---- Setup DHT22 ----
  dht.begin();

  // ---- Setup I2C & OLED ----
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ ERROR: OLED tidak ditemukan!");
    // Lanjut tanpa OLED (jangan hang)
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.println("  Monitoring v2.0");
    display.setCursor(0, 25);
    display.println("  Firebase Edition");
    display.setCursor(0, 45);
    display.println("  Menghubungkan...");
    display.display();
    Serial.println("✅ OLED aktif");
  }

  // ---- Setup RTC ----
  if (!rtc.begin()) {
    Serial.println("⚠️ RTC tidak ditemukan! Menggunakan NTP.");
    rtcValid = false;
  } else {
    Serial.println("✅ RTC DS3231 aktif");
    rtcValid = true;
  }

  // ---- Koneksi WiFi ----
  connectWiFi();

  // ---- Setup NTP ----
  if (wifiConnected) {
    configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
    Serial.print("✅ NTP time sync dimulai");

    time_t now = time(nullptr);
    int tries = 0;
    while (now < 1609459200 && tries < 30) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
      tries++;
    }

    if (now >= 1609459200) {
      Serial.println("\n✅ NTP time sync berhasil");
    } else {
      Serial.println("\n⚠️ NTP time sync gagal, menggunakan RTC jika tersedia");
    }
  }

  // ---- Inisialisasi Firebase ----
  if (wifiConnected) {
    initFirebase();
    startStream();

    // Kirim status awal
    sendHeartbeat();

    // Set konfigurasi default di Firebase
    FirebaseJson confJson;
    confJson.set("suhuMax", (double)SUHU_MAX);
    confJson.set("kelembapanMax", (double)KELEMBAPAN_MAX);
    confJson.set("intervalKirim", (double)(INTERVAL_KIRIM / 1000));
    Firebase.setJSON(fbdo, "/config", confJson);
  }

  Serial.println("==========================================");
  Serial.println("  ✅ SISTEM SIAP!");
  Serial.println("==========================================");
}

// ==========================================
// 🔁 LOOP
// ==========================================
void loop() {
  unsigned long currentMillis = millis();

  // ---- Firebase maintenance ----
  Firebase.reconnectNetwork(true);

  // ---- Check WiFi connection ----
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      wifiConnected = false;
      Serial.println("⚠️ WiFi terputus! ESP32 akan auto-reconnect...");
    }
  } else if (!wifiConnected) {
    wifiConnected = true;
    Serial.println("✅ WiFi terhubung kembali!");
    // Stream Firebase seringkali perlu dipancing ulang jika koneksi putus cukup lama
    if (firebaseReady) {
      streamStarted = false; // Reset flag
      startStream();
    }
  }

  // ---- Baca Sensor (setiap 2 detik) ----
  if (currentMillis - lastSensorRead >= INTERVAL_SENSOR) {
    lastSensorRead = currentMillis;
    bacaSensor();
  }

  // ---- Cek Pintu (setiap 300ms) ----
  if (currentMillis - lastPintuCheck >= INTERVAL_PINTU) {
    lastPintuCheck = currentMillis;
    cekPintu();
  }

  // ---- Kirim Data ke Firebase (setiap 15 detik) ----
  if (currentMillis - lastDataSend >= INTERVAL_KIRIM) {
    lastDataSend = currentMillis;
    kirimDataFirebase();
  }

  // ---- Update OLED (setiap 1 detik) ----
  if (currentMillis - lastOLEDUpdate >= INTERVAL_OLED) {
    lastOLEDUpdate = currentMillis;
    updateOLED();
  }

  // ---- Heartbeat (setiap 1 menit) ----
  if (currentMillis - lastHeartbeat >= INTERVAL_HEARTBEAT) {
    lastHeartbeat = currentMillis;
    sendHeartbeat();
  }

  // ---- Process Buzzer (setiap loop, non-blocking) ----
  processBuzzer();
}
