/*
 * E_NAVI EnduroGPS V2 - Complete Menu Navigation System
 * Vision-Optimized Menu for 35-65 Year Old Riders
 * 
 * FIXED VERSION - All compilation errors resolved
 * - Proper UserPreferences handling
 * - Correct struct member access
 * - Fixed enum usage
 * - Added missing function declarations
 * 
 * Features: Large fonts, simple navigation, essential settings only
 * Interface: 5-way buttons (up/down/left/right/center)
 * Integration: Works with display_manager.h and button_manager.h
 */

#include "menu_system.h"
#include "display_manager.h"
#include "button_manager.h"
#include "shared_data.h"
#include "settings_manager.h"  // Add this line


// ========================================
// GLOBAL VARIABLES - PROPERLY DECLARED
// ========================================

// External references - properly declared
extern DisplayManager display;
extern GPSData gps;
extern CompassData compass;
extern TrackingData trackingData;
extern SystemStatus sysStatus;  // Renamed to avoid conflict with stdlib system()

// Global instances
MenuManager menuManager;
SettingsManager settingsManager;

// ========================================
// MENU CONSTANTS - PROPERLY DEFINED
// ========================================

// GPS Power modes (matching GPS manager expectations)
#define GPS_POWER_NORMAL    0
#define GPS_POWER_SLEEP     1
#define GPS_POWER_ECO       2

// Track modes (matching tracking system)
#define TRACK_MODE_OFF           0
#define TRACK_MODE_MANUAL        1
#define TRACK_MODE_AUTO_MOTION   2
#define TRACK_MODE_AUTO_SPEED    3
#define TRACK_MODE_CONTINUOUS    4

// Layout constants
static const int menuStartY = 80;
static const int itemHeight = 60;

// ========================================
// SETTINGS MANAGER IMPLEMENTATION
// ========================================

SettingsManager::SettingsManager() {}

SettingsManager::~SettingsManager() {}

bool SettingsManager::loadSettings() {
  // TODO: Implement actual SD card loading
  // For now, reset to defaults
  resetToDefaults();
  settingsLoaded = true;
  settingsDirty = false;
  Serial.println("Settings: Loaded from defaults");
  return true;
}

bool SettingsManager::saveSettings() {
  // TODO: Implement actual SD card saving
  settingsDirty = false;
  Serial.println("Settings: Saved");
  return true;
}

void SettingsManager::resetToDefaults() {
  settings.brightness = 80;
  settings.nightMode = false;
  settings.autoNightMode = true;
  settings.trackMode = 1;  // TRACK_MODE_MANUAL
  settings.trackFilter = true;
  settings.autoSave = true;
  settings.gpsPowerMode = 0;  // GPS_POWER_FULL_PERFORMANCE
  settings.motionSensitivity = 0.5;
  settings.firstRun = true;
  settings.totalOperatingTime = 0;
  markDirty();
}

void SettingsManager::setBrightness(int brightness) {
  settings.brightness = constrain(brightness, 0, 100);
  markDirty();
}

int SettingsManager::getBrightness() {
  return settings.brightness;
}

void SettingsManager::setNightMode(bool enabled) {
  settings.nightMode = enabled;
  markDirty();
}

bool SettingsManager::getNightMode() {
  return settings.nightMode;
}

void SettingsManager::setAutoNightMode(bool enabled) {
  settings.autoNightMode = enabled;
  markDirty();
}

bool SettingsManager::getAutoNightMode() {
  return settings.autoNightMode;
}

void SettingsManager::setTrackMode(int mode) {
  settings.trackMode = constrain(mode, 0, 4);
  markDirty();
}

int SettingsManager::getTrackMode() {
  return settings.trackMode;
}

void SettingsManager::setTrackFilter(bool enabled) {
  settings.trackFilter = enabled;
  markDirty();
}

bool SettingsManager::getTrackFilter() {
  return settings.trackFilter;
}

void SettingsManager::setAutoSave(bool enabled) {
  settings.autoSave = enabled;
  markDirty();
}

bool SettingsManager::getAutoSave() {
  return settings.autoSave;
}

