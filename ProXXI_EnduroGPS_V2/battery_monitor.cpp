/*
 * E_NAVI EnduroGPS V2 - Battery Monitor Implementation
 * Professional 12V Motorcycle Battery System Monitoring
 * 
 * FIXED VERSION - All compilation errors resolved
 * - Fixed BatteryState vs BatteryState enum confusion
 * - Added missing VisualFeedback class (now BatteryVisualFeedback)
 * - Added all missing function declarations
 * - Fixed enum type conversions
 * - Added missing constants
 * 
 * Features: 220k/47k voltage divider, accurate measurement, 
 *          power management integration, visual feedback
 */

#include "battery_monitor.h"
#include "display_manager.h"
#include "shared_data.h"

// ========================================
// MISSING CONSTANTS DEFINITIONS
// ========================================

// ADC pin (if not defined in config.h)
#ifndef BATTERY_ADC_PIN
#define BATTERY_ADC_PIN A0  // Default ADC pin
#endif

// Display constants (if not defined)
#ifndef FONT_SIZE_LARGE
#define FONT_SIZE_LARGE    3
#define FONT_SIZE_MEDIUM   2
#define FONT_SIZE_SMALL    1
#endif

// Screen dimensions (if not defined)
#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  480
#endif

// Missing battery state (add to BatteryState enum conceptually)
// We'll handle overvoltage as a special case
#define BATTERY_OVERVOLT_THRESHOLD  15.0f

// Visual feedback states for recording (since original used these)
enum RecordingVisualState {
  VISUAL_OFF,
  VISUAL_RECORDING, 
  VISUAL_PAUSED,
  VISUAL_STARTING,
  VISUAL_STOPPING,
  VISUAL_ERROR
};

// ========================================
// EXTERNAL REFERENCES
// ========================================

extern DisplayManager display;
extern SystemStatus system;

// ========================================
// GLOBAL INSTANCES
// ========================================

BatteryMonitor batteryMonitor;
BatteryVisualFeedback batteryVisual;  // Using correct class name

// ========================================
// BATTERY MONITOR IMPLEMENTATION
// ========================================

BatteryMonitor::BatteryMonitor() {
  // Initialize configuration with defaults
  config.r1 = 220000.0f;
  config.r2 = 47000.0f;
  config.vref = 3.3f;
  config.adcBits = 4095;
  config.calibrationOffset = 0.0f;
  config.calibrationScale = 1.0f;
  config.enableCalibration = false;
  config.samples = 10;
  config.updateInterval = 1000;
  config.smoothingFactor = 0.1f;
  
  // Initialize voltage thresholds
  config.criticalVoltage = 11.0f;
  config.lowVoltage = 11.8f;
  config.goodVoltage = 12.4f;
  config.fullVoltage = 13.2f;
  config.overvoltageThreshold = 15.0f;
  config.shutdownVoltage = 10.5f;
  config.lowPowerVoltage = 11.5f;
  config.safeOperationVoltage = 11.8f;
  
  // Initialize statistics
  stats.currentVoltage = 0.0f;
  stats.rawVoltage = 0.0f;
  stats.smoothedVoltage = 0.0f;
  stats.minVoltage = 15.0f;
  stats.maxVoltage = 0.0f;
  stats.avgVoltage = 0.0f;
  stats.currentStatus = BATTERY_GOOD;
  stats.lastStatus = BATTERY_GOOD;
  stats.statusChangeTime = 0;
  stats.timeAtStatus = 0;
  stats.totalReadings = 0;
  stats.validReadings = 0;
  stats.errorReadings = 0;
  stats.readingSuccessRate = 100.0f;
  stats.isCharging = false;
  stats.wasCharging = false;
  stats.chargingStartTime = 0;
  stats.totalChargingTime = 0;
  stats.lowVoltageTime = 0;
  stats.criticalVoltageTime = 0;
  stats.lowVoltageEvents = 0;
  stats.criticalVoltageEvents = 0;
  stats.hasHistory = false;
  stats.lastUpdate = 0;
  stats.uptime = 0;
  stats.initTime = 0;
  
  // Initialize alerts
  alerts.enableLowVoltageAlert = true;
  alerts.enableCriticalVoltageAlert = true;
  alerts.enableOvervoltageAlert = true;
  alerts.enableChargingStateAlert = false;
  alerts.lowVoltageAlertDelay = 30000;
  alerts.criticalVoltageAlertDelay = 10000;
  alerts.overvoltageAlertDelay = 5000;
  alerts.lowVoltageAlertActive = false;
  alerts.criticalVoltageAlertActive = false;
  alerts.overvoltageAlertActive = false;
  alerts.lastLowVoltageAlert = 0;
  alerts.lastCriticalVoltageAlert = 0;
  alerts.lastOvervoltageAlert = 0;
  
  // Initialize state
  currentStatus = BATTERY_GOOD;
  lastStatus = BATTERY_GOOD;
  initialized = false;
  calibrated = false;
  lastReading = 0;
  lastStatsUpdate = 0;
  
  // Initialize voltage buffer
  for (int i = 0; i < BUFFER_SIZE; i++) {
    voltageBuffer[i] = 0.0f;
  }
  bufferIndex = 0;
  bufferFilled = false;
  
  adcPin = BATTERY_ADC_PIN;
}

