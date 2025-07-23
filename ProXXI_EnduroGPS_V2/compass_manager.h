/*
 * E_NAVI EnduroGPS - Compass Manager
 * Professional LSM303 Magnetometer & Accelerometer Management
 * 
 * FIXED: Removed CompassCalibrationUI class redefinition conflict
 * Features: Loop-loop calibration, motion detection, heading filtering,
 *          magnetic declination correction, advanced compass functionality
 */

#ifndef COMPASS_MANAGER_H
#define COMPASS_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Preferences.h>
#include "config.h"
#include "shared_data.h"

// ========================================
// COMPASS OPERATING MODES
// ========================================

enum CompassMode {
  COMPASS_MODE_NORMAL,          // Normal operation
  COMPASS_MODE_CALIBRATION,     // Calibration mode
  COMPASS_MODE_LOW_POWER,       // Power save mode
  COMPASS_MODE_HIGH_PRECISION,  // High precision mode
  COMPASS_MODE_FAST_SAMPLING    // Fast sampling mode
};

// ========================================
// MOTION DETECTION STATES
// ========================================

enum MotionState {
  MOTION_STATIONARY,            // No movement detected
  MOTION_SLOW,                  // Slow movement (walking pace)
  MOTION_MODERATE,              // Moderate movement (riding)
  MOTION_FAST,                  // Fast movement (highway speed)
  MOTION_VIBRATION              // Vibration/engine idle
};

// ========================================
// CALIBRATION SYSTEM
// ========================================


#define COMPASS_TIMEOUT 5000  // 5 second timeout for valid data

// Calibration data point
struct CalibrationDataPoint {
  float x, y, z;                // Raw magnetometer values
  float lat, lon;               // GPS position when collected
  float speed;                  // Speed when collected
  unsigned long timestamp;      // When collected
  bool isValid;                 // Data point is valid
};

// Complete calibration data
// struct CompassCalibration {
//   // Hard iron offsets
//   float hardIronOffset[3] = {0, 0, 0};   // X, Y, Z offsets
  
//   // Soft iron compensation matrix
//   float softIronMatrix[3][3] = {         // 3x3 transformation matrix
//     {1.0, 0.0, 0.0},
//     {0.0, 1.0, 0.0},
//     {0.0, 0.0, 1.0}
//   };
  
//   Calibration quality metrics
//   float calibrationQuality = 0.0;        // 0-100% quality score
//   float dataSpread = 0.0;                // How well data covers full circle
//   float consistency = 0.0;               // How consistent the data is
  
//   Calibration metadata
//   bool isCalibrated = false;             // Calibration is valid
//   unsigned long calibrationTime = 0;     // When calibrated (timestamp)
//   int totalPoints = 0;                   // Total calibration points used
//   String calibrationLocation = "";       // Where calibrated (optional)
  
//   Validation
//   uint32_t checksum = 0;                 // Data integrity checksum
// };

// Loop calibration data structure
struct LoopCalibrationData {
  static const int MAX_POINTS = 500;     // Maximum calibration points
  CalibrationDataPoint points[MAX_POINTS];
  int pointCount = 0;
  
  // Loop-specific data
  int leftLoopPoints = 0;                // Points collected in left loop
  int rightLoopPoints = 0;               // Points collected in right loop
  float dataRange[3] = {0, 0, 0};       // X, Y, Z axis data ranges
  float centerPoint[3] = {0, 0, 0};     // Calculated center point
  
  // Quality metrics
  float coverage = 0.0;                  // Angle coverage (0-100%)
  float consistency = 0.0;               // Data consistency (0-100%)
  float overallQuality = 0.0;            // Overall quality score (0-100%)
};

// ========================================
// COMPASS MANAGER CLASS
// ========================================

class CompassManager {
  private:
    // Hardware Interface
    Adafruit_LSM303_Accel_Unified accel;  // Accelerometer
    Adafruit_LSM303_Mag_Unified mag;      // Magnetometer
    bool initialized = false;
    bool taskRunning = false;
    
