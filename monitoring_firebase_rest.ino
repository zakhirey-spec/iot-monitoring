// ==========================================
// 🏗️ SISTEM MONITORING KONTAINER LOGISTIK
// ESP32 + Firebase REST API (Kompatibel 2.2.9)
// ==========================================
// REST API version - tidak perlu FirebaseClient library kompleks
// ==========================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
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
#define DATABASE_URL "https://iot-monitoring-1d95d-default-rtdb.asia-southeast1.firebasedatabase.app"

// ==========================================
// 📶 KONFIGURASI WIFI
// ==========================================
#define WIFI_SSID "Goblokloe"
#define WIFI_PASSWORD "ayamjoper"

// ==========================================
// 📌 PIN DEFINITIONS
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
#define BUZZER_DURATION   3000
#define INTERVAL_KIRIM    15000   // 15 detik
#define INTERVAL_SENSOR   2000
#define INTERVAL_PINTU    300
#define INTERVAL_OLED     1000
#define INTERVAL_HEARTBEAT 60000
#define INTERVAL_KONTROL  10000   // Poll kontrol setiap 10 detik

// ==========================================
// 🔧 NTP CONFIGURATION
// ==========================================
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 25200
#define DST_OFFSET 0

// ==========================================
// 📦 OBJECTS
// ==========================================
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

WiFiClientSecure ssl_client;
HTTPClient http;

// ==========================================
// 📊 STATUS SISTEM
// ==========================================
float suhu = 0, kelembapan = 0;
bool statusPintu = false;
bool manualKipas = false;
bool manualSolenoid = false;

bool alarmSuhuAktif = false;
bool alarmHumidAktif = false;
bool alarmPintuAktif = false;
bool alarmSuhuSent = false;
bool alarmHumidSent = false;
bool alarmPintuSent = false;

bool buzzerMuted = false;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;

// Timing
unsigned long lastSensorRead = 0;
unsigned long lastDataSend = 0;
unsigned long lastPintuCheck = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastKontrolPoll = 0;

// Firebase state
bool firebaseReady = false;
bool wifiConnected = false;

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
  Serial.println("🔥 Menginisialisasi Firebase REST API...");
  ssl_client.setInsecure();
  firebaseReady = true;
  Serial.println("✅ Firebase REST API siap!");
}

// ==========================================
// 📡 POLLING KONTROL (REST API GET)
// ==========================================
void pollKontrol() {
  if (!firebaseReady || !wifiConnected) return;

  String url = String(DATABASE_URL) + "/kontrol.json?auth=" + API_KEY;
  http.begin(ssl_client, url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("📡 Kontrol diterima");

    // Parse JSON sederhana
    if (payload.indexOf("\"kipas\"") >= 0) {
      int idx = payload.indexOf("\"kipas\":");
      if (idx >= 0) {
        int val = payload.substring(idx + 9, idx + 12).toInt();
        manualKipas = (val == 1);
        if (manualKipas) {
          digitalWrite(RELAY_KIPAS, HIGH);
          Serial.println("🌀 Kipas: MANUAL ON");
        } else {
          if (suhu <= SUHU_MAX && kelembapan <= KELEMBAPAN_MAX) {
            digitalWrite(RELAY_KIPAS, LOW);
            Serial.println("🌀 Kipas: MATI");
          }
        }
      }
    }

    if (payload.indexOf("\"solenoid\"") >= 0) {
      int idx = payload.indexOf("\"solenoid\":");
      if (idx >= 0) {
        int val = payload.substring(idx + 11, idx + 14).toInt();
        manualSolenoid = (val == 1);
        digitalWrite(RELAY_SOLENOID, val ? HIGH : LOW);
        Serial.printf("🔒 Solenoid: %s\n", val ? "OPEN" : "LOCK");
      }
    }

    if (payload.indexOf("\"buzzerMute\"") >= 0) {
      int idx = payload.indexOf("\"buzzerMute\":");
      if (idx >= 0) {
        int val = payload.substring(idx + 13, idx + 16).toInt();
        buzzerMuted = (val == 1);
        if (buzzerMuted) {
          digitalWrite(BUZZER_PIN, LOW);
          buzzerActive = false;
        }
      }
    }
  }
  http.end();
}

// ==========================================
// 🔊 PROCESS BUZZER
// ==========================================
void processBuzzer() {
  bool adaAlarm = alarmSuhuAktif || alarmHumidAktif || alarmPintuAktif;

  if (buzzerMuted) {
    if (buzzerActive) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerActive = false;
    }
    return;
  }

  if (adaAlarm && !buzzerActive) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerActive = true;
    buzzerStartTime = millis();
    Serial.println("🔔 BUZZER: ON");
  }

  if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
    Serial.println("🔇 BUZZER: OFF");
  }

  if (!adaAlarm && buzzerActive) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}