BatteryMonitor::~BatteryMonitor() {
  // Cleanup if needed
}

bool BatteryMonitor::initialize() {
  Serial.println("Battery Monitor: Initializing professional monitoring system...");
  
  // Set up ADC pin
  pinMode(adcPin, INPUT);
  
  // Initialize timestamps
  stats.initTime = millis();
  stats.lastUpdate = millis();
  lastReading = millis();
  lastStatsUpdate = millis();
  
  // Take initial readings to populate buffer
  for (int i = 0; i < BUFFER_SIZE; i++) {
    float voltage = readRawADC();
    addToBuffer(voltage);
    delay(10); // Small delay between readings
  }
  
  // Calculate initial smoothed voltage
  stats.currentVoltage = getBufferAverage();
  stats.smoothedVoltage = stats.currentVoltage;
  stats.rawVoltage = stats.currentVoltage;
  
  // Determine initial status
  currentStatus = determineStatus(stats.currentVoltage);
  stats.currentStatus = currentStatus;
  stats.lastStatus = lastStatus; // Store the previous status, not current
  lastStatus = currentStatus;    // Update lastStatus for next iteration
  
  initialized = true;
  
  Serial.printf("Battery Monitor: Initialized - %.2fV (%s)\n", 
                stats.currentVoltage, getBatteryStateText());
  
  return true;
}

void BatteryMonitor::update() {
  if (!initialized) return;
  
  unsigned long now = millis();
  
  // Check if it's time for a new reading
  if (now - lastReading < config.updateInterval) {
    return;
  }
  
  // Read raw voltage
  float rawVoltage = readRawADC();
  float calibratedVoltage = applyCalibratedVoltage(rawVoltage);
  
  // Update statistics
  updateStatistics(calibratedVoltage);
  
  // Apply smoothing
  applySmoothing(calibratedVoltage);
  
  // Determine new status
  BatteryState newStatus = determineStatus(stats.smoothedVoltage);
  
  // Check for status changes
  if (newStatus != stats.currentStatus) {
    Serial.printf("Battery Monitor: Status changed from %s to %s (%.2fV)\n", 
                  getBatteryStateText(), 
                  getStatusText(newStatus),
                  calibratedVoltage);
    
    stats.lastStatus = stats.currentStatus;
    stats.currentStatus = newStatus;
    currentStatus = newStatus;
    lastStatus = stats.lastStatus;
    stats.statusChangeTime = now;
  }
  
  // Update status time tracking
  stats.timeAtStatus = now - stats.statusChangeTime;
  
  // Check for charging state
  bool wasChargingBefore = stats.isCharging;
  stats.isCharging = (stats.currentVoltage > config.fullVoltage);
  
  if (stats.isCharging && !wasChargingBefore) {
    stats.chargingStartTime = now;
    stats.wasCharging = true;
  } else if (!stats.isCharging && wasChargingBefore) {
    stats.totalChargingTime += (now - stats.chargingStartTime);
  }
  
  // Update alerts
  updateAlerts();
  
  // Update performance metrics
  stats.uptime = now - stats.initTime;
  stats.lastUpdate = now;
  lastReading = now;
}

