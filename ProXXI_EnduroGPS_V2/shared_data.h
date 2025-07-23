/*
 * E_NAVI EnduroGPS V2 - Shared Data Structures
 * Central repository for all data structures used across components
 * 
 * FIXED VERSION - Removed TrackDisplayUI definition to prevent conflicts
 * FIXED: Circular dependency with track_management.h resolved
 * FIXED: Forward declarations only, no duplicate class definitions
 * FIXED: All enums consolidated here to prevent multiple definitions
 * FIXED: getGPSUpdateRate() now returns unsigned long to match gps_manager.h
 * 
 * This file contains all shared data structures, enums, and type definitions
 * used throughout the GPS navigation system.
 */

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>

// ========================================
// FORWARD DECLARATIONS
// ========================================
class GPSManager;
class CompassManager;
class SDManager;
class DisplayManager;
class ButtonManager;
class MenuManager;
class TrackManager;
class WiFiManager;
class WebFileServer;
class ConnectModeManager;
class SmartTracker;
class NavigationCore;
class TrackDisplayUI;  // Forward declaration only

// ========================================
// ENUMS - CONSOLIDATED HERE TO PREVENT DUPLICATES
// ========================================

// Button IDs (matching CH423S GPIO pins)
enum ButtonID {
  BTN_UP = 0,              // 5-way navigation up
  BTN_5WAY_UP = 0,         // Compatibility alias
  BTN_DOWN = 1,            // 5-way navigation down  
  BTN_5WAY_DOWN = 1,       // Compatibility alias
  BTN_LEFT = 2,            // 5-way navigation left
  BTN_5WAY_LEFT = 2,       // Compatibility alias
  BTN_RIGHT = 3,           // 5-way navigation right
  BTN_5WAY_RIGHT = 3,      // Compatibility alias
  BTN_CENTER = 4,          // 5-way navigation center/select
  BTN_5WAY_CENTER = 4,     // Compatibility alias
  BTN_ZOOM_IN = 5,         // Zoom in (+)
  BTN_ZOOM_OUT = 6,        // Zoom out (-)
  BTN_WAYPOINT = 7,        // Waypoint/marker button
  BTN_COUNT = 8            // Total number of buttons
};

// Button Events
enum ButtonEvent {
  BTN_EVENT_NONE,           // No event
  BTN_EVENT_PRESS,          // Button pressed (immediate)
  BTN_EVENT_RELEASE,        // Button released
  BTN_EVENT_CLICK,          // Short press and release
  BTN_EVENT_LONG_PRESS,     // Long press (1+ seconds)
  BTN_EVENT_REPEAT,         // Repeat while held
  BTN_EVENT_DOUBLE_CLICK,   // Double click
  BTN_EVENT_WAKE,           // Wake from sleep
  // Legacy aliases
  PRESS = BTN_EVENT_PRESS,
  RELEASE = BTN_EVENT_RELEASE,
  LONG_PRESS = BTN_EVENT_LONG_PRESS,
  DOUBLE_PRESS = BTN_EVENT_DOUBLE_CLICK
};

// Display Mode
enum DisplayMode {
    DISPLAY_DAY_MODE = 0,    // Day mode
    DISPLAY_NIGHT_MODE = 1,  // Night mode
    DISPLAY_AUTO_MODE = 2    // Auto-switching mode
};

// GPS Power Mode
enum GPSPowerMode {
    GPS_POWER_FULL = 0,   // Full power mode
    GPS_POWER_LIGHT = 1,  // Light power save mode
    GPS_POWER_DEEP = 2,   // Deep power save mode
    GPS_POWER_SLEEP = 3,  // Sleep mode
    GPS_POWER_NORMAL = GPS_POWER_FULL,  // Alias for compatibility
    // Additional aliases for menu system compatibility
    GPS_POWER_FULL_PERFORMANCE = GPS_POWER_FULL,
    GPS_POWER_BALANCED = GPS_POWER_LIGHT,
    GPS_POWER_POWER_SAVE = GPS_POWER_DEEP,
    GPS_POWER_STANDBY = GPS_POWER_SLEEP
};

// Button State
enum ButtonState {
  BTN_STATE_IDLE,          // Button not pressed
  BTN_STATE_PRESSED,       // Button currently pressed
  BTN_STATE_DEBOUNCING,    // Debouncing period
  BTN_STATE_HELD,          // Button held down
  BTN_STATE_REPEAT         // Button repeating
};

