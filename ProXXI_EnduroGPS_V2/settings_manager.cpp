/*
 * FILE LOCATION: ProXXI_EnduroGPS_V1/storage/settings_manager.cpp
 * 
 * E_NAVI EnduroGPS - Settings Manager Implementation
 * Configuration Persistence for User Preferences
 * 
 * Features: NVS storage, JSON format, default values, validation
 * Integration: Works with shared_data.h and menu_system.h
 */

#include "settings_manager.h"
#include "shared_data.h"
#include "config.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// ========================================
// SETTINGS MANAGER IMPLEMENTATION
// ========================================

SettingsManager::SettingsManager() {
  settingsLoaded = false;
  settingsDirty = false;
  settingsFile = "/settings/user_settings.json";
}

SettingsManager::~SettingsManager() {
  if (settingsDirty) {
    saveSettings();
  }
}

bool SettingsManager::initialize() {
  Serial.println("Settings Manager: Initializing configuration persistence...");
  
  // Try to load existing settings
  if (!loadSettings()) {
    Serial.println("Settings Manager: No valid settings found, using defaults");
    setDefaultSettings();
    
    // Save default settings
    if (saveSettings()) {
      Serial.println("Settings Manager: Default settings saved");
    } else {
      Serial.println("Settings Manager: WARNING - Failed to save default settings");
    }
  }
  
  Serial.println("Settings Manager: Configuration system ready!");
  Serial.println("  - NVS storage for reliability");
  Serial.println("  - JSON format for readability"); 
  Serial.println("  - Automatic validation");
  Serial.println("  - Default value fallbacks");
  
  return true;
}

bool SettingsManager::loadSettings() {
  Serial.println("Settings Manager: Loading user preferences...");
  
  // Try NVS first (more reliable)
  if (loadFromNVS()) {
    settingsLoaded = true;
    settingsDirty = false;
    Serial.println("Settings Manager: Loaded from NVS storage");
    return true;
  }
  
  // Try SD card as backup
  if (loadFromSD()) {
    settingsLoaded = true;
    settingsDirty = false;
    Serial.println("Settings Manager: Loaded from SD card");
    
    // Save to NVS for faster access next time
    saveToNVS();
    return true;
  }
  
  Serial.println("Settings Manager: No valid settings found");
  return false;
}

bool SettingsManager::saveSettings() {
  if (!settingsDirty) {
    return true; // Nothing to save
  }
  
  Serial.println("Settings Manager: Saving user preferences...");
  
  bool nvsSuccess = saveToNVS();
  bool sdSuccess = saveToSD();
  
  if (nvsSuccess || sdSuccess) {
    settingsDirty = false;
    Serial.printf("Settings Manager: Saved (NVS: %s, SD: %s)\n", 
                  nvsSuccess ? "OK" : "FAIL", 
                  sdSuccess ? "OK" : "FAIL");
    return true;
  }
  
  Serial.println("Settings Manager: ERROR - Failed to save settings");
  return false;
}

bool SettingsManager::resetToDefaults() {
  Serial.println("Settings Manager: Resetting to default settings");
  
  setDefaultSettings();
  settingsDirty = true;
  
  return saveSettings();
}

// ========================================
// NVS STORAGE (PRIMARY)
// ========================================

