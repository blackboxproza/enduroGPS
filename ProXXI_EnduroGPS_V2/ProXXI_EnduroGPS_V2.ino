/*
 * E_NAVI EnduroGPS V2 - Professional GPS Navigation System
 * By ProXXi Pty Ltd - "Lost, Never Again"
 * 
 * COMPLETE PROFESSIONAL GPS SYSTEM WITH WIFI FILE TRANSFER
 * Features: Connect Mode, QR Code, File Upload/Download, Re-indexing
 * 
 * USER FLOW:
 * 1. Menu -> Connect -> WiFi Hotspot + QR Code
 * 2. User connects phone/laptop to WiFi
 * 3. Upload/Download GPX files via web interface
 * 4. User selects "Done" -> File re-indexing
 * 5. Back to normal GPS operation
 * 
 * FIXED FOR V2 COMPATIBILITY!
 */

// ========================================
// INCLUDES - ALL SYSTEMS + CONNECTIVITY
// ========================================
#include "config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Shared Data & Configuration
#include "shared_data.h"
#include "User_Setup.h"

// Core Systems
#include "display_manager.h"
#include "map_display.h"
#include "ui_renderer.h"

// Input & Interface Systems  
#include "button_manager.h"
#include "menu_system.h"
#include "zoom_controller.h"

// Storage Systems
#include "sd_manager.h"
#include "track_management.h"
#include "settings_manager.h"

// Navigation Systems
#include "gps_manager.h"
#include "compass_manager.h"
#include "smart_tracker.h"
#include "loop_calibration.h"
#include "gpx_parser.h"
#include "navigation_core.h"

// Power Management
#include "battery_monitor.h"
#include "power_manager.h"

// Connectivity & File Transfer
#include "wifi_manager.h"
#include "web_server.h"
#include "connect_mode.h"

// ========================================
// ENHANCED SYSTEM STATE WITH CONNECT MODE
// ========================================
enum SystemState {
  STATE_BOOTING,
  STATE_READY,
  STATE_MENU,
  STATE_CALIBRATING,
  STATE_TRACK_MANAGER,
  STATE_CONNECTING,          // NEW: Connect mode active
  STATE_SLEEPING,
  STATE_ERROR
};

SystemState currentState = STATE_BOOTING;
SystemState previousState = STATE_BOOTING;

extern SystemStatus systemStatus;

// Timing
unsigned long bootStartTime = 0;
unsigned long lastUIUpdate = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastSystemCheck = 0;

// Enhanced Boot Progress with WiFi
struct BootProgress {
  bool displayInit = false;
  bool sdCardInit = false;
  bool buttonsInit = false;
  bool settingsLoaded = false;
  bool menuInit = false;
  bool trackManagerInit = false;
  bool wifiInit = false;         // NEW
  bool webServerInit = false;    // NEW
  bool compassInit = false;
  bool gpsStarted = false;
  bool systemReady = false;
  float progressPercent = 0.0;
  String currentMessage = "Starting...";
} bootStatus;

// UI State
int currentZoom = ZOOM_DEFAULT;
unsigned long autoZoomDisabledUntil = 0;
bool menuVisible = false;
bool calibrationActive = false;
bool trackManagerActive = false;
bool connectModeActive = false;    // NEW

// Status Messages
String statusMessage = "";
unsigned long statusMessageTime = 0;
bool statusMessageVisible = false;

// ========================================
// FORWARD DECLARATIONS - FIXED FOR V2
// ========================================
// Boot Functions
void showBootSplash();
void updateBootProgress(const String& message, float progress);
void showSystemReady();
void showCriticalError(const String& error);
bool startCore1Tasks();
bool initializeAllSystems();
bool initializeWiFiSystems();     // NEW

// Display Functions
void updateMainDisplay();
void updateMenuDisplay();
void updateCalibrationDisplay();
void updateTrackManagerDisplay();
void updateConnectModeDisplay();   // NEW
void drawStatusBar(const GPSData& gps, const SystemStatus& system);
void drawMainContent(const GPSData& gps, const CompassData& compass, const TrackingData& track);
void drawBottomBar(const GPSData& gps, const CompassData& compass);
void initializeMainUI();

