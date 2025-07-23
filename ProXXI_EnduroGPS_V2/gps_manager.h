/*
 * E_NAVI EnduroGPS V2 - GPS Manager
 * Professional-grade GPS management for serious navigation
 * 
 * FINAL FIXED VERSION - All conflicts resolved
 * FIXED: getGPSUpdateRate() now returns unsigned long (matches shared_data.h)
 * FIXED: Added missing methods expected by main code
 * FIXED: Consistent function signatures throughout
 * Features: 10Hz high precision NMEA processing, power management,
 *          advanced GPS configuration, comprehensive statistics
 */

#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "config.h"
#include "shared_data.h"

// ========================================
// GPS POWER STATES - NOW CONFLICT-FREE
// ========================================

// enum GPSPowerState {
//   GPS_POWER_FULL = 0,          // 10Hz updates, full performance
//   GPS_POWER_LIGHT = 1,         // 5Hz updates, reduced power
//   GPS_POWER_DEEP = 2,          // 1Hz updates, minimum power
//   GPS_POWER_SLEEP = 3          // No updates, wake on command
// };

// ========================================
// GPS QUALITY ASSESSMENT
// ========================================

struct GPSQuality {
  float fixQuality = 0.0;        // 0-100% overall quality
  float satelliteScore = 0.0;    // Based on satellite count
  float hdopScore = 0.0;         // Based on HDOP value
  float signalScore = 0.0;       // Based on signal strength
  bool isExcellent = false;      // HDOP < 1.0, 8+ satellites
  bool isGood = false;           // HDOP < 2.0, 5+ satellites
  bool isMarginal = false;       // HDOP < 5.0, 4+ satellites
  
  // Additional quality metrics
  float geometryScore = 0.0;     // Satellite geometry score
  float accuracyEstimate = 0.0;  // Estimated accuracy in meters
  bool isDifferential = false;   // Differential GPS active
  unsigned long lastUpdate = 0;  // When quality was last calculated
};

// ========================================
// GPS STATISTICS & PERFORMANCE
// ========================================

struct GPSStatistics {
  // Message Processing
  unsigned long totalSentences = 0;     // Total NMEA sentences received
  unsigned long validSentences = 0;     // Valid sentences processed
  unsigned long errorSentences = 0;     // Sentences with errors
  unsigned long unknownSentences = 0;   // Unknown sentence types
  
  // Fix Statistics
  unsigned long totalFixes = 0;         // Total GPS fixes obtained
  unsigned long validFixes = 0;         // Valid 2D/3D fixes
  unsigned long invalidFixes = 0;       // Invalid/poor fixes
  float averageFixTime = 0.0;           // Average time to get fix (seconds)
  
  // Quality Statistics
  float averageHDOP = 0.0;              // Average HDOP over time
  float averageSatellites = 0.0;        // Average satellite count
  int maxSatellites = 0;                // Maximum satellites seen
  float bestHDOP = 99.9;                // Best HDOP achieved
  
  // Performance Metrics
  float successRate = 0.0;              // Sentence success rate (%)
  float fixRate = 0.0;                  // Fix success rate (%)
  unsigned long uptime = 0;             // GPS uptime (ms)
  unsigned long totalBytes = 0;         // Total bytes processed
  
  // Error Tracking
  unsigned long timeoutErrors = 0;      // Communication timeouts
  unsigned long checksumErrors = 0;     // Checksum failures
  unsigned long formatErrors = 0;       // Format/parsing errors
  
  // Update Information
  unsigned long lastStatsUpdate = 0;    // Last statistics update
  unsigned long statisticsVersion = 1;  // Statistics version counter
};

// ========================================
// GPS CONFIGURATION
// ========================================

struct GPSConfig {
  // Update Rates
  int nominalUpdateRate = 10;           // Target update rate (Hz)
  int currentUpdateRate = 10;           // Current actual rate
  bool adaptiveRate = true;             // Adapt rate based on conditions
  
  // Power Management
  GPSPowerMode powerState = GPS_POWER_FULL;
  bool enablePowerManagement = true;
  unsigned long powerStateTimeout = 30000; // Timeout for power transitions
  
  // Message Configuration
  bool enableGGA = true;                // Enable GGA messages
  bool enableRMC = true;                // Enable RMC messages
  bool enableGSV = false;               // Enable GSV messages (satellite info)
  bool enableGSA = false;               // Enable GSA messages (DOP info)
  bool enableVTG = true;                // Enable VTG messages (velocity)
  
