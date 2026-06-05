// ==========================================
// 🏗️ SISTEM MONITORING KONTAINER LOGISTIK
// ESP32 + Firebase Realtime Database
// File: monitoring_final.ino
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
#define API_KEY "AIzaSyAdaq-v_4r3HeY4_lIiaN_J8FVsXgXzVTg"
#define DATABASE_URL "iot-monitoring-1d95d-default-rtdb.asia-southeast1.firebasedatabase.app"

// ==========================================
// 📶 KONFIGURASI WIFI
// ==========================================
#define WIFI_SSID "YANTO GAMING"
#define WIFI_PASSWORD "makesomenoise"

// ==========================================
// 📌 PIN DEFINITIONS
// ==========================================
#define DHTPIN          4
#define DHTTYPE         DHT22
#define REED_PIN        34
#define BUZZER_PIN      25
// Pin 26 dan 27 DISILANG karena fakta fisik membuktikan:
// Saat kode mengaktifkan Pin 26, yang menyala malah Solenoid.
#define RELAY_KIPAS     27 
#define RELAY_SOLENOID  26

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
#define INTERVAL_KIRIM    15000   // 15 detik kirim realtime data
#define INTERVAL_SENSOR   2000    // 2 detik baca sensor
#define INTERVAL_PINTU    300     // 300ms cek pintu
#define INTERVAL_OLED     1000    // 1 detik update OLED
#define INTERVAL_HEARTBEAT 60000  // 1 menit heartbeat

// ==========================================
// 🔧 NTP CONFIGURATION
// ==========================================
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 25200  // GMT+7 (WIB)
#define DST_OFFSET 0

// ==========================================
// 📦 OBJECTS
// ==========================================
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;
bool rtcValid = false;

// Firebase Objects
FirebaseData fbdo;        
FirebaseData fbdoStream;  
FirebaseAuth auth;
FirebaseConfig config;

// ==========================================
// 📊 STATUS SISTEM
// ==========================================
float suhu = 0, kelembapan = 0;
bool statusPintu = false;
bool manualKipas = false;
bool manualSolenoid = false;

// Hardware State Variables
bool hardwareKipasOn = false;
bool hardwareSolenoidOn = false;

// Alarm Flags
bool alarmSuhuAktif = false;
bool alarmHumidAktif = false;
bool alarmPintuAktif = false;

// Hysteresis Spam Prevention
float lastAlarmSuhu = 0;
float lastAlarmHumid = 0;
bool alarmSuhuSent = false;
bool alarmHumidSent = false;
bool alarmPintuSent = false;

// Data Logging History Prevention
float lastLoggedSuhu = 0;
float lastLoggedHumid = 0;
bool lastLoggedPintu = false;
bool lastLoggedKipas = false;
bool lastLoggedSolenoid = false;
unsigned long lastLogTime = 0;

// Buzzer State
bool buzzerMuted = false;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;
#define BUZZER_DURATION 3000 // Bunyi 3 detik saja untuk mengamankan hardware

// Realtime Event Trigger
bool forceRealtimeUpdate = false;

// Timing
unsigned long lastSensorRead = 0;
unsigned long lastDataSend = 0;
unsigned long lastPintuCheck = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastHeartbeat = 0;

bool firebaseReady = false;
bool streamStarted = false;
bool wifiConnected = false;

unsigned long dataSuccessCount = 0;
unsigned long dataFailCount = 0;

// ==========================================
// 📶 KONEKSI WIFI
// ==========================================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }

  Serial.println("📶 Menghubungkan ke WiFi...");
  WiFi.mode(WIFI_STA);
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
    Serial.println("\n✅ WiFi terhubung!");
  } else {
    wifiConnected = false;
    Serial.println("\n❌ Gagal terhubung ke WiFi!");
  }
}