bool SettingsManager::loadFromNVS() {
  Preferences prefs;
  
  if (!prefs.begin("e_navi_settings", true)) { // Read-only
    Serial.println("Settings Manager: Failed to open NVS");
    return false;
  }
  
  // Check if settings exist and are valid
  if (!prefs.isKey("settings_version")) {
    prefs.end();
    return false;
  }
  
  uint32_t version = prefs.getUInt("settings_version", 0);
  if (version != SETTINGS_VERSION) {
    Serial.printf("Settings Manager: Version mismatch (found %u, expected %u)\n", 
                  version, SETTINGS_VERSION);
    prefs.end();
    return false;
  }
  
  // Load all settings with validation
  UserPreferences loadedPrefs;
  
  // Display settings
  loadedPrefs.displayBrightness = prefs.getInt("disp_brightness", 80);
  loadedPrefs.autoNightMode = prefs.getBool("auto_night", true);
  loadedPrefs.nightModeStartHour = prefs.getInt("night_start", 18);
  loadedPrefs.dayModeStartHour = prefs.getInt("day_start", 6);
  loadedPrefs.nightBrightness = prefs.getInt("night_brightness", 20);
  
  // Navigation settings
  loadedPrefs.autoZoomEnabled = prefs.getBool("auto_zoom", true);
  loadedPrefs.autoZoomFactor = prefs.getFloat("zoom_factor", 1.0);
  loadedPrefs.defaultZoomLevel = prefs.getInt("default_zoom", 14);
  
  // Tracking settings
  loadedPrefs.trackingMode = prefs.getInt("track_mode", TRACK_MODE_AUTO);
  loadedPrefs.trackFilterEnabled = prefs.getBool("track_filter", true);
  loadedPrefs.trackMinDistance = prefs.getFloat("track_min_dist", 5.0);
  loadedPrefs.trackMinSpeed = prefs.getFloat("track_min_speed", 2.0);
  loadedPrefs.trackMaxInterval = prefs.getULong("track_max_interval", 30000);
  loadedPrefs.autoSaveEnabled = prefs.getBool("auto_save", true);
  
  // Power settings
  loadedPrefs.sleepTimeout = prefs.getULong("sleep_timeout", 30000);
  loadedPrefs.autoWakeEnabled = prefs.getBool("auto_wake", true);
  loadedPrefs.autoWakeInterval = prefs.getULong("wake_interval", 3600000);
  
  // GPS settings
  loadedPrefs.gpsUpdateRate = prefs.getInt("gps_rate", 10);
  loadedPrefs.gpsLowPowerMode = prefs.getBool("gps_low_power", false);
  loadedPrefs.gpsAccuracyThreshold = prefs.getFloat("gps_accuracy", 2.0);
  
  // Compass settings
  loadedPrefs.compassAutoCalibrate = prefs.getBool("comp_auto_cal", false);
  loadedPrefs.compassHeadingSmoothing = prefs.getFloat("comp_smoothing", 0.1);
  loadedPrefs.compassMotionThreshold = prefs.getFloat("comp_motion", 0.3);
  
  // User info
  String userName = prefs.getString("user_name", "Rider");
  userName.toCharArray(loadedPrefs.userName, sizeof(loadedPrefs.userName));
  
  String bikeName = prefs.getString("bike_name", "Adventure Bike");
  bikeName.toCharArray(loadedPrefs.bikeName, sizeof(loadedPrefs.bikeName));
  
  String units = prefs.getString("units", "metric");
  units.toCharArray(loadedPrefs.preferredUnits, sizeof(loadedPrefs.preferredUnits));
  
  loadedPrefs.timeZoneOffset = prefs.getInt("timezone", 2);
  
  // Advanced settings
  loadedPrefs.debugMode = prefs.getBool("debug", false);
  loadedPrefs.logLevel = prefs.getInt("log_level", 3);
  loadedPrefs.telemetryEnabled = prefs.getBool("telemetry", false);
  
  prefs.end();
  
  // Validate loaded settings
  if (validateSettings(loadedPrefs)) {
    userPrefs = loadedPrefs;
    return true;
  } else {
    Serial.println("Settings Manager: Loaded settings failed validation");
    return false;
  }
}

