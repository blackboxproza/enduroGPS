#include "smart_tracker.h"
#include <Arduino.h>
#include <esp_sleep.h>

// ===================================================================
// SMART TRACKER IMPLEMENTATION - MOTION-BASED INTELLIGENT TRACKING
// Instant motion response vs GPS speed delay
// Core 1 Task Implementation - Priority 1 (Normal)
// ===================================================================

SmartTracker smartTracker; // Global instance
PowerManager powerManager; // Global power manager

bool SmartTracker::initialize(SDManager* sd, GPSManager* gps, CompassManager* compass) {
  Serial.println("Smart Tracker: Initializing motion-based tracking system...");
  
  // Store component references
  sdManager = sd;
  gpsManager = gps;
  compassManager = compass;
  
  // Initialize motion detection configuration
  motionConfig.enabled = true;
  motionConfig.threshold = WAKE_MOTION_THRESH; // 0.3g
  motionConfig.autoStopDelay = 30000;          // 30 seconds
  motionConfig.autoSleepDelay = 60000;         // 1 minute
  motionConfig.instantResponse = true;         // Key feature!
  motionConfig.useGPSSpeed = false;           // Don't wait for GPS
  
  // Initialize wake system
  wakeSystem.motionWakeEnabled = true;
  wakeSystem.timerWakeEnabled = true;
  wakeSystem.timerWakeInterval = 3600000;     // 1 hour
  wakeSystem.lastTimerWake = millis();
  wakeSystem.lastMotionWake = 0;
  wakeSystem.wasWokenByMotion = false;
  wakeSystem.wasWokenByTimer = false;
  
  // Initialize track state
  trackState.isRecording = false;
  trackState.isPaused = false;
  trackState.mode = SMART_TRACK_AUTO;
  trackState.currentTrackName = "";
  trackState.trackStartTime = 0;
  trackState.totalRecordingTime = 0;
  trackState.pausedTime = 0;
  trackState.gpsPowerState = GPS_POWER_FULL;
  trackState.needsResume = false;
  
  // Load persisted state if available
  if (!loadStateFromNVS()) {
    Serial.println("Smart Tracker: No persisted state - starting fresh");
  } else {
    Serial.println("Smart Tracker: Loaded persisted state from NVS");
  }
  
  // Initialize power management
  currentGPSPower = GPS_POWER_FULL;
  lastPowerChange = millis();
  systemSleeping = false;
  
  // Initialize statistics
  totalPointsRecorded = 0;
  totalDistance = 0;
  sessionsRecorded = 0;
  
  lastMotionTime = millis();
  lastGPSPoint = 0;
  
  initialized = true;
  
  Serial.println("Smart Tracker: Motion-based tracking system ready!");
  Serial.println("  - Instant motion response enabled");
  Serial.println("  - GPS power management enabled");
  Serial.println("  - Dual wake system (motion + 1-hour timer)");
  
  return true;
}

// ===================================================================
// CORE TRACKING LOGIC - CALLED FROM CORE 1 TASK
// ===================================================================

void SmartTracker::update() {
  if (!initialized) return;
  
  unsigned long now = millis();
  
  // Process motion-based decisions
  if (motionConfig.enabled) {
    if (shouldStartRecording()) {
      handleAutoStart();
    } else if (shouldStopRecording()) {
      handleAutoStop();
    } else if (shouldPauseRecording()) {
      handleAutoPause();
    }
  }
  
  // Process GPS data if recording
  if (trackState.isRecording && !trackState.isPaused) {
    processGPSData();
  }
  
  // Manage GPS power based on activity
  manageGPSPower();
  
  // Check wake system
  if (wakeSystem.timerWakeEnabled) {
    if (now - wakeSystem.lastTimerWake > wakeSystem.timerWakeInterval) {
      Serial.println("Smart Tracker: Timer wake event");
      wakeSystem.lastTimerWake = now;
      wakeSystem.wasWokenByTimer = true;
      
      // Brief wake to check GPS and save data
      if (currentGPSPower == GPS_POWER_SLEEP) {
        transitionGPSPower(GPS_POWER_LIGHT);
        delay(5000); // 5 seconds to get GPS fix
        processGPSData();
        transitionGPSPower(GPS_POWER_SLEEP);
      }
    }
  }
  
  // Check if should enter sleep
  if (shouldEnterSleep()) {
    enterSystemSleep();
  }
  
  // Update statistics
  updateStatistics();
}

