/*
 * E_NAVI EnduroGPS - Smart Tracker
 * Motion-Based Intelligent Tracking System
 * 
 * FIXED: TrackState enum vs struct conflict - renamed struct to SmartTrackingState
 * Features: Instant motion response, GPS power management, intelligent 
 *          track recording, wake-on-motion, battery-aware operation
 */

#ifndef SMART_TRACKER_H
#define SMART_TRACKER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "shared_data.h"

// Forward declarations (avoid circular includes)
class SDManager;
class GPSManager;
class CompassManager;

// ========================================
// SMART TRACKING MODES
// ========================================

enum SmartTrackMode {
  SMART_TRACK_OFF,        // No automatic tracking
  SMART_TRACK_MANUAL,     // Manual start/stop only
  SMART_TRACK_AUTO,       // Motion-based auto tracking
  SMART_TRACK_ALWAYS      // Always recording (power permitting)
};

// ========================================
// GPS POWER MANAGEMENT STATES
// ========================================

// Note: Also defined in gps_manager.h, but included here for completeness
enum SmartTrackerGPSPower {
  ST_GPS_POWER_FULL = 0,         // 10Hz updates, continuous mode
  ST_GPS_POWER_LIGHT = 1,        // 5Hz updates, power save mode
  ST_GPS_POWER_DEEP = 2,         // 1Hz updates, deep power save
  ST_GPS_POWER_SLEEP = 3         // No updates, wake on motion
};

// ========================================
// SMART TRACKING STATE - RENAMED TO AVOID CONFLICT
// ========================================

struct SmartTrackingState {
  // Recording Status
  bool isRecording = false;
  bool isPaused = false;
  bool autoStartEnabled = true;
  bool autoStopEnabled = true;
  SmartTrackMode mode = SMART_TRACK_AUTO;
  
  // Current Session
  String currentTrackName = "";
  String currentTrackFile = "";
  unsigned long trackStartTime = 0;
  unsigned long sessionStartTime = 0;
  unsigned long totalRecordingTime = 0;
  unsigned long pausedTime = 0;
  unsigned long lastPointTime = 0;
  
  // Track Statistics
  int totalPoints = 0;
  int validPoints = 0;
  int filteredPoints = 0;
  float totalDistance = 0.0;              // km
  float sessionDistance = 0.0;            // km this session
  float maxSpeed = 0.0;                   // km/h max this session
  float averageSpeed = 0.0;               // km/h average this session
  
  // Power Management
  SmartTrackerGPSPower gpsPowerState = ST_GPS_POWER_FULL;
  bool needsResume = false;               // Resume after wake
  bool lowPowerMode = false;              // System in low power mode
  
  // Motion Detection
  bool motionDetected = false;
  unsigned long lastMotionTime = 0;
  float lastMotionLevel = 0.0;
  
  // Session Recovery
  bool hasUnsavedData = false;
  unsigned long lastSaveTime = 0;
  uint32_t magicNumber = 0xDEADBEEF;      // Validation for NVS
};

// ========================================
// MOTION DETECTION CONFIGURATION
// ========================================

struct MotionDetectionConfig {
  bool enabled = true;
  float threshold = 0.3f;                 // g-force for motion detection
  unsigned long debounceTime = 500;       // Debounce motion detection (ms)
  unsigned long autoStartDelay = 2000;    // Delay before auto-start (ms)
  unsigned long autoStopDelay = 30000;    // 30 seconds until auto-stop
  unsigned long autoSleepDelay = 60000;   // 1 minute until sleep
  bool instantResponse = true;            // Respond to motion immediately
  bool useGPSSpeed = false;               // Don't wait for GPS speed
  bool requireGPSFix = true;              // Require GPS fix before recording
  float minSpeedThreshold = 2.0f;         // Minimum speed for recording (km/h)
};

// ========================================
// WAKE SYSTEM CONFIGURATION
// ========================================

struct WakeSystemConfig {
  bool motionWakeEnabled = true;          // Wake on motion
  bool timerWakeEnabled = true;           // Wake every hour for GPS check
  bool gpsWakeEnabled = false;            // Wake on GPS fix (if supported)
  unsigned long timerWakeInterval = 3600000; // 1 hour timer wake
  unsigned long motionTimeout = 300000;   // 5 minutes motion timeout
  
  // Wake state tracking
  unsigned long lastTimerWake = 0;
  unsigned long lastMotionWake = 0;
  bool wasWokenByMotion = false;
  bool wasWokenByTimer = false;
  bool wasWokenByGPS = false;
  