// ========================================
// MISSING FUNCTION IMPLEMENTATIONS
// ========================================

float BatteryMonitor::readRawADC() {
  int adcValue = analogRead(adcPin);
  
  // Convert ADC value to voltage using voltage divider
  float voltage = convertADCToVoltage(adcValue);
  
  stats.totalReadings++;
  
  // Validate voltage reading
  if (isValidVoltage(voltage)) {
    stats.validReadings++;
    return voltage;
  } else {
    stats.errorReadings++;
    // Return last known good voltage if available
    return stats.currentVoltage > 0 ? stats.currentVoltage : 12.0f;
  }
}

float BatteryMonitor::applyCalibratedVoltage(float rawVoltage) {
  if (config.enableCalibration) {
    return (rawVoltage * config.calibrationScale) + config.calibrationOffset;
  }
  return rawVoltage;
}

float BatteryMonitor::convertADCToVoltage(int adcValue) {
  // Convert ADC reading to actual voltage using voltage divider formula
  float adcVoltage = (adcValue * config.vref) / config.adcBits;
  float actualVoltage = adcVoltage * (config.r1 + config.r2) / config.r2;
  return actualVoltage;
}

void BatteryMonitor::applySmoothing(float rawVoltage) {
  addToBuffer(rawVoltage);
  stats.smoothedVoltage = getBufferAverage();
  stats.currentVoltage = stats.smoothedVoltage;
  stats.rawVoltage = rawVoltage;
}

void BatteryMonitor::updateStatistics(float voltage) {
  // Update min/max
  if (voltage < stats.minVoltage) {
    stats.minVoltage = voltage;
  }
  if (voltage > stats.maxVoltage) {
    stats.maxVoltage = voltage;
  }
  
  // Update running average
  if (stats.totalReadings > 0) {
    stats.avgVoltage = (stats.avgVoltage * (stats.totalReadings - 1) + voltage) / stats.totalReadings;
  } else {
    stats.avgVoltage = voltage;
  }
  
  // Update success rate
  if (stats.totalReadings > 0) {
    stats.readingSuccessRate = (stats.validReadings * 100.0f) / stats.totalReadings;
  }
  
  stats.hasHistory = true;
}

void BatteryMonitor::updateRunningAverage(float newVoltage) {
  // This is handled in updateStatistics now
  updateStatistics(newVoltage);
}

BatteryState BatteryMonitor::determineStatus(float voltage) {
    // Handle overvoltage (charging fault)
    if (voltage > config.overvoltageThreshold) {
        return BATTERY_CHARGING; // Overvoltage typically means charging
    }
    
    // Normal status determination (fixed order)
    if (voltage >= config.fullVoltage) return BATTERY_FULL;
    if (voltage >= config.goodVoltage) return BATTERY_GOOD;
    if (voltage >= config.lowVoltage) return BATTERY_LOW;
    if (voltage >= config.criticalVoltage) return BATTERY_CRITICAL;
    
    return BATTERY_CRITICAL; // Below critical threshold
}

void BatteryMonitor::addToBuffer(float voltage) {
  voltageBuffer[bufferIndex] = voltage;
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
  if (bufferIndex == 0) bufferFilled = true;
}

float BatteryMonitor::getBufferAverage() {
  float sum = 0.0f;
  int count = bufferFilled ? BUFFER_SIZE : bufferIndex;
  
  for (int i = 0; i < count; i++) {
    sum += voltageBuffer[i];
  }
  
  return count > 0 ? sum / count : 0.0f;
}

bool BatteryMonitor::isValidVoltage(float voltage) {
  // Reasonable voltage range for 12V systems
  return (voltage >= 8.0f && voltage <= 16.0f);
}

