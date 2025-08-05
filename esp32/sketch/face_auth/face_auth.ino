#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Brimstone";
const char* password = "haggu@515";

// Flask server IP address (change to match your computer's IP)
const char* serverName = "http://192.168.0.108:5000/auth_result";

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");

  // OPTIONAL: Send one-time recognition for testing
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"recognized\": true, \"user_id\": \"toni\"}";
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("✅ Response code: " + String(httpResponseCode));
      Serial.println("📩 Response body: " + response);
    } else {
      Serial.println("❌ Error sending POST: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("❌ WiFi not connected");
  }
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (input.startsWith("START:")) {
      String authId = input.substring(6);
      Serial.println("🔄 Starting face recognition for: " + authId);

      // Simulate recognition (replace this with real EI inference)
      bool recognized = true;
      String user = "toni";

      if (recognized) {
        HTTPClient http;
        http.begin("http://192.168.0.108:5000/auth_result");
        http.addHeader("Content-Type", "application/json");

        String body = "{\"recognized\": true, \"user_id\": \"" + user + "\"}";
        int code = http.POST(body);
        Serial.println("📡 POST sent, response code: " + String(code));

        http.end();
      }
    }
  }
}
