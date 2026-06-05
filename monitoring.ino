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
#define API_KEY "AIzaSyBgEWDpcrYS1vNwytdBDkNu-B8J3arMDRw"

// Database URL (sudah diketahui dari project Anda)
#define DATABASE_URL "https://iot-monitoring-fd31f-default-rtdb.asia-southeast1.firebasedatabase.app"

// ==========================================
// 📶 KONFIGURASI WIFI
// ==========================================
#define WIFI_SSID "Goblokloe"
#define WIFI_PASSWORD "ayamjoper"

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

// Firebase Objects
FirebaseData fbdo;
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
void streamCallback(FirebaseStream data) {
  if (!data.jsonObject()) return;

  FirebaseJson &json = *data.jsonObject();

  Serial.println("📡 Stream event received");

  if (json.isMember("kipas")) {
    int nilai = json.getInt("kipas");
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

  if (json.isMember("solenoid")) {
    int nilai = json.getInt("solenoid");
    manualSolenoid = (nilai == 1);
    digitalWrite(RELAY_SOLENOID, nilai ? HIGH : LOW);
    Serial.printf("🔒 Solenoid: %s (dari web)\n", manualSolenoid ? "TERBUKA" : "TERKUNCI");
  }

  if (json.isMember("buzzerMute")) {
    int nilai = json.getInt("buzzerMute");
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
  Firebase.beginStream(fbdo, "/kontrol");
  fbdo.setStreamCallback(streamCallback, streamTimeoutCallback);
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

  // Set Flag Alarm Suhu
  if (suhu > SUHU_MAX) {
    alarmSuhuAktif = true;
  } else {
    alarmSuhuAktif = false;
    alarmSuhuSent = false;
  }

  // Set Flag Alarm Kelembapan
  if (kelembapan > KELEMBAPAN_MAX) {
    alarmHumidAktif = true;
  } else {
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
  DateTime now = rtc.now();
  timestamp = now.unixtime();

  if (timestamp < 1000000) {
    time_t t;
    time(&t);
    timestamp = (unsigned long)t;
  }

  char tanggal[11];
  snprintf(tanggal, sizeof(tanggal), "%04d-%02d-%02d", now.year(), now.month(), now.day());

  char waktu[9];
  snprintf(waktu, sizeof(waktu), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  String realtimeJson;
  realtimeJson.reserve(512);
  realtimeJson += "{";
  realtimeJson += "\"suhu\":" + String(suhu, 1) + ",";
  realtimeJson += "\"kelembapan\":" + String(kelembapan, 1) + ",";
  realtimeJson += "\"pintu\":" + String(statusPintu ? "true" : "false") + ",";
  realtimeJson += "\"kipas\":" + String(digitalRead(RELAY_KIPAS) == HIGH ? "true" : "false") + ",";
  realtimeJson += "\"solenoid\":" + String(digitalRead(RELAY_SOLENOID) == HIGH ? "true" : "false") + ",";
  realtimeJson += "\"buzzerMuted\":" + String(buzzerMuted ? "true" : "false") + ",";
  realtimeJson += "\"buzzerActive\":" + String(buzzerActive ? "true" : "false") + ",";
  realtimeJson += "\"manualKipas\":" + String(manualKipas ? "true" : "false") + ",";
  realtimeJson += "\"timestamp\":" + String(timestamp) + ",";
  realtimeJson += "\"tanggal\":\"" + String(tanggal) + "\",";
  realtimeJson += "\"waktu\":\"" + String(waktu) + "\",";
  realtimeJson += "\"alarmSuhu\":" + String(alarmSuhuAktif ? "true" : "false") + ",";
  realtimeJson += "\"alarmKelembapan\":" + String(alarmHumidAktif ? "true" : "false") + ",";
  realtimeJson += "\"alarmPintu\":" + String(alarmPintuAktif ? "true" : "false") + ",";
  realtimeJson += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  realtimeJson += "\"uptime\":" + String(millis() / 1000);
  realtimeJson += "}";

  bool setOk = Firebase.setJSON(fbdo, "/realtime", realtimeJson);
  if (setOk) {
    dataSuccessCount++;
    Serial.println("  ✅ Realtime data terkirim");
  } else {
    dataFailCount++;
    Serial.println("  ❌ Gagal kirim realtime data");
  }

  // ---- 2. Push data historis (append) ----
  String logJson = "{";
  logJson += "\"suhu\":" + String(suhu, 1) + ",";
  logJson += "\"kelembapan\":" + String(kelembapan, 1) + ",";
  logJson += "\"pintu\":" + String(statusPintu ? "true" : "false") + ",";
  logJson += "\"kipas\":" + String(digitalRead(RELAY_KIPAS) == HIGH ? "true" : "false") + ",";
  logJson += "\"timestamp\":" + String(timestamp) + ",";
  logJson += "\"tanggal\":\"" + String(tanggal) + "\",";
  logJson += "\"waktu\":\"" + String(waktu) + "\"";
  logJson += "}";

  String pushResult = Firebase.pushJSON(fbdo, "/log", logJson);
  if (fbdo.httpCode() == FIREBASE_HTTP_CODE_OK) {
    Serial.println("  ✅ Log historis tersimpan");
  } else {
    Serial.println("  ❌ Gagal simpan log historis");
  }

  // ---- 3. Kirim alarm jika ada (hanya sekali per event) ----
  if (alarmSuhuAktif && !alarmSuhuSent) {
    String alarmJson = "{";
    alarmJson += "\"tipe\":\"suhu_tinggi\",";
    alarmJson += "\"pesan\":\"PERINGATAN! Suhu: " + String(suhu, 1) + "°C\",";
    alarmJson += "\"nilai\":" + String(suhu, 1) + ",";
    alarmJson += "\"batas\":" + String(SUHU_MAX, 1) + ",";
    alarmJson += "\"timestamp\":" + String(timestamp) + ",";
    alarmJson += "\"dibaca\":false";
    alarmJson += "}";
    Firebase.pushJSON(fbdo, "/alarm", alarmJson);
    alarmSuhuSent = true;
    Serial.println("  🚨 Alarm suhu tinggi terkirim!");
  }

  if (alarmHumidAktif && !alarmHumidSent) {
    String alarmJson = "{";
    alarmJson += "\"tipe\":\"kelembapan_tinggi\",";
    alarmJson += "\"pesan\":\"PERINGATAN! Kelembapan: " + String(kelembapan, 1) + "%\",";
    alarmJson += "\"nilai\":" + String(kelembapan, 1) + ",";
    alarmJson += "\"batas\":" + String(KELEMBAPAN_MAX, 1) + ",";
    alarmJson += "\"timestamp\":" + String(timestamp) + ",";
    alarmJson += "\"dibaca\":false";
    alarmJson += "}";
    Firebase.pushJSON(fbdo, "/alarm", alarmJson);
    alarmHumidSent = true;
    Serial.println("  🚨 Alarm kelembapan tinggi terkirim!");
  }

  if (alarmPintuAktif && !alarmPintuSent) {
    String alarmJson = "{";
    alarmJson += "\"tipe\":\"pintu_terbuka\",";
    alarmJson += "\"pesan\":\"PERINGATAN! Pintu kontainer terbuka!\",";
    alarmJson += "\"timestamp\":" + String(timestamp) + ",";
    alarmJson += "\"dibaca\":false";
    alarmJson += "}";
    Firebase.pushJSON(fbdo, "/alarm", alarmJson);
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
  DateTime now = rtc.now();
  timestamp = now.unixtime();
  if (timestamp < 1000000) {
    time_t t;
    time(&t);
    timestamp = (unsigned long)t;
  }

  String heartbeatJson;
  heartbeatJson.reserve(256);
  heartbeatJson += "{";
  heartbeatJson += "\"online\":true,";
  heartbeatJson += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  heartbeatJson += "\"uptime\":" + String(millis() / 1000) + ",";
  heartbeatJson += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  heartbeatJson += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  heartbeatJson += "\"timestamp\":" + String(timestamp);
  heartbeatJson += "}";

  Firebase.setJSON(fbdo, "/status", heartbeatJson);
}

// ==========================================
// 📺 FUNGSI UPDATE OLED
// ==========================================
void updateOLED() {
  DateTime now = rtc.now();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Baris 1: Waktu & Tanggal
  display.setCursor(0, 0);
  display.printf("%02d:%02d:%02d %02d/%02d/%04d",
    now.hour(), now.minute(), now.second(),
    now.day(), now.month(), now.year());

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
  } else {
    Serial.println("✅ RTC DS3231 aktif");
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
    String configJson = "{";
    configJson += "\"suhuMax\":" + String(SUHU_MAX, 1) + ",";
    configJson += "\"kelembapanMax\":" + String(KELEMBAPAN_MAX, 1) + ",";
    configJson += "\"intervalKirim\":" + String(INTERVAL_KIRIM / 1000);
    configJson += "}";
    Firebase.setJSON(fbdo, "/config", configJson);
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
      Serial.println("⚠️ WiFi terputus! Mencoba reconnect...");
    }
    // Coba reconnect setiap 30 detik
    static unsigned long lastReconnect = 0;
    if (currentMillis - lastReconnect >= 30000) {
      lastReconnect = currentMillis;
      connectWiFi();
      if (wifiConnected) {
        initFirebase();
        startStream();
      }
    }
  } else if (!wifiConnected) {
    wifiConnected = true;
    Serial.println("✅ WiFi terhubung kembali!");
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
