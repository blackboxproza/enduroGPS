ProXXI_EnduroGPS_V2/
├── 📁 connectivity/               (WiFi & File Transfer)
│   ├── connect_mode.cpp          • Connect mode state machine
│   ├── connect_mode.h            • Connect mode interface
│   ├── web_server.cpp            • HTTP server for file transfer
│   ├── web_server.h              • Web server interface
│   ├── wifi_manager.cpp          • WiFi hotspot management
│   └── wifi_manager.h            • WiFi manager interface
├── 📁 data/
│   ├── index.html                • Main HTML page
│   ├── script.js                 • JavaScript functionality
│   └── style.css                 • CSS styling
├── 📁 display/
│   ├── display_manager.cpp       • Display management
│   ├── display_manager.h         • Display manager interface
│   ├── map_display.cpp           • Map rendering
│   ├── map_display.h             • Map display interface
│   ├── ui_renderer.cpp           • UI rendering
│   └── ui_renderer.h             • UI renderer interface
├── 📁 input/
│   ├── button_manager.cpp        • Button handling
│   ├── button_manager.h          • Button manager interface
│   ├── menu_system.cpp           • Menu system logic
│   ├── menu_system.h             • Menu system interface
│   ├── zoom_controller.cpp       • Zoom control
│   └── zoom_controller.h         • Zoom controller interface
├── 📁 libraries_config/
│   ├── User_Setup.h              • User setup configuration
├── 📁 navigation/
│   ├── compass_manager.cpp       • Compass handling
│   ├── compass_manager.h         • Compass manager interface
│   ├── gps_manager.cpp           • GPS data management
│   ├── gps_manager.h             • GPS manager interface
│   ├── gpx_parser.cpp            • GPX file parsing
│   ├── gpx_parser.h              • GPX parser interface
│   ├── loop_calibration.cpp      • Calibration loop
│   ├── loop_calibration.h        • Calibration loop interface
│   ├── navigation_core.cpp       • Navigation core logic
│   ├── navigation_core.h         • Navigation core interface
│   ├── smart_tracker.cpp         • Smart tracking
│   └── smart_tracker.h           • Smart tracker interface
├── 📁 power/
│   ├── battery_monitor.cpp       • Battery monitoring
│   ├── battery_monitor.h         • Battery monitor interface
│   ├── power_manager.cpp         • Power management
│   └── power_manager.h           • Power manager interface
├── 📁 storage/
│   ├── sd_manager.cpp            • SD card management
│   ├── sd_manager.h              • SD card manager interface
│   ├── settings_manager.cpp      • Settings management
│   ├── settings_manager.h        • Settings manager interface
│   ├── track_management.cpp      • Track management
│   ├── track_management.h        • Track management interface
── config.h                  • Storage configuration
── file_structure_SCHEME.md  • File structure documentation
── ProXXI_EnduroGPS_V2.ino   • Main Arduino sketch
── shared_data.h             • Shared data definitions