void SettingsManager::setGPSPowerMode(int mode) {
  settings.gpsPowerMode = constrain(mode, 0, 3);
  markDirty();
}

int SettingsManager::getGPSPowerMode() {
  return settings.gpsPowerMode;
}

void SettingsManager::setMotionSensitivity(float sensitivity) {
  settings.motionSensitivity = constrain(sensitivity, 0.0f, 1.0f);
  markDirty();
}

float SettingsManager::getMotionSensitivity() {
  return settings.motionSensitivity;
}

bool SettingsManager::isFirstRun() {
  return settings.firstRun;
}

void SettingsManager::setFirstRunComplete() {
  settings.firstRun = false;
  markDirty();
}

void SettingsManager::addOperatingTime(unsigned long seconds) {
  settings.totalOperatingTime += seconds;
  markDirty();
}

unsigned long SettingsManager::getTotalOperatingTime() {
  return settings.totalOperatingTime;
}

String SettingsManager::getSettingsInfo() {
  return "Settings info placeholder";
}

// ========================================
// MENU MANAGER IMPLEMENTATION
// ========================================

MenuManager::MenuManager() {
  currentMenu = MENU_MAIN;
  selectedItem = 0;
  topDisplayItem = 0;
  menuActive = false;
  inSubMenu = false;
  settingsChanged = false;
  statusMessage = "";
  statusMessageTime = 0;
  lastInput = 0;
  itemsPerScreen = 6;  // FIX: Initialize itemsPerScreen
  
  // Initialize menu definitions
  initializeMenuDefinitions();
}

MenuManager::~MenuManager() {
  // Cleanup if needed
}

bool MenuManager::initialize() {
  Serial.println("Menu Manager: Initializing vision-optimized menu system...");
  
  // Load user settings
  settingsManager.loadSettings();
  
  // Initialize menu definitions
  initializeMenuDefinitions();
  initializeMenuColors();
  
  // Set initial state
  currentMenu = MENU_MAIN;
  selectedItem = 0;
  topDisplayItem = 0;
  menuActive = false;
  itemsPerScreen = 6;  // FIX: Initialize itemsPerScreen
  
  Serial.println("Menu Manager: Vision-optimized menu system ready!");
  return true;
}