    // I2C Configuration
    int sdaPin = I2C_SDA;
    int sclPin = I2C_SCL;
    uint32_t i2cFreq = 400000;            // 400kHz I2C frequency
    
    // Interrupt Configuration  
    int drdyPin = -1;                     // Data ready interrupt (optional)
    int int1Pin = -1;                     // Motion interrupt (optional)
    volatile bool dataReady = false;      // DRDY interrupt flag
    volatile bool motionDetected = false; // Motion interrupt flag
    
    // FreeRTOS Task
    TaskHandle_t compassTaskHandle = NULL;
    static void compassTaskWrapper(void* parameter);
    void compassTask();
    
    // Current State
    CompassMode currentMode = COMPASS_MODE_NORMAL;
    MotionState motionState = MOTION_STATIONARY;
    float currentHeading = 0.0;
    float filteredHeading = 0.0;
    bool headingHoldActive = false;
    
    // Motion Detection
    float motionThreshold = 0.5f;          // Motion detection threshold (g)
    unsigned long lastMotionTime = 0;
    float motionLevel = 0.0;
    float motionHistory[10] = {0};         // Motion level history
    int motionIndex = 0;
    
    // Heading Filtering
    float headingSmoothing = 0.8f;         // Heading smoothing factor
    float speedThreshold = 2.0f;           // Speed threshold for heading hold
    bool enableHeadingFilter = true;
    
    // NEW: Enhanced Configuration Structures
    struct HeadingHoldConfig {
        bool enabled = false;
        float speedThreshold = 2.0;
        float smoothingFactor = 0.1;
        float heldHeading = 0.0;
        unsigned long lastUpdate = 0;
    } headingHoldConfig;
    
    struct MotionConfig {
        float threshold = 0.3;
        unsigned long timeout = 5000;
        bool enableSleep = true;
        bool enableWake = true;
    } motionConfig;
    
    // Calibration System
    CompassCalibration calibration;
    CalibrationState calibrationPhase = CAL_IDLE;
    LoopCalibrationData calibrationData;
    unsigned long calibrationStartTime = 0;
    int calibrationPointCount = 0;
    CalibrationDataPoint calibrationPoints[500];
    
    // Temperature & Environmental
    float temperature = 0.0;
    float currentTemperature = 25.0f;           // Current sensor temperature
    bool temperatureValid = false;
    
    // Performance & Statistics
    unsigned long totalReadings = 0;
    unsigned long validReadings = 0;
    unsigned long errorReadings = 0;
    unsigned long lastStatsUpdate = 0;
    unsigned long lastDataUpdate = 0;
    
    // Magnetic Declination
    float magneticDeclination = 0.0;       // Local magnetic declination
    bool autoDeclinationUpdate = true;
    double lastDeclinationLat = 0.0;
    double lastDeclinationLon = 0.0;
    
    // NVS Storage
    Preferences nvs;
    
    // Internal Methods
    bool initializeHardware();
    bool configureAccelerometer();
    bool configureMagnetometer();
    void updateHeading();
    void updateMotionDetection();
    void applyCalibration(float& x, float& y, float& z);
    float calculateHeading(float x, float y, float z);
    void updateMagneticDeclination(double lat, double lon);
    bool loadCalibrationFromNVS();
    bool saveCalibrationToNVS();
    void resetCalibrationData();
    void collectLoopCalibrationData();
    void calculateLoopCalibration();
    float calculateCalibrationQuality();
    void showCalibrationInstructions();
    void showCalibrationResults(float quality);
    void resetMotionTimer();
    bool hasValidData();
    unsigned long getLastUpdateTime();
    CalibrationPhase getCurrentCalibrationPhase();
    
    // NEW: Missing Function Declarations
    void updateMotionStatus();
    float calculateRawHeading();
    float calculateCalibratedHeading();
    float applyHeadingHold(float rawHeading, float speed);
    float applyLowPassFilter(float newValue, float oldValue, float factor);
    float normalizeHeading(float heading);
    void showLoopProgress(const char* phase, const char* instruction, 
                          unsigned long elapsed, unsigned long duration);
    float calculateAccelMagnitude(sensors_event_t& accelEvent);
    bool detectMotionChange(float currentMag, float lastMag);

    
  public:
    CompassManager();
    ~CompassManager();
    