// ==========================================
// 🔥 INISIALISASI FIREBASE
// ==========================================
void initFirebase() {
  Serial.println("🔥 Menginisialisasi Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
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
  if (data.dataType() == "json") {
    FirebaseJson *json = data.jsonObjectPtr();
    if (!json) return;

    FirebaseJsonData result;
    if (json->get(result, "kipas")) {
      manualKipas = (result.intValue == 1);
    }
    if (json->get(result, "solenoid")) {
      manualSolenoid = (result.intValue == 1);
    }
    if (json->get(result, "buzzerMute")) {
      buzzerMuted = (result.intValue == 1);
    }
  } 
  else if (data.dataType() == "int" || data.dataType() == "boolean" || data.dataType() == "double") {
    String path = data.dataPath();
    int nilai = data.intData();

    if (path == "/kipas") manualKipas = (nilai == 1);
    else if (path == "/solenoid") manualSolenoid = (nilai == 1);
    else if (path == "/buzzerMute") buzzerMuted = (nilai == 1);
  }

  // Update hardware pin states immediately based on new manual controls
  updateHardwarePins();
  forceRealtimeUpdate = true; // Langsung update Firebase agar Web langsung berubah
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("⚠️ Firebase stream timeout!");
}

void startStream() {
  if (!firebaseReady || streamStarted) return;
  Serial.println("📡 Memulai stream kontrol...");
  Firebase.beginStream(fbdoStream, "/kontrol");
  Firebase.setStreamCallback(fbdoStream, streamCallback, streamTimeoutCallback);
  streamStarted = true;
  Serial.println("✅ Stream kontrol aktif pada /kontrol");
}

// ==========================================
// ⚙️ UPDATE HARDWARE PINS (Menerapkan Logika)
// ==========================================
void updateHardwarePins() {
  // 1. LOGIKA KIPAS: Menyala jika Suhu ATAU Kelembapan Tinggi ATAU dinyalakan manual
  bool autoKipasOn = (suhu > SUHU_MAX || kelembapan > KELEMBAPAN_MAX);
  hardwareKipasOn = manualKipas || autoKipasOn; 

  // 2. LOGIKA SOLENOID: Murni dari kontrol manual web
  hardwareSolenoidOn = manualSolenoid;

  // 3. APPLY TO HARDWARE PINS (Tanpa Swap - Sesuai kode lama teman Anda)
  digitalWrite(RELAY_KIPAS, hardwareKipasOn ? HIGH : LOW);
  digitalWrite(RELAY_SOLENOID, hardwareSolenoidOn ? HIGH : LOW);
}

// ==========================================
// 🔊 PROCESS BUZZER
// ==========================================
void processBuzzer() {
  if (alarmPintuAktif) {
    if (buzzerMuted) {
      if (buzzerActive) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
      }
    } else {
      if (!buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION + 1000 || buzzerStartTime == 0)) {
        // Hanya trigger ON pertama kali
        digitalWrite(BUZZER_PIN, HIGH);
        buzzerActive = true;
        buzzerStartTime = millis();
        Serial.println("🔔 BUZZER ON");
      }
      
      // Auto-off setelah 3 detik untuk melindungi buzzer agar tidak jebol
      if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
        Serial.println("🔇 BUZZER AUTO-OFF (Timeout 3s)");
      }
    }
  } else {
    // Pintu tertutup -> matikan buzzer
    if (buzzerActive) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerActive = false;
      buzzerStartTime = 0;
    }
  }
}

// ==========================================
// 🌡️ BACA SENSOR DHT22
// ==========================================
void bacaSensor() {
  float bacaSuhu = dht.readTemperature();
  float bacaHumid = dht.readHumidity();

  if (!isnan(bacaSuhu) && !isnan(bacaHumid)) {
    suhu = bacaSuhu;
    kelembapan = bacaHumid;
  } else {
    Serial.println("⚠️ ERROR: Gagal baca DHT22");
    // (return; DIHAPUS DISINI AGAR KOMPONEN LAIN TETAP JALAN)
  }

  updateHardwarePins(); // Update hardware immediately after reading

  // Hysteresis Logika untuk Alarm Suhu/Kelembapan
  if (suhu > SUHU_MAX) {
    alarmSuhuAktif = true;
  } else if (suhu <= SUHU_MAX - 2.0) { 
    // Turun 2 derajat baru reset alarm
    alarmSuhuAktif = false;
    alarmSuhuSent = false; 
  }

  if (kelembapan > KELEMBAPAN_MAX) {
    alarmHumidAktif = true;
  } else if (kelembapan <= KELEMBAPAN_MAX - 5.0) {
    // Turun 5% baru reset alarm
    alarmHumidAktif = false;
    alarmHumidSent = false;
  }
}

