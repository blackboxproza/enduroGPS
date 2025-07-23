/*
 * E_NAVI EnduroGPS - Settings Manager Header
 * Configuration Persistence for User Preferences
 * 
 * Features: NVS storage, JSON format, default values, validation
 * Integration: Works with shared_data.h and menu_system.h
 * 
 * MATCHES THE ACTUAL IMPLEMENTATION IN settings_manager.cpp
 * FIXED: getCurrentGPSUpdateRate() now returns float to match gps_manager.h
 */

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <FS.h>

// Forward declarations to avoid circular includes
class SDManager;

// ========================================
// SETTINGS VERSION CONTROL (from actual implementation)
// ========================================

#define SETTINGS_VERSION        1       // Settings format version

// ========================================
// TRACKING MODES (from actual implementation)
// ========================================

#define TRACK_MODE_OFF          0       // No tracking
#define TRACK_MODE_AUTO         1       // Auto motion-based tracking  
#define TRACK_MODE_CONTINUOUS   2       // Continuous tracking
#define TRACK_AUTO_MOTION       1       // Alias for auto mode

// ========================================
// USER PREFERENCES STRUCTURE (from actual implementation)
// ========================================

struct UserPreferences {
  // ========================================
  // DISPLAY SETTINGS
  // ========================================
  
  int displayBrightness = 80;              // Display brightness (10-100%)
  bool autoNightMode = true;               // Automatic night mode
  int nightModeStartHour = 18;             // Night mode start hour (24h format)
  int dayModeStartHour = 6;                // Day mode start hour (24h format)
  int nightBrightness = 20;                // Night mode brightness (10-100%)
  
  // ========================================
  // NAVIGATION SETTINGS
  // ========================================
  
  bool autoZoomEnabled = true;             // Enable auto-zoom based on speed
  float autoZoomFactor = 1.0;              // Auto-zoom scaling factor
  int defaultZoomLevel = 14;               // Default map zoom level (8-18)
  
  // ========================================
  // TRACKING SETTINGS
  // ========================================
  
  int trackingMode = TRACK_AUTO_MOTION;    // Tracking mode (see defines above)
  bool trackFilterEnabled = true;          // Enable track point filtering
  float trackMinDistance = 5.0;            // Minimum distance between points (m)
  float trackMinSpeed = 2.0;               // Minimum speed to record (km/h)
  unsigned long trackMaxInterval = 30000;  // Maximum time between points (ms)
  bool autoSaveEnabled = true;             // Enable automatic track saving
  
  // ========================================
  // POWER SETTINGS
  // ========================================
  
  unsigned long sleepTimeout = 30000;      // Sleep timeout (ms)
  bool autoWakeEnabled = true;             // Enable automatic wake
  unsigned long autoWakeInterval = 3600000; // Auto wake interval (ms) - 1 hour
  
  // ========================================
  // GPS SETTINGS
  // ========================================
  
  int gpsUpdateRate = 10;                  // GPS update rate (1-10 Hz)
  bool gpsLowPowerMode = false;            // GPS low power mode
  float gpsAccuracyThreshold = 2.0;        // GPS accuracy threshold (m)
  
  // ========================================
  // COMPASS SETTINGS
  // ========================================
  
  bool compassAutoCalibrate = false;       // Auto compass calibration
  float compassHeadingSmoothing = 0.1;     // Heading smoothing factor (0.01-1.0)
  float compassMotionThreshold = 0.3;      // Motion detection threshold (g)
  
  // ========================================
  // USER INFORMATION
  // ========================================
  
  char userName[32] = "Rider";             // User name
  char bikeName[32] = "Adventure Bike";    // Bike name
  char preferredUnits[16] = "metric";      // Preferred units (metric/imperial)
  int timeZoneOffset = 2;                  // Timezone offset from UTC
  
  // ========================================
  // ADVANCED SETTINGS
  // ========================================
  
  bool debugMode = false;                  // Enable debug mode
  int logLevel = 3;                        // Logging level (0-5)
  bool telemetryEnabled = false;           // Enable telemetry
};

