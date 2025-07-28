#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Pin definitions nodemcu
//#define RELAY_PIN 12     // GPIO12 for relay control
//#define LED_PIN 2        // GPIO2 for status LED (active LOW)
//#define BUTTON_PIN 0     // GPIO0 for push button

// Pin definitions esp01
#define RELAY_PIN 0     // GPIO12 for relay control
#define LED_PIN 2        // GPIO2 for status LED (active LOW)
#define BUTTON_PIN 1     // GPIO0 for push button

// Mode and state variables
bool settingMode = false;
bool relayState = false;
bool wifiConnected = false;
unsigned long lastButtonPress = 0;
unsigned long lastBlink = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long lastScheduleCheck = 0;
unsigned long wifiDisconnectedTime = 0;

// WiFi and server
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "np.pool.ntp.org", 20700, 60000); // Nepal UTC+5:45 (20700 seconds)

// Schedule storage
struct Schedule {
  String time;
  String action;
};
Schedule schedules[20];

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial to initialize
  Serial.println("\n\nSmart Switch System Booting...");
  
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initial state - both relay and LED OFF
  digitalWrite(RELAY_PIN, HIGH); // Relay OFF (reversed logic)
  digitalWrite(LED_PIN, HIGH);   // LED OFF (active low)
  
  Serial.println("Pins initialized");

  // Initialize filesystem
  if (!LittleFS.begin()) {
    Serial.println("ERROR: Failed to mount LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully");

  // Load saved settings
  loadRelayState();
  updateOutputs(); // Set relay and LED based on loaded state
  Serial.printf("Initial relay state: %s\n", relayState ? "ON" : "OFF");

  loadSchedules();
  connectToWiFi();
}

// Update both relay and LED based on current state
void updateOutputs() {
  // REVERSED LOGIC:
  //   relayState = true  -> Relay ON  (LOW signal)
  //   relayState = false -> Relay OFF (HIGH signal)
  digitalWrite(RELAY_PIN, relayState ? LOW : HIGH);
  
  // LED matches relay state:
  //   relayState = true  -> LED ON  (LOW signal)
  //   relayState = false -> LED OFF (HIGH signal)
  digitalWrite(LED_PIN, relayState ? LOW : HIGH);
}

void loop() {
  handleButton();

  if (settingMode) {
    handleSettingMode();
  } else {
    handleRunningMode();
  }

  server.handleClient();
  delay(10);
}

void loadRelayState() {
  File file = LittleFS.open("/state.txt", "r");
  if (file) {
    relayState = file.readString().toInt();
    file.close();
    Serial.printf("Loaded relay state: %s\n", relayState ? "ON" : "OFF");
  } else {
    Serial.println("No saved relay state found - using default OFF");
  }
}

void saveRelayState() {
  File file = LittleFS.open("/state.txt", "w");
  if (file) {
    file.print(relayState);
    file.close();
    Serial.printf("Saved relay state: %s\n", relayState ? "ON" : "OFF");
  } else {
    Serial.println("ERROR: Failed to save relay state");
  }
}

void loadSchedules() {
  File file = LittleFS.open("/schedule.txt", "r");
  if (file) {
    Serial.println("Loading schedules:");
    for (int i = 0; i < 20 && file.available(); i++) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        int commaPos = line.indexOf(',');
        if (commaPos != -1) {
          schedules[i].time = line.substring(0, commaPos);
          schedules[i].action = line.substring(commaPos + 1);
          Serial.printf("  Slot %02d: %s -> %s\n", i+1, schedules[i].time.c_str(), schedules[i].action.c_str());
        }
      }
    }
    file.close();
  } else {
    Serial.println("No schedule file found - starting with empty schedule");
  }
}

void toggleRelay() {
  relayState = !relayState;
  updateOutputs(); // Update both relay and LED
  saveRelayState();
  Serial.printf("Relay toggled to: %s\n", relayState ? "ON" : "OFF");
}