// ==========================================
// 🚪 CEK PINTU (Reed Switch)
// ==========================================
void cekPintu() {
  bool bacaPintu = digitalRead(REED_PIN); // HIGH = Terbuka, LOW = Tertutup

  if (bacaPintu == HIGH && !statusPintu) {
    statusPintu = true;
    alarmPintuAktif = true;
    alarmPintuSent = false; 
    buzzerMuted = false;    // Auto-unmute buzzer ketika pintu baru dibuka lagi
    buzzerStartTime = 0;    // Reset timer buzzer
    forceRealtimeUpdate = true; // Langsung update Firebase!
    Serial.println("🚪 PINTU: TERBUKA");
  }
  else if (bacaPintu == LOW && statusPintu) {
    statusPintu = false;
    alarmPintuAktif = false;
    forceRealtimeUpdate = true; // Langsung update Firebase!
    Serial.println("🚪 PINTU: TERTUTUP");
  }
}

// ==========================================
// 📤 KIRIM DATA KE FIREBASE
// ==========================================
void kirimDataFirebase() {
  if (!firebaseReady || !wifiConnected) return;

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

  // ---- 1. REALTIME PUSH ----
  FirebaseJson rtJson;
  rtJson.set("suhu", (double)suhu);
  rtJson.set("kelembapan", (double)kelembapan);
  rtJson.set("pintu", statusPintu);
  rtJson.set("kipas", hardwareKipasOn);       // Kirim state kipas yg sebenarnya
  rtJson.set("solenoid", hardwareSolenoidOn); // Kirim state solenoid yg sebenarnya
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

  Firebase.setJSON(fbdo, "/realtime", rtJson);

  // ---- 2. PUSH HISTORIS LOG (BERDASARKAN PERUBAHAN SIGNIFIKAN) ----
  bool signifikantChange = false;
  unsigned long currentMillis = millis();

  // Cek beda absolut suhu > 1.0 atau humid > 5.0
  if (abs(suhu - lastLoggedSuhu) >= 1.0) signifikantChange = true;
  if (abs(kelembapan - lastLoggedHumid) >= 5.0) signifikantChange = true;
  // Cek perubahan status digital
  if (statusPintu != lastLoggedPintu) signifikantChange = true;
  if (hardwareKipasOn != lastLoggedKipas) signifikantChange = true;
  if (hardwareSolenoidOn != lastLoggedSolenoid) signifikantChange = true;
  // Keep-alive push setiap 1 jam (3600000 ms)
  if (currentMillis - lastLogTime >= 3600000) signifikantChange = true;

  if (signifikantChange) {
    FirebaseJson logJsonObj;
    logJsonObj.set("suhu", (double)suhu);
    logJsonObj.set("kelembapan", (double)kelembapan);
    logJsonObj.set("pintu", statusPintu);
    logJsonObj.set("kipas", hardwareKipasOn);
    logJsonObj.set("solenoid", hardwareSolenoidOn);
    logJsonObj.set("timestamp", (double)timestamp);
    logJsonObj.set("tanggal", tanggal);
    logJsonObj.set("waktu", waktu);

    Firebase.pushJSON(fbdo, "/log", logJsonObj);

    // Update last logged values
    lastLoggedSuhu = suhu;
    lastLoggedHumid = kelembapan;
    lastLoggedPintu = statusPintu;
    lastLoggedKipas = hardwareKipasOn;
    lastLoggedSolenoid = hardwareSolenoidOn;
    lastLogTime = currentMillis;
    Serial.println("  ✅ Log Historis disimpan (Perubahan Signifikan)");
  }

  // ---- 3. KIRIM ALARM (Hanya sekali per kejadian trigger) ----
  if (alarmSuhuAktif && !alarmSuhuSent) {
    FirebaseJson alarmObj;
    alarmObj.set("tipe", "suhu_tinggi");
    alarmObj.set("pesan", "PERINGATAN! Suhu: " + String(suhu, 1) + "°C");
    alarmObj.set("nilai", (double)suhu);
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
    alarmObj2.set("nilai", (double)kelembapan);
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
}

// ==========================================
// 💓 HEARTBEAT
// ==========================================
void sendHeartbeat() {
  if (!firebaseReady || !wifiConnected) return;

  unsigned long timestamp = millis() / 1000;
  if (rtcValid) timestamp = rtc.now().unixtime();
  time_t t = time(nullptr);
  if (t > 1609459200) timestamp = (unsigned long)t;

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
// 📺 UPDATE OLED
// ==========================================
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Sistem Monitoring");

  display.setCursor(0, 14);
  display.print("Suhu  : "); display.print(suhu, 1); display.print(" C");

  display.setCursor(0, 26);
  display.print("Humid : "); display.print(kelembapan, 1); display.print(" %");

  display.setCursor(0, 38);
  display.print("Pintu : ");
  if (statusPintu) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(" BUKA! ");
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.print("Tertutup");
  }

  display.setCursor(0, 50);
  if (buzzerActive) display.print("Bzr:ON  ");
  else if (buzzerMuted) display.print("Bzr:MUTE");
  else display.print("Bzr:RDY ");

  display.setCursor(72, 50);
  if (wifiConnected) {
    display.print("WiFi:"); display.print(WiFi.RSSI());
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
  Serial.println("\n🏗️ SISTEM MONITORING KONTAINER - FINAL");

  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_KIPAS, OUTPUT);
  pinMode(RELAY_SOLENOID, OUTPUT);

  digitalWrite(RELAY_KIPAS, LOW);
  digitalWrite(RELAY_SOLENOID, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();
  Wire.begin(21, 22);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 20); display.println("   Monitoring Final");
    display.setCursor(0, 40); display.println("   Menghubungkan...");
    display.display();
  }

  if (rtc.begin()) {
    rtcValid = true;
    Serial.println("✅ RTC DS3231 aktif");
  } else {
    Serial.println("⚠️ RTC tidak ditemukan!");
  }

  connectWiFi();

  if (wifiConnected) {
    configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
    initFirebase();
    startStream();
    sendHeartbeat();
  }
}