    // ========================================
    // INITIALIZATION & SETUP
    // ========================================
    bool initialize();
    bool shutdown();
    bool reset();
    bool isInitialized() const { return initialized; }
    
    // Hardware Testing
    bool testI2CConnection();
    bool testAccelerometer();
    bool testMagnetometer();
    bool runDiagnostics();
    
    // ========================================
    // CORE COMPASS FUNCTIONALITY
    // ========================================
    void update();                          // Main update function (Core 1)
    void processDataReady();                // Process DRDY interrupt
    void processMotionDetection();          // Process motion interrupt
    
    // Heading Information
    float getCurrentHeading();              // Get current heading (0-359°)
    float getFilteredHeading();             // Get filtered heading
    float getTrueHeading();                 // Get true heading (with declination)
    float getMagneticHeading();             // Get magnetic heading (raw)
    bool isHeadingValid();                  // Heading data is valid
    unsigned long getHeadingAge();          // Age of heading data (ms)
    
    // Motion Detection
    bool isMotionDetected();                // Motion currently detected
    MotionState getMotionState();           // Current motion state
    float getMotionLevel();                 // Motion magnitude (g)
    unsigned long getTimeSinceLastMotion(); // Time since motion (ms)
    bool isStationary();                    // Device is stationary
    
    // ========================================
    // CALIBRATION SYSTEM
    // ========================================
    
    // Calibration Control
    bool startLoopLoopCalibration();        // Start loop-loop calibration
    void stopCalibration();                 // Stop active calibration
    void cancelCalibration();               // Cancel calibration
    bool isCalibrationActive();             // Calibration in progress
    bool isCalibrated();                    // Device is calibrated
    
    // Calibration Progress
    CalibrationPhase getCalibrationPhase(); // Current calibration phase
    float getCalibrationProgress();         // Progress percentage (0-100%)
    void updateCalibrationProgress();       // Update calibration (call in loop)
    int getCalibrationPointCount();         // Number of points collected
    
    // Calibration Data Management
    CompassCalibration getCalibration();    // Get current calibration
    void setCalibration(const CompassCalibration& cal); // Set calibration
    void resetCalibration();                // Reset calibration
    bool validateCalibration();             // Validate current calibration
    float getCalibrationQuality();          // Get calibration quality (0-100%)
    
    // ========================================
    // CONFIGURATION & SETTINGS
    // ========================================
    
    // Operating Mode
    void setCompassMode(CompassMode mode);
    CompassMode getCompassMode() const { return currentMode; }
    
    // Motion Detection Settings
    void setMotionThreshold(float threshold);
    float getMotionThreshold() const { return motionThreshold; }
    
    // Heading Filter Settings
    void enableHeadingHold(bool enable);
    bool isHeadingHoldEnabled() const { return headingHoldActive; }
    void setHeadingHoldSpeed(float speedThreshold);
    void setHeadingSmoothing(float smoothing);
    
    // Magnetic Declination
    void setMagneticDeclination(float declination);
    float getMagneticDeclination() const { return magneticDeclination; }
    void enableAutoDeclinationUpdate(bool enable);
    void updateDeclinationFromPosition(double lat, double lon);
    
    // ========================================
    // ENVIRONMENTAL DATA
    // ========================================
    
    // Temperature Information
    float getTemperature();                 // Get temperature (°C)
    bool isTemperatureValid();              // Temperature reading valid
    
    // Raw Sensor Data
    bool getRawMagnetometer(float& x, float& y, float& z);
    bool getRawAccelerometer(float& x, float& y, float& z);
    bool getCalibratedMagnetometer(float& x, float& y, float& z);
    
    // ========================================
    // STATISTICS & DIAGNOSTICS
    // ========================================
    
    // Performance Statistics
    unsigned long getTotalReadings() const { return totalReadings; }
    unsigned long getValidReadings() const { return validReadings; }
    unsigned long getErrorReadings() const { return errorReadings; }
    float getSuccessRate();                 // Reading success rate (%)
    
