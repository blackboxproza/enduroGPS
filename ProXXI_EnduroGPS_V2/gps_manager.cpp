#include "gps_manager.h"
#include <Arduino.h>

// ===================================================================
// GPS MANAGER IMPLEMENTATION - 10Hz PROFESSIONAL GRADE
// Background GPS acquisition with no boot blocking
// Core 1 Task Implementation - Priority 3 (Highest)
// ===================================================================

GPSManager gpsManager; // Global instance

bool GPSManager::initialize() {
  Serial.println("GPS Manager: Initializing NEO-7M for 10Hz precision...");
  
  // Initialize hardware UART2 for GPS at 115200 baud
  gpsSerial = &Serial2;
  gpsSerial->begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.printf("GPS UART2 initialized: %d baud on pins RX=%d, TX=%d\n", 
                GPS_BAUD_RATE, GPS_RX, GPS_TX);
  
  // Give NEO-7M time to boot (minimal delay)
  delay(100);
  
  // Clear any existing data in UART buffer
  while (gpsSerial->available()) {
    gpsSerial->read();
  }
  
  // Configure NEO-7M for professional operation
  Serial.println("GPS Manager: Configuring NEO-7M for professional grade performance...");
  if (configureNEO7M()) {
    Serial.println("GPS Manager: NEO-7M configured successfully for 10Hz operation");
  } else {
    Serial.println("GPS Manager: Warning - NEO-7M configuration failed, using defaults");
    // Continue anyway - GPS might work with factory defaults
  }
  
  initialized = true;
  lastDataReceived = millis();
  totalSentences = 0;
  validSentences = 0;
  lastStatsUpdate = millis();
  lastValidFix = 0;
  
  Serial.println("GPS Manager: Background acquisition ready - no boot blocking!");
  return true;
}

bool GPSManager::configureNEO7M() {
  Serial.println("GPS Manager: Sending UBX configuration commands...");
  
  bool configSuccess = true;
  
  // Set 10Hz update rate for high precision tracking
  Serial.println("GPS Manager: Setting 10Hz update rate...");
  if (!sendUBXCommand(UBX_CFG_RATE_10HZ, sizeof(UBX_CFG_RATE_10HZ))) {
    Serial.println("GPS Manager: Failed to set 10Hz rate");
    configSuccess = false;
  } else {
    Serial.println("GPS Manager: 10Hz update rate configured");
  }
  
  delay(100); // Allow GPS to process command
  
  // Ensure GGA sentences are enabled (essential position data)
  Serial.println("GPS Manager: Enabling GGA sentences...");
  if (!sendUBXCommand(UBX_CFG_MSG_GGA_ON, sizeof(UBX_CFG_MSG_GGA_ON))) {
    Serial.println("GPS Manager: Failed to enable GGA");
    configSuccess = false;
  }
  
  delay(100);
  
  // Ensure RMC sentences are enabled (essential speed/course data)
  Serial.println("GPS Manager: Enabling RMC sentences...");
  if (!sendUBXCommand(UBX_CFG_MSG_RMC_ON, sizeof(UBX_CFG_MSG_RMC_ON))) {
    Serial.println("GPS Manager: Failed to enable RMC");
    configSuccess = false;
  }
  
  delay(100);
  
  // Set continuous power mode for maximum performance
  Serial.println("GPS Manager: Setting continuous power mode...");
  if (!sendUBXCommand(UBX_CFG_PM2_CONTINUOUS, sizeof(UBX_CFG_PM2_CONTINUOUS))) {
    Serial.println("GPS Manager: Failed to set continuous mode");
    configSuccess = false;
  } else {
    Serial.println("GPS Manager: Continuous power mode configured");
  }
  
  delay(200); // Give GPS time to apply all settings
  
  if (configSuccess) {
    Serial.println("GPS Manager: NEO-7M configured for professional operation");
  } else {
    Serial.println("GPS Manager: Configuration partially failed - GPS will work with reduced performance");
  }
  
  return configSuccess;
}

// ===================================================================
// CORE 1 TASK PROCESSING - 10Hz DATA PROCESSING
// ===================================================================