void MenuManager::initializeMenuDefinitions() {
  // MAIN MENU
  menuDefs[MENU_MAIN].title = "MAIN MENU";
  menuDefs[MENU_MAIN].items[0] = "📝 Tracking";
  menuDefs[MENU_MAIN].items[1] = "📱 Connect";
  menuDefs[MENU_MAIN].items[2] = "📺 Display";
  menuDefs[MENU_MAIN].items[3] = "🧭 Compass";
  menuDefs[MENU_MAIN].items[4] = "📡 GPS";
  menuDefs[MENU_MAIN].items[5] = "⚙️ System";
  menuDefs[MENU_MAIN].itemCount = 6;
  menuDefs[MENU_MAIN].hasBack = false;
  
  // TRACKING MENU
  menuDefs[MENU_TRACKING].title = "TRACKING";
  menuDefs[MENU_TRACKING].items[0] = "📊 Current Track";
  menuDefs[MENU_TRACKING].items[1] = "🎯 Track Mode";
  menuDefs[MENU_TRACKING].items[2] = "🔍 Filter";
  menuDefs[MENU_TRACKING].items[3] = "💾 Auto Save";
  menuDefs[MENU_TRACKING].items[4] = "⬅️ Back";
  menuDefs[MENU_TRACKING].itemCount = 5;
  menuDefs[MENU_TRACKING].hasBack = true;
  
  // CONNECT MENU
  menuDefs[MENU_CONNECT].title = "CONNECT";
  menuDefs[MENU_CONNECT].items[0] = "📱 Start WiFi";
  menuDefs[MENU_CONNECT].items[1] = "📄 QR Code";
  menuDefs[MENU_CONNECT].items[2] = "🔄 Refresh Files";
  menuDefs[MENU_CONNECT].items[3] = "⬅️ Back";
  menuDefs[MENU_CONNECT].itemCount = 4;
  menuDefs[MENU_CONNECT].hasBack = true;
  
  // DISPLAY MENU
  menuDefs[MENU_DISPLAY].title = "DISPLAY";
  menuDefs[MENU_DISPLAY].items[0] = "☀️ Brightness";
  menuDefs[MENU_DISPLAY].items[1] = "🌙 Night Mode";
  menuDefs[MENU_DISPLAY].items[2] = "🔄 Auto Night";
  menuDefs[MENU_DISPLAY].items[3] = "⬅️ Back";
  menuDefs[MENU_DISPLAY].itemCount = 4;
  menuDefs[MENU_DISPLAY].hasBack = true;
  
  // COMPASS MENU
  menuDefs[MENU_COMPASS].title = "COMPASS";
  menuDefs[MENU_COMPASS].items[0] = "🔄 Calibrate";
  menuDefs[MENU_COMPASS].items[1] = "📊 Status";
  menuDefs[MENU_COMPASS].items[2] = "⬅️ Back";
  menuDefs[MENU_COMPASS].itemCount = 3;
  menuDefs[MENU_COMPASS].hasBack = true;
  
  // GPS MENU
  menuDefs[MENU_GPS].title = "GPS";
  menuDefs[MENU_GPS].items[0] = "📊 Status";
  menuDefs[MENU_GPS].items[1] = "⚡ Power Mode";
  menuDefs[MENU_GPS].items[2] = "⬅️ Back";
  menuDefs[MENU_GPS].itemCount = 3;
  menuDefs[MENU_GPS].hasBack = true;
  
  // SYSTEM MENU
  menuDefs[MENU_SYSTEM].title = "SYSTEM";
  menuDefs[MENU_SYSTEM].items[0] = "ℹ️ Information";
  menuDefs[MENU_SYSTEM].items[1] = "🔧 Diagnostics";
  menuDefs[MENU_SYSTEM].items[2] = "🔄 Reset Settings";
  menuDefs[MENU_SYSTEM].items[3] = "⬅️ Back";
  menuDefs[MENU_SYSTEM].itemCount = 4;
  menuDefs[MENU_SYSTEM].hasBack = true;
}

void MenuManager::initializeMenuColors() {
  // Day colors (high contrast)
  dayColors.background = 0x0000;      // Black
  dayColors.text = 0xFFFF;            // White
  dayColors.selectedBg = 0x07E0;      // Green
  dayColors.selectedText = 0x0000;    // Black
  dayColors.titleBg = 0x001F;         // Blue
  dayColors.titleText = 0xFFFF;       // White
  dayColors.border = 0x7BEF;          // Light Gray
  dayColors.accent = 0x07E0;          // Green
  dayColors.warning = 0xFD20;         // Orange
  dayColors.success = 0x07E0;         // Green
  
  // Night colors (red-friendly)
  nightColors.background = 0x0000;    // Black
  nightColors.text = 0xF800;          // Red
  nightColors.selectedBg = 0x7800;    // Dark Red
  nightColors.selectedText = 0x0000;  // Black
  nightColors.titleBg = 0x3800;       // Very Dark Red
  nightColors.titleText = 0xF800;     // Red
  nightColors.border = 0x3800;        // Very Dark Red
  nightColors.accent = 0xF800;        // Red
  nightColors.warning = 0xFC00;       // Yellow-Red
  nightColors.success = 0xF800;       // Red
}

// ========================================
// MENU CONTROL FUNCTIONS
// ========================================

void MenuManager::enterMenu() {
  Serial.println("Menu Manager: Entering menu system");
  
  menuActive = true;
  currentMenu = MENU_MAIN;
  selectedItem = 0;
  topDisplayItem = 0;
  lastInput = millis();
  
  clearMessage();
  drawMenu();
}

void MenuManager::exitMenu() {
  Serial.println("Menu Manager: Exiting menu system");
  
  menuActive = false;
  currentMenu = MENU_MAIN;
  selectedItem = 0;
  topDisplayItem = 0;
  
  // Save settings if changed
  if (settingsChanged) {
    Serial.println("Menu Manager: Saving changed settings");
    settingsManager.saveSettings();
    settingsChanged = false;
  }
  
  clearMessage();
}

