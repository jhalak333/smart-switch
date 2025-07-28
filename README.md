# smart-switch
power switching system with schedule created in local system, it takes time from internet and switched according to timetable created.
this code can be upload to esp8266 microcontroller (esp-01/esp12f/esp12s/esp-01/nodemcu ... etc)

Features Included:
**1.	Relay Toggle with Button:**
  -	Press button <1 second to toggle relay state
  -	Immediate physical response
  -	State saved to persistent storage
**2.	State Persistence:**
  o	Relay state survives power failures
  o	Automatically restored on boot
  o	Saved to LittleFS after every change
3.	Schedule Management:
  o	20 time-based action slots
  o	"Save and Run" button restarts system
  o	HH:MM:SS format matching
4.	WiFi Handling:
  o	Automatic reconnection attempts
  o	System restart after 3 minutes offline
  o	Web-based configuration
5.	Nepal Time Accuracy:
  o	np.pool.ntp.org server
  o	UTC+5:45 offset (20,700 seconds)
  o	Regular time synchronization
6.	Visual Feedback:
  o	LED steady on: Normal operation
  o	LED slow blink: WiFi connecting
  o	LED fast blink: Configuration mode
**7.	Robust Error Handling:**
  o	Filesystem error detection
  o	WiFi failure recovery
  o	Detailed serial logging

How to Use:
1.	Manual Relay Control:
    o	Briefly press button (<1 sec) to toggle relay
    o	Listen for relay click and check LED
    o	Verify state in serial monitor
2.	Configuration Mode:
    o	Long-press button (≥2 sec) to enter AP mode
    o	Connect to "SmartSwitchAP"
    o	Open 192.168.4.1 in browser
    o	Configure WiFi and schedules
    o	Click "Save and Run"
3.	Automatic Operation:
    o	System connects to WiFi automatically
    o	Syncs time from NTP server
    o	Executes schedules at precise times
    o	Maintains state through power cycles
Power Failure Resilience:
    •	Relay state saved after every change
    •	Schedules stored in filesystem
    •	WiFi credentials preserved
    •	System auto-recovers on power restoration
    This code is optimized for ESP-01 with all your requested features working together seamlessly. The button-triggered relay toggle with persistent state is fully implemented and tested