// Button Handling - FIXED API
void handleButtonEvents();
void handleMainUIButtons(ButtonID button, ButtonEvent event);
void handleMenuButtons(ButtonID button, ButtonEvent event);
void handleCalibrationButtons(ButtonID button, ButtonEvent event);
void handleTrackManagerButtons(ButtonID button, ButtonEvent event);
void handleConnectModeButtons(ButtonID button, ButtonEvent event);  // NEW
void handleGlobalButtons(ButtonID button, ButtonEvent event);

// State Management
void enterMenuMode();
void exitMenu();
void enterCalibrationMode();
void exitCalibration();
void enterTrackManagerMode();
void exitTrackManager();
void enterConnectMode();          // NEW
void exitConnectMode();           // NEW
void changeState(SystemState newState);

// UI Actions
void toggleTrackRecording();
void createWaypointAtCurrentPosition();
void adjustZoom(int direction);
void showStatusMessage(const String& message, unsigned long duration = 3000);
void clearStatusMessage();

// System Management
void handleSystemStates();
void updateSystemStatus();
void checkSleepConditions();
void enterSleepMode();
void wakeFromSleep();
void handleSystemError();

// Utility Functions
float getBatteryVoltage();
int getBatteryPercentage(float voltage);
void updateBatteryStatus();
void updatePerformanceMetrics();
void performHealthCheck();
bool isSystemHealthy();

// Information Functions
void printSystemInfo();
void runSystemDiagnostics();
String formatTime(int hour, int minute, int second, bool use24Hour);
String formatLatitude(double lat, int precision);
String formatLongitude(double lon, int precision);
String getCardinalFromHeading(float heading);

// Menu Integration Functions (Enhanced)
void initializeMenuSystem();
void handleMenuSelection(int menuItem);
void handleConnectMenuSelection();    // NEW

// ========================================
// SETTINGS FUNCTIONS (Missing Declarations)
// ========================================
bool loadUserSettings();                     // Load user preferences from storage
bool saveUserSettings();                     // Save user preferences to storage
bool hasUnsavedSettings();                   // Check if settings need saving

// ========================================
// CALIBRATION FUNCTIONS (Missing Declarations)  
// ========================================
bool startLoopCalibration();                 // Start compass calibration
void updateLoopCalibration();                // Update calibration process
bool isCalibrationActive();                  // Check if calibration is running
void handleCalibrationInput(ButtonID button, ButtonEvent event); // Handle calibration input
float getCalibrationQuality();               // Get calibration quality score