bool SettingsManager::saveToNVS() {
  Preferences prefs;
  
  if (!prefs.begin("e_navi_settings", false)) { // Read-write
    Serial.println("Settings Manager: Failed to open NVS for writing");
    return false;
  }
  
  // Save version first
  prefs.putUInt("settings_version", SETTINGS_VERSION);
  
  // Save all settings
  prefs.putInt("disp_brightness", userPrefs.displayBrightness);
  prefs.putBool("auto_night", userPrefs.autoNightMode);
  prefs.putInt("night_start", userPrefs.nightModeStartHour);
  prefs.putInt("day_start", userPrefs.dayModeStartHour);
  prefs.putInt("night_brightness", userPrefs.nightBrightness);
  
  prefs.putBool("auto_zoom", userPrefs.autoZoomEnabled);
  prefs.putFloat("zoom_factor", userPrefs.autoZoomFactor);
  prefs.putInt("default_zoom", userPrefs.defaultZoomLevel);
  
  prefs.putInt("track_mode", userPrefs.trackingMode);
  prefs.putBool("track_filter", userPrefs.trackFilterEnabled);
  prefs.putFloat("track_min_dist", userPrefs.trackMinDistance);
  prefs.putFloat("track_min_speed", userPrefs.trackMinSpeed);
  prefs.putULong("track_max_interval", userPrefs.trackMaxInterval);
  prefs.putBool("auto_save", userPrefs.autoSaveEnabled);
  
  prefs.putULong("sleep_timeout", userPrefs.sleepTimeout);
  prefs.putBool("auto_wake", userPrefs.autoWakeEnabled);
  prefs.putULong("wake_interval", userPrefs.autoWakeInterval);
  
  prefs.putInt("gps_rate", userPrefs.gpsUpdateRate);
  prefs.putBool("gps_low_power", userPrefs.gpsLowPowerMode);
  prefs.putFloat("gps_accuracy", userPrefs.gpsAccuracyThreshold);
  
  prefs.putBool("comp_auto_cal", userPrefs.compassAutoCalibrate);
  prefs.putFloat("comp_smoothing", userPrefs.compassHeadingSmoothing);
  prefs.putFloat("comp_motion", userPrefs.compassMotionThreshold);
  
  prefs.putString("user_name", String(userPrefs.userName));
  prefs.putString("bike_name", String(userPrefs.bikeName));
  prefs.putString("units", String(userPrefs.preferredUnits));
  prefs.putInt("timezone", userPrefs.timeZoneOffset);
  
  prefs.putBool("debug", userPrefs.debugMode);
  prefs.putInt("log_level", userPrefs.logLevel);
  prefs.putBool("telemetry", userPrefs.telemetryEnabled);
  
  prefs.end();
  return true;
}

// ========================================
// SD CARD STORAGE (BACKUP)
// ========================================

bool SettingsManager::loadFromSD() {
  if (!SD.exists(settingsFile)) {
    return false;
  }
  
  File file = SD.open(settingsFile, FILE_READ);
  if (!file) {
    Serial.println("Settings Manager: Failed to open settings file for reading");
    return false;
  }
  
  String jsonContent = file.readString();
  file.close();
  
  if (jsonContent.length() == 0) {
    Serial.println("Settings Manager: Settings file is empty");
    return false;
  }
  
  return parseJSON(jsonContent);
}

bool SettingsManager::saveToSD() {
  // Ensure settings directory exists
  if (!SD.exists("/settings")) {
    if (!SD.mkdir("/settings")) {
      Serial.println("Settings Manager: Failed to create settings directory");
      return false;
    }
  }
  
  // Generate JSON content
  String jsonContent = generateJSON();
  if (jsonContent.length() == 0) {
    Serial.println("Settings Manager: Failed to generate JSON");
    return false;
  }
  
  // Write to file
  File file = SD.open(settingsFile, FILE_WRITE);
  if (!file) {
    Serial.println("Settings Manager: Failed to open settings file for writing");
    return false;
  }
  
  size_t written = file.print(jsonContent);
  file.close();
  
  if (written != jsonContent.length()) {
    Serial.println("Settings Manager: Incomplete write to settings file");
    return false;
  }
  
  return true;
}

// ========================================
// JSON PARSING AND GENERATION
// ========================================