    // Diagnostic Information
    String getDiagnosticInfo();             // Comprehensive diagnostic info
    void printDiagnostics();                // Print diagnostics to Serial
    void printStatistics();                 // Print statistics to Serial
    void resetStatistics();                 // Reset all statistics
    
    // System Health
    bool isSystemHealthy();                 // Overall system health
    unsigned long getUptime();              // Compass uptime (ms)
    int getI2CErrors();                     // I2C communication errors
    
    // ========================================
    // POWER MANAGEMENT
    // ========================================
    
    // Power Control
    void enablePowerSave(bool enable);
    void setSampleRate(int hz);             // Set sensor sample rate
    void enableInterrupts(bool drdy, bool motion);
    
    // Low Power Features
    void enterLowPowerMode();               // Enter low power mode
    void exitLowPowerMode();                // Exit low power mode
    bool isInLowPowerMode();                // Currently in low power mode
};

// ========================================
// GLOBAL COMPASS MANAGER INSTANCE
// ========================================
extern CompassManager compassManager;

// ========================================
// COMPASS CALIBRATION UI CLASS - RENAMED TO AVOID CONFLICT
// ========================================

class CompassCalibrationInterface {
  private:
    CalibrationPhase currentPhase = CAL_IDLE;
    unsigned long phaseStartTime = 0;
    int progressPercent = 0;
    String statusMessage = "";
    
  public:
    // Calibration Flow Control
    void showCalibrationIntro();          // Show introduction screen
    void showInstructions();              // Show riding instructions
    void showLoopProgress(bool leftLoop); // Show progress during loops
    void showProcessing();                // Show processing screen
    void showResults(float quality);      // Show calibration results
    void showError(const String& error);  // Show error message
    
    // Progress Updates
    void updateProgress(int percent);
    void updateStatus(const String& message);
    void updateTimeRemaining(int seconds);
    
    // User Interaction
    bool waitForUserConfirmation(unsigned long timeout = 30000);
    bool checkForCancelRequest();
    void handleUserInput();
    
    // Visual Feedback
    void drawCompassRose(float heading);  // Show compass rose
    void drawMotionIndicator(float motion); // Show motion level
    void drawDataQuality(float quality);  // Show data quality
};

extern CompassCalibrationInterface compassCalibrationInterface;

// ========================================
// COMPASS UTILITY FUNCTIONS
// ========================================

// Heading Calculations
float normalizeHeading(float heading);           // Ensure 0-359° range
float headingDifference(float h1, float h2);     // Difference between headings
float averageHeading(float* headings, int count); // Average multiple headings
String formatHeading(float heading, bool includeCardinal = true);

// Cardinal Direction Conversion
String getCardinalFromHeading(float heading);
String getCardinalShort(float heading);          // N, NE, E, etc.
String getCardinalLong(float heading);           // North, Northeast, etc.
float getHeadingFromCardinal(const String& cardinal);

// Motion Analysis
float calculateMotionMagnitude(float x, float y, float z);
MotionState classifyMotion(float magnitude);
bool isStationaryForTime(unsigned long ms);
String getMotionDescription(MotionState state);

// Calibration Utilities
float calculateHardIronOffset(float* values, int count);
float calculateSoftIronScale(float* values, int count, float avgRange);
bool validateCalibrationData(const LoopCalibrationData& data);
float assessCalibrationQuality(const LoopCalibrationData& data);

// ========================================
// ADVANCED COMPASS FEATURES
// ========================================

// Heading Prediction (for smooth display)
class HeadingPredictor {
  private:
    float lastHeading = 0.0;
    float headingRate = 0.0;              // Degrees per second
    unsigned long lastTime = 0;
    
  public:
    void updateHeading(float heading);
    float predictHeading(unsigned long futureTime);
    float getHeadingRate() const { return headingRate; }
    void reset();
};

extern HeadingPredictor headingPredictor;