  // Quality Thresholds
  int minSatellites = 4;                // Minimum satellites for valid fix
  float maxHDOP = 5.0;                  // Maximum acceptable HDOP
  float excellentHDOP = 1.0;            // HDOP threshold for excellent fix
  float goodHDOP = 2.0;                 // HDOP threshold for good fix
  
  // Timing Configuration
  unsigned long fixTimeout = 120000;    // Cold start timeout (2 minutes)
  unsigned long dataTimeout = 5000;     // Data reception timeout
  unsigned long validityTimeout = 10000; // Fix validity timeout
  
  // Advanced Settings
  bool enableSBAS = true;               // Enable SBAS (WAAS/EGNOS)
  bool enableGalileoSupport = false;    // Enable Galileo constellation
  bool enableGLONASS = false;           // Enable GLONASS constellation
  bool enableHighDynamics = true;       // High dynamics mode for vehicles
};

// ========================================
// UBX MESSAGE DEFINITIONS
// ========================================

// UBX message templates for GPS configuration
extern const uint8_t UBX_CFG_RATE_10HZ[];      // 10Hz update rate
extern const uint8_t UBX_CFG_RATE_5HZ[];       // 5Hz update rate  
extern const uint8_t UBX_CFG_RATE_1HZ[];       // 1Hz update rate
extern const uint8_t UBX_CFG_MSG_GGA_ON[];     // Enable GGA messages
extern const uint8_t UBX_CFG_MSG_RMC_ON[];     // Enable RMC messages
extern const uint8_t UBX_CFG_PM2_CONTINUOUS[]; // Continuous mode
extern const uint8_t UBX_CFG_PM2_POWERSAVE[];  // Power save mode

// ========================================
// GPS MANAGER CLASS
// ========================================

class GPSManager {
  private:
    // Core Components
    TinyGPSPlus gpsParser;              // NMEA parser
    HardwareSerial* gpsSerial;          // Hardware serial interface
    
    // State Management
    bool initialized = false;
    bool taskRunning = false;           // Task running status
    bool configurationComplete = false;
    bool hasEverHadFix = false;
    unsigned long initializationTime = 0;
    
    // Data Tracking
    unsigned long lastValidFix = 0;     // Time of last valid fix
    unsigned long lastDataReceived = 0; // Time of last data reception
    unsigned long lastPositionUpdate = 0; // Time of last position update
    unsigned long fixAcquisitionTime = 0; // Time when fix was first acquired
    
    // Configuration & Statistics
    GPSConfig config;
    GPSStatistics stats;
    GPSQuality currentQuality;
    
    // Power Management
    GPSPowerMode currentPowerMode = GPS_POWER_FULL;
    unsigned long lastPowerChange = 0;
    bool powerTransitionPending = false;
    
    // Update Rate Tracking
    float actualUpdateRate = 10.0;      // Measured update rate
    unsigned long lastUpdateTime = 0;   // Last update timestamp
    unsigned long updateCount = 0;      // Update counter for rate calculation
    
    // Internal Methods
    bool sendUBXCommand(const uint8_t* command, size_t length);
    bool configureUpdateRate(int hz);
    bool configureMessageTypes();
    bool configurePowerMode(GPSPowerMode mode);
    void updateStatistics();
    void calculateQuality();
    void updateActualRate();            // Calculate actual update rate
    void resetInternalStatistics();     // Private statistics reset
    
  public:
    GPSManager();
    ~GPSManager();
    
    // ========================================
    // INITIALIZATION & CONFIGURATION
    // ========================================
    bool initialize();
    bool shutdown();
    bool reset();
    bool isInitialized() const { return initialized; }
    bool isTaskRunning() const { return taskRunning; }
    
    // Configuration Management
    bool applyConfiguration(const GPSConfig& newConfig);
    GPSConfig getConfiguration() const { return config; }
    bool setUpdateRate(int hz);
    bool setPowerMode(GPSPowerMode mode);
    bool enableMessageType(const String& messageType, bool enable);
    
    // Hardware Configuration
    bool configureNEO7M();              // Configure NEO-7M specific settings
    bool configureNEO8M();              // Configure NEO-8M specific settings
    bool testCommunication();           // Test GPS communication
    bool validateConfiguration();       // Validate current configuration
    