enum TrackState {
  TRACK_STOPPED,
  TRACK_RECORDING,
  TRACK_PAUSED,
  TRACK_ERROR
};

// CHANGE the BatteryState struct back to an enum with the missing constants:
enum BatteryState {
  BATTERY_UNKNOWN,
  BATTERY_CRITICAL,         // < 11.5V
  BATTERY_LOW,             // 11.5V - 12.0V  
  BATTERY_GOOD,            // 12.0V - 13.5V
  BATTERY_CHARGING,        // > 13.5V (charging)
  BATTERY_FULL            // Fully charged
};

// WiFi States
enum WiFiState {
  WIFI_STATE_OFF,      // Changed from WIFI_OFF
  WIFI_STATE_STARTING,
  WIFI_STATE_AP_MODE,
  WIFI_STATE_CONNECTED,
  WIFI_STATE_ERROR
};

// Menu States
enum MenuState {
  MENU_MAIN,              // Main menu - simplified
  MENU_TRACKING,          // Track recording settings
  MENU_CONNECT,           // WiFi file transfer (V2 feature)
  MENU_DISPLAY,           // Display settings
  MENU_COMPASS,           // Compass settings & calibration
  MENU_GPS,               // GPS settings
  MENU_SYSTEM,            // System info & diagnostics
  MENU_CALIBRATION        // Compass calibration process
};

enum MenuItem {
  // Main Menu Items (Streamlined from V1)
  MAIN_TRACKING = 0,
  MAIN_CONNECT,           // NEW: Connect mode for file transfer
  MAIN_DISPLAY,
  MAIN_COMPASS,
  MAIN_GPS,
  MAIN_SYSTEM,
  MAIN_EXIT,
  
  // Tracking Menu Items (Essential only)
  TRACK_VIEW_CURRENT = 0,     // View current track stats
  TRACK_MODE,         // Off/Manual/Auto
  TRACK_FILTER,           // Enable/Disable filtering
  TRACK_AUTO_SAVE,        // Auto-save tracks
  TRACK_BACK,
  
  // Connect Menu Items (V2 WiFi Features)
  CONNECT_START = 0,      // Start WiFi hotspot
  CONNECT_QR_CODE,        // Show QR code
  CONNECT_STATUS,         // Connection status
  //CONNECT_DONE,           // Finish and re-index
  CONNECT_BACK,
  
  // Display Menu Items (Vision-Optimized)
  DISP_BRIGHTNESS = 0,    // Brightness level
  DISP_NIGHT_MODE,        // Day/Night mode
  DISP_AUTO_NIGHT,        // Auto night mode
  DISP_BACK,
  
  // Compass Menu Items (Simplified)
  COMP_CALIBRATE = 0,     // Start Loop-Loop calibration
  COMP_VIEW_STATUS,       // View calibration status
  COMP_BACK,
  
  // GPS Menu Items (Essential)
  GPS_VIEW_STATUS = 0,    // GPS status and satellites
  MENU_GPS_POWER_MODE,    // GPS power management (renamed to avoid conflict)
  GPS_BACK,
  
  // System Menu Items (Diagnostic)
  SYS_INFO = 0,           // System information
  SYS_DIAGNOSTICS,        // Run diagnostics
  SYS_RESET_SETTINGS,     // Reset to defaults
  SYS_BACK
};

// ADD this enum to shared_data.h (near other enums)
enum CalibrationState {
  CAL_IDLE,                     // Not calibrating
  CAL_PREPARATION,              // Show instructions and wait for user
  CAL_LEFT_LOOP,                // Collecting left loop data (30s)
  CAL_RIGHT_LOOP,               // Collecting right loop data (30s)  
  CAL_PROCESSING,               // Processing calibration data
  CAL_COMPLETE,                 // Calibration successful
  CAL_FAILED                    // Calibration failed
};

struct CompassCalibration {
  float magOffsetX = 0.0f;
  float magOffsetY = 0.0f;
  float magOffsetZ = 0.0f;
  float magScaleX = 1.0f;
  float magScaleY = 1.0f;
  float magScaleZ = 1.0f;
  float declination = 0.0f;
  float calibrationQuality = 0.0f;
  bool isCalibrated = false;
  unsigned long calibrationTime = 0;
  int calibrationPoints = 0;
  String calibrationMethod = "Loop-Loop";
};