void SmartTracker::processMotionEvent(bool motion) {
  motionDetected = motion;
  
  if (motion) {
    lastMotionTime = millis();
    wakeSystem.lastMotionWake = millis();
    wakeSystem.wasWokenByMotion = true;
    
    Serial.println("Smart Tracker: Motion detected - instant response!");
    
    // INSTANT RESPONSE - key advantage over GPS speed
    if (motionConfig.instantResponse) {
      if (trackState.mode == SMART_TRACK_AUTO && !trackState.isRecording) {
        Serial.println("Smart Tracker: Auto-starting track (motion-based)");
        startTracking();
      } else if (trackState.isPaused) {
        Serial.println("Smart Tracker: Auto-resuming track (motion-based)");
        resumeTracking();
      }
    }
    
    // Wake GPS if sleeping
    if (currentGPSPower == GPS_POWER_SLEEP || currentGPSPower == GPS_POWER_DEEP) {
      Serial.println("Smart Tracker: Waking GPS for motion event");
      transitionGPSPower(GPS_POWER_FULL);
    }
  }
}

void SmartTracker::processGPSData() {
  if (!gpsManager || !sdManager) return;
  
  double lat, lon;
  float speed, heading, altitude;
  
  // Get current GPS data
  if (!gpsManager->getPosition(lat, lon)) {
    return; // No valid GPS data
  }
  
  gpsManager->getSpeed(speed, heading);
  gpsManager->getAltitude(altitude);
  
  // Create GPS point
  GPXPoint point = {
    .latitude = lat,
    .longitude = lon,
    .altitude = altitude,
    .timestamp = millis(),
    .speed = speed,
    .heading = heading,
    .isValid = true
  };
  
  // Record point if actively tracking
  if (trackState.isRecording && !trackState.isPaused) {
    if (sdManager->addTrackPoint(point)) {
      totalPointsRecorded++;
      lastGPSPoint = millis();
    }
  }
}

// ===================================================================
// MOTION-BASED DECISION LOGIC
// ===================================================================

bool SmartTracker::shouldStartRecording() {
  if (trackState.mode != SMART_TRACK_AUTO) return false;
  if (trackState.isRecording) return false;
  
  // Start based on motion detection (instant response)
  return isSignificantMotion();
}

bool SmartTracker::shouldStopRecording() {
  if (!trackState.isRecording) return false;
  if (trackState.mode == SMART_TRACK_ALWAYS) return false;
  
  // Stop based on motion timeout
  return hasMotionTimeout();
}

bool SmartTracker::shouldPauseRecording() {
  if (!trackState.isRecording) return false;
  if (trackState.isPaused) return false;
  if (trackState.mode == SMART_TRACK_ALWAYS) return false;
  
  // Pause based on motion timeout (before stopping)
  unsigned long timeSinceMotion = millis() - lastMotionTime;
  return timeSinceMotion > (motionConfig.autoStopDelay / 2); // Pause at 15 seconds
}

bool SmartTracker::shouldEnterSleep() {
  if (trackState.isRecording && !trackState.isPaused) return false;
  if (trackState.mode == SMART_TRACK_ALWAYS) return false;
  
  return hasSleepTimeout();
}

bool SmartTracker::isSignificantMotion() {
  if (!compassManager) return false;
  
  return compassManager->isMotionDetected();
}

bool SmartTracker::hasMotionTimeout() {
  unsigned long timeSinceMotion = millis() - lastMotionTime;
  return timeSinceMotion > motionConfig.autoStopDelay;
}

bool SmartTracker::hasSleepTimeout() {
  unsigned long timeSinceMotion = millis() - lastMotionTime;
  return timeSinceMotion > motionConfig.autoSleepDelay;
}

// ===================================================================
// TRACK CONTROL FUNCTIONS
// ===================================================================

bool SmartTracker::startTracking(const String& trackName) {
  if (!sdManager) return false;
  
  Serial.println("Smart Tracker: Starting track recording");
  
  if (!sdManager->startTrack(trackName)) {
    Serial.println("Smart Tracker: Failed to start track file");
    return false;
  }
  
  trackState.isRecording = true;
  trackState.isPaused = false;
  trackState.trackStartTime = millis();
  trackState.currentTrackName = trackName;
  trackState.needsResume = false;
  
  // Ensure GPS is at full power for recording
  if (currentGPSPower != GPS_POWER_FULL) {
    transitionGPSPower(GPS_POWER_FULL);
  }
  
  saveStateToNVS();
  sessionsRecorded++;
  
  Serial.println("Smart Tracker: Track recording started successfully");
  return true;
}

bool SmartTracker::stopTracking() {
  if (!trackState.isRecording) return true;
  
  Serial.println("Smart Tracker: Stopping track recording");
  
  if (sdManager) {
    sdManager->stopTrack();
  }
  
  trackState.isRecording = false;
  trackState.isPaused = false;
  trackState.totalRecordingTime += millis() - trackState.trackStartTime;
  trackState.currentTrackName = "";
  trackState.needsResume = false;
  
  saveStateToNVS();
  
  Serial.printf("Smart Tracker: Track stopped. Total recording time: %lu ms\n", 
                trackState.totalRecordingTime);
  return true;
}