// ========================================
// SETTINGS MANAGER CLASS (from actual implementation)
// ========================================

class SettingsManager {
  private:
    // ========================================
    // CORE STATE (from actual implementation)
    // ========================================
    
    bool settingsLoaded = false;
    bool settingsDirty = false;
    String settingsFile = "/settings/user_settings.json";
    UserPreferences userPrefs;
    
    // ========================================
    // STORAGE METHODS (from actual implementation)
    // ========================================
    
    // NVS Storage (Primary)
    bool loadFromNVS();
    bool saveToNVS();
    
    // SD Card Storage (Backup)
    bool loadFromSD();
    bool saveToSD();
    
    // ========================================
    // JSON HANDLING (from actual implementation)
    // ========================================
    
    bool parseJSON(const String& jsonContent);
    String generateJSON();
    
    // ========================================
    // SIMPLE PARSER (from actual implementation)
    // ========================================
    
    String generateSettingsContent();       // Generate simple key=value content
    bool parseSimpleSettings(const String& content);
    
    // ========================================
    // VALIDATION (from actual implementation)
    // ========================================
    
    bool validateSettings(const UserPreferences& prefs);
    
  public:
    // ========================================
    // CONSTRUCTOR & DESTRUCTOR (from actual implementation)
    // ========================================
    
    SettingsManager();
    ~SettingsManager();
    
    // ========================================
    // INITIALIZATION (from actual implementation)
    // ========================================
    
    bool initialize();
    
    // ========================================
    // LOAD/SAVE OPERATIONS (from actual implementation)
    // ========================================
    
    bool loadSettings();                     // Load settings from storage
    bool saveSettings();                     // Save settings to storage
    bool resetToDefaults();                  // Reset to default settings
    void setDefaultSettings();               // Apply default settings
    
    // ========================================
    // STATE MANAGEMENT (from actual implementation)
    // ========================================
    
    bool isLoaded() const;                   // Check if settings are loaded
    bool hasUnsavedChanges() const;          // Check for unsaved changes
    void markDirty();                        // Mark settings as dirty
    
    // ========================================
    // INDIVIDUAL GETTERS (from actual implementation)
    // ========================================
    
    // Display getters
    int getDisplayBrightness();
    bool getAutoNightMode();
    int getNightModeStartHour() { return userPrefs.nightModeStartHour; }
    int getDayModeStartHour() { return userPrefs.dayModeStartHour; }
    int getNightBrightness() { return userPrefs.nightBrightness; }
    
    // Navigation getters
    bool getAutoZoomEnabled() { return userPrefs.autoZoomEnabled; }
    float getAutoZoomFactor() { return userPrefs.autoZoomFactor; }
    int getDefaultZoomLevel() { return userPrefs.defaultZoomLevel; }
    
    // Tracking getters
    int getTrackingMode();
    bool getTrackFilterEnabled();
    float getTrackMinDistance() { return userPrefs.trackMinDistance; }
    float getTrackMinSpeed() { return userPrefs.trackMinSpeed; }
    unsigned long getTrackMaxInterval() { return userPrefs.trackMaxInterval; }
    bool getAutoSaveEnabled() { return userPrefs.autoSaveEnabled; }
    
    // Power getters
    unsigned long getSleepTimeout() { return userPrefs.sleepTimeout; }
    bool getAutoWakeEnabled() { return userPrefs.autoWakeEnabled; }
    unsigned long getAutoWakeInterval() { return userPrefs.autoWakeInterval; }
    
    // GPS getters
    int getGPSUpdateRate();
    bool getGPSLowPowerMode() { return userPrefs.gpsLowPowerMode; }
    float getGPSAccuracyThreshold() { return userPrefs.gpsAccuracyThreshold; }
    
    // Compass getters
    bool getCompassAutoCalibrate() { return userPrefs.compassAutoCalibrate; }
    float getCompassHeadingSmoothing() { return userPrefs.compassHeadingSmoothing; }
    float getCompassMotionThreshold() { return userPrefs.compassMotionThreshold; }
    