bool SettingsManager::parseJSON(const String& jsonContent) {
  DynamicJsonDocument doc(2048);
  
  DeserializationError error = deserializeJson(doc, jsonContent);
  if (error) {
    Serial.printf("Settings Manager: JSON parse error: %s\n", error.c_str());
    return false;
  }
  
  // Check version
  if (!doc.containsKey("version") || doc["version"] != SETTINGS_VERSION) {
    Serial.println("Settings Manager: JSON version mismatch");
    return false;
  }
  
  // Parse settings with defaults
  UserPreferences loadedPrefs;
  
  // Display settings
  loadedPrefs.displayBrightness = doc["display"]["brightness"] | 80;
  loadedPrefs.autoNightMode = doc["display"]["auto_night"] | true;
  loadedPrefs.nightModeStartHour = doc["display"]["night_start"] | 18;
  loadedPrefs.dayModeStartHour = doc["display"]["day_start"] | 6;
  loadedPrefs.nightBrightness = doc["display"]["night_brightness"] | 20;
  
  // Navigation settings
  loadedPrefs.autoZoomEnabled = doc["navigation"]["auto_zoom"] | true;
  loadedPrefs.autoZoomFactor = doc["navigation"]["zoom_factor"] | 1.0;
  loadedPrefs.defaultZoomLevel = doc["navigation"]["default_zoom"] | 14;
  
  // Tracking settings
  loadedPrefs.trackingMode = doc["tracking"]["mode"] | TRACK_MODE_AUTO;
  loadedPrefs.trackFilterEnabled = doc["tracking"]["filter_enabled"] | true;
  loadedPrefs.trackMinDistance = doc["tracking"]["min_distance"] | 5.0;
  loadedPrefs.trackMinSpeed = doc["tracking"]["min_speed"] | 2.0;
  loadedPrefs.trackMaxInterval = doc["tracking"]["max_interval"] | 30000UL;
  loadedPrefs.autoSaveEnabled = doc["tracking"]["auto_save"] | true;
  
  // Power settings
  loadedPrefs.sleepTimeout = doc["power"]["sleep_timeout"] | 30000UL;
  loadedPrefs.autoWakeEnabled = doc["power"]["auto_wake"] | true;
  loadedPrefs.autoWakeInterval = doc["power"]["wake_interval"] | 3600000UL;
  
  // GPS settings
  loadedPrefs.gpsUpdateRate = doc["gps"]["update_rate"] | 10;
  loadedPrefs.gpsLowPowerMode = doc["gps"]["low_power"] | false;
  loadedPrefs.gpsAccuracyThreshold = doc["gps"]["accuracy_threshold"] | 2.0;
  
  // Compass settings
  loadedPrefs.compassAutoCalibrate = doc["compass"]["auto_calibrate"] | false;
  loadedPrefs.compassHeadingSmoothing = doc["compass"]["heading_smoothing"] | 0.1;
  loadedPrefs.compassMotionThreshold = doc["compass"]["motion_threshold"] | 0.3;
  
  // User info
  String userName = doc["user"]["name"] | "Rider";
  userName.toCharArray(loadedPrefs.userName, sizeof(loadedPrefs.userName));
  
  String bikeName = doc["user"]["bike"] | "Adventure Bike";
  bikeName.toCharArray(loadedPrefs.bikeName, sizeof(loadedPrefs.bikeName));
  
  String units = doc["user"]["units"] | "metric";
  units.toCharArray(loadedPrefs.preferredUnits, sizeof(loadedPrefs.preferredUnits));
  
  loadedPrefs.timeZoneOffset = doc["user"]["timezone"] | 2;
  
  // Advanced settings
  loadedPrefs.debugMode = doc["advanced"]["debug"] | false;
  loadedPrefs.logLevel = doc["advanced"]["log_level"] | 3;
  loadedPrefs.telemetryEnabled = doc["advanced"]["telemetry"] | false;
  
  // Validate and apply
  if (validateSettings(loadedPrefs)) {
    userPrefs = loadedPrefs;
    return true;
  }
  
  return false;
}