    // ========================================
    // CORE DATA PROCESSING
    // ========================================
    void update();                      // Main update function (Core 1)
    void processIncomingData();         // Process incoming NMEA data
    bool hasNewData();                  // Check for new GPS data
    void clearDataFlags();              // Clear new data flags
    
    // ========================================
    // DATA ACCESS FUNCTIONS
    // ========================================
    
    // Position Information
    bool getPosition(double& lat, double& lon);
    bool getLatitude(double& lat);
    bool getLongitude(double& lon);
    bool getAltitude(float& altMeters);
    bool getGeoidHeight(float& geoidMeters);
    
    // Motion Information  
    bool getSpeed(float& speedKmh);
    bool getSpeed(float& speedKmh, float& courseHeading);
    bool getCourse(float& courseDegrees);
    bool getVerticalSpeed(float& verticalSpeedMps);
    
    // Time Information
    bool getDateTime(int& year, int& month, int& day, int& hour, int& minute, int& second);
    bool getTime(int& hour, int& minute, int& second, int& centisecond);
    bool getDate(int& year, int& month, int& day);
    unsigned long getGPSTimestamp();
    
    // Quality & Accuracy Information
    bool getSatelliteInfo(int& satellites, int& satellitesUsed);
    bool getHDOP(float& hdop);
    bool getPDOP(float& pdop);
    bool getVDOP(float& vdop);
    bool getFixType(int& fixType);
    
    // ========================================
    // STATUS & VALIDATION
    // ========================================
    
    // Fix Status
    bool hasValidFix();
    bool is3DFix();
    bool isDifferentialFix();
    unsigned long getTimeSinceLastFix();
    unsigned long getFixAge();
    bool isFixStale();
    
    // Data Validity
    bool isPositionValid();
    bool isSpeedValid();
    bool isCourseValid();
    bool isAltitudeValid();
    bool isTimeValid();
    
    // Quality Assessment
    GPSQuality getQuality();
    float getFixQuality();              // 0-100% overall quality score
    bool isQualityGood();               // Above good threshold
    bool isQualityExcellent();          // Above excellent threshold
    String getQualityDescription();      // Human-readable quality
    
    // ========================================
    // STATISTICS & DIAGNOSTICS
    // ========================================
    
    // Performance Statistics
    GPSStatistics getStatistics();
    void resetStatistics();             // Only one declaration (public)
    float getSuccessRate();             // NMEA sentence success rate
    float getFixRate();                 // GPS fix success rate
    unsigned long getUptime();          // GPS uptime in milliseconds
    
    // Update Rate Information - FIXED: Now returns unsigned long
    unsigned long getCurrentUpdateRate() const;    // FIXED: Returns unsigned long
    int getConfiguredUpdateRate() const { return config.currentUpdateRate; }
    float getMeasuredUpdateRate() const { return actualUpdateRate; }
    
    // Diagnostic Information
    String getDiagnosticInfo();         // Comprehensive diagnostic string
    void printDiagnostics();            // Print diagnostics to Serial
    void printStatistics();             // Print statistics to Serial
    bool runDiagnosticTest();           // Run complete diagnostic test
    
    // Error Information
    unsigned long getErrorCount();      // Total errors
    String getLastError();              // Last error description
    void clearErrors();                 // Clear error counters
    
    // ========================================
    // POWER MANAGEMENT
    // ========================================
    
    // Power State Control
    bool transitionToPowerState(GPSPowerMode newState);
    GPSPowerMode getCurrentPowerState() const { return currentPowerMode; }
    bool isPowerTransitionPending() const { return powerTransitionPending; }
    String getPowerStateDescription();
    
    // Power Optimization
    void optimizePowerUsage();          // Optimize based on usage patterns
    bool shouldReducePower();           // Check if power should be reduced
    void handleMotionDetected();        // Handle motion detection event
    void handleNoMotion();              // Handle no motion detected
    
    // ========================================
    // ADVANCED FEATURES
    // ========================================
    
    // Navigation Support
    bool getDistanceAndBearing(double lat, double lon, float& distance, float& bearing);
    bool getSpeedOverGround(float& sogKmh);
    bool getCourseOverGround(float& cogDegrees);
    