enum ConnectModeState {
  CONNECT_IDLE,
  CONNECT_STARTING,
  CONNECT_QR_DISPLAY,           // ADD this
  CONNECT_ACTIVE,
  CONNECT_FILE_TRANSFER,        // ADD this
  CONNECT_STOPPING,             // ADD this
  CONNECT_REINDEXING,           // ADD this
  CONNECT_COMPLETE,             // ADD this
  CONNECT_ERROR,
  CONNECT_DONE
};

// ========================================
// BASIC DATA TYPES & STRUCTURES
// ========================================

// Geographic position
struct Position {
  double latitude = 0.0;
  double longitude = 0.0;
  float altitude = 0.0;              // meters above sea level
  float accuracy = 0.0;              // estimated accuracy in meters
  unsigned long timestamp = 0;       // milliseconds since boot
  bool valid = false;                // position is valid
};

// Speed and heading information
struct Motion {
  float speedKmh = 0.0;              // Speed in km/h
  float speedMs = 0.0;               // Speed in m/s
  float course = 0.0;                // Course over ground (degrees)
  float heading = 0.0;               // Compass heading (degrees)
  float verticalSpeed = 0.0;         // Vertical speed (m/s)
  bool valid = false;                // motion data is valid
};

// Time information
struct TimeInfo {
  int year = 2024;
  int month = 1;
  int day = 1;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int centisecond = 0;
  unsigned long timestamp = 0;       // GPS timestamp
  bool valid = false;                // time is valid
};

// ========================================
// GPS DATA STRUCTURES
// ========================================

struct GPSData {
  // Position Information
  double latitude = 0.0;             // Latitude in decimal degrees
  double longitude = 0.0;            // Longitude in decimal degrees
  float altitude = 0.0;              // Altitude in meters above sea level
  float geoidHeight = 0.0;           // Height above geoid (meters)
  
  // Motion Information
  float speedKmh = 0.0;              // Speed in km/h
  float speedKnots = 0.0;            // Speed in knots
  float course = 0.0;                // Course over ground (degrees true)
  float magneticVariation = 0.0;     // Magnetic variation (degrees)
  
  // Quality Information
  int satellites = 0;                // Number of satellites in view
  int satellitesUsed = 0;            // Number of satellites used in fix
  float hdop = 99.9;                 // Horizontal dilution of precision
  float pdop = 99.9;                 // Position dilution of precision
  float vdop = 99.9;                 // Vertical dilution of precision
  int fixType = 0;                   // 0=no fix, 2=2D, 3=3D
  bool differentialMode = false;     // Differential GPS active
  
  // Time Information
  TimeInfo time;                     // GPS time information
  
  // Data Validity & Age
  bool positionValid = false;        // Position data is valid
  bool speedValid = false;           // Speed data is valid
  bool courseValid = false;          // Course data is valid
  bool altitudeValid = false;        // Altitude data is valid
  bool timeValid = false;            // Time data is valid
  bool isValid = false;              // Overall GPS validity flag
  unsigned long lastUpdate = 0;      // Last successful GPS update
  unsigned long fixAge = 0;          // Age of GPS fix (milliseconds)
  
  // Statistics
  unsigned long totalUpdates = 0;    // Total GPS updates received
  unsigned long validUpdates = 0;    // Valid GPS updates received
  unsigned long errorCount = 0;      // GPS error count
  float successRate = 0.0;           // Update success rate (%)
};

// ========================================
// NAVIGATION DATA - CLEAN VERSION (NO DUPLICATES)
// ========================================

struct NavigationData {
  // Current Position & Motion
  Position currentPosition;
  Motion currentMotion;
  
  // Destination Information
  double destinationLat = 0.0;
  double destinationLon = 0.0;
  String destinationName = "";
  
  // Distance and Direction
  float distanceToDestination = 0.0;   // meters
  float bearingToDestination = 0.0;    // degrees true north
  float crossTrackError = 0.0;         // meters off track (+ = right, - = left)
  
  // Track Following (for GPX tracks)
  float totalTrackDistance = 0.0;      // km total track distance
  float completedDistance = 0.0;       // km completed
  float remainingDistance = 0.0;       // km remaining
  int progressPercent = 0;             // 0-100% progress
  