// ========================================
// ENHANCED BOOT SEQUENCE WITH WIFI SYSTEMS
// ========================================
bool initializeAllSystems() {
  // Initialize shared data structures FIRST
  Serial.println("Boot: Initializing shared data...");
  if (!initializeSharedData()) {
    Serial.println("CRITICAL: Shared data initialization failed!");
    return false;
  }
  
  // STEP 1: Display (0.3 seconds) - HIGHEST PRIORITY
  Serial.println("Boot: Display initializing...");
  updateBootProgress("Initializing Display...", 8.0);
  
  if (display.initialize()) {
    bootStatus.displayInit = true;
    showBootSplash();
    updateBootProgress("Display Ready", 12.0);
    Serial.println("Boot: Display ready");
  } else {
    Serial.println("ERROR: Display initialization failed!");
    return false;
  }
  
  // STEP 2: SD Card (0.4 seconds) - ESSENTIAL FOR OPERATION
  updateBootProgress("Initializing Storage...", 20.0);
  if (sdManager.initialize()) {
    bootStatus.sdCardInit = true;
    updateBootProgress("Storage Ready", 25.0);
    Serial.println("Boot: SD Card ready");
  } else {
    Serial.println("WARNING: SD Card failed - continuing anyway");
    updateBootProgress("Storage Failed - Continuing", 25.0);
    delay(500);
  }
  
  // STEP 3: Buttons (0.2 seconds) - USER INTERACTION
  updateBootProgress("Initializing Controls...", 35.0);
  if (buttons.initialize()) {
    bootStatus.buttonsInit = true;
    updateBootProgress("Controls Ready", 40.0);
    Serial.println("Boot: Buttons ready");
  } else {
    Serial.println("ERROR: Button initialization failed!");
    return false;
  }
  
  // STEP 4: Settings (0.3 seconds) - USER PREFERENCES
  updateBootProgress("Loading Settings...", 45.0);
  if (loadUserSettings()) {
    bootStatus.settingsLoaded = true;
    updateBootProgress("Settings Loaded", 50.0);
    Serial.println("Boot: Settings loaded");
  } else {
    Serial.println("WARNING: Settings load failed - using defaults");
    updateBootProgress("Using Default Settings", 50.0);
  }
  
  // STEP 5: Menu System (0.2 seconds)
  updateBootProgress("Initializing Menu System...", 55.0);
  if (menuManager.initialize()) {
    bootStatus.menuInit = true;
    updateBootProgress("Menu System Ready", 60.0);
    Serial.println("Boot: Menu system ready");
  } else {
    Serial.println("ERROR: Menu system failed!");
    return false;
  }
  
  // STEP 6: Track Manager (0.3 seconds)
  updateBootProgress("Initializing Track Manager...", 65.0);
  if (trackManager.initialize()) {
    bootStatus.trackManagerInit = true;
    updateBootProgress("Track Manager Ready", 70.0);
    Serial.println("Boot: Track manager ready");
  } else {
    Serial.println("WARNING: Track manager failed - continuing anyway");
    updateBootProgress("Track Manager Failed", 70.0);
  }
  
  // STEP 7: WiFi Systems (0.3 seconds) - NEW
  updateBootProgress("Initializing WiFi Systems...", 75.0);
  if (initializeWiFiSystems()) {
    bootStatus.wifiInit = true;
    bootStatus.webServerInit = true;
    updateBootProgress("WiFi Systems Ready", 80.0);
    Serial.println("Boot: WiFi systems ready");
  } else {
    Serial.println("WARNING: WiFi systems failed - continuing anyway");
    updateBootProgress("WiFi Failed - Continuing", 80.0);
  }
  
  // STEP 8: Start Core 1 Tasks (0.5 seconds) - SENSOR PROCESSING
  updateBootProgress("Starting Navigation...", 85.0);
  if (startCore1Tasks()) {
    bootStatus.gpsStarted = true;
    bootStatus.compassInit = true;
    updateBootProgress("Navigation Started", 95.0);
    Serial.println("Boot: Core 1 tasks started");
  } else {
    Serial.println("ERROR: Core 1 task startup failed!");
    updateBootProgress("Navigation Failed", 95.0);
  }
  
  // STEP 9: System Ready (0.2 seconds)
  updateBootProgress("System Ready!", 100.0);
  bootStatus.systemReady = true;
  
  return true;
}

bool initializeWiFiSystems() {
  // Initialize WiFi manager (but don't start hotspot yet)
  if (!wifiManager.initialize()) {
    Serial.println("WiFi: Manager initialization failed");
    return false;
  }
  
  // Initialize web server (but don't start yet)
  if (!webServer.start()) {  // Changed from begin() to start()
    Serial.println("WiFi: Web server start failed");  // Updated message
    return false;
  }
  
  // Initialize connect mode manager
  if (!connectMode.initialize()) {
    Serial.println("WiFi: Connect mode initialization failed");
    return false;
  }
  
  Serial.println("WiFi: All systems initialized (not started)");
  return true;
}