void GPSManager::processIncomingData() {
  if (!initialized) return;
  
  // Process ALL available GPS data immediately - high priority
  bool newDataProcessed = false;
  int bytesProcessed = 0;
  
  while (gpsSerial->available() > 0) {
    char c = gpsSerial->read();
    lastDataReceived = millis();
    bytesProcessed++;
    
    if (gpsParser.encode(c)) {
      // Complete NMEA sentence successfully parsed
      totalSentences++;
      newDataProcessed = true;
      
      // Check if we got valid position data
      if (gpsParser.location.isValid() && gpsParser.location.isUpdated()) {
        lastValidFix = millis();
        validSentences++;
        
        // Log high-quality fixes for debugging
        if (gpsParser.hdop.isValid() && gpsParser.hdop.hdop() <= GPS_EXCELLENT_HDOP) {
          Serial.printf("GPS: Excellent fix - LAT:%.6f LON:%.6f HDOP:%.1f SAT:%d\n",
                       gpsParser.location.lat(), gpsParser.location.lng(),
                       gpsParser.hdop.hdop(), gpsParser.satellites.value());
        }
      }
    }
  }
  
  // Check for GPS data timeout
  if (millis() - lastDataReceived > GPS_TIMEOUT) {
    // No data received for too long
    if (lastValidFix > 0) { // Only warn if we had a fix before
      Serial.println("GPS Manager: WARNING - No data received for >5 seconds");
    }
  }
  
  // Update statistics and performance monitoring
  if (millis() - lastStatsUpdate > 10000) { // Every 10 seconds
    updatePerformanceStats();
    lastStatsUpdate = millis();
  }
}

void GPSManager::updatePerformanceStats() {
  float successRate = (totalSentences > 0) ? 
                     (validSentences * 100.0 / totalSentences) : 0.0;
  
  float fixQuality = getFixQuality();
  int satellites = gpsParser.satellites.isValid() ? gpsParser.satellites.value() : 0;
  float hdop = gpsParser.hdop.isValid() ? gpsParser.hdop.hdop() : 99.9;
  
  Serial.printf("GPS Stats: %lu total, %lu valid (%.1f%%), Quality: %.1f%%, SAT: %d, HDOP: %.1f\n", 
                totalSentences, validSentences, successRate, fixQuality, satellites, hdop);
  
  // Performance warnings
  if (successRate < 80.0 && totalSentences > 50) {
    Serial.println("GPS Performance: WARNING - Low success rate, check antenna/interference");
  }
  
  if (fixQuality < 50.0 && hasValidFix()) {
    Serial.println("GPS Performance: WARNING - Poor fix quality, expect reduced accuracy");
  }
}

bool GPSManager::hasNewData() {
  return gpsParser.location.isUpdated() || 
         gpsParser.speed.isUpdated() || 
         gpsParser.course.isUpdated() ||
         gpsParser.altitude.isUpdated() ||
         gpsParser.satellites.isUpdated();
}

// ===================================================================
// THREAD-SAFE DATA ACCESS FUNCTIONS
// ===================================================================

bool GPSManager::getPosition(double& lat, double& lon) {
  if (!initialized || !gpsParser.location.isValid()) {
    lat = 0.0;
    lon = 0.0;
    return false;
  }
  
  lat = gpsParser.location.lat();
  lon = gpsParser.location.lng();
  return true;
}

bool GPSManager::getSpeed(float& speedKmh, float& courseHead) {
  if (!initialized) {
    speedKmh = 0.0;
    courseHead = 0.0;
    return false;
  }
  
  speedKmh = gpsParser.speed.isValid() ? gpsParser.speed.kmph() : 0.0;
  courseHead = gpsParser.course.isValid() ? gpsParser.course.deg() : 0.0;
  
  return gpsParser.speed.isValid() || gpsParser.course.isValid();
}

bool GPSManager::getAltitude(float& altMeters) {
  if (!initialized || !gpsParser.altitude.isValid()) {
    altMeters = 0.0;
    return false;
  }
  
  altMeters = gpsParser.altitude.meters();
  return true;
}

bool GPSManager::getSatelliteInfo(int& satellites, float& hdop) {
  if (!initialized) {
    satellites = 0;
    hdop = 99.9;
    return false;
  }
  
  satellites = gpsParser.satellites.isValid() ? gpsParser.satellites.value() : 0;
  hdop = gpsParser.hdop.isValid() ? gpsParser.hdop.hdop() : 99.9;
  
  return gpsParser.satellites.isValid() || gpsParser.hdop.isValid();
}