  // Current Waypoint
  String currentWaypointName = "";
  double currentWaypointLat = 0.0;
  double currentWaypointLon = 0.0;
  float distanceToWaypoint = 0.0;      // meters
  float bearingToWaypoint = 0.0;       // degrees
  bool waypointReached = false;
  
  // Next Waypoint
  String nextWaypointName = "";
  double nextWaypointLat = 0.0;
  double nextWaypointLon = 0.0;
  
  // Route Information
  int totalWaypoints = 0;
  int currentWaypointIndex = 0;
  float averageSpeed = 0.0;            // km/h over last 5 minutes
  
  // Estimated Times (CONSOLIDATED - NO DUPLICATES)
  float estimatedTimeToWaypoint = 0.0; // minutes to current waypoint
  float estimatedTimeToDestination = 0.0; // minutes to final destination
  float estimatedTimeToFinish = 0.0;   // minutes to track end
  
  // Navigation Status
  bool isNavigationActive = false;
  bool isOnTrack = true;               // Following the intended route
  bool hasValidRoute = false;          // A valid route is loaded
  bool arrivedAtDestination = false;
  bool arrivedAtWaypoint = false;      // Just arrived at waypoint
  
  // Route Deviation (CONSOLIDATED - NO DUPLICATES)
  bool routeDeviation = false;         // Off-route warning
  float maxDeviationDistance = 100.0;  // Max allowed deviation (meters)
  unsigned long deviationStartTime = 0; // When deviation started
  
  // Timing & Status
  unsigned long navigationStartTime = 0;   // Start time of navigation
  unsigned long lastUpdate = 0;            // Last navigation calculation
};

// ========================================
// COMPASS & MOTION DATA
// ========================================

struct CompassData {
  // Heading Information
  float magneticHeading = 0.0;             // Raw magnetic heading (0-359)
  float trueHeading = 0.0;                 // True heading (corrected for declination)
  float headingFinal = 0.0;                // Final processed heading (for compatibility)
  float headingError = 0.0;                // Heading error estimate (degrees)
  
  // Motion Detection
  bool motionDetected = false;             // Movement detected
  float motionLevel = 0.0;                 // Motion intensity (0-100%)
  float accelerationX = 0.0;               // X-axis acceleration (g)
  float accelerationY = 0.0;               // Y-axis acceleration (g)
  float accelerationZ = 0.0;               // Z-axis acceleration (g)
  unsigned long lastMotionTime = 0;        // Time of last motion detection
  
  // Calibration Status
  bool isCalibrated = false;               // Compass is calibrated
  float calibrationQuality = 0.0;          // Calibration quality (0-100%)
  unsigned long lastCalibration = 0;       // Last calibration time
  bool calibrationRequired = false;        // Calibration needed
  
  // Environmental
  float temperature = 0.0;                 // Ambient temperature (°C)
  
  // Sensor Status
  bool sensorOK = true;                    // Sensor health status
  
  // Status Information
  bool isValid = false;                    // Data is valid and recent
  unsigned long lastUpdate = 0;            // Last successful update
  unsigned long updateCount = 0;           // Total successful updates
  unsigned long errorCount = 0;            // Total errors

  float heading = 0.0;         // Current heading in degrees
  float declination = 0.0;     // Magnetic declination
  float pitch = 0.0;           // Pitch angle
  float roll = 0.0;            // Roll angle
  
  // Raw magnetometer data
  float rawMagX = 0.0;
  float rawMagY = 0.0; 
  float rawMagZ = 0.0;
  
  // Accelerometer data
  float accelX = 0.0;
  float accelY = 0.0;
  float accelZ = 0.0;
  
  // Status (use existing isCalibrated and isValid from above)
  unsigned long timestamp = 0;
};

// ========================================
// TRACKING DATA
// ========================================

struct TrackingData {
  // Current Track Information
  String currentTrackName = "";           // Name of current track
  String currentTrackFile = "";           // Filename of current track
  bool isRecording = false;               // Currently recording
  bool isPaused = false;                  // Recording is paused
  bool autoSave = true;                   // Auto-save enabled
  
  // Track Statistics
  int totalPoints = 0;                    // Total points recorded
  int validPoints = 0;                    // Valid GPS points
  int filteredPoints = 0;                 // Points filtered out
  float totalDistance = 0.0;              // Total distance (km)
  float sessionDistance = 0.0;            // Current session distance (km)
  unsigned long recordingTime = 0;        // Total recording time (ms)
  unsigned long sessionTime = 0;          // Current session time (ms)
  
