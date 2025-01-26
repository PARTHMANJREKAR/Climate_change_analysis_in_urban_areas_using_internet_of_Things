#include <WiFi.h>
#include <FirebaseESP32.h>
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// Wi-Fi credentials
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";

// Firebase details
#define FIREBASE_HOST "your_firebase_database_url"
#define FIREBASE_AUTH "your_firebase_database_secret"
FirebaseData firebaseData;

// DHT Sensor
#define DHTPIN 4  // GPIO pin for DHT22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// BMP280 Sensor
Adafruit_BMP280 bmp;

// Gas sensor pins
#define CO2_PIN 34
#define CO_PIN 35
#define SMOKE_PIN 32
#define METHANE_PIN 33

// Function to connect to Wi-Fi
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
}

// Function to get the current timestamp (from NTP or similar methods)
String getTimeStamp() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "1970-01-01T00:00:00Z";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buffer);
}

// Function to read gas concentration from analog pin
float readGasSensor(int pin) {
  int value = analogRead(pin);
  return (float)value / 4095.0 * 100.0; // Assuming 0-100% scale
}

void setup() {
  Serial.begin(115200);

  // Initialize sensors
  dht.begin();
  if (!bmp.begin(0x76)) { // Default I2C address for BMP280
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // Connect to Wi-Fi
  connectToWiFi();

  // Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  // Initialize NTP for time sync
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float co2 = readGasSensor(CO2_PIN);
  float co = readGasSensor(CO_PIN);
  float smoke = readGasSensor(SMOKE_PIN);
  float methane = readGasSensor(METHANE_PIN);
  float pressure = bmp.readPressure(); // Pressure in Pascals

  // Get timestamp
  String timestamp = getTimeStamp();

  // Prepare Firebase JSON payload
  FirebaseJson json;
  json.set("timestamp", timestamp);
  json.set("temperature", temperature);
  json.set("humidity", humidity);
  json.set("co2", co2);
  json.set("co", co);
  json.set("smoke", smoke);
  json.set("methane", methane);
  json.set("pressure", pressure);

  // Send data to Firebase
  if (Firebase.pushJSON(firebaseData, "/sensor_data", json)) {
    Serial.println("Data sent successfully:");
    Serial.println(json.raw());
  } else {
    Serial.print("Firebase push failed: ");
    Serial.println(firebaseData.errorReason());
  }

  // Wait for 10 minutes
  delay(10 * 60 * 1000);
}
