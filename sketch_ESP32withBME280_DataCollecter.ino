/*************************************************************
 *  ESP32 SENSOR DATA ACQUISITION & CLOUD UPLOAD
 *
 *  Sensors Used:
 *  - Ultrasonic Sensor (HC-SR04) → Distance
 *  - BME280 → Temperature, Humidity, Pressure
 *
 *  Function:
 *  - Read sensor data every 5 seconds
 *  - Send data to Google Sheets via Google Apps Script
 *  - Communication uses HTTPS (POST JSON)
 *************************************************************/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ===================== PIN CONFIGURATION =====================
#define TRIG_PIN 13   // Trigger pin for ultrasonic sensor
#define ECHO_PIN 12   // Echo pin for ultrasonic sensor

// ===================== SENSOR OBJECT =====================
Adafruit_BME280 bme;  // BME280 sensor using I2C communication

// ===================== WIFI CREDENTIALS =====================
const char* wifi_ssid     = "POCO X3 NFC";
const char* wifi_password = "12345671";

// ===================== GOOGLE APPS SCRIPT URL =====================
// Web App URL that receives sensor data and stores it in Google Sheets
const char* googleScriptURL =
"https://script.google.com/macros/s/AKfycby-haceUB3idc8yUnmIYpLYie18hYconyHx0B1rldNszZ6d1rHV7SoiPD6oO7vCEi_pUw/exec";

// ===================== TIMING SETTINGS =====================
const unsigned long SEND_INTERVAL = 5000; // Send data every 5 seconds

/*************************************************************
 *  FUNCTION: getDistanceCM()
 *  Description:
 *  - Triggers ultrasonic pulse
 *  - Measures echo duration
 *  - Converts time to distance in centimeters
 *************************************************************/
float getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure echo pulse duration (timeout: 30 ms)
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  // If no echo received, return invalid value
  if (duration == 0) return -1;

  // Convert duration to distance (speed of sound)
  return duration * 0.034 / 2.0;
}

/*************************************************************
 *  FUNCTION: sendDataToGoogleSheet()
 *  Description:
 *  - Sends sensor data to Google Sheets
 *  - Uses HTTPS POST with JSON payload
 *************************************************************/
void sendDataToGoogleSheet(float distance,
                           float temperature,
                           float humidity,
                           float pressure) {

  // Check WiFi connection
  if (WiFi.status() == WL_CONNECTED) {

    // Secure client (certificate verification disabled)
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, googleScriptURL);
    http.addHeader("Content-Type", "application/json");

    // Construct JSON payload
    String jsonData =
      "{\"distance\":" + String(distance, 2) +
      ",\"temperature\":" + String(temperature, 2) +
      ",\"humidity\":" + String(humidity, 2) +
      ",\"pressure\":" + String(pressure, 2) + "}";

    Serial.println("\nSending data to Google Sheets...");
    Serial.println(jsonData);

    // Send POST request
    int httpCode = http.POST(jsonData);
    String payload = http.getString();

    // Print server response
    Serial.printf("HTTP Response code: %d\n", httpCode);
    Serial.println("Response: " + payload);

    http.end();
  } else {
    Serial.println("WiFi disconnected, data not sent.");
  }
}

/*************************************************************
 *  SETUP FUNCTION
 *  - Initialize serial communication
 *  - Configure pins
 *  - Initialize BME280 sensor
 *  - Connect to WiFi
 *************************************************************/
void setup() {
  Serial.begin(115200);

  // Configure ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize BME280 sensor
  Serial.println("Initializing BME280...");
  if (!bme.begin(0x76)) {
    Serial.println("BME280 not detected. Check wiring or I2C address.");
    while (1); // Stop execution if sensor fails
  }
  Serial.println("BME280 initialized successfully.");

  // Connect to WiFi network
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi");

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  // WiFi connection result
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed.");
  }
}

/*************************************************************
 *  MAIN LOOP
 *  - Read sensors periodically
 *  - Display readings in Serial Monitor
 *  - Upload data to Google Sheets
 *************************************************************/
void loop() {
  static unsigned long lastSend = 0;

  // Check if it is time to send new data
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    // Read sensor values
    float distance     = getDistanceCM();
    float temperature  = bme.readTemperature();
    float humidity     = bme.readHumidity();
    float pressureAtm  = bme.readPressure() / 101325.0; // Convert Pa → atm

    // Display sensor readings
    Serial.println("\n--- Sensor Data ---");
    Serial.printf("Distance    : %.2f cm\n", distance);
    Serial.printf("Temperature : %.2f °C\n", temperature);
    Serial.printf("Humidity    : %.2f %%\n", humidity);
    Serial.printf("Pressure    : %.4f atm\n", pressureAtm);

    // Send data to Google Sheets
    sendDataToGoogleSheet(distance, temperature, humidity, pressureAtm);
  }
}