    // Waypoint Navigation
    bool calculateDistanceToWaypoint(double waypointLat, double waypointLon, float& distance);
    bool calculateBearingToWaypoint(double waypointLat, double waypointLon, float& bearing);
    bool isNearWaypoint(double waypointLat, double waypointLon, float radiusMeters);
    
    // Track Recording Support
    bool shouldRecordPoint();           // Check if point should be recorded
    bool hasSignificantMovement();      // Check for significant position change
    float getDistanceFromLastPoint();   // Distance from last recorded point
    
    // ========================================
    // TASK MANAGEMENT - FOR COMPATIBILITY
    // ========================================
    void startTask();                   // Start GPS task
    void stopTask();                    // Stop GPS task
    void setTaskRunning(bool running) { taskRunning = running; }
    bool isTaskActive() const { return taskRunning; }  // Alternative name

    void wakeFromSleep();      // ADD THIS
    void enterLightSleep();    // ADD THIS  
    void enterDeepSleep();     // ADD THIS
};

// ========================================
// GLOBAL GPS MANAGER INSTANCE
// ========================================
extern GPSManager gpsManager;

// ========================================
// GLOBAL ACCESS FUNCTIONS - FIXED SIGNATURES
// ========================================

// Initialization
bool initializeGPS();
void updateGPS();
bool shutdownGPS();

// Quick Data Access
bool getGPSPosition(double& lat, double& lon);
bool getGPSSpeed(float& speed);
bool getGPSHeading(float& heading);
bool getGPSAltitude(float& altitude);
bool isGPSValid();

// Status Functions  
GPSQuality getGPSQuality();
float getGPSFixQuality();
bool hasGPSFix();
unsigned long getGPSUptime();

// Power Management
bool setGPSPowerMode(GPSPowerMode mode);
GPSPowerMode getGPSPowerMode();

// Configuration - FIXED: Consistent return types
bool setGPSUpdateRate(int hz);
unsigned long getGPSUpdateRate();      // FIXED: Now returns unsigned long
float getCurrentGPSUpdateRate();       // Get measured update rate (float)

// Statistics
GPSStatistics getGPSStatistics();
void printGPSDiagnostics();

// ========================================
// UTILITY FUNCTIONS - FIXED SIGNATURES
// ========================================

// FIXED: Now matches shared_data.h function signatures (double return type)
double calculateGPSDistance(double lat1, double lon1, double lat2, double lon2);
double calculateGPSBearing(double lat1, double lon1, double lat2, double lon2);

// Additional GPS utility functions
float normalizeHeading(float heading);
bool isValidLatitude(double lat);
bool isValidLongitude(double lon);
String formatGPSCoordinate(double coord, bool isLatitude);
String formatGPSPosition(double lat, double lon);

// ========================================
// GPS MESSAGE CONFIGURATION CONSTANTS
// ========================================

// UBX Configuration Messages
extern const uint8_t UBX_CFG_RATE_10HZ[];
extern const uint8_t UBX_CFG_RATE_5HZ[];
extern const uint8_t UBX_CFG_RATE_1HZ[];
extern const uint8_t UBX_CFG_MSG_GGA_ON[];
extern const uint8_t UBX_CFG_MSG_RMC_ON[];
extern const uint8_t UBX_CFG_PM2_CONTINUOUS[];
extern const uint8_t UBX_CFG_PM2_POWERSAVE[];

// Message lengths
#define UBX_CFG_RATE_LENGTH     14
#define UBX_CFG_MSG_LENGTH      11  
#define UBX_CFG_PM2_LENGTH      48

// ========================================
// GPS CONFIGURATION PRESETS
// ========================================

// Standard GPS configurations for different use cases
extern const GPSConfig HIGH_PERFORMANCE_CONFIG;    // Maximum accuracy & update rate
extern const GPSConfig BALANCED_CONFIG;            // Good balance of accuracy & power
extern const GPSConfig POWER_SAVE_CONFIG;          // Optimized for battery life
extern const GPSConfig NAVIGATION_CONFIG;          // Optimized for navigation
extern const GPSConfig TRACKING_CONFIG;            // Optimized for track recording

// Apply configuration presets
bool applyHighPerformanceGPS();
bool applyBalancedGPS();
bool applyPowerSaveGPS();
bool applyNavigationGPS();
bool applyTrackingGPS();

#endif // GPS_MANAGER_H