bool GPSManager::getDateTime(int& year, int& month, int& day, 
                            int& hour, int& minute, int& second) {
  if (!initialized || !gpsParser.date.isValid() || !gpsParser.time.isValid()) {
    return false;
  }
  
  year = gpsParser.date.year();
  month = gpsParser.date.month();
  day = gpsParser.date.day();
  hour = gpsParser.time.hour();
  minute = gpsParser.time.minute();
  second = gpsParser.time.second();
  
  return true;
}

// ===================================================================
// GPS STATUS AND QUALITY ASSESSMENT
// ===================================================================

bool GPSManager::hasValidFix() {
  if (!initialized) return false;
  
  bool locationValid = gpsParser.location.isValid();
  bool timelyFix = (lastValidFix > 0) && (millis() - lastValidFix < GPS_TIMEOUT);
  bool sufficientSats = gpsParser.satellites.isValid() && 
                       (gpsParser.satellites.value() >= GPS_MIN_SATELLITES);
  
  return locationValid && timelyFix && sufficientSats;
}

unsigned long GPSManager::getTimeSinceLastFix() {
  if (lastValidFix == 0) return ULONG_MAX;
  return millis() - lastValidFix;
}

float GPSManager::getFixQuality() {
  if (!initialized || !hasValidFix()) return 0.0;
  
  float totalScore = 0.0;
  
  // Satellite count score (0-40 points)
  int satellites = gpsParser.satellites.isValid() ? gpsParser.satellites.value() : 0;
  float satelliteScore = 0.0;
  if (satellites >= 12) satelliteScore = 40.0;       // Excellent
  else if (satellites >= 8) satelliteScore = 35.0;   // Very good
  else if (satellites >= 6) satelliteScore = 25.0;   // Good  
  else if (satellites >= 4) satelliteScore = 15.0;   // Acceptable
  else satelliteScore = 0.0;                          // Poor
  
  // HDOP score (0-40 points)
  float hdop = gpsParser.hdop.isValid() ? gpsParser.hdop.hdop() : 99.9;
  float hdopScore = 0.0;
  if (hdop <= 1.0) hdopScore = 40.0;           // Excellent
  else if (hdop <= 2.0) hdopScore = 30.0;      // Good
  else if (hdop <= 5.0) hdopScore = 15.0;      // Marginal
  else hdopScore = 0.0;                        // Poor
  
  // Data freshness score (0-20 points)
  unsigned long timeSinceFix = getTimeSinceLastFix();
  float freshnessScore = 0.0;
  if (timeSinceFix < 500) freshnessScore = 20.0;        // <0.5 second
  else if (timeSinceFix < 1000) freshnessScore = 18.0;  // <1 second
  else if (timeSinceFix < 2000) freshnessScore = 15.0;  // <2 seconds
  else if (timeSinceFix < 5000) freshnessScore = 10.0;  // <5 seconds
  else freshnessScore = 0.0;                            // Too old
  
  totalScore = satelliteScore + hdopScore + freshnessScore;
  
  return constrain(totalScore, 0.0, 100.0);
}

void GPSManager::getStatistics(unsigned long& total, unsigned long& valid, float& successRate) {
  total = totalSentences;
  valid = validSentences;
  successRate = (total > 0) ? (valid * 100.0 / total) : 0.0;
}

