#include <WiFi.h>
#include <WebServer.h>

// --------------------------------------------------------------------------------
// CONFIGURATION
// --------------------------------------------------------------------------------
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Hardware Pins
const int MOISTURE_PIN = 34; // Analog Input
const int PUMP_PIN = 2;      // Digital Output (Built-in LED is usually 2)

// Dry/Wet value calibration (Capacitive sensors usually: Air ~3000, Water ~1500)
// ADJUST THESE VALUES BASED ON YOUR SENSOR!
const int DATA_WET = 1500; 
const int DATA_DRY = 3200;

WebServer server(80);

// --------------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // Off initially

  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Define Routes
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/water", handleWater);
  server.onNotFound(handleNotFound);

  // Enable CORS
  server.enableCORS(true); 

  server.begin();
  Serial.println("HTTP server started");
}

// --------------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------------
void loop() {
  server.handleClient();
}

// --------------------------------------------------------------------------------
// HANDLERS
// --------------------------------------------------------------------------------

void handleRoot() {
  server.send(200, "text/plain", "Gravity Meter ESP32 Controller is Running.");
}

void handleStatus() {
  int rawValue = analogRead(MOISTURE_PIN);
  
  // Convert raw value to percentage (0% = Dry, 100% = Wet)
  int moisturePercent = map(rawValue, DATA_DRY, DATA_WET, 0, 100);
  
  // Constrain to 0-100
  if (moisturePercent < 0) moisturePercent = 0;
  if (moisturePercent > 100) moisturePercent = 100;

  // Determine status text
  String statusText = "ok";
  if (moisturePercent < 30) statusText = "dry";
  else if (moisturePercent > 70) statusText = "wet";

  // Check if pump is currently valid (simple check against last action not easy without state, assuming pump is OFF in status loop for now or add state var)
  // For this simple example, we'll just report the sensor data structured nicely.
  
   //JSON Structure:
//{
  //   "sensor": {
  //     "moisture": 45,
  //     "raw": 2400
  //   },
  //   "status": {
  //     "text": "ok",
  //     "alert": false
  //   }
  // }

  String json = "{";
  json += "\"sensor\": {";
  json += "\"moisture\": " + String(moisturePercent) + ",";
  json += "\"raw\": " + String(rawValue);
  json += "},";
  json += "\"status\": {";
  json += "\"text\": \"" + statusText + "\",";
  json += "\"alert\": " + String(moisturePercent < 30 ? "true" : "false");
  json += "}";
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleWater() {
  // Turn on pump
  digitalWrite(PUMP_PIN, HIGH);
  Serial.println("Pump ON");
  
  // Keep it on for 2 seconds (blocking delay is okay for simple demo)
  delay(2000);
  
  digitalWrite(PUMP_PIN, LOW);
  Serial.println("Pump OFF");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\": \"watered\"}");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