    // User info getters
    const char* getUserName() { return userPrefs.userName; }
    const char* getBikeName() { return userPrefs.bikeName; }
    const char* getPreferredUnits() { return userPrefs.preferredUnits; }
    int getTimeZoneOffset() { return userPrefs.timeZoneOffset; }
    
    // Advanced getters
    bool getDebugMode() { return userPrefs.debugMode; }
    int getLogLevel() { return userPrefs.logLevel; }
    bool getTelemetryEnabled() { return userPrefs.telemetryEnabled; }
    
    // ========================================
    // INDIVIDUAL SETTERS (from actual implementation)
    // ========================================
    
    // Display setters (with validation)
    void setDisplayBrightness(int brightness);
    void setAutoNightMode(bool enabled);
    void setNightModeStartHour(int hour) { if (hour >= 0 && hour < 24) { userPrefs.nightModeStartHour = hour; markDirty(); } }
    void setDayModeStartHour(int hour) { if (hour >= 0 && hour < 24) { userPrefs.dayModeStartHour = hour; markDirty(); } }
    void setNightBrightness(int brightness) { userPrefs.nightBrightness = constrain(brightness, 10, 100); markDirty(); }
    
    // Navigation setters
    void setAutoZoomEnabled(bool enabled) { userPrefs.autoZoomEnabled = enabled; markDirty(); }
    void setAutoZoomFactor(float factor) { userPrefs.autoZoomFactor = constrain(factor, 0.5, 2.0); markDirty(); }
    void setDefaultZoomLevel(int level) { userPrefs.defaultZoomLevel = constrain(level, 8, 18); markDirty(); }
    
    // Tracking setters (with validation)
    void setTrackingMode(int mode);
    void setTrackFilterEnabled(bool enabled);
    void setTrackMinDistance(float distance) { userPrefs.trackMinDistance = constrain(distance, 1.0, 100.0); markDirty(); }
    void setTrackMinSpeed(float speed) { userPrefs.trackMinSpeed = constrain(speed, 0.0, 20.0); markDirty(); }
    void setTrackMaxInterval(unsigned long interval) { userPrefs.trackMaxInterval = constrain(interval, 5000UL, 300000UL); markDirty(); }
    void setAutoSaveEnabled(bool enabled) { userPrefs.autoSaveEnabled = enabled; markDirty(); }
    
    // Power setters
    void setSleepTimeout(unsigned long timeout) { userPrefs.sleepTimeout = timeout; markDirty(); }
    void setAutoWakeEnabled(bool enabled) { userPrefs.autoWakeEnabled = enabled; markDirty(); }
    void setAutoWakeInterval(unsigned long interval) { userPrefs.autoWakeInterval = interval; markDirty(); }
    
    // GPS setters (with validation)
    void setGPSUpdateRate(int rate);
    void setGPSLowPowerMode(bool enabled) { userPrefs.gpsLowPowerMode = enabled; markDirty(); }
    void setGPSAccuracyThreshold(float threshold) { userPrefs.gpsAccuracyThreshold = constrain(threshold, 0.5, 10.0); markDirty(); }
    
    // Compass setters
    void setCompassAutoCalibrate(bool enabled) { userPrefs.compassAutoCalibrate = enabled; markDirty(); }
    void setCompassHeadingSmoothing(float factor) { userPrefs.compassHeadingSmoothing = constrain(factor, 0.01, 1.0); markDirty(); }
    void setCompassMotionThreshold(float threshold) { userPrefs.compassMotionThreshold = constrain(threshold, 0.1, 2.0); markDirty(); }
    
    // User info setters
    void setUserName(const char* name) { strncpy(userPrefs.userName, name, sizeof(userPrefs.userName) - 1); markDirty(); }
    void setBikeName(const char* name) { strncpy(userPrefs.bikeName, name, sizeof(userPrefs.bikeName) - 1); markDirty(); }
    void setPreferredUnits(const char* units) { strncpy(userPrefs.preferredUnits, units, sizeof(userPrefs.preferredUnits) - 1); markDirty(); }
    void setTimeZoneOffset(int offset) { userPrefs.timeZoneOffset = constrain(offset, -12, 14); markDirty(); }
    