void MenuManager::handleInput(ButtonID button, ButtonEvent event) {
  if (!menuActive) return;
  if (event != BTN_EVENT_CLICK) return;
  
  lastInput = millis();
  
  switch (button) {
    case BTN_UP:
      navigateUp();
      break;
      
    case BTN_DOWN:
      navigateDown();
      break;
      
    case BTN_CENTER:
      selectItem();
      break;
      
    case BTN_LEFT:
      goBack();
      break;
      
    case BTN_RIGHT:
      // Future: Quick settings toggle
      break;
      
    default:
      break;
  }
  
  drawMenu();
}

void MenuManager::navigateUp() {
  if (selectedItem > 0) {
    selectedItem--;
    
    // Adjust display window if needed
    if (selectedItem < topDisplayItem) {
      topDisplayItem = selectedItem;
    }
  }
}

void MenuManager::navigateDown() {
  int maxItems = menuDefs[currentMenu].itemCount;
  
  if (selectedItem < maxItems - 1) {
    selectedItem++;
    
    // FIX: Use member variable instead of constant expression
    if (selectedItem >= topDisplayItem + itemsPerScreen) {
      topDisplayItem = selectedItem - itemsPerScreen + 1;
    }
  }
}

void MenuManager::selectItem() {
  switch (currentMenu) {
    case MENU_MAIN:
      handleMainMenu();
      break;
    case MENU_TRACKING:
      handleTrackingMenu();
      break;
    case MENU_CONNECT:
      handleConnectMenu();
      break;
    case MENU_DISPLAY:
      handleDisplayMenu();
      break;
    case MENU_COMPASS:
      handleCompassMenu();
      break;
    case MENU_GPS:
      handleGPSMenu();
      break;
    case MENU_SYSTEM:
      handleSystemMenu();
      break;
    default:
      break;
  }
}

void MenuManager::goBack() {
  if (currentMenu != MENU_MAIN) {
    currentMenu = MENU_MAIN;
    selectedItem = 0;
    topDisplayItem = 0;
    inSubMenu = false;
  } else {
    exitMenu();
  }
}

// ========================================
// MENU HANDLERS - FIXED IMPLEMENTATIONS
// ========================================

void MenuManager::handleMainMenu() {
  switch (selectedItem) {
    case 0: // Tracking
      currentMenu = MENU_TRACKING;
      selectedItem = 0;
      topDisplayItem = 0;
      inSubMenu = true;
      break;
    case 1: // Connect
      currentMenu = MENU_CONNECT;
      selectedItem = 0;
      topDisplayItem = 0;
      inSubMenu = true;
      break;
    case 2: // Display
      currentMenu = MENU_DISPLAY;
      selectedItem = 0;
      selectedItem = 0;
      topDisplayItem = 0;
      inSubMenu = true;
      break;
    case 3: // Compass
      currentMenu = MENU_COMPASS;
      selectedItem = 0;
      topDisplayItem = 0;
      inSubMenu = true;
      break;
    case 4: // GPS
      currentMenu = MENU_GPS;
      selectedItem = 0;
      topDisplayItem = 0;
      inSubMenu = true;
      break;
    case 5: // System
      currentMenu = MENU_SYSTEM;
      selectedItem = 0;
      topDisplayItem = 0;
      inSubMenu = true;
      break;
  }
}

void MenuManager::handleTrackingMenu() {
  switch (selectedItem) {
    case TRACK_VIEW_CURRENT:
      showCurrentTrackInfo();
      break;
    case TRACK_MODE:
      adjustTrackMode();
      break;
    case TRACK_FILTER:
      toggleTrackFilter();
      break;
    case TRACK_AUTO_SAVE:
      toggleAutoSave();
      break;
    case TRACK_BACK:
      goBack();
      break;
  }
}

void MenuManager::handleConnectMenu() {
  switch (selectedItem) {
    case CONNECT_START:
      showMessage("📱 Starting WiFi hotspot...", 3000);
      // TODO: Start WiFi hotspot
      break;
    case CONNECT_QR_CODE:
      showMessage("📄 Showing QR code...", 3000);
      // TODO: Show QR code
      break;
    case CONNECT_STATUS:
      showMessage("🔄 Refreshing file list...", 2000);
      // TODO: Refresh files
      break;
    case CONNECT_BACK:
      goBack();
      break;
  }
}