void handleButton() {
  static bool buttonActive = false;
  static unsigned long pressStart = 0;

  if (digitalRead(BUTTON_PIN) == LOW) {
    if (!buttonActive) {
      buttonActive = true;
      pressStart = millis();
      Serial.println("Button pressed - timing started");
    }
    
    // Long press detection (2+ seconds)
    if (buttonActive && (millis() - pressStart > 2000)) {
      settingMode = !settingMode;
      Serial.printf("Long press detected - Setting mode: %s\n", settingMode ? "ON" : "OFF");
      
      if (settingMode) {
        startAPMode();
      } else {
        WiFi.softAPdisconnect(true);
        connectToWiFi();
      }
      
      buttonActive = false;
      lastButtonPress = millis();
    }
  } 
  else if (buttonActive) {
    unsigned long pressDuration = millis() - pressStart;
    buttonActive = false;
    
    // Short press detection (<1 second)
    if (pressDuration < 1000) {
      toggleRelay();
    }
    Serial.printf("Button released after %d ms\n", pressDuration);
  }
}

void startAPMode() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("SmartSwitchAP", "");
  Serial.println("AP Mode Started");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", HTTP_GET, []() {
    String html = "<html><head><title>Smart Switch Config</title></head><body>";
    html += "<h1>Smart Switch Config</h1>";
    
    // WiFi Form
    html += "<h2>WiFi Settings</h2>";
    html += "<form action='/savewifi' method='post'>";
    
    // Load saved WiFi if exists
    File wifiFile = LittleFS.open("/wifi.txt", "r");
    String savedSSID = "";
    if (wifiFile) {
      savedSSID = wifiFile.readStringUntil('\n');
      savedSSID.trim();
      wifiFile.close();
      Serial.printf("Loaded saved WiFi SSID: %s\n", savedSSID.c_str());
    }
    
    html += "SSID: <input name='ssid' value='" + savedSSID + "' required><br>";
    html += "Password: <input type='password' name='password' required><br>";
    html += "<button type='submit'>Save WiFi</button>";
    html += "</form>";
    
    // Schedule Form
    html += "<h2>Schedule</h2>";
    html += "<form action='/saveschedule' method='post'>";
    html += "<table border='1'><tr><th>#</th><th>Time (HH:MM:SS)</th><th>Action</th></tr>";
    
    for (int i = 0; i < 20; i++) {
      html += "<tr><td>" + String(i+1) + "</td>";
      html += "<td><input type='time' step='1' name='time" + String(i) + "' value='" + schedules[i].time + "'></td>";
      html += "<td><select name='action" + String(i) + "'>";
      
      // Dropdown options
      html += "<option value='ON'";
      if (schedules[i].action == "ON") html += " selected";
      html += ">ON</option>";
      
      html += "<option value='OFF'";
      if (schedules[i].action == "OFF") html += " selected";
      html += ">OFF</option>";
      
      html += "</select></td></tr>";
    }
    
    html += "</table><button type='submit'>Save and Run</button></form>";
    
    // Reset Form
    html += "<h2>System</h2>";
    html += "<form action='/reset' method='post'>";
    html += "<button type='submit'>Reset All Settings</button>";
    html += "</form></body></html>";
    
    server.send(200, "text/html", html);
    Serial.println("Served configuration page");
  });

  server.on("/savewifi", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    File file = LittleFS.open("/wifi.txt", "w");
    if (file) {
      file.println(ssid);
      file.println(password);
      file.close();
      Serial.printf("Saved WiFi credentials - SSID: %s\n", ssid.c_str());
    } else {
      Serial.println("ERROR: Failed to save WiFi credentials");
    }
    
    server.send(200, "text/plain", "WiFi settings saved! Returning to normal mode...");
    settingMode = false;
    WiFi.softAPdisconnect(true);
    connectToWiFi();
  });

  server.on("/saveschedule", HTTP_POST, []() {
    Serial.println("Saving new schedules:");
    for (int i = 0; i < 20; i++) {
      String time = server.arg("time" + String(i));
      String action = server.arg("action" + String(i));
      
      if (time.length() == 8) { // HH:MM:SS format
        schedules[i].time = time;
        schedules[i].action = action;
        Serial.printf("  Slot %02d: %s -> %s\n", i+1, time.c_str(), action.c_str());
      }
    }
    
    File file = LittleFS.open("/schedule.txt", "w");
    if (file) {
      for (int i = 0; i < 20; i++) {
        if (schedules[i].time.length() == 8) {
          file.println(schedules[i].time + "," + schedules[i].action);
        }
      }
      file.close();
      Serial.println("Schedules saved successfully");
    } else {
      Serial.println("ERROR: Failed to save schedules");
    }
    
    server.send(200, "text/plain", "Schedule saved! Restarting system...");
    Serial.println("Restarting system to apply schedules...");
    delay(1000);
    ESP.restart();
  });

  server.on("/reset", HTTP_POST, []() {
    LittleFS.remove("/wifi.txt");
    LittleFS.remove("/schedule.txt");
    LittleFS.remove("/state.txt");
    server.send(200, "text/plain", "All settings reset. Restarting...");
    Serial.println("All settings reset - restarting system");
    delay(1000);
    ESP.restart();
  });

  server.begin();
}