void showBootSplash() {
  display.clearScreen();
  
  // E_NAVI Logo (large, centered)
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(100, "E_NAVI", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // Tagline
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(150, "Lost, Never Again", display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // ProXXi branding
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(180, "by ProXXi Pty Ltd", display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  
  // Version with WiFi capability
  display.drawCenteredText(210, "Professional GPS v2.0 + WiFi", display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  
  // South African focus
  display.drawCenteredText(240, "Designed for SA Enduro Riders", display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  
  // NEW: WiFi file transfer feature highlight
  display.setTextSize(FONT_SIZE_SMALL);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(270, "📱 WiFi File Transfer • 🔄 Smart Tracking", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
  display.drawCenteredText(290, "🧭 Loop-Loop Calibration", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void updateBootProgress(const String& message, float progress) {
  bootStatus.currentMessage = message;
  bootStatus.progressPercent = progress;
  
  if (bootStatus.displayInit) {
    // Clear progress area
    display.fillRect(0, 320, SCREEN_WIDTH, 130, display.getCurrentColors().background);
    
    // Progress message
    display.setTextSize(FONT_SIZE_MEDIUM);
    display.setTextColor(display.getCurrentColors().text);
    display.drawCenteredText(340, message, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
    
    // Progress bar
    int barWidth = 280;
    int barX = (SCREEN_WIDTH - barWidth) / 2;
    int barY = 370;
    
    display.drawRect(barX, barY, barWidth, 20, display.getCurrentColors().text);
    
    int fillWidth = (progress * barWidth) / 100.0;
    display.fillRect(barX + 2, barY + 2, fillWidth - 4, 16, display.getCurrentColors().accent);
    
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawCenteredText(400, String((int)progress) + "%", display.getCurrentColors().text, FONT_SIZE_SMALL);
    
    // Component status indicators (enhanced with WiFi)
    int statusY = 420;
    display.setTextSize(FONT_SIZE_TINY);
    String status = "";
    if (bootStatus.displayInit) status += "DISP ";
    if (bootStatus.sdCardInit) status += "SD ";
    if (bootStatus.buttonsInit) status += "BTN ";
    if (bootStatus.menuInit) status += "MENU ";
    if (bootStatus.trackManagerInit) status += "TRACK ";
    if (bootStatus.wifiInit) status += "WIFI ";
    if (bootStatus.webServerInit) status += "WEB ";
    if (bootStatus.compassInit) status += "COMP ";
    if (bootStatus.gpsStarted) status += "GPS ";
    
    display.drawCenteredText(statusY, status, display.getCurrentColors().textSecondary, FONT_SIZE_TINY);
  }
  
  Serial.printf("Boot: %s (%.1f%%)\n", message.c_str(), progress);
}

// ========================================
// FIXED BUTTON HANDLING WITH PROPER API
// ========================================
void handleButtonEvents() {
  buttons.update();
  
  // Use the actual API from button_manager.h
  if (buttons.hasButtonEvent()) {
    ButtonID button;
    ButtonEvent event = buttons.getButtonEvent(button);
    
    switch (currentState) {
      case STATE_READY:
        handleMainUIButtons(button, event);
        break;
      case STATE_MENU:
        handleMenuButtons(button, event);
        break;
      case STATE_CALIBRATING:
        handleCalibrationButtons(button, event);
        break;
      case STATE_TRACK_MANAGER:
        handleTrackManagerButtons(button, event);
        break;
      case STATE_CONNECTING:
        handleConnectModeButtons(button, event);
        break;
      case STATE_SLEEPING:
        if (button == BTN_CENTER && event == BTN_EVENT_PRESS) {
          wakeFromSleep();
        }
        break;
      default:
        handleGlobalButtons(button, event);
        break;
    }
  }
}

void handleMainUIButtons(ButtonID button, ButtonEvent event) {
  switch (button) {
    case BTN_CENTER:
      if (event == BTN_EVENT_CLICK) {
        enterMenuMode();
      } else if (event == BTN_EVENT_LONG_PRESS) {
        toggleTrackRecording();
      }
      break;
      
    case BTN_ZOOM_IN:
      if (event == BTN_EVENT_CLICK) {
        adjustZoom(1);
        showStatusMessage("🔍 Zoom In: Level " + String(currentZoom));
      }
      break;
      
    case BTN_ZOOM_OUT:
      if (event == BTN_EVENT_CLICK) {
        adjustZoom(-1);
        showStatusMessage("🔍 Zoom Out: Level " + String(currentZoom));
      }
      break;
      
    case BTN_WAYPOINT:
      if (event == BTN_EVENT_CLICK) {
        createWaypointAtCurrentPosition();
      }
      break;
      
    case BTN_UP:
    case BTN_DOWN:
    case BTN_LEFT:
    case BTN_RIGHT:
      // Future: Map panning
      break;
  }
}

void handleConnectModeButtons(ButtonID button, ButtonEvent event) {
  // Pass input to connect mode manager
  connectMode.handleInput(button, event);
}

// ========================================
// STATE MANAGEMENT - ENHANCED WITH CONNECT MODE
// ========================================
void changeState(SystemState newState) {
  previousState = currentState;
  currentState = newState;
  
  Serial.printf("State: %d -> %d\n", previousState, currentState);
  
  // State entry actions
  switch (newState) {
    case STATE_MENU:
      menuVisible = true;
      showStatusMessage("📋 Menu Active");
      break;
      
    case STATE_CALIBRATING:
      calibrationActive = true;
      showStatusMessage("🧭 Compass Calibration");
      break;
      
    case STATE_TRACK_MANAGER:
      trackManagerActive = true;
      showStatusMessage("📊 Track Manager");
      break;
      
    case STATE_CONNECTING:
      connectModeActive = true;
      showStatusMessage("📱 Connect Mode Active");
      break;
      
    case STATE_READY:
      menuVisible = false;
      calibrationActive = false;
      trackManagerActive = false;
      connectModeActive = false;
      clearStatusMessage();
      break;
  }
}

void enterConnectMode() {
  changeState(STATE_CONNECTING);
  
  // Start connect mode
  if (connectMode.startConnectMode()) {
    Serial.println("UI: Entered connect mode successfully");
    showStatusMessage("📱 Starting WiFi Hotspot...", 3000);
  } else {
    Serial.println("ERROR: Failed to start connect mode");
    showStatusMessage("❌ WiFi Failed to Start", 3000);
    exitConnectMode();
  }
}

void exitConnectMode() {
  // Stop connect mode services
  connectMode.stopConnectMode();
  
  // Check if files were added/changed
  if (connectMode.hasNewFiles()) {
    showStatusMessage("🔄 Updating file index...", 2000);
    delay(2000);
    
    // Trigger file re-indexing
    trackManager.refreshTrackList();
    
    int newFiles = connectMode.getNewFileCount();
    showStatusMessage("✅ Found " + String(newFiles) + " new files!", 3000);
    delay(3000);
  }
  
  changeState(STATE_READY);
  Serial.println("UI: Exited connect mode");
}

// ========================================
// ENHANCED DISPLAY SYSTEM WITH CONNECT MODE
// ========================================
void handleSystemStates() {
  switch (currentState) {
    case STATE_READY:
      // Normal operation
      break;
      
    case STATE_MENU:
      updateMenuDisplay();
      menuManager.update();
      
      // Check for menu exit
      if (!menuManager.isActive()) {
        exitMenu();
      }
      break;
      
    case STATE_CALIBRATING:
      updateCalibrationDisplay();
      updateLoopCalibration();
      
      // Check for calibration completion
      if (!isCalibrationActive()) {
        exitCalibration();
        
        float quality = getCalibrationQuality();
        if (quality > 70.0) {
          showStatusMessage("✅ Calibration Complete (" + 
                           String((int)quality) + "%)", 3000);
        } else {
          showStatusMessage("❌ Calibration Failed", 3000);
        }
      }
      break;
      
    case STATE_TRACK_MANAGER:
      updateTrackManagerDisplay();
      // Track UI update logic here
      break;
      
    case STATE_CONNECTING:
      updateConnectModeDisplay();
      connectMode.update();
      
      // Check for connect mode completion
      if (!connectMode.isActive()) {
        exitConnectMode();
      }
      break;
      
    case STATE_ERROR:
      handleSystemError();
      break;
  }
}

void updateConnectModeDisplay() {
  // Connect mode handles its own display updates
  // This function can add system status overlays
  
  if (connectModeActive) {
    // Show WiFi status in corner
    if (wifiManager.isActive()) {
      display.setTextSize(FONT_SIZE_TINY);
      display.setTextColor(display.getCurrentColors().recording);
      display.setCursor(5, 5);
      display.printf("WiFi: %d clients", wifiManager.getConnectedClients());
    }
    
    // Show transfer status if active
    if (webServer.isRunning()) {
      display.setTextSize(FONT_SIZE_TINY);
      display.setTextColor(display.getCurrentColors().accent);
      display.setCursor(5, 20);
      display.print("Web Server Running");
    }
  }
}

// ========================================
// ENHANCED MENU INTEGRATION WITH CONNECT OPTION
// ========================================
void handleMenuButtons(ButtonID button, ButtonEvent event) {
  // Standard menu handling
  menuManager.handleInput(button, event);
  
  // Check for connect mode selection
  if (button == BTN_CENTER && event == BTN_EVENT_CLICK) {
    // If user selected "Connect" option in menu
    if (menuManager.getCurrentMenu() == MENU_MAIN && 
        menuManager.getSelectedItem() == MAIN_CONNECT) {
      enterConnectMode();
    }
  }
}

// ========================================
// ENHANCED UI ACTIONS - FIXED API CALLS
// ========================================
void toggleTrackRecording() {
  TrackingData track = getTrackingData();
  
  if (track.isRecording) {
    smartTracker.pauseTracking();  // FIXED: use correct method name
    showStatusMessage("🛑 Recording Paused", 2000);
    Serial.println("Track: Recording paused");
  } else {
    if (track.isPaused) {  // FIXED: use correct member name
      smartTracker.resumeTracking();  // FIXED: use correct method name
      showStatusMessage("▶️ Recording Resumed", 2000);
      Serial.println("Track: Recording resumed");
    } else {
      smartTracker.startTracking();  // FIXED: use correct method name
      showStatusMessage("🔴 Recording Started", 2000);
      Serial.println("Track: Recording started");
    }
  }
}

void createWaypointAtCurrentPosition() {
  GPSData gps = getGPSData();
  if (gps.isValid) {
    String waypointName = "WPT_" + String(millis());
    // Create waypoint logic here
    showStatusMessage("📍 Waypoint Created", 2000);
    Serial.println("Waypoint: Created at current position");
  } else {
    showStatusMessage("❌ No GPS Fix", 2000);
    Serial.println("Waypoint: Cannot create - no GPS fix");
  }
}

void showStatusMessage(const String& message, unsigned long duration) {
  statusMessage = message;
  statusMessageTime = millis();
  statusMessageVisible = true;
  
  // Also show in menu system if active
  if (currentState == STATE_MENU) {
    menuManager.showMessage(message, duration);
  }
}

// ========================================
// FIXED SYSTEM STATUS WITH PROPER API
// ========================================
void updateSystemStatus() {
  SystemStatus status = getSystemStatus();
  
  // Update battery voltage
  status.batteryVoltage = getBatteryVoltage();
  status.batteryLevel = getBatteryPercentage(status.batteryVoltage);
  
  // Update system health flags - FIXED API calls
  status.gpsInitialized = gpsManager.isInitialized();  // FIXED: use correct member
  status.compassInitialized = compassManager.isInitialized();  // FIXED: use correct member
  status.sdCardSize = sdManager.getCardSize();  // FIXED: use existing member
  status.buttonsInitialized = buttons.isInitialized();  // FIXED: use correct member
  status.displayInitialized = display.isInitialized();  // FIXED: use correct member
  
  // Update WiFi status
  status.wifiEnabled = wifiManager.isActive();
  status.wifiClients = wifiManager.getConnectedClients();
  status.webServerActive = webServer.isRunning();  // FIXED: use correct method
  
  // Update performance metrics
  status.uptime = (millis() - bootStartTime) / 1000;
  status.freeHeap = ESP.getFreeHeap();
  status.freePSRAM = ESP.getFreePsram();
  
  // Check for battery warnings
  if (status.batteryVoltage < BATTERY_LOW_VOLTAGE && !status.lowBattery) {
    status.lowBattery = true;
    showStatusMessage("⚠️ Low Battery: " + String(status.batteryVoltage, 1) + "V", 5000);
    Serial.println("WARNING: Low battery detected");
  }
  
  if (status.batteryVoltage < BATTERY_CRITICAL_VOLTAGE && !status.criticalBattery) {
    status.criticalBattery = true;
    showStatusMessage("🔋 Critical Battery!", 10000);
    Serial.println("CRITICAL: Critical battery level!");
  }
  
  // Update shared data
  updateSystemStatus(status);
}

void checkSleepConditions() {
  CompassData compass = getCompassData();
  
  // Don't sleep during WiFi operations or any active mode
  if (currentState != STATE_READY) {
    return;
  }
  
  // Don't sleep if WiFi is active
  if (wifiManager.isActive()) {
    return;
  }
  
  // Check if no motion for timeout period
  if (!compass.motionDetected && 
      (millis() - compass.lastMotionTime) > MOTION_TIMEOUT_MS) {  // FIXED: use correct member
    
    showStatusMessage("💤 Entering Sleep Mode...", 2000);
    delay(2000);
    Serial.println("System: Entering sleep mode due to inactivity");
    enterSleepMode();
  }
}

// ========================================
// ENHANCED SYSTEM INFORMATION WITH WIFI
// ========================================
void printSystemInfo() {
  Serial.println("========================================");
  Serial.println("E_NAVI System Information V2.0");
  Serial.println("========================================");
  Serial.printf("Hardware: %s\n", HARDWARE_VERSION);
  Serial.printf("Software: %s + WiFi File Transfer\n", SOFTWARE_VERSION);
  Serial.printf("Build: %s %s\n", BUILD_DATE, BUILD_TIME);
  Serial.printf("ESP32 Core: %s\n", ESP.getSdkVersion());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("PSRAM Size: %d bytes\n", ESP.getPsramSize());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
  
  SystemStatus status = getSystemStatus();
  Serial.printf("GPS Status: %s\n", status.gpsInitialized ? "OK" : "FAIL");
  Serial.printf("Compass Status: %s\n", status.compassInitialized ? "OK" : "FAIL");
  Serial.printf("SD Card Size: %lu bytes\n", status.sdCardSize);
  Serial.printf("Display Status: %s\n", status.displayInitialized ? "OK" : "FAIL");
  Serial.printf("Buttons Status: %s\n", status.buttonsInitialized ? "OK" : "FAIL");
  Serial.printf("Menu System: %s\n", menuManager.isActive() ? "ACTIVE" : "READY");
  Serial.printf("Track Manager: %s\n", trackManager.isInitialized() ? "OK" : "FAIL");
  
  // WiFi System Status
  Serial.printf("WiFi Manager: %s\n", wifiManager.isActive() ? "ACTIVE" : "READY");
  Serial.printf("Web Server: %s\n", webServer.isRunning() ? "RUNNING" : "STOPPED");
  Serial.printf("Connect Mode: %s\n", connectMode.isActive() ? "ACTIVE" : "READY");
  if (wifiManager.isActive()) {
    Serial.printf("WiFi SSID: %s\n", wifiManager.getSSID().c_str());
    Serial.printf("WiFi Clients: %d\n", wifiManager.getConnectedClients());
  }
  
  Serial.printf("Battery Voltage: %.2f V (%d%%)\n", status.batteryVoltage, status.batteryLevel);
  Serial.printf("System Uptime: %lu seconds\n", status.uptime);
  
  Serial.println("========================================");
}

void showSystemReady() {
  display.clearScreen();
  
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().recording);
  display.drawCenteredText(120, "SYSTEM READY", display.getCurrentColors().recording, FONT_SIZE_LARGE);
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(160, "All systems operational", display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Show key features including WiFi
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(190, "✅ 10Hz GPS • Motion Tracking • Loop Calibration", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(210, "✅ WiFi File Transfer • Smart Recording", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  
  unsigned long bootTime = millis() - bootStartTime;
  String bootMsg = "Boot time: " + String(bootTime) + "ms";
  display.drawCenteredText(240, bootMsg, display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  
  // Ready for adventure with WiFi capability
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(290, "🏍️ Ready for Adventure! 🗺️", 
                          display.getCurrentColors().accent, FONT_SIZE_MEDIUM);
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(320, "📱 WiFi File Transfer Available", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

// ========================================
// STUB IMPLEMENTATIONS FOR MISSING FUNCTIONS
// ========================================

// Temporary implementations until we fix the other files
bool loadUserSettings() { return true; }
bool saveUserSettings() { return true; }
bool hasUnsavedSettings() { return false; }
bool startLoopCalibration() { return true; }
void updateLoopCalibration() {}
bool isCalibrationActive() { return false; }
void handleCalibrationInput(ButtonID button, ButtonEvent event) {}
float getCalibrationQuality() { return 85.0; }
bool startCore1Tasks() { return true; }
void updateMainDisplay() {}
void updateMenuDisplay() {}
void updateCalibrationDisplay() {}
void updateTrackManagerDisplay() {}
void initializeMainUI() {}
void enterMenuMode() { changeState(STATE_MENU); }
void exitMenu() { changeState(STATE_READY); }
void enterCalibrationMode() { changeState(STATE_CALIBRATING); }
void exitCalibration() { changeState(STATE_READY); }
void enterTrackManagerMode() { changeState(STATE_TRACK_MANAGER); }
void exitTrackManager() { changeState(STATE_READY); }
void adjustZoom(int direction) { currentZoom += direction; }
void clearStatusMessage() { statusMessage = ""; statusMessageVisible = false; }
void handleSystemError() {}
void enterSleepMode() {}
void wakeFromSleep() {}
float getBatteryVoltage() { return 3.7; }
int getBatteryPercentage(float voltage) { return 75; }
void updateBatteryStatus() {}
void updatePerformanceMetrics() {}
void performHealthCheck() {}
bool isSystemHealthy() { return true; }
void runSystemDiagnostics() {}
String formatTime(int hour, int minute, int second, bool use24Hour) { return "12:00:00"; }
String formatLatitude(double lat, int precision) { return "0.0000"; }
String formatLongitude(double lon, int precision) { return "0.0000"; }
String getCardinalFromHeading(float heading) { return "N"; }
void initializeMenuSystem() {}
void handleMenuSelection(int menuItem) {}
void handleConnectMenuSelection() {}
void handleCalibrationButtons(ButtonID button, ButtonEvent event) {}
void handleTrackManagerButtons(ButtonID button, ButtonEvent event) {}
void handleGlobalButtons(ButtonID button, ButtonEvent event) {}
void showCriticalError(const String& error) { Serial.println("CRITICAL: " + error); }


// Simple stub version
bool testCH423Connection() {     
  Serial.println("CH423: Connection test (stub)"); 
  return true; 
}

// ========================================
// SETUP FUNCTION - V2 COMPLETE
// ========================================
void setup() {
  bootStartTime = millis();
  
  // Critical: Initialize Serial for debug output
  Serial.begin(115200);
  Serial.println("========================================");
  Serial.println("E_NAVI EnduroGPS V2.0 - PROFESSIONAL GPS");
  Serial.println("By ProXXi Pty Ltd - \"Lost, Never Again\"");
  Serial.println("Professional GPS + WiFi File Transfer");
  Serial.println("For South African Enduro Riders");
  Serial.println("========================================");
  
  // Initialize all systems including WiFi
  if (!initializeAllSystems()) {
    showCriticalError("System Initialization Failed");
    return;
  }
  
  unsigned long bootTime = millis() - bootStartTime;
  Serial.printf("Boot: Complete in %lu ms\n", bootTime);
  
  // Show system ready screen
  showSystemReady();
  delay(3000);  // Show longer to highlight WiFi feature
  
  // Initialize main UI
  initializeMainUI();
  changeState(STATE_READY);
  
  Serial.println("========================================");
  Serial.println("E_NAVI: Ready for adventure! 🏍️🗺️📱");
  Serial.println("Features: GPS • Compass • Smart Tracking");
  Serial.println("WiFi File Transfer • Loop Calibration");
  Serial.println("Vision-Optimized for 35-65 Year Old Riders");
  Serial.println("========================================");
  
  // Print system information
  printSystemInfo();
  
  // Initial status message
  showStatusMessage("🏍️ Ready to Ride! Press [CENTER] for Menu", 5000);
}

// ========================================
// MAIN LOOP FUNCTION - V2 COMPLETE
// ========================================
void loop() {
  static unsigned long lastDisplayUpdate = 0;
  static unsigned long lastButtonCheck = 0;
  static unsigned long lastSystemCheck = 0;
  
  unsigned long now = millis();
  
  // High-frequency button checking (every 5ms)
  if (now - lastButtonCheck >= BUTTON_CHECK_INTERVAL_MS) {
    handleButtonEvents();
    lastButtonCheck = now;
  }
  
  // Display updates (60fps = 16.67ms)
  if (now - lastDisplayUpdate >= UI_REFRESH_INTERVAL_MS) {
    switch (currentState) {
      case STATE_READY:
        updateMainDisplay();
        break;
      case STATE_MENU:
      case STATE_CALIBRATING:
      case STATE_TRACK_MANAGER:
      case STATE_CONNECTING:
        // These states handle their own display updates
        break;
    }
    lastDisplayUpdate = now;
  }
  
  // System management (every 100ms)
  if (now - lastSystemCheck >= SYSTEM_CHECK_INTERVAL_MS) {
    updateSystemStatus();
    checkSleepConditions();
    updateBatteryStatus();
    updatePerformanceMetrics();
    performHealthCheck();
    lastSystemCheck = now;
  }
  
  // Handle system state transitions
  handleSystemStates();
  
  // WiFi services (when active)
  if (wifiManager.isActive()) {
    wifiManager.update();
    webServer.handleClient();  // FIXED: use correct method name
  }
  
  // Small delay to prevent watchdog triggers
  vTaskDelay(pdMS_TO_TICKS(1));
}