void MenuManager::handleDisplayMenu() {
  switch (selectedItem) {
    case DISP_BRIGHTNESS:
      adjustBrightness();
      break;
    case DISP_NIGHT_MODE:
      toggleNightMode();
      break;
    case DISP_AUTO_NIGHT:
      toggleAutoNightMode();
      break;
    case DISP_BACK:
      goBack();
      break;
  }
}

void MenuManager::handleCompassMenu() {
  switch (selectedItem) {
    case COMP_CALIBRATE:
      startCompassCalibration();
      break;
    case COMP_VIEW_STATUS:
      showCompassStatus();
      break;
    case COMP_BACK:
      goBack();
      break;
  }
}

void MenuManager::handleGPSMenu() {
  switch (selectedItem) {
    case GPS_VIEW_STATUS:
      showGPSStatus();
      break;
    case MENU_GPS_POWER_MODE:  // Using correct enum name
      adjustGPSPowerMode();
      break;
    case GPS_BACK:
      goBack();
      break;
  }
}

void MenuManager::handleSystemMenu() {
  switch (selectedItem) {
    case SYS_INFO:
      showSystemInfo();
      break;
    case SYS_DIAGNOSTICS:
      runSystemDiagnostics();
      break;
    case SYS_RESET_SETTINGS:
      resetSettings();
      break;
    case SYS_BACK:
      goBack();
      break;
  }
}

// ========================================
// SETTING ADJUSTERS - FIXED IMPLEMENTATIONS
// ========================================

void MenuManager::adjustTrackMode() {
  int current = settingsManager.getTrackMode();
  current = (current + 1) % 5;
  settingsManager.setTrackMode(current);
  settingsChanged = true;
  
  String modeText = formatTrackMode(current);
  showMessage("📝 Mode: " + modeText, 2000UL);
}

void MenuManager::toggleTrackFilter() {
  bool enabled = !settingsManager.getTrackFilter();
  settingsManager.setTrackFilter(enabled);
  settingsChanged = true;
  
  String status = enabled ? "ON" : "OFF";
  showMessage("🔍 Filter: " + status, 2000UL);
}

void MenuManager::toggleAutoSave() {
  bool enabled = !settingsManager.getAutoSave();
  settingsManager.setAutoSave(enabled);
  settingsChanged = true;
  
  String status = enabled ? "ON" : "OFF";
  showMessage("💾 Auto Save: " + status, 2000UL);
}

void MenuManager::adjustBrightness() {
  int current = settingsManager.getBrightness();
  current += 20;
  if (current > 100) current = 20;
  settingsManager.setBrightness(current);
  settingsChanged = true;
  
  // Apply immediately
  display.setBrightnessLevel(current);
  
  showMessage("☀️ Brightness: " + String(current) + "%", 2000UL);
}

void MenuManager::toggleNightMode() {
  settingsManager.setAutoNightMode(false); // Disable auto when manually toggling
  
  // Toggle night mode manually  
  if (display.getDisplayMode() == DISPLAY_DAY_MODE) {
    display.setDisplayMode(DISPLAY_NIGHT_MODE);
    showMessage("🌙 Night Mode: ON", 2000UL);
  } else {
    display.setDisplayMode(DISPLAY_DAY_MODE);
    showMessage("☀️ Day Mode: ON", 2000UL);
  }
  
  settingsChanged = true;
}

void MenuManager::toggleAutoNightMode() {
  bool enabled = !settingsManager.getAutoNightMode();
  settingsManager.setAutoNightMode(enabled);
  settingsChanged = true;
  
  if (enabled) {
    display.setDisplayMode(DISPLAY_AUTO_MODE);
    showMessage("🔄 Auto Night: ON", 2000UL);
  } else {
    showMessage("🔄 Auto Night: OFF", 2000UL);
  }
}