void connectToWiFi() {
  Serial.println("Attempting to connect to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  File file = LittleFS.open("/wifi.txt", "r");
  if (file) {
    String ssid = file.readStringUntil('\n');
    String pass = file.readStringUntil('\n');
    ssid.trim();
    pass.trim();
    file.close();

    Serial.printf("Connecting to: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    wifiDisconnectedTime = millis();
    
    for (int i = 0; i < 20; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("WiFi connected successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        
        timeClient.begin();
        timeClient.forceUpdate();
        Serial.printf("NTP time synchronized: %s\n", timeClient.getFormattedTime().c_str());
        return;
      }
      Serial.print(".");
      delay(500);
    }
    Serial.println("\nERROR: Failed to connect to WiFi");
  } else {
    Serial.println("No WiFi credentials found - please configure");
  }
  wifiConnected = false;
}

void handleSettingMode() {
  // Fast blink LED (250ms interval) for setting mode
  if (millis() - lastBlink > 250) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
}

void handleRunningMode() {
  // WiFi connection management
  if (!wifiConnected) {
    // Track how long we've been disconnected
    if (wifiDisconnectedTime == 0) {
      wifiDisconnectedTime = millis();
    }
    
    // Check if 3 minutes have passed since disconnection
    if (millis() - wifiDisconnectedTime > 180000) { // 3 minutes = 180000ms
      Serial.println("WiFi disconnected for 3 minutes - restarting system...");
      delay(1000);
      ESP.restart();
    }
    
    // Slow blink LED (1s interval) for connection issues
    if (millis() - lastBlink > 1000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
    
    // Reconnect attempt every 30 seconds
    if (millis() - lastReconnectAttempt > 30000) {
      Serial.println("Attempting WiFi reconnection...");
      connectToWiFi();
      lastReconnectAttempt = millis();
    }
    return;
  }
  
  // Reset disconnected timer if connected
  wifiDisconnectedTime = 0;
  
  // Update time every minute
  timeClient.update();
  
  // Check schedules every second
  if (millis() - lastScheduleCheck >= 1000) {
    String currentTime = timeClient.getFormattedTime();
    Serial.printf("Current time: %s\n", currentTime.c_str());
    
    for (int i = 0; i < 20; i++) {
      if (schedules[i].time.length() == 8 && schedules[i].time == currentTime) {
        bool newState = (schedules[i].action == "ON");
        if (relayState != newState) {
          relayState = newState;
          updateOutputs(); // Update both relay and LED
          saveRelayState();
          Serial.printf("Schedule triggered! Slot %02d: %s -> Relay %s\n", 
                       i+1, schedules[i].time.c_str(), relayState ? "ON" : "OFF");
        }
      }
    }
    lastScheduleCheck = millis();
  }
}