    // Advanced setters
    void setDebugMode(bool enabled) { userPrefs.debugMode = enabled; markDirty(); }
    void setLogLevel(int level) { userPrefs.logLevel = constrain(level, 0, 5); markDirty(); }
    void setTelemetryEnabled(bool enabled) { userPrefs.telemetryEnabled = enabled; markDirty(); }
    
    // ========================================
    // BULK OPERATIONS (from actual implementation)
    // ========================================
    
    UserPreferences& getPreferences() { return userPrefs; }
    const UserPreferences& getPreferences() const { return userPrefs; }
    void setPreferences(const UserPreferences& prefs) { userPrefs = prefs; markDirty(); }
    
    // ========================================
    // EXPORT/IMPORT (from actual implementation)
    // ========================================
    
    bool exportSettings(const String& filename);  // Export settings to file
    bool importSettings(const String& filename);  // Import settings from file
    
    // ========================================
    // DIAGNOSTIC METHODS
    // ========================================
    
    String getSettingsReport() const;            // Get detailed settings report
    void printSettings() const;                  // Print current settings to serial
    bool selfTest();                            // Run settings system self-test
};

// ========================================
// GLOBAL INSTANCE (from actual implementation)
// ========================================

extern SettingsManager settingsManager;

// ========================================
// GLOBAL ACCESS FUNCTIONS (for compatibility)
// ========================================

// Initialization
bool initializeSettingsManager();               // Initialize settings manager
bool saveAllSettings();                         // Save all settings
bool loadAllSettings();                         // Load all settings
bool resetAllSettings();                        // Reset to defaults

// Quick access functions for commonly used settings
int getCurrentDisplayBrightness();              // Get display brightness
bool isAutoNightModeEnabled();                  // Check auto night mode
int getCurrentTrackingMode();                   // Get tracking mode
bool isTrackFilterEnabled();                    // Check track filtering
float getCurrentGPSUpdateRate();                // FIXED: Get GPS update rate (float)

// Settings modification functions
void setDisplayBrightnessGlobal(int brightness);
void setTrackingModeGlobal(int mode);
void setGPSUpdateRateGlobal(int rate);

// State functions
bool areSettingsLoaded();                       // Check if settings are loaded
bool hasUnsavedSettingsChanges();               // Check for unsaved changes
String getSettingsManagerReport();              // Get settings report

// ========================================
// SETTINGS CONSTANTS
// ========================================

// Validation limits
#define SETTINGS_MIN_BRIGHTNESS          10      // Minimum display brightness
#define SETTINGS_MAX_BRIGHTNESS          100     // Maximum display brightness
#define SETTINGS_MIN_ZOOM_LEVEL          8       // Minimum zoom level
#define SETTINGS_MAX_ZOOM_LEVEL          18      // Maximum zoom level
#define SETTINGS_MIN_GPS_RATE            1       // Minimum GPS update rate (Hz)
#define SETTINGS_MAX_GPS_RATE            10      // Maximum GPS update rate (Hz)
#define SETTINGS_MIN_TRACK_DISTANCE      1.0     // Minimum track distance (m)
#define SETTINGS_MAX_TRACK_DISTANCE      100.0   // Maximum track distance (m)
#define SETTINGS_MIN_TRACK_SPEED         0.0     // Minimum track speed (km/h)
#define SETTINGS_MAX_TRACK_SPEED         20.0    // Maximum track speed (km/h)

// Default values
#define SETTINGS_DEFAULT_BRIGHTNESS      80      // Default brightness
#define SETTINGS_DEFAULT_ZOOM_LEVEL      14      // Default zoom level
#define SETTINGS_DEFAULT_GPS_RATE        10      // Default GPS rate (Hz)
#define SETTINGS_DEFAULT_TRACK_MODE      TRACK_AUTO_MOTION  // Default tracking mode

// File paths
#define SETTINGS_FILE_PATH              "/settings/user_settings.json"
#define SETTINGS_BACKUP_PATH            "/settings/settings_backup.json"

#endif // SETTINGS_MANAGER_H