bool SmartTracker::pauseTracking() {
  if (!trackState.isRecording || trackState.isPaused) return false;
  
  Serial.println("Smart Tracker: Pausing track recording");
  
  if (sdManager) {
    sdManager->pauseTrack();
  }
  
  trackState.isPaused = true;
  trackState.pausedTime = millis();
  
  saveStateToNVS();
  
  Serial.println("Smart Tracker: Track recording paused");
  return true;
}

bool SmartTracker::resumeTracking() {
  if (!trackState.isRecording || !trackState.isPaused) return false;
  
  Serial.println("Smart Tracker: Resuming track recording");
  
  if (sdManager) {
    sdManager->resumeTrack();
  }
  
  trackState.isPaused = false;
  trackState.pausedTime = 0;
  
  // Ensure GPS is ready for recording
  if (currentGPSPower != GPS_POWER_FULL) {
    transitionGPSPower(GPS_POWER_FULL);
  }
  
  saveStateToNVS();
  
  Serial.println("Smart Tracker: Track recording resumed");
  return true;
}

// ===================================================================
// GPS POWER MANAGEMENT
// ===================================================================

void SmartTracker::manageGPSPower() {
  if (!gpsManager) return;
  
  unsigned long timeSinceMotion = millis() - lastMotionTime;
  unsigned long timeSinceChange = millis() - lastPowerChange;
  
  // Don't change power state too frequently
  if (timeSinceChange < 10000) return; // 10 second minimum between changes
  
  GPSPowerState targetState = currentGPSPower;
  
  if (trackState.isRecording && !trackState.isPaused) {
    // Recording - maintain full power
    targetState = GPS_POWER_FULL;
  } else if (timeSinceMotion < 30000) {
    // Recent motion - light sleep
    targetState = GPS_POWER_LIGHT;
  } else if (timeSinceMotion < 60000) {
    // No motion for 30-60 seconds - deep sleep
    targetState = GPS_POWER_DEEP;
  } else {
    // No motion for >60 seconds - sleep mode
    targetState = GPS_POWER_SLEEP;
  }
  
  if (targetState != currentGPSPower) {
    transitionGPSPower(targetState);
  }
}

void SmartTracker::transitionGPSPower(GPSPowerState newState) {
  if (newState == currentGPSPower) return;
  
  Serial.printf("Smart Tracker: GPS power transition %d -> %d\n", 
                currentGPSPower, newState);
  
  switch(newState) {
    case GPS_POWER_FULL:
      if (gpsManager) gpsManager->wakeFromSleep();
      Serial.println("GPS Power: FULL (10Hz, continuous)");
      break;
      
    case GPS_POWER_LIGHT:
      if (gpsManager) gpsManager->enterLightSleep();
      Serial.println("GPS Power: LIGHT (5Hz, power save)");
      break;
      
    case GPS_POWER_DEEP:
      if (gpsManager) gpsManager->enterDeepSleep();
      Serial.println("GPS Power: DEEP (1Hz, deep power save)");
      break;
      
    case GPS_POWER_SLEEP:
      Serial.println("GPS Power: SLEEP (no updates, wake on motion)");
      break;
  }
  
  currentGPSPower = newState;
  trackState.gpsPowerState = newState;
  lastPowerChange = millis();
  
  saveStateToNVS();
}

// ===================================================================
// SLEEP/WAKE SYSTEM
// ===================================================================

void SmartTracker::enterSystemSleep() {
  Serial.println("Smart Tracker: Entering system sleep mode");
  
  // Save current state before sleeping
  saveTrackStateBeforeSleep();
  
  // Configure wake sources
  setupWakeInterrupts();
  
  // Set GPS to sleep mode
  transitionGPSPower(GPS_POWER_SLEEP);
  
  systemSleeping = true;
  
  // Enter ESP32 light sleep (maintains RAM, quick wake)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 1); // Motion interrupt
  esp_sleep_enable_timer_wakeup(wakeSystem.timerWakeInterval * 1000); // 1 hour timer
  
  Serial.println("Smart Tracker: Entering light sleep...");
  esp_light_sleep_start();
  
  // Execution continues here after wake
  Serial.println("Smart Tracker: Waking from sleep");
  wakeFromSleep(true, false); // Assume motion wake for now
}