  // Track Timing
  unsigned long trackStartTime = 0;       // Track start timestamp
  unsigned long sessionStartTime = 0;     // Session start timestamp
  unsigned long lastPointTime = 0;        // Last point recorded time
  unsigned long pausedTime = 0;           // Total paused time (ms)
  
  // Track Quality
  float averageSpeed = 0.0;               // Average speed (km/h)
  float maxSpeed = 0.0;                   // Maximum speed recorded (km/h)
  float minSpeed = 0.0;                   // Minimum speed recorded (km/h)
  float averageAccuracy = 0.0;            // Average GPS accuracy (m)
  
  // Position Information
  Position lastRecordedPoint;             // Last recorded GPS point
  Position sessionStartPoint;             // Start point of current session
  
  // Track Mode & Settings
  int trackMode = 0;                      // Track recording mode
  bool filterEnabled = true;              // Point filtering enabled
  float filterDistance = 5.0;             // Minimum distance filter (m)
  float filterSpeed = 2.0;                // Minimum speed filter (km/h)
  
  // Status & Alerts
  bool needsSave = false;                 // Track needs to be saved
  bool lowMemory = false;                 // Low memory warning
  bool storageError = false;              // Storage error occurred
  String lastError = "";                  // Last error message
  unsigned long lastErrorTime = 0;        // Last error timestamp
  
  // Statistics
  unsigned long totalTracks = 0;          // Total tracks recorded
  float totalDistanceAllTracks = 0.0;     // Total distance all tracks (km)
  unsigned long totalRecordingTime = 0;   // Total recording time all tracks (ms)
};

// ========================================
// WAYPOINT & TRACK STRUCTURES
// ========================================

struct Waypoint {
  String name = "";                       // Waypoint name
  String description = "";                // Description/notes
  double latitude = 0.0;                  // Latitude
  double longitude = 0.0;                 // Longitude
  float altitude = 0.0;                   // Altitude (meters)
  String category = "";                   // Category/type
  String symbol = "";                     // Map symbol
  unsigned long timestamp = 0;            // Creation timestamp
  bool visited = false;                   // Has been visited
  unsigned long visitTime = 0;            // Time visited
};

struct TrackPoint {
  double latitude = 0.0;                  // Latitude
  double longitude = 0.0;                 // Longitude  
  float altitude = 0.0;                   // Altitude (meters)
  float speed = 0.0;                      // Speed (km/h)
  float course = 0.0;                     // Course (degrees)
  unsigned long timestamp = 0;            // GPS timestamp
  float hdop = 99.9;                      // Horizontal dilution of precision
  int satellites = 0;                     // Number of satellites
  bool valid = false;                     // Point is valid
};

// ========================================
// GPX FILE STRUCTURES
// ========================================

struct GPXTrackPoint {
  double lat = 0.0;                       // Latitude
  double lon = 0.0;                       // Longitude
  float ele = 0.0;                        // Elevation (meters)
  String time = "";                       // ISO 8601 timestamp
  float elevation = 0.0;                  // Elevation (meters)
  float speed = 0.0;                      // Speed (m/s)
  float course = 0.0;                     // Course (degrees)
  float hdop = 0.0;                       // HDOP
  int sat = 0;                            // Satellites
  double latitude = 0.0;     // ADD IF MISSING
  double longitude = 0.0;    // ADD IF MISSING
  float altitude = 0.0;
  unsigned long timestamp = 0;
  bool isValid = false;
  
  // Constructor for easy creation
  GPXTrackPoint() {}
  GPXTrackPoint(double latitude, double longitude, float elevation = 0.0) 
    : lat(latitude), lon(longitude), ele(elevation) {}
};

struct GPXWaypoint {
  double lat = 0.0;                       // Latitude
  double lon = 0.0;                       // Longitude
  float ele = 0.0;                        // Elevation (meters)
  String name = "";                       // Waypoint name
  String desc = "";                       // Description
  String sym = "";                        // Symbol
  String time = "";                       // Timestamp
  double latitude = 0.0;                  // Latitude (alias for lat)
  double longitude = 0.0;                 // Longitude (alias for lon)
  String description = "";                // Description (alias for desc)
  float elevation = 0.0;                  // Elevation (alias for ele)
  String symbol = "";                     // Symbol (alias for sym)
  bool isValid = false;                   // Validity flag
  