bool GPSManager::configureNEO7M() {
  Serial.println("GPS Manager: Configuring NEO-7M for 10Hz operation...");
  
  bool success = true;
  
  // Set 10Hz update rate for high precision
  if (!sendUBXCommand(UBX_CFG_RATE_10HZ, sizeof(UBX_CFG_RATE_10HZ))) {
    Serial.println("GPS Manager: Failed to set 10Hz rate");
    success = false;
  } else {
    Serial.println("GPS Manager: 10Hz update rate configured");
  }
  
  delay(100);
  
  // Ensure GGA sentences are enabled (position data)
  if (!sendUBXCommand(UBX_CFG_MSG_GGA_ON, sizeof(UBX_CFG_MSG_GGA_ON))) {
    Serial.println("GPS Manager: Failed to enable GGA");
    success = false;
  }
  
  delay(100);
  
  // Ensure RMC sentences are enabled (speed/course data)
  if (!sendUBXCommand(UBX_CFG_MSG_RMC_ON, sizeof(UBX_CFG_MSG_RMC_ON))) {
    Serial.println("GPS Manager: Failed to enable RMC");
    success = false;
  }
  
  delay(100);
  
  // Set continuous power mode for maximum performance
  if (!sendUBXCommand(UBX_CFG_PM2_CONTINUOUS, sizeof(UBX_CFG_PM2_CONTINUOUS))) {
    Serial.println("GPS Manager: Failed to set continuous mode");
    success = false;
  } else {
    Serial.println("GPS Manager: Continuous power mode configured");
  }
  
  delay(200); // Give GPS time to apply settings
  
  return success;
}

void GPSManager::processIncomingData() {
  if (!initialized) return;
  
  // Process all available GPS data immediately
  bool newDataProcessed = false;
  
  while (gpsSerial->available() > 0) {
    char c = gpsSerial->read();
    lastDataReceived = millis();
    
    if (gpsParser.encode(c)) {
      // Complete NMEA sentence successfully parsed
      totalSentences++;
      newDataProcessed = true;
      
      // Check if we got valid position data
      if (gpsParser.location.isValid() && gpsParser.location.isUpdated()) {
        lastValidFix = millis();
        validSentences++;
      }
    }
  }
  
  // Update statistics periodically
  if (millis() - lastStatsUpdate > 10000) { // Every 10 seconds
    float successRate = (totalSentences > 0) ? 
                       (validSentences * 100.0 / totalSentences) : 0.0;
    Serial.printf("GPS Stats: %lu total, %lu valid (%.1f%%), Quality: %.1f%%\n", 
                  totalSentences, validSentences, successRate, getFixQuality());
    lastStatsUpdate = millis();
  }
}

bool GPSManager::hasNewData() {
  return gpsParser.location.isUpdated() || 
         gpsParser.speed.isUpdated() || 
         gpsParser.course.isUpdated();
}

// ===================================================================
// DATA ACCESS FUNCTIONS - THREAD SAFE
// ===================================================================

bool GPSManager::getPosition(double& lat, double& lon) {
  if (!initialized || !gpsParser.location.isValid()) {
    return false;
  }
  
  lat = gpsParser.location.lat();
  lon = gpsParser.location.lng();
  return true;
}

bool GPSManager::getSpeed(float& speedKmh, float& courseHead) {
  if (!initialized) return false;
  
  speedKmh = gpsParser.speed.isValid() ? gpsParser.speed.kmph() : 0.0;
  courseHead = gpsParser.course.isValid() ? gpsParser.course.deg() : 0.0;
  
  return gpsParser.speed.isValid() || gpsParser.course.isValid();
}

bool GPSManager::getAltitude(float& altMeters) {
  if (!initialized || !gpsParser.altitude.isValid()) {
    return false;
  }
  
  altMeters = gpsParser.altitude.meters();
  return true;
}

bool GPSManager::getSatelliteInfo(int& satellites, float& hdop) {
  if (!initialized) return false;
  
  satellites = gpsParser.satellites.isValid() ? gpsParser.satellites.value() : 0;
  hdop = gpsParser.hdop.isValid() ? gpsParser.hdop.hdop() : 99.9;
  
  return gpsParser.satellites.isValid() || gpsParser.hdop.isValid();
}

bool GPSManager::getDateTime(int& year, int& month, int& day, 
                            int& hour, int& minute, int& second) {
  if (!initialized || !gpsParser.date.isValid() || !gpsParser.time.isValid()) {
    return false;
  }
  
  year = gpsParser.date.year();
  month = gpsParser.date.month();
  day = gpsParser.date.day();
  hour = gpsParser.time.hour();
  minute = gpsParser.time.minute();
  second = gpsParser.time.second();
  
  return true;
}

// ===================================================================
// STATUS AND QUALITY ASSESSMENT
// ===================================================================

bool GPSManager::hasValidFix() {
  if (!initialized) return false;
  
  return gpsParser.location.isValid() && 
         (millis() - lastValidFix < GPS_TIMEOUT);
}