String SettingsManager::generateJSON() {
  DynamicJsonDocument doc(2048);
  
  // Version and metadata
  doc["version"] = SETTINGS_VERSION;
  doc["created"] = millis();
  doc["device"] = "E_NAVI EnduroGPS";
  doc["description"] = "User preferences for professional motorcycle GPS";
  
  // Display settings
  doc["display"]["brightness"] = userPrefs.displayBrightness;
  doc["display"]["auto_night"] = userPrefs.autoNightMode;
  doc["display"]["night_start"] = userPrefs.nightModeStartHour;
  doc["display"]["day_start"] = userPrefs.dayModeStartHour;
  doc["display"]["night_brightness"] = userPrefs.nightBrightness;
  
  // Navigation settings
  doc["navigation"]["auto_zoom"] = userPrefs.autoZoomEnabled;
  doc["navigation"]["zoom_factor"] = userPrefs.autoZoomFactor;
  doc["navigation"]["default_zoom"] = userPrefs.defaultZoomLevel;
  
  // Tracking settings
  doc["tracking"]["mode"] = userPrefs.trackingMode;
  doc["tracking"]["filter_enabled"] = userPrefs.trackFilterEnabled;
  doc["tracking"]["min_distance"] = userPrefs.trackMinDistance;
  doc["tracking"]["min_speed"] = userPrefs.trackMinSpeed;
  doc["tracking"]["max_interval"] = userPrefs.trackMaxInterval;
  doc["tracking"]["auto_save"] = userPrefs.autoSaveEnabled;
  
  // Power settings
  doc["power"]["sleep_timeout"] = userPrefs.sleepTimeout;
  doc["power"]["auto_wake"] = userPrefs.autoWakeEnabled;
  doc["power"]["wake_interval"] = userPrefs.autoWakeInterval;
  
  // GPS settings
  doc["gps"]["update_rate"] = userPrefs.gpsUpdateRate;
  doc["gps"]["low_power"] = userPrefs.gpsLowPowerMode;
  doc["gps"]["accuracy_threshold"] = userPrefs.gpsAccuracyThreshold;
  
  // Compass settings
  doc["compass"]["auto_calibrate"] = userPrefs.compassAutoCalibrate;
  doc["compass"]["heading_smoothing"] = userPrefs.compassHeadingSmoothing;
  doc["compass"]["motion_threshold"] = userPrefs.compassMotionThreshold;
  
  // User info
  doc["user"]["name"] = String(userPrefs.userName);
  doc["user"]["bike"] = String(userPrefs.bikeName);
  doc["user"]["units"] = String(userPrefs.preferredUnits);
  doc["user"]["timezone"] = userPrefs.timeZoneOffset;
  
  // Advanced settings
  doc["advanced"]["debug"] = userPrefs.debugMode;
  doc["advanced"]["log_level"] = userPrefs.logLevel;
  doc["advanced"]["telemetry"] = userPrefs.telemetryEnabled;
  
  String output;
  serializeJsonPretty(doc, output);
  return output;
}

// ========================================
// DEFAULT SETTINGS
// ========================================