void SmartTracker::wakeFromSleep(bool motionWake, bool timerWake) {
  Serial.printf("Smart Tracker: Wake event (Motion: %s, Timer: %s)\n",
                motionWake ? "YES" : "NO", timerWake ? "YES" : "NO");
  
  systemSleeping = false;
  wakeSystem.wasWokenByMotion = motionWake;
  wakeSystem.wasWokenByTimer = timerWake;
  
  if (motionWake) {
    wakeSystem.lastMotionWake = millis();
    lastMotionTime = millis(); // Reset motion timer
  }
  
  if (timerWake) {
    wakeSystem.lastTimerWake = millis();
  }
  
  // Restore track state after wake
  restoreTrackStateAfterWake();
  
  // Wake GPS if needed
  if (motionWake || trackState.needsResume) {
    transitionGPSPower(GPS_POWER_FULL);
  } else {
    transitionGPSPower(GPS_POWER_LIGHT);
  }
}

void SmartTracker::setupWakeInterrupts() {
  // Configure motion interrupt as wake source
  pinMode(COMPASS_INT1, INPUT_PULLUP);
}

// ===================================================================
// STATE PERSISTENCE
// ===================================================================

void SmartTracker::saveTrackStateBeforeSleep() {
  if (trackState.isRecording && !trackState.isPaused) {
    Serial.println("Smart Tracker: Pausing track before sleep");
    pauseTracking();
    trackState.needsResume = true;
  }
  
  saveStateToNVS();
  Serial.println("Smart Tracker: Track state saved before sleep");
}

void SmartTracker::restoreTrackStateAfterWake() {
  if (loadStateFromNVS()) {
    Serial.println("Smart Tracker: Track state restored after wake");
    
    if (trackState.needsResume && wakeSystem.wasWokenByMotion) {
      Serial.println("Smart Tracker: Resuming track after motion wake");
      resumeTracking();
      trackState.needsResume = false;
      saveStateToNVS();
    }
  }
}

bool SmartTracker::loadStateFromNVS() {
  if (!nvs.begin("smart_tracker", true)) {
    return false;
  }
  
  size_t stateSize = nvs.getBytesLength("track_state");
  if (stateSize != sizeof(TrackState)) {
    nvs.end();
    return false;
  }
  
  TrackState tempState;
  if (nvs.getBytes("track_state", &tempState, sizeof(TrackState)) == 0) {
    nvs.end();
    return false;
  }
  
  if (!validatePersistedState(tempState)) {
    nvs.end();
    return false;
  }
  
  trackState = tempState;
  nvs.end();
  return true;
}

bool SmartTracker::saveStateToNVS() {
  if (!nvs.begin("smart_tracker", false)) {
    return false;
  }
  
  trackState.magicNumber = 0xDEADBEEF;
  
  size_t written = nvs.putBytes("track_state", &trackState, sizeof(TrackState));
  nvs.end();
  
  return written == sizeof(TrackState);
}

bool SmartTracker::validatePersistedState(const TrackState& state) {
  return state.magicNumber == 0xDEADBEEF;
}

// ===================================================================
// AUTO-HANDLERS
// ===================================================================

void SmartTracker::handleAutoStart() {
  if (!trackState.isRecording) {
    startTracking();
  }
}

void SmartTracker::handleAutoStop() {
  if (trackState.isRecording) {
    stopTracking();
  }
}

void SmartTracker::handleAutoPause() {
  if (trackState.isRecording && !trackState.isPaused) {
    pauseTracking();
  }
}

// ===================================================================
// STATUS AND STATISTICS
// ===================================================================

bool SmartTracker::isRecording() {
  return trackState.isRecording && !trackState.isPaused;
}

bool SmartTracker::isPaused() {
  return trackState.isPaused;
}

SmartTrackMode SmartTracker::getTrackMode() {
  return trackState.mode;
}

TrackState SmartTracker::getTrackState() {
  return trackState;
}

unsigned long SmartTracker::getTimeSinceLastMotion() {
  return millis() - lastMotionTime;
}

void SmartTracker::updateStatistics() {
  // Statistics updated in real-time by other functions
}

unsigned long SmartTracker::getTotalPointsRecorded() {
  return totalPointsRecorded;
}

// ===================================================================
// GLOBAL ACCESS FUNCTIONS FOR CORE 1 TASK
// ===================================================================

bool initializeSmartTracking() {
  return smartTracker.initialize(&sdManager, &gpsManager, &compassManager);
}

void updateSmartTracking() {
  smartTracker.update();
}

void handleMotionDetection(bool motion) {
  smartTracker.processMotionEvent(motion);
}

bool isCurrentlyTracking() {
  return smartTracker.isRecording();
}

void enterTrackingSleep() {
  smartTracker.enterSystemSleep();
}

void wakeFromTrackingSleep() {
  smartTracker.wakeFromSleep(true, false);
} 