  // Constructor for easy creation
  GPXWaypoint() {}
  GPXWaypoint(double latitude, double longitude, const String& waypointName)
    : lat(latitude), lon(longitude), name(waypointName) {}
};

struct GPXTrackSegment {
  GPXTrackPoint* points = nullptr;
  int pointCount = 0;
  int maxPoints = 0;
  String name = "";
  bool isLoaded = false;
};

struct GPXTrack {
  String name = "";                       // Track name
  String description = "";                // Track description
  String comment = "";                    // Comments
  String source = "";                     // Data source
  
  // Track segments (from gpx_parser.h)
  GPXTrackSegment* segments = nullptr;    // Array of track segments
  int segmentCount = 0;                   // Number of segments
  int maxSegments = 0;                    // Maximum segments capacity
  
  // Track points (retained from shared_data.h)
  GPXTrackPoint* points = nullptr;        // Array of track points
  int pointCount = 0;                     // Number of points
  int maxPoints = 0;                      // Maximum points capacity
  
  // Track statistics
  float totalDistance = 0.0;              // Total distance (km)
  unsigned long duration = 0;             // Duration (milliseconds, from gpx_parser.h)
  float maxSpeed = 0.0;                  // Maximum speed (km/h)
  float avgSpeed = 0.0;                   // Average speed (km/h)
  float maxElevation = 0.0;               // Maximum elevation (m)
  float minElevation = 0.0;               // Minimum elevation (m)
  float elevationGain = 0.0;              // Total elevation gain (m)
  float elevationLoss = 0.0;              // Total elevation loss (m)
  float totalElevationGain = 0.0;         // Total elevation gain (m, from gpx_parser.h)
  unsigned long totalTime = 0;            // Total time (milliseconds)
  bool isLoaded = false;                  // Track is loaded (from gpx_parser.h)
  bool isValid = false;                   // Track is valid (from gpx_parser.h)
  
  // Constructor & Destructor
  GPXTrack() {}
  ~GPXTrack() { 
    if (points) delete[] points; 
    if (segments) delete[] segments; 
  }
  
  // Track manipulation
  bool allocatePoints(int count);
  bool addPoint(const GPXTrackPoint& point);
  bool addPoint(double lat, double lon, float ele = 0.0);
  void clear();
  bool isTrackValid() const { return pointCount > 0 && points != nullptr; }
};

// ========================================
// SYSTEM STATUS STRUCTURES
// ========================================

struct SystemStatus {
  // Component Status - COMPLETE SET
  bool gpsActive = false;                 // GPS is active and working
  bool gpsInitialized = false;            // GPS system initialized
  bool compassActive = false;             // Compass is active
  bool compassInitialized = false;        // Compass initialized
  bool sdCardActive = false;              // SD card is mounted
  bool sdCardInitialized = false;         // SD card initialized
  bool displayActive = false;             // Display is working
  bool displayInitialized = false;        // Display initialized
  bool buttonsInitialized = false;        // Buttons initialized
  bool batteryOK = false;                 // Battery is OK
  bool wifiActive = false;                // WiFi is active
  bool wifiEnabled = false;               // WiFi system enabled
  bool webServerActive = false;           // Web server is running
  
  // System Health
  bool systemHealthy = true;              // Overall system health
  float cpuUsage = 0.0;                   // CPU usage percentage
  unsigned long freeHeap = 0;             // Free heap memory
  unsigned long freePSRAM = 0;            // Free PSRAM memory
  unsigned long uptime = 0;             // System uptime (ms)
  float temperature = 0.0;                // System temperature (°C)
  
  // Battery Information
  float batteryVoltage = 0.0;             // Battery voltage (V)
  int batteryLevel = 0;                   // Battery percentage (0-100%)
  bool lowBattery = false;                // Low battery warning
  bool criticalBattery = false;           // Critical battery warning
  
  // Storage Information
  unsigned long sdCardSize = 0;           // SD card size (bytes)
  unsigned long sdCardUsed = 0;           // SD card used space (bytes)
  unsigned long sdCardFree = 0;           // SD card free space (bytes)
  