// ==========================================
// 🌡️ BACA SENSOR DHT22
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
    Serial.println("⚠️ ERROR: DHT22 read failed");
    return;
  }

  suhu = bacaSuhu;
  kelembapan = bacaHumid;

  if (!manualKipas) {
    if (suhu > SUHU_MAX || kelembapan > KELEMBAPAN_MAX) {
      digitalWrite(RELAY_KIPAS, HIGH);
    } else {
      digitalWrite(RELAY_KIPAS, LOW);
    }
  }

  if (suhu > SUHU_MAX) {
    alarmSuhuAktif = true;
  } else {
    alarmSuhuAktif = false;
    alarmSuhuSent = false;
  }

  if (kelembapan > KELEMBAPAN_MAX) {
    alarmHumidAktif = true;
  } else {
    alarmHumidAktif = false;
    alarmHumidSent = false;
  }
}

// ==========================================
// 🚪 CEK PINTU
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
// 📤 KIRIM DATA KE FIREBASE (REST API POST)
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

  // Buat JSON payload
  String realtimeJson = "{";
  realtimeJson += "\"suhu\":" + String(suhu, 1) + ",";
  realtimeJson += "\"kelembapan\":" + String(kelembapan, 1) + ",";
  realtimeJson += "\"pintu\":" + String(statusPintu ? "true" : "false") + ",";
  realtimeJson += "\"timestamp\":" + String(timestamp) + ",";
  realtimeJson += "\"tanggal\":\"" + String(tanggal) + "\",";
  realtimeJson += "\"waktu\":\"" + String(waktu) + "\"";
  realtimeJson += "}";

  // Set realtime data
  String setUrl = String(DATABASE_URL) + "/realtime.json?auth=" + API_KEY;
  http.begin(ssl_client, setUrl);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.PUT(realtimeJson);
  
  if (httpCode == 200) {
    dataSuccessCount++;
    Serial.println("  ✅ Realtime data sent");
  } else {
    dataFailCount++;
    Serial.println("  ❌ Failed to send realtime");
  }
  http.end();

  // Push log
  String logJson = "{";
  logJson += "\"suhu\":" + String(suhu, 1) + ",";
  logJson += "\"kelembapan\":" + String(kelembapan, 1) + ",";
  logJson += "\"timestamp\":" + String(timestamp);
  logJson += "}";

  String pushUrl = String(DATABASE_URL) + "/log.json?auth=" + API_KEY;
  http.begin(ssl_client, pushUrl);
  http.addHeader("Content-Type", "application/json");
  httpCode = http.POST(logJson);
  
  if (httpCode == 200) {
    Serial.println("  ✅ Log saved");
  } else {
    Serial.println("  ❌ Failed to save log");
  }
  http.end();

  // Kirim alarm
  if (alarmSuhuAktif && !alarmSuhuSent) {
    String alarmJson = "{";
    alarmJson += "\"tipe\":\"suhu_tinggi\",";
    alarmJson += "\"nilai\":" + String(suhu, 1) + ",";
    alarmJson += "\"timestamp\":" + String(timestamp);
    alarmJson += "}";

    String alarmUrl = String(DATABASE_URL) + "/alarm.json?auth=" + API_KEY;
    http.begin(ssl_client, alarmUrl);
    http.addHeader("Content-Type", "application/json");
    http.POST(alarmJson);
    http.end();
    
    alarmSuhuSent = true;
    Serial.println("  🚨 Alarm suhu sent!");
  }

  if (alarmHumidAktif && !alarmHumidSent) {
    String alarmJson = "{";
    alarmJson += "\"tipe\":\"kelembapan_tinggi\",";
    alarmJson += "\"nilai\":" + String(kelembapan, 1) + ",";
    alarmJson += "\"timestamp\":" + String(timestamp);
    alarmJson += "}";

    String alarmUrl = String(DATABASE_URL) + "/alarm.json?auth=" + API_KEY;
    http.begin(ssl_client, alarmUrl);
    http.addHeader("Content-Type", "application/json");
    http.POST(alarmJson);
    http.end();
    
    alarmHumidSent = true;
    Serial.println("  🚨 Alarm humidity sent!");
  }

  if (alarmPintuAktif && !alarmPintuSent) {
    String alarmJson = "{";
    alarmJson += "\"tipe\":\"pintu_terbuka\",";
    alarmJson += "\"timestamp\":" + String(timestamp);
    alarmJson += "}";

    String alarmUrl = String(DATABASE_URL) + "/alarm.json?auth=" + API_KEY;
    http.begin(ssl_client, alarmUrl);
    http.addHeader("Content-Type", "application/json");
    http.POST(alarmJson);
    http.end();
    
    alarmPintuSent = true;
    Serial.println("  🚨 Alarm door sent!");
  }

  Serial.printf("📊 Stats: %lu success, %lu failed\n", dataSuccessCount, dataFailCount);
}