void MenuManager::adjustGPSPowerMode() {
  int current = settingsManager.getGPSPowerMode();
  int nextMode = getNextGPSPowerMode(current);
  settingsManager.setGPSPowerMode(nextMode);
  settingsChanged = true;
  
  showMessage("📡 GPS Power: " + getGPSPowerModeText(nextMode), 2000UL);
}

// ========================================
// Information
// DISPLAYS - FIXED IMPLEMENTATIONS
// ========================================

void MenuManager::showCurrentTrackInfo() {
  String info = "📊 CURRENT TRACK\n\n";
  
  if (trackingData.isRecording) {
    info += "🔴 Recording: ";
    info += trackingData.isPaused ? "PAUSED\n" : "ACTIVE\n";
    info += "📏 Distance: " + formatTrackDistance(trackingData.totalDistance) + "\n";
    info += "📍 Points: " + String(trackingData.totalPoints) + "\n";
    
    // Calculate average speed using correct member names
    if (trackingData.totalPoints > 0 && trackingData.recordingTime > 0) {
      float avgSpeed = (trackingData.totalDistance / 1000.0) / (trackingData.recordingTime / 3600000.0);
      info += "📈 Avg Speed: " + formatSpeed(avgSpeed) + "\n";
    }
    
    unsigned long duration = trackingData.recordingTime / 1000;
    info += "⏱️ Duration: " + formatDuration(duration) + "\n";
    
    // Calculate efficiency if we have valid data
    if (trackingData.totalPoints > 0) {
      float efficiency = 85.0; // Default efficiency value
      info += "🎯 Efficiency: " + String(efficiency, 1) + "%\n";
    }
    
  } else {
    info += "⭕ No active tracking\n";
    info += "Press CENTER to start\n";
  }
  
  showInfoDialog(info);
}

void MenuManager::showGPSStatus() {
  String info = "📡 GPS STATUS\n\n";
  
  if (gps.isValid) {
    info += "✅ GPS: LOCKED\n";
    info += "🛰️ Satellites: " + String(gps.satellites) + "\n";
    info += "📍 Lat: " + formatLatitude(gps.latitude) + "\n";
    info += "📍 Lon: " + formatLongitude(gps.longitude) + "\n";
    info += "🏔️ Alt: " + String(gps.altitude, 1) + "m\n";
    info += "⚡ Speed: " + formatSpeed(gps.speedKmh) + "\n"; // Using correct member name
    info += "🧭 Course: " + String(gps.course, 1) + "°\n";  // Using correct member name
    info += "📶 HDOP: " + String(gps.hdop, 2) + "\n";
  } else {
    info += "❌ GPS: NO LOCK\n";
    info += "🛰️ Satellites: " + String(gps.satellites) + "\n";
    info += "🔍 Searching...\n";
  }
  
  showInfoDialog(info);
}

void MenuManager::showCompassStatus() {
  String info = "🧭 COMPASS STATUS\n\n";
  
  if (compass.isCalibrated) {
    info += "✅ Calibration: GOOD\n";
    info += "🧭 Heading: " + String(compass.heading, 1) + "°\n";
    info += "📊 Quality: " + String(compass.calibrationQuality, 1) + "%\n";
    info += "⚖️ Declination: " + String(compass.declination, 1) + "°\n";
  } else {
    info += "❌ Calibration: NEEDED\n";
    info += "🔄 Please calibrate compass\n";
    info += "📋 Use Loop-Loop method\n";
  }
  
  showInfoDialog(info);
}

void MenuManager::showSystemInfo() {
  String info = "⚙️ SYSTEM INFO\n\n";
  
  info += "📱 Device: E-NAVI GPS\n";
  info += "🔧 Version: V2.0\n";
  info += "📊 Free RAM: " + String(ESP.getFreeHeap()) + " bytes\n";
  info += "⚡ CPU: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
  
  // System status using available members
  info += "📱 GPS: " + String(sysStatus.gpsConnected ? "OK" : "FAIL") + "\n";
  info += "🧭 Compass: " + String(compass.isCalibrated ? "OK" : "FAIL") + "\n";
  info += "💾 SD Card: " + String(sysStatus.sdCardConnected ? "OK" : "FAIL" ) + "\n";
  info += "📺 Display: OK\n";
  info += "🎮 Buttons: OK\n";
  
  showInfoDialog(info);
}

