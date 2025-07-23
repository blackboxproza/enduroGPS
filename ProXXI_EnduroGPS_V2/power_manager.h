/*
 * E_NAVI EnduroGPS - Power Manager Header
 * Advanced power management for motorcycle GPS system
 * 
 * Features: Sleep/wake management, motion detection, battery monitoring,
 *          performance scaling, low-power modes
 * 
 * MATCHES THE ACTUAL IMPLEMENTATION IN power_manager.cpp
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "shared_data.h"
#include <esp_sleep.h>
#include <esp_pm.h>
#include <Preferences.h>

// Forward declarations
class BatteryMonitor;
class DisplayManager;

// ========================================
// POWER MODES (from actual implementation)
// ========================================

enum PowerMode {
  POWER_NORMAL,                         // Normal operation - full power
  POWER_LOW,                           // Low power mode - reduced performance
  POWER_HIGH,                          // High performance mode - maximum power
  POWER_CRITICAL                       // Critical mode - minimum power only
};

// ========================================
// PERFORMANCE LEVELS (from actual implementation)
// ========================================

enum PerformanceLevel {
  PERFORMANCE_MINIMAL,                 // Minimal performance - 80MHz
  PERFORMANCE_LOW,                     // Low performance - 160MHz
  PERFORMANCE_MEDIUM,                  // Medium performance - 240MHz
  PERFORMANCE_HIGH                     // High performance - 240MHz
};

// ========================================
// WAKEUP REASONS (from actual implementation)
// ========================================

enum WakeupReason {
  WAKEUP_REASON_RESET,                 // System reset/power on
  WAKEUP_REASON_MOTION,                // Motion detected
  WAKEUP_REASON_BUTTON,                // Button press
  WAKEUP_REASON_TIMER,                 // Timer wakeup
  WAKEUP_REASON_EXTERNAL               // External interrupt
};

// ========================================
// POWER MANAGER CLASS (from actual implementation)
// ========================================

class PowerManager {
  private:
    // ========================================
    // CORE STATE (from actual implementation)
    // ========================================
    
    bool initialized = false;
    PowerMode currentPowerMode = POWER_NORMAL;
    PowerMode previousPowerMode = POWER_NORMAL;
    
    // ========================================
    // SLEEP/WAKE MANAGEMENT (from actual implementation)
    // ========================================
    
    bool sleepEnabled = true;
    bool motionWakeEnabled = true;
    bool timerWakeEnabled = true;
    
    // ========================================
    // TIMING & ACTIVITY (from actual implementation)
    // ========================================
    
    unsigned long lastMotionTime = 0;
    unsigned long lastActivityTime = 0;
    unsigned long sleepTimer = 0;
    WakeupReason wakeupReason = WAKEUP_REASON_RESET;
    
    // ========================================
    // CONFIGURATION (from actual implementation)
    // ========================================
    
    unsigned long sleepTimeout = MOTION_TIMEOUT_MS;  // From config.h
    float motionThreshold = WAKE_MOTION_THRESHOLD;   // From config.h
    
    // ========================================
    // BATTERY & PERFORMANCE (from actual implementation)
    // ========================================
    
    float batteryVoltage = 12.0;
    float lowPowerThreshold = 11.5;
    float criticalPowerThreshold = 11.0;
    
    // Performance scaling
    int cpuFrequency = 240;                          // MHz
    PerformanceLevel performanceLevel = PERFORMANCE_HIGH;
    
    // ========================================
    // INTERNAL METHODS (from actual implementation)
    // ========================================
    
    bool configureWakeupSources();
    bool setupPerformanceManagement();
    bool setupMotionDetection();
    void checkWakeupReason();
    
    void updateBatteryStatus();
    void updateMotionTracking();
    void checkSleepConditions();
    void updatePerformanceScaling();
    void updatePowerMode();
    
    PowerMode determinePowerMode();
    PerformanceLevel determinePerformanceLevel();
    
    void setCPUFrequency(int freqMHz);
    void setPerformanceLevel(PerformanceLevel level);
    
    void applyCriticalPowerSettings();
    void applyLowPowerSettings();
    void applyNormalPowerSettings();
    void applyHighPowerSettings();
    
    bool isSystemBusy();
    
  public:
    // ========================================
    // CONSTRUCTOR & DESTRUCTOR (from actual implementation)
    // ========================================
    
    PowerManager();
    ~PowerManager();
    
    // ========================================
    // INITIALIZATION (from actual implementation)
    // ========================================
    
    bool initialize();
    bool isInitialized() const { return initialized; }
    
    // ========================================
    // MAIN UPDATE (from actual implementation)
    // ========================================
    
    void update();                               // Main update function (call from main loop)
    
    // ========================================
    // POWER MODE CONTROL (from actual implementation)
    // ========================================
    
    void setPowerMode(PowerMode mode);
    PowerMode getCurrentPowerMode() const { return currentPowerMode; }
    bool isLowPowerMode() const { return currentPowerMode == POWER_LOW || currentPowerMode == POWER_CRITICAL; }
    
    // ========================================
    // ACTIVITY MANAGEMENT (from actual implementation)
    // ========================================
    
    void registerActivity();                     // Register user activity
    void registerMotion();                       // Register motion detection
    unsigned long getTimeSinceLastActivity() const;
    unsigned long getTimeSinceLastMotion() const;
    
    // ========================================
    // SLEEP MANAGEMENT (from actual implementation)
    // ========================================
    
    void enterSleepMode();                       // Enter sleep mode immediately
    void wakeFromSleep();                        // Wake from sleep
    bool canSleep() const;                       // Can system sleep now?
    unsigned long getTimeToSleep() const;        // Time until sleep (ms)
    
    // ========================================
    // CONFIGURATION (from actual implementation)
    // ========================================
    
    void setSleepTimeout(unsigned long timeoutMs);
    void setMotionThreshold(float threshold);
    void setLowPowerThreshold(float voltage);
    void setCriticalPowerThreshold(float voltage);
    
    // ========================================
    // STATUS & DIAGNOSTICS (from actual implementation)
    // ========================================
    
    PowerStatistics getStatistics() const;       // Get power statistics (if struct exists)
    String getPowerReport() const;               // Get detailed power report
    float getBatteryVoltage() const { return batteryVoltage; }
    WakeupReason getWakeupReason() const { return wakeupReason; }
    
    // System health
    bool isSystemHealthy() const;
    bool isBatteryLow() const;
    bool isBatteryCritical() const;

    void prepareSleep();
  void configureSleepGPIOs();
  void saveSystemState();
  void restoreSystemState();
  void reinitializeHardware();
};

// ========================================
// GLOBAL INSTANCE (from actual implementation)
// ========================================

extern PowerManager powerManager;

// ========================================
// GLOBAL ACCESS FUNCTIONS (from actual implementation)
// ========================================

bool initializePowerManager();                   // Initialize power management
void updatePowerManager();                       // Update power manager
PowerMode getCurrentPowerMode();                 // Get current power mode
bool isLowPowerMode();                          // Check if in low power mode
void registerPowerActivity();                   // Register general activity
void enterSleepMode();                          // Enter sleep mode
void wakeFromSleep();                           // Wake from sleep
String getPowerManagerReport();                 // Get power manager report

// ========================================
// POWER MANAGEMENT CONSTANTS (from config.h and implementation)
// ========================================

// Default timeouts (defined in config.h)
#ifndef MOTION_TIMEOUT_MS
#define MOTION_TIMEOUT_MS          300000    // 5 minutes
#endif

#ifndef WAKE_MOTION_THRESHOLD
#define WAKE_MOTION_THRESHOLD      0.3       // Motion threshold (g-force)
#endif

// CPU frequencies (MHz) - from actual implementation
#define POWER_CPU_FREQ_MINIMAL     80        // Minimal CPU frequency
#define POWER_CPU_FREQ_LOW         160       // Low CPU frequency
#define POWER_CPU_FREQ_MEDIUM      240       // Medium CPU frequency
#define POWER_CPU_FREQ_HIGH        240       // High CPU frequency

// Battery thresholds (Volts) - from actual implementation
#define POWER_BATTERY_NORMAL       12.4      // Normal battery voltage
#define POWER_BATTERY_LOW          11.8      // Low battery threshold
#define POWER_BATTERY_CRITICAL     11.0      // Critical battery threshold
#define POWER_BATTERY_SHUTDOWN     10.5      // Emergency shutdown

#endif // POWER_MANAGER_H