  // WiFi Information
  int wifiClients = 0;                    // Connected WiFi clients
  String wifiSSID = "";                   // WiFi SSID
  String wifiIP = "";                     // WiFi IP address
  
  // Error Status
  bool hasErrors = false;                 // System has errors
  String lastError = "";                  // Last error message
  unsigned long lastErrorTime = 0;        // Last error timestamp
  int errorCount = 0;                     // Total error count
  
  // Performance
  unsigned long loopTime = 0;             // Main loop time (microseconds)
  unsigned long maxLoopTime = 0;          // Maximum loop time
  float loopsPerSecond = 0.0;             // Main loop frequency
  
  // Update Information
  unsigned long lastUpdate = 0;           // Last status update
  unsigned long updateCount = 0;          // Status update counter
};

// ========================================
// BATTERY STATUS - STRUCT ONLY (FIXED)
// ========================================

struct BatteryStatus {
  // Voltage & Power
  float voltage = 0.0;                    // Battery voltage (V)
  float rawVoltage = 0.0;                 // Raw ADC voltage
  int percentage = 0;                     // Battery percentage (0-100%)
  bool isCharging = false;                // Battery is charging
  
  // Status Flags
  bool isLow = false;                     // Battery is low
  bool isCritical = false;                // Battery is critical
  bool isOK = true;                       // Battery is OK for operation
  bool isGood = true;                     // Battery is in good condition
  bool isFull = false;                    // Battery is full
  
  // Power consumption
  float currentDraw = 0.0;                // Current draw (mA)
  float powerConsumption = 0.0;           // Power consumption (W)
  unsigned long estimatedRuntime = 0;     // Estimated runtime (minutes)
  
  // Status Information
  unsigned long lastUpdate = 0;           // Last battery check
  String statusText = "";                 // Human-readable status
  uint16_t statusColor = 0xFFFF;          // Display color for status
  
  // Historical Data
  float minVoltage = 15.0;                // Minimum voltage seen
  float maxVoltage = 0.0;                 // Maximum voltage seen
  unsigned long totalReadings = 0;        // Total readings taken
};

// ========================================
// CONNECTIVITY STRUCTURES
// ========================================

struct ConnectivityStatus {
  WiFiState wifiState = WIFI_STATE_OFF;
  bool webServerRunning = false;
  String apSSID = "";                     // AP mode SSID
  String apPassword = "";                 // AP mode password
  String apIP = "";                       // AP IP address
  int connectedClients = 0;               // Connected clients
  
  // File transfer status
  bool fileTransferActive = false;
  String transferFilename = "";
  int transferProgress = 0;               // 0-100%
  unsigned long bytesTransferred = 0;
  
  // Statistics
  unsigned long sessionTime = 0;          // Connection session time
  unsigned long totalSessions = 0;        // Total sessions
  unsigned long filesTransferred = 0;     // Files transferred
  unsigned long bytesTotal = 0;           // Total bytes transferred
};

// Constants moved from menu_system.h to avoid circular dependency
#define MAX_TRACKS_DISPLAYED 15

// ========================================
// GLOBAL INSTANCES & ACCESS FUNCTIONS
// ========================================

// Global instances (declared extern, defined in .ino file)
extern GPSManager gpsManager;
extern CompassManager compassManager;
extern SDManager sdManager;
extern DisplayManager display;
extern ButtonManager buttons;
extern MenuManager menuManager;
extern TrackManager trackManager;
extern WiFiManager wifiManager;
extern WebFileServer webServer;
extern ConnectModeManager connectMode;
extern SmartTracker smartTracker;
extern NavigationCore navigationCore;

// Extern declarations for UI components (defined in their respective source files)
extern TrackDisplayUI trackDisplayUI;

// ========================================
// DATA ACCESS FUNCTIONS - COMPLETE SET
// ========================================

// GPS Functions
GPSData getGPSData();                           // Get current GPS data
bool getGPSPosition(double& lat, double& lon);  // Get GPS position
bool getGPSSpeed(float& speed);                 // Get GPS speed
bool getGPSHeading(float& heading);             // Get GPS heading
bool getGPSAltitude(float& altitude);           // Get GPS altitude
bool isGPSValid();                              // Check GPS validity
unsigned long getGPSUpdateRate();  // FIXED: Return unsigned long to match gps_manager.h
bool setGPSUpdateRate(int rate);                // Set GPS update rate