// Compass Rose Display Configuration
struct CompassRoseConfig {
  int centerX = 160;                      // Center X coordinate
  int centerY = 240;                      // Center Y coordinate
  int radius = 80;                        // Rose radius
  bool showCardinals = true;              // Show N, E, S, W
  bool showIntercardinals = true;         // Show NE, SE, SW, NW
  bool showDegreeMarks = true;            // Show degree markings
  bool showHeadingTriangle = true;        // Show current heading
  uint16_t roseColor = 0xFFFF;           // Rose color
  uint16_t headingColor = 0xF800;        // Heading indicator color
};

void drawCompassRose(const CompassRoseConfig& config, float heading);
void drawHeadingIndicator(int x, int y, int size, float heading, uint16_t color);

// Magnetic Declination Correction
class MagneticDeclination {
  private:
    float declination = 0.0;              // Current magnetic declination
    double lastLat = 0.0, lastLon = 0.0;  // Last position for declination
    bool autoUpdate = true;                // Auto-update based on position
    
  public:
    void setDeclination(float degrees);
    float getDeclination() const { return declination; }
    void enableAutoUpdate(bool enable);
    void updateFromPosition(double lat, double lon);
    float applyDeclination(float magneticHeading);
    float getTrueHeading(float magneticHeading);
};

extern MagneticDeclination magneticDeclination;

// ========================================
// COMPASS ERROR HANDLING
// ========================================

enum CompassError {
  COMPASS_ERROR_NONE = 0,
  COMPASS_ERROR_I2C_FAIL,                // I2C communication failure
  COMPASS_ERROR_SENSOR_TIMEOUT,          // Sensor not responding
  COMPASS_ERROR_INVALID_DATA,            // Invalid sensor readings
  COMPASS_ERROR_CALIBRATION_FAIL,        // Calibration failed
  COMPASS_ERROR_MOTION_INTERRUPT,        // Motion interrupt not working
  COMPASS_ERROR_DRDY_INTERRUPT,          // DRDY interrupt not working
  COMPASS_ERROR_TEMPERATURE_FAIL         // Temperature sensor failure
};

class CompassErrorHandler {
  private:
    CompassError lastError = COMPASS_ERROR_NONE;
    unsigned long errorTime = 0;
    int errorCount = 0;
    bool recoveryAttempted = false;
    
  public:
    void reportError(CompassError error);
    CompassError getLastError() const { return lastError; }
    bool hasError() const { return lastError != COMPASS_ERROR_NONE; }
    String getErrorDescription(CompassError error);
    bool attemptRecovery();
    void clearError();
    int getErrorCount() const { return errorCount; }
};

extern CompassErrorHandler compassErrorHandler;

// ========================================
// COMPASS PERFORMANCE MONITORING
// ========================================

struct CompassPerformance {
  unsigned long updateCount = 0;          // Total updates
  unsigned long errorCount = 0;           // Total errors
  float averageUpdateRate = 0.0;          // Updates per second
  float headingStability = 0.0;           // Heading stability (0-100%)
  float calibrationDrift = 0.0;           // Calibration drift over time
  unsigned long lastUpdateTime = 0;       // Last successful update
  unsigned long uptime = 0;               // Compass uptime
};

CompassPerformance getCompassPerformance();
void resetCompassPerformance();
bool isCompassPerformanceGood();

// ========================================
// GLOBAL ACCESS FUNCTIONS
// ========================================

// Initialization
bool initializeCompass();
void updateCompass();
bool shutdownCompass();

// Quick Data Access
float getCompassHeading();
bool isMotionDetected();
float getMotionLevel();
MotionState getMotionState();
float getCompassTemperature();

// Calibration Functions
void startCompassCalibration(); // Fixed to void for consistency with .cpp
void stopCompassCalibration();
bool isCompassCalibrated();
float getCompassCalibrationQuality();

// Status Functions
bool isCompassHealthy();
CompassError getCompassError();
void clearCompassErrors();

// Configuration Functions
void setCompassMotionThreshold(float threshold);
void enableCompassHeadingHold(bool enable);
void setCompassMagneticDeclination(float declination);

#endif // COMPASS_MANAGER_H