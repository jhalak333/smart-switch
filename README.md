# smart-switch
power switching system with schedule created in local system, it takes time from internet and switched according to timetable created.
this code can be upload to esp8266 microcontroller (esp-01/esp12f/esp12s/esp-01/nodemcu ... etc)

**Features Included:**
**1.	Relay Toggle with Button:**
    -	Press button <1 second to toggle relay state
    -	Immediate physical response
    -	State saved to persistent storage
**2.	State Persistence:**
    -	Relay state survives power failures
    -	Automatically restored on boot
    -	Saved to LittleFS after every change
**3.	Schedule Management:**
    -	20 time-based action slots
    -	"Save and Run" button restarts system
    -	HH:MM:SS format matching
**4.	WiFi Handling:**
  -	Automatic reconnection attempts
  -	System restart after 3 minutes offline
  -	Web-based configuration
**5.	Nepal Time Accuracy:**
  -	np.pool.ntp.org server
  -	UTC+5:45 offset (20,700 seconds)
  -	Regular time synchronization
**6.	Visual Feedback:**
  -	LED steady on: Normal operation
  -	LED slow blink: WiFi connecting
  -	LED fast blink: Configuration mode
**7.	Robust Error Handling:**
  -	Filesystem error detection
  -	WiFi failure recovery
  -	Detailed serial logging

**How to Use:**
**1.	Manual Relay Control:**
    -	Briefly press button (<1 sec) to toggle relay
    -	Listen for relay click and check LED
    -	Verify state in serial monitor
**2.	Configuration Mode:**
    -	Long-press button (≥2 sec) to enter AP mode
    -	Connect to "SmartSwitchAP"
    -	Open 192.168.4.1 in browser
    -	Configure WiFi and schedules
    -	Click "Save and Run"
**3.	Automatic Operation:**
    -	System connects to WiFi automatically
    -	Syncs time from NTP server
    -	Executes schedules at precise times
    -	Maintains state through power cycles
**Power Failure Resilience:**
    -	Relay state saved after every change
    -	Schedules stored in filesystem
    -	WiFi credentials preserved
    -	System auto-recovers on power restoration
    
   