  // Wake statistics
  int totalMotionWakes = 0;
  int totalTimerWakes = 0;
  int totalGPSWakes = 0;
};

// ========================================
// INTELLIGENT FILTERING CONFIGURATION
// ========================================

struct TrackFilteringConfig {
  bool enabled = true;
  
  // Distance filtering
  float minDistance = 5.0f;               // Minimum distance between points (m)
  float maxDistance = 1000.0f;            // Maximum distance between points (m)
  
  // Speed filtering
  float minSpeed = 1.0f;                  // Minimum speed for recording (km/h)
  float maxSpeed = 200.0f;                // Maximum believable speed (km/h)
  
  // Time filtering
  unsigned long minInterval = 1000;       // Minimum time between points (ms)
  unsigned long maxInterval = 30000;      // Maximum time between points (ms)
  
  // Quality filtering
  int minSatellites = 4;                  // Minimum satellites required
  float maxHDOP = 5.0f;                   // Maximum HDOP allowed
  
  // Adaptive filtering
  bool adaptiveFiltering = true;          // Adapt filtering based on conditions
  float adaptiveSpeedFactor = 2.0f;       // Speed-based distance scaling
  float adaptiveTurnThreshold = 15.0f;    // Degrees for turn detection
  
  // Advanced filtering
  bool douglasPeucker = false;            // Enable Douglas-Peucker simplification
  float dpEpsilon = 2.0f;                 // Douglas-Peucker epsilon (meters)
  bool motionBasedFiltering = true;       // Use motion detection for filtering
};

// ========================================
// POWER MANAGEMENT INTEGRATION
// ========================================

struct PowerManagementConfig {
  bool enabled = true;
  
  // Battery thresholds
  float lowBatteryThreshold = 11.5f;      // Voltage for low battery mode
  float criticalBatteryThreshold = 11.0f; // Voltage for emergency stop
  
  // Power state management
  bool aggressivePowerSave = false;       // Aggressive power saving
  bool batteryAwareRecording = true;      // Stop recording on low battery
  bool wakeOnCharging = true;             // Wake when charging detected
  
  // GPS power management
  unsigned long gpsFullPowerTime = 30000; // Keep GPS full power for 30s after motion
  unsigned long gpsLightPowerTime = 60000; // Switch to light power after 1 min
  unsigned long gpsDeepPowerTime = 300000; // Switch to deep power after 5 min
  
  // System power management
  bool enableDeepSleep = true;            // Allow deep sleep mode
  unsigned long deepSleepTimeout = 300000; // 5 minutes until deep sleep
  bool maintainRTC = true;                // Keep RTC running during sleep
};

// ========================================
// SMART TRACKER STATISTICS
// ========================================

struct SmartTrackerStats {
  // Session statistics
  unsigned long totalSessions = 0;
  unsigned long totalRecordingTime = 0;   // Total recording time (ms)
  unsigned long totalPoints = 0;          // Total points recorded
  float totalDistance = 0.0f;             // Total distance recorded (km)
  
  // Performance statistics
  unsigned long totalMotionEvents = 0;
  unsigned long totalAutoStarts = 0;
  unsigned long totalAutoStops = 0;
  unsigned long totalWakeEvents = 0;
  
  // Power statistics
  unsigned long totalPowerTransitions = 0;
  unsigned long timeInLowPower = 0;       // Time spent in low power (ms)
  unsigned long timeInDeepSleep = 0;      // Time spent in deep sleep (ms)
  
  // Error statistics
  unsigned long gpsTimeouts = 0;
  unsigned long sdCardErrors = 0;
  unsigned long motionSensorErrors = 0;
  unsigned long powerManagementErrors = 0;
  
  // Quality statistics
  float averageGPSQuality = 0.0f;
  float averagePointInterval = 0.0f;      // Average time between points (ms)
  int filteredPointsRatio = 0;            // Percentage of points filtered out
};

// ========================================
// MAIN SMART TRACKER CLASS
// ========================================

class SmartTracker {
  private:
    // Core Component References
    SDManager* sdManager = nullptr;
    GPSManager* gpsManager = nullptr;
    CompassManager* compassManager = nullptr;
    
    // State Management
    bool initialized = false;
    SmartTrackingState trackingState;
    MotionDetectionConfig motionConfig;
    WakeSystemConfig wakeConfig;
    TrackFilteringConfig filterConfig;
    PowerManagementConfig powerConfig;
    SmartTrackerStats statistics;
    
    // NVS Storage
    Preferences nvs;
    bool nvsInitialized = false;
    