// Compass Functions
CompassData getCompassData();                   // Get compass data
bool getCompassHeading(float& heading);         // Get compass heading
bool isCompassCalibrated();                     // Check calibration status
void startCompassCalibration();                 // Start calibration
float getCompassCalibrationQuality();           // Get calibration quality

// Navigation Functions
NavigationData getNavigationData();             // Get navigation data
double calculateGPSDistance(double lat1, double lon1, double lat2, double lon2);
double calculateGPSBearing(double lat1, double lon1, double lat2, double lon2);
bool calculateDistanceAndBearing(double lat1, double lon1, double lat2, double lon2,
                                float& distance, float& bearing);

// Tracking Functions
TrackingData getTrackingData();                 // Get tracking data
bool startTrackRecording(const String& trackName);
bool stopTrackRecording();
bool pauseTrackRecording();
bool resumeTrackRecording();
bool isTrackRecording();

// System Functions
SystemStatus getSystemStatus();                 // Get system status
BatteryStatus getBatteryStatus();               // Get battery status
BatteryState getBatteryState();    // Add this for the struct version
ConnectivityStatus getConnectivityStatus();     // Get connectivity status
bool isSystemHealthy();                         // Check system health
unsigned long getSystemUptime();                // Get system uptime
float getCPUUsage();                           // Get CPU usage
unsigned long getFreeHeap();                   // Get free heap memory

// Shared Data Initialization
bool initializeSharedData();                    // Initialize shared data system
void updateSystemStatus(const SystemStatus& status);  // Update system status

// Utility Functions
String formatLatitude(double lat, int precision = 6);
String formatLongitude(double lon, int precision = 6);
String formatDistance(float distanceKm);
String formatSpeed(float speedKmh);
String formatDuration(unsigned long seconds);
String formatTimestamp(unsigned long timestamp);

// File Functions
bool saveWaypoint(const Waypoint& waypoint);
bool loadWaypoint(const String& name, Waypoint& waypoint);
bool saveTrackPoint(const TrackPoint& point);
bool loadGPXTrack(const String& filename, GPXTrack& track);
bool saveGPXTrack(const String& filename, const GPXTrack& track);

// ========================================
// MATHEMATICAL CONSTANTS
// ========================================
#define EARTH_RADIUS_KM         6371.0      // Earth radius in kilometers
#define DEGREES_TO_RADIANS      0.0174532925 // π/180
#define RADIANS_TO_DEGREES      57.2957795   // 180/π
#define NAUTICAL_MILE_TO_KM     1.852        // Nautical mile conversion
#define KNOTS_TO_KMH            1.852        // Knots to km/h conversion
#define MPS_TO_KMH              3.6          // m/s to km/h conversion

// ========================================
// TIME CONSTANTS
// ========================================
#define SECONDS_PER_MINUTE      60
#define MINUTES_PER_HOUR        60
#define HOURS_PER_DAY           24
#define SECONDS_PER_HOUR        3600
#define SECONDS_PER_DAY         86400
#define MILLISECONDS_PER_SECOND 1000

// ========================================
// COMPATIBILITY ALIASES
// ========================================

// For code that might use different structure names
typedef GPSData GPSInfo;
typedef NavigationData NavData;
typedef CompassData CompassInfo;
//typedef TrackingData TrackInfo;
typedef SystemStatus SysStatus;
typedef BatteryStatus BattInfo;

// For code that might use different function names
#define getCurrentGPSData() getGPSData()
#define getCurrentCompassData() getCompassData()
#define getCurrentTrackData() getTrackingData()
#define getCurrentSystemData() getSystemStatus()

// ========================================
// STATUS INDICATORS NAMESPACE (ADD TO SHARED_DATA.H)
// ========================================

namespace StatusIndicators {
  // GPS Status Levels
  enum GPSStatus {
    GPS_NO_FIX,
    GPS_POOR,
    GPS_MARGINAL,
    GPS_GOOD
  };
  
  // Recording Status
  enum RecordStatus {
    REC_OFF,
    REC_RECORDING,
    REC_PAUSED,
    REC_STARTING,
    REC_ERROR
  };
}

struct PowerStatistics {
  int cpuFrequency = 240;
  float batteryVoltage = 12.0f;
  unsigned long uptime = 0;
  bool sleepEnabled = true;
  // Add other members as needed
};



#endif // SHARED_DATA_H