#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

const char* WIFI_SSID = "SSID";
const char* WIFI_PASS = "PASSWORD";

#define DHT_PIN D5
#define DHTTYPE DHT22
#define RELAY_PIN D6

const bool RELAY_ACTIVE_LOW = true;

const long UTC_OFFSET_SECONDS = 7L * 3600L;

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_SECONDS, 60000);

DHT dht(DHT_PIN, DHTTYPE);

unsigned long lastDHTRead = 0;
const unsigned long DHT_INTERVAL = 3000;

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("LampControl_DHT22_LCD starting...");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);

  Wire.begin(D2, D1); // SDA = D2, SCL = D1 (NodeMCU)
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Starting...");

  dht.begin();

  connectWiFi();

  timeClient.begin();
  timeClient.update();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      connectWiFi();
    }
  }

  timeClient.update();

  int hour = timeClient.getHours();
  int minute = timeClient.getMinutes();

  bool shouldBeOn = (hour >= 17 || hour < 5);

  if (shouldBeOn) {
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? LOW : HIGH);
  } else {
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  }

  static float lastTemp = NAN;
  if (millis() - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = millis();
    float t = dht.readTemperature();
    if (!isnan(t)) {
      lastTemp = t;
      Serial.printf("DHT read: %.1f*C\n", t);
    } else {
      Serial.println("DHT read failed");
    }
  }

  char line1[17];
  char line2[17];

  snprintf(line1, sizeof(line1), "Lamp:%s %02d:%02d", shouldBeOn ? "ON " : "OFF", hour, minute);
  if (!isnan(lastTemp)) {
    snprintf(line2, sizeof(line2), "Temp: %5.1f C     ", lastTemp);
  } else {
    snprintf(line2, sizeof(line2), "Temp:  --.- C     ");
  }

  lcd.setCursor(0,0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
  delay(500);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.printf("Connecting to WiFi '%s' ...\n", WIFI_SSID);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi connecting");
  lcd.setCursor(0,1);
  lcd.print(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("WiFi connected");
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP().toString());
    delay(1000);
  } else {
    Serial.println();
    Serial.println("WiFi connect failed");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("WiFi FAILED");
    lcd.setCursor(0,1);
    lcd.print("Check creds");
  }
}