    // Motion Tracking
    bool motionDetected = false;
    unsigned long lastMotionTime = 0;
    unsigned long lastStationaryTime = 0;
    float currentMotionLevel = 0.0f;
    float motionHistory[10] = {0};          // Motion level history
    int motionHistoryIndex = 0;
    
    // GPS Point Tracking
    double lastRecordedLat = 0.0;
    double lastRecordedLon = 0.0;
    unsigned long lastGPSPointTime = 0;
    float lastGPSSpeed = 0.0f;
    float lastGPSHeading = 0.0f;
    
    // Power Management
    SmartTrackerGPSPower currentGPSPower = ST_GPS_POWER_FULL;
    unsigned long lastPowerTransition = 0;
    bool systemSleeping = false;
    bool lowPowerModeActive = false;
    
    // Internal Methods
    bool initializeNVS();
    bool loadConfigurationFromNVS();
    bool saveConfigurationToNVS();
    bool loadTrackingStateFromNVS();
    bool saveTrackingStateToNVS();
    bool validatePersistedState(const SmartTrackingState& state);
    void resetToDefaults();
    
    // Motion Processing
    void updateMotionDetection();
    void processMotionEvent(bool motionDetected, float motionLevel);
    bool shouldStartRecording();
    bool shouldStopRecording();
    bool shouldPauseRecording();
    bool shouldResumeRecording();
    
    // GPS Point Processing
    bool shouldRecordGPSPoint();
    bool validateGPSPoint();
    void recordGPSPoint();
    bool applyPointFiltering(double lat, double lon, float speed, float heading);
    float calculateDistanceFromLastPoint(double lat, double lon);
    
    // Power Management
    void updatePowerManagement();
    void transitionGPSPower(SmartTrackerGPSPower newPower);
    bool shouldEnterLowPowerMode();
    bool shouldEnterDeepSleep();
    void prepareForSleep();
    void wakeFromSleep(bool motionWake, bool timerWake);
    
    // Statistics
    void updateStatistics();
    void recordMotionEvent();
    void recordAutoStart();
    void recordAutoStop();
    void recordWakeEvent();
    void recordPowerTransition();
    
  public:
    SmartTracker();
    ~SmartTracker();
    
    // ========================================
    // INITIALIZATION & CONFIGURATION
    // ========================================
    bool initialize(SDManager* sd, GPSManager* gps, CompassManager* compass);
    bool shutdown();
    bool reset();
    bool isInitialized() const { return initialized; }
    
    // Configuration Management
    void setMotionConfig(const MotionDetectionConfig& config);
    MotionDetectionConfig getMotionConfig() const { return motionConfig; }
    void setWakeConfig(const WakeSystemConfig& config);
    WakeSystemConfig getWakeConfig() const { return wakeConfig; }
    void setFilterConfig(const TrackFilteringConfig& config);
    TrackFilteringConfig getFilterConfig() const { return filterConfig; }
    void setPowerConfig(const PowerManagementConfig& config);
    PowerManagementConfig getPowerConfig() const { return powerConfig; }
    
    // ========================================
    // CORE TRACKING FUNCTIONALITY
    // ========================================
    void update();                          // Main update function (Core 1)
    void processMotionInterrupt();          // Handle motion interrupt
    void processGPSUpdate();                // Handle GPS data update
    void processPowerStateChange();         // Handle power state changes
    
    // Manual Track Control
    bool startTracking(const String& trackName = "");
    bool stopTracking();
    bool pauseTracking();
    bool resumeTracking();
    bool isRecording() const { return trackingState.isRecording; }
    bool isPaused() const { return trackingState.isPaused; }
    
    // Automatic Control
    void enableAutoStart(bool enable);
    void enableAutoStop(bool enable);
    bool isAutoStartEnabled() const { return trackingState.autoStartEnabled; }
    bool isAutoStopEnabled() const { return trackingState.autoStopEnabled; }
    
    // ========================================
    // STATE & STATUS QUERIES
    // ========================================
    
    // Tracking State
    SmartTrackingState getTrackingState() const { return trackingState; }
    TrackState getCurrentTrackState();      // Convert to shared_data.h enum
    String getCurrentTrackName() const { return trackingState.currentTrackName; }
    unsigned long getSessionDuration();     // Current session duration (ms)
    float getSessionDistance() const { return trackingState.sessionDistance; }
    int getSessionPoints() const { return trackingState.totalPoints; }
    
    // Motion Status
    bool isMotionDetected() const { return motionDetected; }
    float getCurrentMotionLevel() const { return currentMotionLevel; }
    unsigned long getTimeSinceLastMotion();
    bool isStationary();
    