// ==========================================
// 💓 HEARTBEAT
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

  String heartbeatJson = "{";
  heartbeatJson += "\"online\":true,";
  heartbeatJson += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  heartbeatJson += "\"timestamp\":" + String(timestamp);
  heartbeatJson += "}";

  String statusUrl = String(DATABASE_URL) + "/status.json?auth=" + API_KEY;
  http.begin(ssl_client, statusUrl);
  http.addHeader("Content-Type", "application/json");
  http.PUT(heartbeatJson);
  http.end();
}

// ==========================================
// 📺 UPDATE OLED
// ==========================================
void updateOLED() {
  DateTime now = rtc.now();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.printf("%02d:%02d:%02d %02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month());

  display.setCursor(0, 14);
  display.print("T: ");
  display.print(suhu, 1);
  display.print("C");

  display.setCursor(0, 26);
  display.print("H: ");
  display.print(kelembapan, 1);
  display.print("%");

  display.setCursor(0, 38);
  display.print("Door: ");
  display.print(statusPintu ? "OPEN" : "CLOSED");

  display.setCursor(0, 50);
  display.print("WiFi: ");
  display.print(wifiConnected ? "OK" : "OFF");

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
  Serial.println("  🏗️ MONITORING v2.0 - REST API Edition");
  Serial.println("==========================================");

  // Setup Pin
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_KIPAS, OUTPUT);
  pinMode(RELAY_SOLENOID, OUTPUT);

  digitalWrite(RELAY_KIPAS, LOW);
  digitalWrite(RELAY_SOLENOID, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Setup DHT22
  dht.begin();

  // Setup I2C & OLED
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED not found!");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 25);
    display.println("  Monitoring v2.0");
    display.setCursor(0, 45);
    display.println("  Connecting...");
    display.display();
    Serial.println("✅ OLED active");
  }

  // Setup RTC
  if (!rtc.begin()) {
    Serial.println("⚠️ RTC not found!");
  } else {
    Serial.println("✅ RTC active");
  }

  // WiFi
  connectWiFi();

  // NTP
  if (wifiConnected) {
    configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
    Serial.print("✅ NTP sync starting");
    time_t now = time(nullptr);
    int tries = 0;
    while (now < 1609459200 && tries < 30) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
      tries++;
    }
    Serial.println(now >= 1609459200 ? "\n✅ NTP done" : "\n⚠️ NTP failed");
  }

  // Firebase
  if (wifiConnected) {
    initFirebase();
    sendHeartbeat();
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

  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      wifiConnected = false;
      Serial.println("⚠️ WiFi disconnected!");
    }
    static unsigned long lastReconnect = 0;
    if (currentMillis - lastReconnect >= 30000) {
      lastReconnect = currentMillis;
      connectWiFi();
    }
  } else if (!wifiConnected) {
    wifiConnected = true;
    Serial.println("✅ WiFi reconnected!");
  }

  // Sensor
  if (currentMillis - lastSensorRead >= INTERVAL_SENSOR) {
    lastSensorRead = currentMillis;
    bacaSensor();
  }

  // Door
  if (currentMillis - lastPintuCheck >= INTERVAL_PINTU) {
    lastPintuCheck = currentMillis;
    cekPintu();
  }

  // Send data
  if (currentMillis - lastDataSend >= INTERVAL_KIRIM) {
    lastDataSend = currentMillis;
    kirimDataFirebase();
  }

  // Poll kontrol
  if (currentMillis - lastKontrolPoll >= INTERVAL_KONTROL) {
    lastKontrolPoll = currentMillis;
    pollKontrol();
  }

  // OLED
  if (currentMillis - lastOLEDUpdate >= INTERVAL_OLED) {
    lastOLEDUpdate = currentMillis;
    updateOLED();
  }

  // Heartbeat
  if (currentMillis - lastHeartbeat >= INTERVAL_HEARTBEAT) {
    lastHeartbeat = currentMillis;
    sendHeartbeat();
  }

  // Buzzer
  processBuzzer();

  delay(10);
}