// ==========================================
// 🔁 LOOP
// ==========================================
void loop() {
  unsigned long currentMillis = millis();

  Firebase.reconnectNetwork(true);

  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) wifiConnected = false;
  } else if (!wifiConnected) {
    wifiConnected = true;
    if (firebaseReady) {
      streamStarted = false; 
      startStream();
    }
  }

  if (currentMillis - lastSensorRead >= INTERVAL_SENSOR) {
    lastSensorRead = currentMillis;
    bacaSensor();
  }

  if (currentMillis - lastPintuCheck >= INTERVAL_PINTU) {
    lastPintuCheck = currentMillis;
    cekPintu();
  }

  // EVENT-DRIVEN / REALTIME UPDATE
  if (forceRealtimeUpdate || currentMillis - lastDataSend >= INTERVAL_KIRIM) {
    lastDataSend = currentMillis;
    forceRealtimeUpdate = false;
    kirimDataFirebase();
  }

  if (currentMillis - lastOLEDUpdate >= INTERVAL_OLED) {
    lastOLEDUpdate = currentMillis;
    updateOLED();
  }

  if (currentMillis - lastHeartbeat >= INTERVAL_HEARTBEAT) {
    lastHeartbeat = currentMillis;
    sendHeartbeat();
  }

  processBuzzer();
}