    // Power Status
    SmartTrackerGPSPower getCurrentGPSPower() const { return currentGPSPower; }
    bool isInLowPowerMode() const { return lowPowerModeActive; }
    bool isSystemSleeping() const { return systemSleeping; }
    
    // ========================================
    // STATISTICS & DIAGNOSTICS
    // ========================================
    
    // Performance Statistics
    SmartTrackerStats getStatistics() const { return statistics; }
    void resetStatistics();
    float getFilteringEfficiency();         // Percentage of points filtered
    float getAverageGPSQuality();
    unsigned long getTotalRecordingTime();
    
    // Diagnostic Information
    String getDiagnosticInfo();
    void printDiagnostics();
    void printConfiguration();
    void printTrackingState();
    bool runDiagnosticTest();
    
    // System Health
    bool isSystemHealthy();
    unsigned long getUptime();
    int getErrorCount();
    void clearErrors();
    
    // ========================================
    // SLEEP & WAKE MANAGEMENT
    // ========================================
    
    // Sleep Control
    void enterLowPowerMode();
    void exitLowPowerMode();
    void enterDeepSleep();
    void setupWakeInterrupts();
    void handleWakeEvent(bool motionWake, bool timerWake, bool gpsWake);
    
    // Wake Statistics
    int getTotalWakeEvents();
    int getMotionWakeCount();
    int getTimerWakeCount();
    unsigned long getLastWakeTime();
    bool wasLastWakeMotion();
    
    // ========================================
    // CONFIGURATION PRESETS
    // ========================================
    
    // Preset Configurations
    void applyEnduroConfig();               // Optimized for enduro riding
    void applyTouringConfig();              // Optimized for touring
    void applyUrbanConfig();                // Optimized for urban riding
    void applyPowerSaveConfig();            // Maximum battery life
    void applyHighPrecisionConfig();        // Maximum accuracy
    
    // Advanced Configuration
    void enableAggressiveFiltering(bool enable);
    void setMotionSensitivity(float sensitivity);
    void setBatteryThresholds(float low, float critical);
    void setRecordingQualityThresholds(int minSats, float maxHDOP);
};

// ========================================
// GLOBAL SMART TRACKER INSTANCE
// ========================================
extern SmartTracker smartTracker;

// ========================================
// GLOBAL ACCESS FUNCTIONS
// ========================================

// Initialization
bool initializeSmartTracker();
void updateSmartTracker();
bool shutdownSmartTracker();

// Quick Status Functions
bool isTrackRecording();
bool isMotionDetected();
float getMotionLevel();
String getCurrentTrackName();
float getSessionDistance();

// Quick Control Functions
bool startTracking(const String& trackName = "");
bool stopTracking();
bool pauseTracking();
bool resumeTracking();

// Configuration Functions
void enableAutoTracking(bool enable);
void setMotionThreshold(float threshold);
void enablePowerSaveMode(bool enable);

// Statistics Functions
SmartTrackerStats getSmartTrackerStats();
void printSmartTrackerDiagnostics();
void resetSmartTrackerStats();

// ========================================
// CONFIGURATION CONSTANTS
// ========================================

// Default motion detection settings
#define DEFAULT_MOTION_THRESHOLD        0.3f    // g
#define DEFAULT_AUTO_START_DELAY        2000    // ms
#define DEFAULT_AUTO_STOP_DELAY         30000   // ms
#define DEFAULT_AUTO_SLEEP_DELAY        60000   // ms

// Default filtering settings
#define DEFAULT_MIN_DISTANCE           5.0f     // meters
#define DEFAULT_MIN_SPEED              1.0f     // km/h
#define DEFAULT_MIN_INTERVAL           1000     // ms
#define DEFAULT_MIN_SATELLITES         4       // satellites
#define DEFAULT_MAX_HDOP               5.0f     // HDOP

// Default power management settings
#define DEFAULT_LOW_BATTERY_THRESHOLD   11.5f   // volts
#define DEFAULT_CRITICAL_BATTERY_THRESHOLD 11.0f // volts
#define DEFAULT_GPS_FULL_POWER_TIME     30000   // ms
#define DEFAULT_DEEP_SLEEP_TIMEOUT      300000  // ms

// Wake system settings
#define DEFAULT_TIMER_WAKE_INTERVAL     3600000 // ms (1 hour)
#define DEFAULT_MOTION_TIMEOUT          300000  // ms (5 minutes)

#endif // SMART_TRACKER_H