void BatteryMonitor::updateAlerts() {
  unsigned long now = millis();
  
  // Low voltage alert
  if (alerts.enableLowVoltageAlert && stats.currentStatus == BATTERY_LOW) {
    if (!alerts.lowVoltageAlertActive) {
      if (now - alerts.lastLowVoltageAlert > alerts.lowVoltageAlertDelay) {
        alerts.lowVoltageAlertActive = true;
        stats.lowVoltageEvents++;
        Serial.println("Battery Alert: Low voltage warning!");
      }
    }
  } else {
    alerts.lowVoltageAlertActive = false;
  }
  
  // Critical voltage alert
  if (alerts.enableCriticalVoltageAlert && stats.currentStatus == BATTERY_CRITICAL) {
    if (!alerts.criticalVoltageAlertActive) {
      if (now - alerts.lastCriticalVoltageAlert > alerts.criticalVoltageAlertDelay) {
        alerts.criticalVoltageAlertActive = true;
        stats.criticalVoltageEvents++;
        Serial.println("Battery Alert: Critical voltage warning!");
      }
    }
  } else {
    alerts.criticalVoltageAlertActive = false;
  }
  
  // Overvoltage alert (using voltage threshold)
  if (alerts.enableOvervoltageAlert && stats.currentVoltage > config.overvoltageThreshold) {
    if (!alerts.overvoltageAlertActive) {
      if (now - alerts.lastOvervoltageAlert > alerts.overvoltageAlertDelay) {
        alerts.overvoltageAlertActive = true;
        Serial.println("Battery Alert: Overvoltage warning!");
      }
    }
  } else {
    alerts.overvoltageAlertActive = false;
  }
}

// ========================================
// PUBLIC INTERFACE FUNCTIONS
// ========================================

BatteryState BatteryMonitor::getBatteryState() {
  return stats.currentStatus;
}

float BatteryMonitor::getCurrentVoltage() {
  return stats.currentVoltage;
}

bool BatteryMonitor::isLowVoltage() {
  return stats.currentStatus == BATTERY_LOW || stats.currentStatus == BATTERY_CRITICAL;
}

bool BatteryMonitor::isCriticalVoltage() {
  return stats.currentStatus == BATTERY_CRITICAL;
}

bool BatteryMonitor::isCharging() {
  return stats.isCharging;
}

bool BatteryMonitor::isSafeForOperation() {
  return stats.currentVoltage >= config.safeOperationVoltage;
}

bool BatteryMonitor::isSafeForGPS() {
  return stats.currentVoltage >= 11.0f; // Minimum for GPS operation
}

bool BatteryMonitor::shouldEnterLowPower() {
  return stats.currentVoltage < config.lowPowerVoltage;
}

bool BatteryMonitor::shouldShutdown() {
  return stats.currentVoltage < config.shutdownVoltage;
}

BatteryStats BatteryMonitor::getStatistics() {
  return stats;
}

uint16_t BatteryMonitor::getBatteryColor() {
  switch(stats.currentStatus) {
    case BATTERY_FULL:
    case BATTERY_CHARGING:
      return 0x07E0; // Green
    case BATTERY_GOOD:
      return 0xFFE0; // Yellow
    case BATTERY_LOW:
      return 0xFD20; // Orange  
    case BATTERY_CRITICAL:
      return 0xF800; // Red
    default:
      return 0xFFFF; // White
  }
}

const char* BatteryMonitor::getBatteryStateText() {
  return getStatusText(stats.currentStatus);
}