void SettingsManager::setDefaultSettings() {
  Serial.println("Settings Manager: Setting default preferences for SA enduro riders");
  
  // Display settings optimized for aging eyes
  userPrefs.displayBrightness = 80;          // Bright for outdoor use
  userPrefs.autoNightMode = true;            // Automatic day/night
  userPrefs.nightModeStartHour = 18;         // 6 PM (SA sunset)
  userPrefs.dayModeStartHour = 6;            // 6 AM (SA sunrise)
  userPrefs.nightBrightness = 20;            // Dim for night vision
  
  // Navigation settings
  userPrefs.autoZoomEnabled = true;          // Speed-based zoom
  userPrefs.autoZoomFactor = 1.0;            // Standard factor
  userPrefs.defaultZoomLevel = 14;           // Good for trails
  strcpy(userPrefs.preferredMapType, "topo"); // Topographic for SA
  
  // Tracking settings for professional use
  userPrefs.trackingMode = TRACK_MODE_AUTO;  // Motion-based auto
  userPrefs.trackFilterEnabled = true;       // 80-90% file reduction
  userPrefs.trackMinDistance = 5.0;          // 5 meters minimum
  userPrefs.trackMinSpeed = 2.0;             // 2 km/h minimum
  userPrefs.trackMaxInterval = 30000;        // 30 seconds maximum
  userPrefs.autoSaveEnabled = true;          // Professional reliability
  
  // Power settings for long rides
  userPrefs.sleepTimeout = 30000;            // 30 seconds motion timeout
  userPrefs.autoWakeEnabled = true;          // 1-hour timer wake
  userPrefs.autoWakeInterval = 3600000;      // 1 hour
  
  // GPS settings for SA conditions
  userPrefs.gpsUpdateRate = 10;              // 10Hz for precision
  userPrefs.gpsLowPowerMode = false;         // Full power for accuracy
  userPrefs.gpsAccuracyThreshold = 2.0;      // 2m HDOP threshold
  
  // Compass settings
  userPrefs.compassAutoCalibrate = false;    // Manual loop-loop only
  userPrefs.compassHeadingSmoothing = 0.1;   // Light smoothing
  userPrefs.compassMotionThreshold = 0.3;    // 0.3g motion detection
  
  // User info defaults for SA market
  strcpy(userPrefs.userName, "Rider");
  strcpy(userPrefs.bikeName, "Adventure Bike");
  strcpy(userPrefs.preferredUnits, "metric");  // Metric system in SA
  userPrefs.timeZoneOffset = 2;              // GMT+2 (SAST)
  
  // Advanced settings
  userPrefs.debugMode = false;               // Production mode
  userPrefs.logLevel = 3;                    // Info level
  userPrefs.telemetryEnabled = false;        // Privacy first
  userPrefs.preferencesVersion = SETTINGS_VERSION;
  
  settingsLoaded = true;
  settingsDirty = true;
}

// ========================================
// VALIDATION
// ========================================

bool SettingsManager::validateSettings(const UserPreferences& prefs) {
  // Display validation
  if (prefs.displayBrightness < 10 || prefs.displayBrightness > 100) {
    Serial.println("Settings Manager: Invalid display brightness");
    return false;
  }
  
  if (prefs.nightModeStartHour < 0 || prefs.nightModeStartHour > 23 ||
      prefs.dayModeStartHour < 0 || prefs.dayModeStartHour > 23) {
    Serial.println("Settings Manager: Invalid day/night hours");
    return false;
  }
  
  // Navigation validation
  if (prefs.autoZoomFactor < 0.1 || prefs.autoZoomFactor > 3.0) {
    Serial.println("Settings Manager: Invalid zoom factor");
    return false;
  }
  
  if (prefs.defaultZoomLevel < 8 || prefs.defaultZoomLevel > 18) {
    Serial.println("Settings Manager: Invalid default zoom level");
    return false;
  }
  
  // Tracking validation
  if (prefs.trackingMode < TRACK_MODE_OFF || prefs.trackingMode > TRACK_MODE_CONTINUOUS) {
    Serial.println("Settings Manager: Invalid tracking mode");
    return false;
  }
  
  if (prefs.trackMinDistance < 1.0 || prefs.trackMinDistance > 100.0) {
    Serial.println("Settings Manager: Invalid track minimum distance");
    return false;
  }
  
  if (prefs.trackMinSpeed < 0.0 || prefs.trackMinSpeed > 20.0) {
    Serial.println("Settings Manager: Invalid track minimum speed");
    return false;
  }
  
  // GPS validation
  if (prefs.gpsUpdateRate < 1 || prefs.gpsUpdateRate > 10) {
    Serial.println("Settings Manager: Invalid GPS update rate");
    return false;
  }
  
  if (prefs.gpsAccuracyThreshold < 0.5 || prefs.gpsAccuracyThreshold > 10.0) {
    Serial.println("Settings Manager: Invalid GPS accuracy threshold");
    return false;
  }
  
  // Compass validation
  if (prefs.compassHeadingSmoothing < 0.01 || prefs.compassHeadingSmoothing > 1.0) {
    Serial.println("Settings Manager: Invalid compass smoothing");
    return false;
  }
  
  if (prefs.compassMotionThreshold < 0.1 || prefs.compassMotionThreshold > 2.0) {
    Serial.println("Settings Manager: Invalid motion threshold");
    return false;
  }
  
  // Timezone validation (reasonable range for global use)
  if (prefs.timeZoneOffset < -12 || prefs.timeZoneOffset > 14) {
    Serial.println("Settings Manager: Invalid timezone offset");
    return false;
  }
  
  Serial.println("Settings Manager: Settings validation passed");
  return true;
}