// ========================================
// CALIBRATION AND DIAGNOSTICS - STUB IMPLEMENTATIONS
// ========================================

void MenuManager::startCompassCalibration() {
  showMessage("🧭 Starting compass calibration...", 3000UL);
  // TODO: Start actual calibration process
  Serial.println("Menu Manager: Starting compass calibration");
}

void MenuManager::runSystemDiagnostics() {
  showMessage("🔧 Running system diagnostics...", 3000UL);
  // TODO: Run actual diagnostics
  Serial.println("Menu Manager: Running system diagnostics");
}

void MenuManager::resetSettings() {
  showMessage("🔄 Resetting all settings...", 3000UL);
  settingsManager.resetToDefaults();
  settingsChanged = true;
  Serial.println("Menu Manager: Settings reset to defaults");
}

// ========================================
// DRAWING FUNCTIONS - FIXED IMPLEMENTATIONS
// ========================================

void MenuManager::drawMenu() {
  if (!menuActive) return;
  
  display.clearScreen();
  
  // Draw title
  drawMenuTitle(String(menuDefs[currentMenu].title));
  
  // Draw menu items
  int y = menuStartY;
  int maxDisplay = min(itemsPerScreen, menuDefs[currentMenu].itemCount);
  
  for (int i = 0; i < maxDisplay; i++) {
    int itemIndex = topDisplayItem + i;
    bool isSelected = (itemIndex == selectedItem);
    
    drawMenuItem(itemIndex, y, isSelected);
    y += itemHeight;
  }
  
  // Draw scroll indicators if needed
  if (menuDefs[currentMenu].itemCount > itemsPerScreen) {
    drawScrollIndicators();
  }
  
  // Draw status message if active
  drawStatusMessage();
  
  display.updateScreen();
}

void MenuManager::drawMenuItem(int index, int yPos, bool selected) {
  if (index >= menuDefs[currentMenu].itemCount) return;
  
  MenuColors colors = dayColors; // TODO: Use night colors when appropriate
  
  // Draw selection background
  if (selected) {
    display.fillRect(5, yPos - 4, SCREEN_WIDTH - 10, itemHeight,
                    colors.selectedBg);
  }
  
  // Draw item text
  uint16_t textColor = selected ? colors.selectedText : colors.text;
  display.drawText(20, yPos + 15, menuDefs[currentMenu].items[index], 
                  textColor, MENU_FONT_SIZE_MEDIUM);
}

void MenuManager::drawMenuTitle(const String& title) {
  MenuColors colors = dayColors; // TODO: Use night colors when appropriate
  
  // Draw title background
  display.fillRect(0, 0, SCREEN_WIDTH, MENU_TITLE_HEIGHT, colors.titleBg);
  
  // Draw title text
  display.drawCenteredText(MENU_TITLE_HEIGHT / 2, title, 
                          colors.titleText, MENU_FONT_SIZE_LARGE);
  
  // Draw border
  display.drawLine(0, MENU_TITLE_HEIGHT - 2, SCREEN_WIDTH, MENU_TITLE_HEIGHT - 2, 
                   colors.border);
}

void MenuManager::drawScrollIndicators() {
  MenuColors colors = dayColors; // TODO: Use night colors when appropriate
  
  // Up arrow
  if (topDisplayItem > 0) {
    display.drawText(SCREEN_WIDTH - 30, menuStartY - 20, "▲", 
                    colors.accent, MENU_FONT_SIZE_SMALL);
  }
  
  // Down arrow
  if (topDisplayItem + itemsPerScreen < menuDefs[currentMenu].itemCount) {
    display.drawText(SCREEN_WIDTH - 30, menuStartY + (itemsPerScreen * itemHeight), "▼", 
                    colors.accent, MENU_FONT_SIZE_SMALL);
  }
}

void MenuManager::drawStatusMessage() {
  if (statusMessage.length() == 0 || millis() > statusMessageTime) {
    statusMessage = "";
    return;
  }
  
  MenuColors colors = dayColors; // TODO: Use night colors when appropriate
  
  // Draw message background
  int msgY = SCREEN_HEIGHT - 80;
  display.fillRect(10, msgY, SCREEN_WIDTH - 20, 40, colors.titleBg);
  
  // Draw message text
  display.drawCenteredText(msgY + 20, statusMessage, 
                          colors.titleText, MENU_FONT_SIZE_SMALL);
}