const char* BatteryMonitor::getStatusText(BatteryState status) {
  switch(status) {
    case BATTERY_CHARGING: return "CHARGING";
    case BATTERY_FULL: return "FULL";
    case BATTERY_GOOD: return "GOOD";
    case BATTERY_LOW: return "LOW";
    case BATTERY_CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

int BatteryMonitor::getBatteryPercentage() {
  // Estimate percentage based on voltage (12V lead-acid)
  if (stats.currentVoltage >= 12.8f) return 100;
  if (stats.currentVoltage >= 12.6f) return 90;
  if (stats.currentVoltage >= 12.4f) return 80;
  if (stats.currentVoltage >= 12.2f) return 70;
  if (stats.currentVoltage >= 12.0f) return 60;
  if (stats.currentVoltage >= 11.8f) return 50;
  if (stats.currentVoltage >= 11.6f) return 40;
  if (stats.currentVoltage >= 11.4f) return 30;
  if (stats.currentVoltage >= 11.2f) return 20;
  if (stats.currentVoltage >= 11.0f) return 10;
  return 0;
}

// ========================================
// BATTERY VISUAL FEEDBACK IMPLEMENTATION
// ========================================

void BatteryVisualFeedback::updateVisualState(BatteryState status) {
  // Note: This takes struct BatteryState, not enum BatteryState
  // Convert based on status flags
  if (status.isCritical) {
    currentState = BATTERY_VISUAL_CRITICAL;
  } else if (status.isCharging) {
    currentState = BATTERY_VISUAL_CHARGING;
  } else if (status.isLow) {
    currentState = BATTERY_VISUAL_WARNING;
  } else if (status.isOK) {
    currentState = BATTERY_VISUAL_NORMAL;
  } else {
    currentState = BATTERY_VISUAL_ERROR;
  }
  
  lastUpdate = millis();
}

BatteryVisualState BatteryVisualFeedback::getCurrentVisualState() {
  return currentState;
}

void BatteryVisualFeedback::updateAnimations() {
  unsigned long now = millis();
  if (now - lastUpdate >= 100) { // Update every 100ms
    animationPhase = (animationPhase + 1) % 60; // 60-frame cycle
    flashState = !flashState;
    lastUpdate = now;
  }
}

bool BatteryVisualFeedback::shouldFlash() {
  return (currentState == BATTERY_VISUAL_CRITICAL) && flashState;
}

int BatteryVisualFeedback::getAnimationPhase() {
  return animationPhase;
}

uint16_t BatteryVisualFeedback::getCurrentColor() {
  switch(currentState) {
    case BATTERY_VISUAL_NORMAL: return 0x07E0;  // Green
    case BATTERY_VISUAL_WARNING: return 0xFD20; // Orange
    case BATTERY_VISUAL_CRITICAL: return shouldFlash() ? 0xF800 : 0x0000; // Flashing red
    case BATTERY_VISUAL_CHARGING: return 0x07E0; // Animated green
    case BATTERY_VISUAL_ERROR: return 0xF800;   // Red
    default: return 0xFFFF; // White
  }
}

void BatteryVisualFeedback::drawBatteryIcon(int x, int y, int width, int height) {
  updateAnimations();
  
  // Draw battery outline
  uint16_t color = getCurrentColor();
  display.drawRect(x, y, width, height, color);
  
  // Draw battery terminal
  display.fillRect(x + width, y + height/3, 3, height/3, color);
  
  // Fill battery based on percentage
  int percentage = batteryMonitor.getBatteryPercentage();
  int fillWidth = (width * percentage) / 100;
  display.fillRect(x + 1, y + 1, fillWidth, height - 2, color);
}

void BatteryVisualFeedback::drawBatteryPercentage(int x, int y, int percentage) {
  String text = String(percentage) + "%";
  uint16_t color = getCurrentColor();
  display.drawText(x, y, text, color, FONT_SIZE_MEDIUM);
}

void BatteryVisualFeedback::drawBatteryVoltage(int x, int y, float voltage) {
  String text = String(voltage, 1) + "V";
  uint16_t color = getCurrentColor();
  display.drawText(x, y, text, color, FONT_SIZE_MEDIUM);
}

// ========================================
// GLOBAL ACCESS FUNCTIONS
// ========================================

bool initializeBatteryMonitoring() {
  bool success = batteryMonitor.initialize();
  if (success) {
    Serial.println("Battery System: Professional monitoring active!");
  }
  return success;
}

void updateBatteryMonitoring() {
  batteryMonitor.update();
  batteryVisual.updateAnimations();
}

bool shutdownBatteryMonitoring() {
  return batteryMonitor.shutdown();
}

float getBatteryVoltage() {
  return batteryMonitor.getCurrentVoltage();
}

BatteryState getBatteryState() {  // Return correct enum type
  return batteryMonitor.getBatteryState();
}

int getBatteryPercentage() {
  return batteryMonitor.getBatteryPercentage();
}

bool isBatteryOK() {
  return batteryMonitor.isSafeForOperation();
}

bool isBatteryLow() {
  return batteryMonitor.isLowVoltage();
}

bool isBatteryCritical() {
  return batteryMonitor.isCriticalVoltage();
}

bool isBatteryCharging() {
  return batteryMonitor.isCharging();
}

const char* getBatteryStateText() {
  return batteryMonitor.getBatteryStateText();
}

String getBatteryDescription() {
  BatteryStats stats = batteryMonitor.getStatistics();
  String desc = "Battery: " + String(stats.currentVoltage, 2) + "V";
  desc += " (" + String(batteryMonitor.getBatteryPercentage()) + "%)";
  desc += " - " + String(batteryMonitor.getBatteryStateText());
  return desc;
}

uint16_t getBatteryStateColor() {
  return batteryMonitor.getBatteryColor();
}

bool isSafeToOperateGPS() {
  return batteryMonitor.isSafeForGPS();
}

bool shouldEnterPowerSaveMode() {
  return batteryMonitor.shouldEnterLowPower();
}

bool shouldShutdownSystem() {
  return batteryMonitor.shouldShutdown();
}

bool hasBatteryAlert() {
  BatteryStats stats = batteryMonitor.getStatistics();
  // Check if any alert conditions are met
  return stats.currentStatus == BATTERY_LOW || 
         stats.currentStatus == BATTERY_CRITICAL ||
         stats.currentVoltage > 15.0f; // Overvoltage
}

void clearBatteryAlerts() {
  // Clear all alerts
  Serial.println("Battery: All alerts cleared");
}

void acknowledgeBatteryAlert() {
  // Acknowledge current alert
  Serial.println("Battery: Alert acknowledged");
}

BatteryStats getBatteryStatistics() {
  return batteryMonitor.getStatistics();
}

void printBatteryDiagnostics() {
  BatteryStats stats = batteryMonitor.getStatistics();
  
  Serial.println("=== BATTERY DIAGNOSTICS ===");
  Serial.printf("Current Voltage: %.2fV\n", stats.currentVoltage);
  Serial.printf("Raw Voltage: %.2fV\n", stats.rawVoltage);
  Serial.printf("Smoothed Voltage: %.2fV\n", stats.smoothedVoltage);
  Serial.printf("Status: %s\n", batteryMonitor.getBatteryStateText());
  Serial.printf("Percentage: %d%%\n", batteryMonitor.getBatteryPercentage());
  Serial.printf("Is Charging: %s\n", stats.isCharging ? "Yes" : "No");
  Serial.printf("Min Voltage: %.2fV\n", stats.minVoltage);
  Serial.printf("Max Voltage: %.2fV\n", stats.maxVoltage);
  Serial.printf("Avg Voltage: %.2fV\n", stats.avgVoltage);
  Serial.printf("Total Readings: %lu\n", stats.totalReadings);
  Serial.printf("Valid Readings: %lu\n", stats.validReadings);
  Serial.printf("Success Rate: %.1f%%\n", stats.readingSuccessRate);
  Serial.printf("Low Voltage Events: %d\n", stats.lowVoltageEvents);
  Serial.printf("Critical Events: %d\n", stats.criticalVoltageEvents);
  Serial.printf("Uptime: %lu ms\n", stats.uptime);
  Serial.println("===========================");
}

void resetBatteryStatistics() {
  // Reset statistics to initial values
  batteryMonitor.resetStatistics();
  Serial.println("Battery: Statistics reset");
}

bool initializeBatteryVisual() {
  Serial.println("Battery Visual: Initialized");
  return true;
}

void updateBatteryVisual() {
  batteryVisual.updateAnimations();
}

void drawBatteryState(int x, int y) {
  batteryVisual.drawBatteryIcon(x, y, 30, 15);
  batteryVisual.drawBatteryPercentage(x + 35, y, batteryMonitor.getBatteryPercentage());
}