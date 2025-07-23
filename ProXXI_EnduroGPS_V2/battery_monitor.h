/*
 * E_NAVI EnduroGPS - Battery Monitor
 * Professional 12V Motorcycle Battery System Monitoring
 * 
 * FIXED: Battery status enum conflicts with removed config.h macros
 * Features: 220k/47k voltage divider, accurate measurement, 
 *          power management integration, visual feedback
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>
#include "config.h"
#include "shared_data.h"

// ========================================
// BATTERY MONITORING CONFIGURATION
// ========================================

struct BatteryConfig {
  // Hardware Configuration (220k/47k voltage divider)
  float r1 = 220000.0f;             // 220kΩ high-side resistor
  float r2 = 47000.0f;              // 47kΩ low-side resistor
  float vref = 3.3f;                // 3.3V ADC reference voltage
  int adcBits = 4095;               // 4095 for 12-bit ADC (ESP32)
  
  // Calibration Settings
  float calibrationOffset = 0.0f;   // Calibration offset (V)
  float calibrationScale = 1.0f;    // Calibration scale factor
  bool enableCalibration = false;   // Use calibration values
  
  // Measurement Settings
  int samples = 10;                 // Number of samples to average
  unsigned long updateInterval = 1000; // Update every 1 second
  float smoothingFactor = 0.1f;     // Exponential smoothing (0.0-1.0)
  
  // Voltage Thresholds (12V lead-acid battery)
  float criticalVoltage = 11.0f;    // Critical - engine won't start
  float lowVoltage = 11.8f;         // Low - warning level
  float goodVoltage = 12.4f;        // Good - normal operation  
  float fullVoltage = 13.2f;        // Full - engine running
  float overvoltageThreshold = 15.0f; // Overvoltage - charging fault
  
  // Safety Thresholds
  float shutdownVoltage = 10.5f;    // Emergency shutdown
  float lowPowerVoltage = 11.5f;    // Enter low power mode
  float safeOperationVoltage = 11.8f; // Safe for GPS operation
};

// ========================================
// BATTERY STATISTICS & HISTORY
// ========================================

struct BatteryStats {
  // Current Measurements
  float currentVoltage = 0.0f;      // Current battery voltage
  float rawVoltage = 0.0f;          // Raw uncalibrated voltage
  float smoothedVoltage = 0.0f;     // Smoothed voltage reading
  
  // Historical Data
  float minVoltage = 15.0f;         // Minimum voltage seen (reset high)
  float maxVoltage = 0.0f;          // Maximum voltage seen
  float avgVoltage = 0.0f;          // Running average voltage
  
  // Status Information
  BatteryState currentStatus = BATTERY_GOOD;
  BatteryState lastStatus = BATTERY_GOOD;
  unsigned long statusChangeTime = 0; // When status last changed
  unsigned long timeAtStatus = 0;   // Time spent in current status (ms)
  
  // Performance Tracking
  unsigned long totalReadings = 0;  // Total voltage readings taken
  unsigned long validReadings = 0;  // Valid readings (within range)
  unsigned long errorReadings = 0;  // Error readings (out of range)
  float readingSuccessRate = 100.0f; // Success rate percentage
  
  // Power State Tracking
  bool isCharging = false;          // Engine running/charging detected
  bool wasCharging = false;         // Previous charging state
  unsigned long chargingStartTime = 0; // When charging started
  unsigned long totalChargingTime = 0; // Total charging time
  
  // Warning Tracking
  unsigned long lowVoltageTime = 0; // Total time below lowVoltage threshold
  unsigned long criticalVoltageTime = 0; // Total time in critical state
  int lowVoltageEvents = 0;         // Number of low voltage events
  int criticalVoltageEvents = 0;    // Number of critical voltage events
  
  // System Integration
  bool hasHistory = false;          // Valid historical data available
  unsigned long lastUpdate = 0;     // Last measurement timestamp
  unsigned long uptime = 0;         // Battery monitor uptime (ms)
  unsigned long initTime = 0;       // Initialization timestamp
};

// ========================================
// BATTERY ALERT CONFIGURATION
// ========================================

struct BatteryAlerts {
  // Alert Thresholds
  bool enableLowVoltageAlert = true;
  bool enableCriticalVoltageAlert = true;
  bool enableOvervoltageAlert = true;
  bool enableChargingStateAlert = false;
  
  // Alert Timing
  unsigned long lowVoltageAlertDelay = 30000;    // 30 seconds
  unsigned long criticalVoltageAlertDelay = 10000; // 10 seconds
  unsigned long overvoltageAlertDelay = 5000;    // 5 seconds
  
  // Alert State Tracking
  bool lowVoltageAlertActive = false;
  bool criticalVoltageAlertActive = false;
  bool overvoltageAlertActive = false;
  unsigned long lastLowVoltageAlert = 0;
  unsigned long lastCriticalVoltageAlert = 0;
  unsigned long lastOvervoltageAlert = 0;
};

// ========================================
// MAIN BATTERY MONITOR CLASS
// ========================================

class BatteryMonitor {
  private:
    // Configuration & Statistics
    BatteryConfig config;
    BatteryStats stats;
    BatteryAlerts alerts;

    BatteryState currentStatus = BATTERY_GOOD;
    BatteryState lastStatus = BATTERY_GOOD;
    
    // State Management
    bool initialized = false;
    bool calibrated = false;
    unsigned long lastReading = 0;
    unsigned long lastStatsUpdate = 0;
    
    // Voltage Smoothing Buffer
    static const int BUFFER_SIZE = 10;
    float voltageBuffer[BUFFER_SIZE];
    int bufferIndex = 0;
    bool bufferFilled = false;
    
    // ADC Pin Assignment
    int adcPin = BATTERY_ADC_PIN;     // From config.h
    
    // Internal Processing Methods
    float readRawADC();
    float convertADCToVoltage(int adcValue);
    void addToBuffer(float voltage);
    float getBufferAverage();
    void updateRunningStatistics(float voltage);
    BatteryState determineStatus(float voltage);
    void checkStatusChange(BatteryState newStatus);
    void updateAlerts();
    void processVoltageReading(float voltage);
    bool isValidVoltage(float voltage);
    float applyCalibratedVoltage(float rawVoltage); // Added missing
    void updateStatistics(float voltage); // Added missing
    void applySmoothing(float voltage); // Added missing
    const char* getStatusText(BatteryState status); // Added missing
    
  public:
    BatteryMonitor();
    ~BatteryMonitor();
    
    // ========================================
    // INITIALIZATION & CONFIGURATION
    // ========================================
    bool initialize();
    bool shutdown();
    bool isInitialized() const { return initialized; }
    bool reset();
    
    // Configuration Management
    void setConfig(const BatteryConfig& newConfig);
    BatteryConfig getConfig() const { return config; }
    void setUpdateInterval(unsigned long ms);
    void setSampleCount(int samples);
    void setSmoothingFactor(float factor);
    
    // Calibration
    bool calibrateWithKnownVoltage(float knownVoltage);
    bool enableCalibration(bool enable);
    bool isCalibrated() const { return calibrated; }
    void resetCalibration();
    void setCalibrationValues(float offset, float scale);
    
    // ========================================
    // CORE MONITORING FUNCTIONS
    // ========================================
    void update();                          // Main update function (call in loop)
    float readCurrentVoltage();             // Read voltage now (blocking)
    float readSmoothedVoltage();            // Get smoothed voltage reading
    bool hasNewReading();                   // Check if new reading available
    void clearNewReadingFlag();             // Clear new reading flag
    
    // ========================================
    // BATTERY STATUS QUERIES
    // ========================================
    
    // Current Status
    BatteryStatus getBatteryStatus();
    BatteryState getBatteryState();
    float getCurrentVoltage();
    float getSmoothedVoltage();
    float getRawVoltage();
    int getBatteryPercentage();             // 0-100% estimate
    
    // Status Checks
    bool isLowVoltage();                    // Below low threshold
    bool isCriticalVoltage();               // Below critical threshold
    bool isOvervoltage();                   // Above overvoltage threshold
    bool isCharging();                      // Engine running/charging
    bool isDischarging();                   // Battery discharging
    bool hasStatusChanged();                // Status changed since last check
    
    // Safety & Operational Checks
    bool isSafeForOperation();              // Safe for GPS operation
    bool isSafeForGPS();                    // Minimum for GPS operation
    bool shouldEnterLowPower();             // Should enter power save mode
    bool shouldShutdown();                  // Critical - should shutdown
    bool isVoltageStable();                 // Voltage is stable (not fluctuating)
    
    // ========================================
    // STATISTICS & HISTORY
    // ========================================
    
    // Statistical Data
    BatteryStats getStatistics();
    float getMinVoltage();                  // Minimum voltage seen
    float getMaxVoltage();                  // Maximum voltage seen  
    float getAverageVoltage();              // Average voltage over time
    float getVoltageRange();                // Max - Min voltage
    
    // Performance Metrics
    unsigned long getUptime();              // Monitor uptime (ms)
    unsigned long getTimeAtCurrentStatus(); // Time in current status (ms)
    unsigned long getTotalChargingTime();   // Total time charging (ms)
    float getReadingSuccessRate();          // ADC reading success rate (%)
    int getTotalReadings();                 // Total readings taken
    
    // Event Tracking
    int getLowVoltageEventCount();          // Number of low voltage events
    int getCriticalVoltageEventCount();     // Number of critical events
    unsigned long getLowVoltageTime();      // Total time in low voltage (ms)
    unsigned long getCriticalVoltageTime(); // Total time in critical (ms)
    
    // ========================================
    // VISUAL & UI SUPPORT
    // ========================================
    
    // Display Support
    uint16_t getBatteryColor();             // Color for status display
    const char* getBatteryStatusText();     // Human-readable status
    const char* getBatteryStatusShort();    // Short status (3-4 chars)
    String getBatteryDescription();         // Detailed description
    
    // Icon & Symbol Support
    char getBatteryIcon();                  // Unicode battery icon
    int getBatteryBarCount();               // Number of bars (1-5)
    bool shouldShowBatteryWarning();        // Show warning indicator
    bool shouldBlinkBatteryIcon();          // Blink icon for critical
    
    // ========================================
    // ALERT & NOTIFICATION SYSTEM
    // ========================================
    
    // Alert Configuration
    void setAlerts(const BatteryAlerts& newAlerts);
    BatteryAlerts getAlerts() const { return alerts; }
    void enableLowVoltageAlert(bool enable);
    void enableCriticalVoltageAlert(bool enable);
    void enableOvervoltageAlert(bool enable);
    
    // Alert Status
    bool hasActiveAlert();                  // Any alert currently active
    bool isLowVoltageAlertActive();         // Low voltage alert active
    bool isCriticalVoltageAlertActive();    // Critical voltage alert active
    bool isOvervoltageAlertActive();        // Overvoltage alert active
    void clearAlerts();                     // Clear all active alerts
    void acknowledgeAlert(BatteryState alertType); // Acknowledge specific alert
    
    // ========================================
    // DIAGNOSTICS & TESTING
    // ========================================
    
    // Hardware Testing
    bool testADCConnection();               // Test ADC hardware
    bool testVoltageDivider();              // Test voltage divider circuit
    bool runDiagnostics();                  // Complete diagnostic test
    bool validateVoltageReading();          // Validate current reading
    
    // Diagnostic Information
    String getDiagnosticInfo();             // Comprehensive diagnostic string
    void printDiagnostics();                // Print diagnostics to Serial
    void printStatistics();                 // Print statistics to Serial
    void printConfiguration();              // Print current configuration
    
    // Debug Functions
    void dumpVoltageBuffer();               // Dump voltage buffer contents
    void simulateVoltage(float voltage);    // Simulate voltage for testing
    void resetStatistics();                 // Reset all statistics

    void updateRunningAverage(float newVoltage);
    BatteryState getBatteryState();  
    const char* getBatteryStateText();
};

// ========================================
// VISUAL FEEDBACK SYSTEM
// ========================================

// Visual state for battery status
enum BatteryVisualState {
  BATTERY_VISUAL_NORMAL,      // Normal operation - green/yellow
  BATTERY_VISUAL_WARNING,     // Warning state - orange/red
  BATTERY_VISUAL_CRITICAL,    // Critical state - flashing red
  BATTERY_VISUAL_CHARGING,    // Charging state - animated green
  BATTERY_VISUAL_ERROR        // Error state - red X
};

class BatteryVisualFeedback {
  private:
    BatteryVisualState currentState = BATTERY_VISUAL_NORMAL;
    unsigned long lastUpdate = 0;
    int animationPhase = 0;
    bool flashState = false;
    
  public:
    // Visual State Management
    void updateVisualState(BatteryState status);
    BatteryVisualState getCurrentVisualState();
    
    // Animation Support
    void updateAnimations();
    bool shouldFlash();
    int getAnimationPhase();
    uint16_t getCurrentColor();
    
    // Integration
    void drawBatteryIcon(int x, int y, int width, int height);
    void drawBatteryPercentage(int x, int y, int percentage);
    void drawBatteryVoltage(int x, int y, float voltage);
};

// ========================================
// GLOBAL INSTANCES & ACCESS FUNCTIONS
// ========================================

// Global battery monitor instance
extern BatteryMonitor batteryMonitor;
extern BatteryVisualFeedback batteryVisual;

// Quick initialization functions
bool initializeBatteryMonitoring();
void updateBatteryMonitoring();
bool shutdownBatteryMonitoring();

// Quick access functions
float getBatteryVoltage();
BatteryState getBatteryState();
int getBatteryPercentage();
bool isBatteryOK();
bool isBatteryLow();
bool isBatteryCritical();
bool isBatteryCharging();

// Status text functions
const char* getBatteryStatusText();
String getBatteryDescription();
uint16_t getBatteryStatusColor();

// Safety check functions  
bool isSafeToOperateGPS();
bool shouldEnterPowerSaveMode();
bool shouldShutdownSystem();

// Alert functions
bool hasBatteryAlert();
void clearBatteryAlerts();
void acknowledgeBatteryAlert();

// Statistics functions
BatteryStats getBatteryStatistics();
void printBatteryDiagnostics();
void resetBatteryStatistics();

// Visual feedback functions
bool initializeBatteryVisual();
void updateBatteryVisual();
void drawBatteryStatus(int x, int y);

// ========================================
// BATTERY VOLTAGE CONSTANTS
// ========================================

// Standard 12V lead-acid battery voltage levels
#define BATTERY_VOLTAGE_CRITICAL    11.0f   // Engine won't start
#define BATTERY_VOLTAGE_LOW         11.8f   // Warning level
#define BATTERY_VOLTAGE_GOOD        12.4f   // Normal operation
#define BATTERY_VOLTAGE_FULL        13.2f   // Engine running
#define BATTERY_VOLTAGE_OVERCHARGE  15.0f   // Charging system fault

// Safety voltage levels
#define BATTERY_VOLTAGE_SHUTDOWN    10.5f   // Emergency shutdown
#define BATTERY_VOLTAGE_LOW_POWER   11.5f   // Enter low power mode
#define BATTERY_VOLTAGE_GPS_MIN     11.0f   // Minimum for GPS operation

// Percentage estimation lookup (12V lead-acid)
#define BATTERY_100_PERCENT         12.8f
#define BATTERY_90_PERCENT          12.6f
#define BATTERY_80_PERCENT          12.4f
#define BATTERY_70_PERCENT          12.2f
#define BATTERY_60_PERCENT          12.0f
#define BATTERY_50_PERCENT          11.8f
#define BATTERY_40_PERCENT          11.6f
#define BATTERY_30_PERCENT          11.4f
#define BATTERY_20_PERCENT          11.2f
#define BATTERY_10_PERCENT          11.0f

#endif // BATTERY_MONITOR_H