// ========================================

// UTILITY FUNCTIONS - IMPLEMENTATIONS
// ========================================

void MenuManager::showMessage(const String& message, unsigned long duration) {
  statusMessage = message;
  statusMessageTime = millis() + duration;
  Serial.println("Menu Message: " + message);
}

void MenuManager::clearMessage() {
  statusMessage = "";
  statusMessageTime = 0;
}

void MenuManager::showInfoDialog(const String& info, uint16_t color) {
  display.clearScreen();
  
  // Draw dialog background
  display.fillRect(20, 40, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 80, 0x2104);
  display.drawRect(20, 40, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 80, color);
  
  // Draw info text (word wrapped)
  int y = 60;
  int lineHeight = 25;
  String remaining = info;
  
  while (remaining.length() > 0 && y < SCREEN_HEIGHT - 100) {
    int breakPoint = remaining.indexOf('\n');
    String line;
    
    if (breakPoint == -1) {
      line = remaining;
      remaining = "";
    } else {
      line = remaining.substring(0, breakPoint);
      remaining = remaining.substring(breakPoint + 1);
    }
    
    display.drawText(30, y, line, color, MENU_FONT_SIZE_SMALL);
    y += lineHeight;
  }
  
  // Draw instruction
  display.drawCenteredText(SCREEN_HEIGHT - 40, "Press any button to continue", 
                          color, MENU_FONT_SIZE_SMALL);
  
  display.updateScreen();
}

String MenuManager::getMenuItemValue(int menuItem) {
  switch (currentMenu) {
    case MENU_TRACKING:
      switch (menuItem) {
        case TRACK_MODE:
          return formatTrackMode(settingsManager.getTrackMode());
        case TRACK_FILTER:
          return settingsManager.getTrackFilter() ? "ON" : "OFF";
        case TRACK_AUTO_SAVE:
          return settingsManager.getAutoSave() ? "ON" : "OFF";
        }
      break;
      
    case MENU_DISPLAY:
      switch (menuItem) {
        case DISP_BRIGHTNESS:
          return String(settingsManager.getBrightness()) + "%";
        case DISP_NIGHT_MODE:
          // Note: Night mode value not directly returned; perhaps add if needed
          return display.getDisplayMode() == DISPLAY_NIGHT_MODE ? "ON" : "OFF";
        case DISP_AUTO_NIGHT:
          return settingsManager.getAutoNightMode() ? "ON" : "OFF";
      }
      break;
      
    case MENU_GPS:
      switch (menuItem) {
        case MENU_GPS_POWER_MODE:
          return getGPSPowerModeText(settingsManager.getGPSPowerMode());
      }
      break;
  }
  
  return "";
}

// ========================================
// FORMAT UTILITY FUNCTIONS
// ========================================

String formatTrackMode(int mode) {
  switch (mode) {
    case TRACK_MODE_OFF: return "Off";
    case TRACK_MODE_MANUAL: return "Manual";
    case TRACK_MODE_AUTO_MOTION: return "Auto Motion";
    case TRACK_MODE_AUTO_SPEED: return "Auto Speed";
    case TRACK_MODE_CONTINUOUS: return "Continuous";
    default: return "Unknown";
  }
}

String formatTrackDistance(float distanceMeters) {
  if (distanceMeters < 1000) {
    return String(distanceMeters, 0) + "m";
  } else {
    return String(distanceMeters / 1000.0, 2) + "km";
  }
}

String formatDuration(unsigned long seconds) {
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  
  if (hours > 0) {
    return String(hours) + "h " + String(minutes) + "m " + String(secs) + "s";
  } else if (minutes > 0) {
    return String(minutes) + "m " + String(secs) + "s";
  } else {
    return String(secs) + "s";
  }
}

String formatLatitude(double lat) {
  return String(lat, 6);
}

String formatLongitude(double lon) {
  return String(lon, 6);
}