// ========================================
// GETTERS AND SETTERS
// ========================================

bool SettingsManager::isLoaded() const {
  return settingsLoaded;
}

bool SettingsManager::hasUnsavedChanges() const {
  return settingsDirty;
}

void SettingsManager::markDirty() {
  settingsDirty = true;
}

// Individual setting getters (with validation)
int SettingsManager::getDisplayBrightness() {
  return constrain(userPrefs.displayBrightness, 10, 100);
}

bool SettingsManager::getAutoNightMode() {
  return userPrefs.autoNightMode;
}

int SettingsManager::getTrackingMode() {
  return constrain(userPrefs.trackingMode, TRACK_MODE_OFF, TRACK_MODE_CONTINUOUS);
}

bool SettingsManager::getTrackFilterEnabled() {
  return userPrefs.trackFilterEnabled;
}

int SettingsManager::getGPSUpdateRate() {
  return constrain(userPrefs.gpsUpdateRate, 1, 10);
}

// Individual setting setters (with validation)
void SettingsManager::setDisplayBrightness(int brightness) {
  int validBrightness = constrain(brightness, 10, 100);
  if (userPrefs.displayBrightness != validBrightness) {
    userPrefs.displayBrightness = validBrightness;
    settingsDirty = true;
  }
}

void SettingsManager::setAutoNightMode(bool enabled) {
  if (userPrefs.autoNightMode != enabled) {
    userPrefs.autoNightMode = enabled;
    settingsDirty = true;
  }
}

void SettingsManager::setTrackingMode(int mode) {
  int validMode = constrain(mode, TRACK_MODE_OFF, TRACK_MODE_CONTINUOUS);
  if (userPrefs.trackingMode != validMode) {
    userPrefs.trackingMode = validMode;
    settingsDirty = true;
  }
}

void SettingsManager::setTrackFilterEnabled(bool enabled) {
  if (userPrefs.trackFilterEnabled != enabled) {
    userPrefs.trackFilterEnabled = enabled;
    settingsDirty = true;
  }
}

void SettingsManager::setGPSUpdateRate(int rate) {
  int validRate = constrain(rate, 1, 10);
  if (userPrefs.gpsUpdateRate != validRate) {
    userPrefs.gpsUpdateRate = validRate;
    settingsDirty = true;
  }
}

// ========================================
// EXPORT/IMPORT
// ========================================

bool SettingsManager::exportSettings(const String& filename) {
  String jsonContent = generateJSON();
  if (jsonContent.length() == 0) {
    return false;
  }
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    return false;
  }
  
  size_t written = file.print(jsonContent);
  file.close();
  
  return written == jsonContent.length();
}

bool SettingsManager::importSettings(const String& filename) {
  if (!SD.exists(filename)) {
    return false;
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    return false;
  }
  
  String jsonContent = file.readString();
  file.close();
  
  if (parseJSON(jsonContent)) {
    settingsDirty = true;
    return saveSettings();
  }
  
  return false;
}

// ========================================
// GLOBAL INSTANCE
// ========================================
SettingsManager settingsManager; 