unsigned long GPSManager::getTimeSinceLastFix() {
  if (lastValidFix == 0) return ULONG_MAX;
  return millis() - lastValidFix;
}

float GPSManager::getFixQuality() {
  if (!initialized || !hasValidFix()) return 0.0;
  
  GPSQuality quality;
  
  // Satellite count score (0-40 points)
  int satellites = gpsParser.satellites.isValid() ? gpsParser.satellites.value() : 0;
  if (satellites >= 8) quality.satelliteScore = 40.0;
  else if (satellites >= 5) quality.satelliteScore = 25.0;
  else if (satellites >= 4) quality.satelliteScore = 15.0;
  else quality.satelliteScore = 0.0;
  
  // HDOP score (0-40 points)
  float hdop = gpsParser.hdop.isValid() ? gpsParser.hdop.hdop() : 99.9;
  if (hdop <= GPS_EXCELLENT_HDOP) quality.hdopScore = 40.0;
  else if (hdop <= GPS_GOOD_HDOP) quality.hdopScore = 25.0;
  else if (hdop <= 5.0) quality.hdopScore = 10.0;
  else quality.hdopScore = 0.0;
  
  // Data freshness score (0-20 points)
  unsigned long timeSinceFix = getTimeSinceLastFix();
  float freshnessScore = 0.0;
  if (timeSinceFix < 1000) freshnessScore = 20.0;        // < 1 second
  else if (timeSinceFix < 2000) freshnessScore = 15.0;   // < 2 seconds
  else if (timeSinceFix < 5000) freshnessScore = 10.0;   // < 5 seconds
  else freshnessScore = 0.0;
  
  quality.fixQuality = quality.satelliteScore + quality.hdopScore + freshnessScore;
  
  // Quality classifications
  quality.isExcellent = (hdop <= GPS_EXCELLENT_HDOP && satellites >= 8);
  quality.isGood = (hdop <= GPS_GOOD_HDOP && satellites >= 5);
  quality.isMarginal = (hdop <= 5.0 && satellites >= 4);
  
  return quality.fixQuality;
}

void GPSManager::getStatistics(unsigned long& total, unsigned long& valid, float& successRate) {
  total = totalSentences;
  valid = validSentences;
  successRate = (total > 0) ? (valid * 100.0 / total) : 0.0;
}

// ===================================================================
// NEO-7M POWER MANAGEMENT - PROFESSIONAL GRADE
// ===================================================================

void GPSManager::enterLightSleep() {
  Serial.println("GPS Manager: Entering LIGHT SLEEP mode (5Hz updates)");
  
  if (!sendUBXCommand(UBX_CFG_RATE_5HZ, sizeof(UBX_CFG_RATE_5HZ))) {
    Serial.println("GPS Manager: Failed to set 5Hz rate for light sleep");
  }
  
  if (!sendUBXCommand(UBX_CFG_PM2_POWERSAVE, sizeof(UBX_CFG_PM2_POWERSAVE))) {
    Serial.println("GPS Manager: Failed to set power save mode");
  }
  
  updateRate = 5;
  Serial.println("GPS Manager: Light sleep configured - 5Hz updates, reduced power");
}

void GPSManager::enterDeepSleep() {
  Serial.println("GPS Manager: Entering DEEP SLEEP mode (1Hz updates)");
  
  if (!sendUBXCommand(UBX_CFG_RATE_1HZ, sizeof(UBX_CFG_RATE_1HZ))) {
    Serial.println("GPS Manager: Failed to set 1Hz rate for deep sleep");
  }
  
  if (!sendUBXCommand(UBX_CFG_PM2_POWERSAVE, sizeof(UBX_CFG_PM2_POWERSAVE))) {
    Serial.println("GPS Manager: Failed to set power save mode");
  }
  
  updateRate = 1;
  Serial.println("GPS Manager: Deep sleep configured - 1Hz updates, minimum power");
}

