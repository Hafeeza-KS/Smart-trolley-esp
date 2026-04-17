#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ================= WIFI =================
const char* ssid = "Airtel_moha_5748";
const char* password = "Air@08294";

// ================= BACKEND =================
String backendUrl = "http://192.168.1.3";

// ================= GLOBAL =================
String barcode = "";
String sessionToken = "";

// ================= START SESSION =================
void startSession() {

  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  String url = backendUrl + ":8000/start-session?trolley_code=TRL001";

  Serial.println("Starting session...");

  http.begin(url);
  http.setTimeout(8000);

  int code = http.POST("");

  if (code > 0) {

    String response = http.getString();
    Serial.println(response);

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      sessionToken = doc["session_token"].as<String>();
      Serial.println("Session Token: " + sessionToken);
    } else {
      Serial.println("Session JSON Parse Failed!");
    }

  } else {
    Serial.println("Session Failed: " + String(code));
  }

  http.end();
}

// ================= RESET SESSION =================
void resetSession() {

  Serial.println("Resetting session...");

  sessionToken = "";

  delay(1000);

  startSession();
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  Serial.println("Connecting WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.println("Ready to scan...");

  startSession();  // 🔥 create session
}

// ================= LOOP =================
void loop() {

  if (Serial2.available()) {

    barcode = Serial2.readStringUntil('\n');
    barcode.trim();

    Serial.println("\nScanned Barcode: " + barcode);

    sendToBackend(barcode);
  }
}

// ================= SEND TO BACKEND =================
void sendToBackend(String barcode) {

  if (WiFi.status() != WL_CONNECTED) return;

  if (sessionToken == "") {
    Serial.println("No session found! Creating...");
    startSession();
    return;
  }

  HTTPClient http;

  String url = backendUrl + ":8000/scan?barcode=" + barcode +
               "&trolley_code=TRL001" +
               "&session_token=" + sessionToken;

  Serial.println("URL: " + url);

  if (!http.begin(url)) {
    Serial.println("HTTP begin failed!");
    return;
  }

  http.setTimeout(8000);

  Serial.println("Processing via backend...");

  int httpResponseCode = http.POST("");

  if (httpResponseCode > 0) {

    Serial.println("Cart Insert Response: " + String(httpResponseCode));

    String response = http.getString();
    Serial.println(response);

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
      Serial.println("JSON Parse Failed!");
      return;
    }

    Serial.println("------ PRODUCT DETAILS ------");
    String(doc["product"].as<const char*>());
    Serial.println("Price: " + String(doc["price"].as<float>()));
    Serial.println("Expected Weight: " + String(doc["expected_weight"].as<float>()) + " g");
    Serial.println("Detected Weight: " + String(doc["detected_weight"].as<float>()) + " g");
    Serial.println("-----------------------------");

  } else {

    Serial.println("Error: " + String(httpResponseCode));

    if (httpResponseCode == 401) {
      Serial.println("Session expired. Restarting...");
      startSession();
    }
  }

  http.end();
}