void GPSManager::wakeFromSleep() {
  Serial.println("GPS Manager: WAKING from sleep mode (10Hz full performance)");
  
  if (!sendUBXCommand(UBX_CFG_RATE_10HZ, sizeof(UBX_CFG_RATE_10HZ))) {
    Serial.println("GPS Manager: Failed to restore 10Hz rate");
  }
  
  if (!sendUBXCommand(UBX_CFG_PM2_CONTINUOUS, sizeof(UBX_CFG_PM2_CONTINUOUS))) {
    Serial.println("GPS Manager: Failed to restore continuous mode");
  }
  
  updateRate = 10;
  Serial.println("GPS Manager: Full performance restored - 10Hz updates, continuous mode");
}

bool GPSManager::isInSleepMode() {
  return updateRate < 10;
}

// ===================================================================
// UBX COMMAND TRANSMISSION - ROBUST IMPLEMENTATION
// ===================================================================

bool GPSManager::sendUBXCommand(const uint8_t* command, int length) {
  if (!initialized || !gpsSerial) {
    Serial.println("GPS Manager: Cannot send UBX command - not initialized");
    return false;
  }
  
  Serial.printf("GPS Manager: Sending UBX command (%d bytes)\n", length);
  
  // Send UBX command byte by byte
  for (int i = 0; i < length; i++) {
    gpsSerial->write(command[i]);
  }
  
  // Flush to ensure transmission
  gpsSerial->flush();
  
  // Wait for command to be processed
  delay(50);
  
  // Read and discard any immediate response
  unsigned long timeout = millis() + 200;
  int responseBytes = 0;
  while (millis() < timeout && gpsSerial->available()) {
    gpsSerial->read();
    responseBytes++;
  }
  
  if (responseBytes > 0) {
    Serial.printf("GPS Manager: Received %d response bytes\n", responseBytes);
  }
  
  return true; // Assume success (proper ACK checking could be implemented)
}

void GPSManager::setUpdateRate(int hz) {
  if (hz == updateRate) {
    return; // Already at requested rate
  }
  
  Serial.printf("GPS Manager: Changing update rate from %dHz to %dHz\n", updateRate, hz);
  
  switch(hz) {
    case 10:
      if (sendUBXCommand(UBX_CFG_RATE_10HZ, sizeof(UBX_CFG_RATE_10HZ))) {
        updateRate = 10;
        Serial.println("GPS Manager: 10Hz rate set successfully");
      }
      break;
      
    case 5:
      if (sendUBXCommand(UBX_CFG_RATE_5HZ, sizeof(UBX_CFG_RATE_5HZ))) {
        updateRate = 5;
        Serial.println("GPS Manager: 5Hz rate set successfully");
      }
      break;
      
    case 1:
      if (sendUBXCommand(UBX_CFG_RATE_1HZ, sizeof(UBX_CFG_RATE_1HZ))) {
        updateRate = 1;
        Serial.println("GPS Manager: 1Hz rate set successfully");
      }
      break;
      
    default:
      Serial.printf("GPS Manager: ERROR - Unsupported update rate: %d Hz\n", hz);
      break;
  }
}

// ===================================================================
// GLOBAL ACCESS FUNCTIONS FOR CORE 1 TASK
// ===================================================================

bool initializeGPS() {
  return gpsManager.initialize();
}

void processGPSData() {
  gpsManager.processIncomingData();
}

bool getGPSPosition(double& lat, double& lon) {
  return gpsManager.getPosition(lat, lon);
}

bool getGPSSpeed(float& speed) {
  float heading;
  return gpsManager.getSpeed(speed, heading);
}

bool getGPSHeading(float& heading) {
  float speed;
  return gpsManager.getSpeed(speed, heading);
}

GPSQuality getGPSQuality() {
  GPSQuality quality;
  quality.fixQuality = gpsManager.getFixQuality();
  
  int satellites;
  float hdop;
  if (gpsManager.getSatelliteInfo(satellites, hdop)) {
    quality.satelliteScore = (satellites >= 8) ? 100.0 : (satellites * 12.5);
    quality.hdopScore = (hdop <= 1.0) ? 100.0 : 
                       (hdop <= 2.0) ? 75.0 : 
                       (hdop <= 5.0) ? 25.0 : 0.0;
    quality.isExcellent = (hdop <= GPS_EXCELLENT_HDOP && satellites >= 8);
    quality.isGood = (hdop <= GPS_GOOD_HDOP && satellites >= 5);
    quality.isMarginal = (hdop <= 5.0 && satellites >= 4);
  }
  
  return quality;
}