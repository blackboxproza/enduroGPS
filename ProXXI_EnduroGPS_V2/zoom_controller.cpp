/*
 * FILE LOCATION: ProXXI_EnduroGPS_V2/input/zoom_controller.cpp
 * 
 * E_NAVI EnduroGPS - Professional Zoom Controller Implementation
 * Speed-responsive auto-zoom with intelligent manual override system
 * 
 * Features:
 * - Auto-zoom based on GPS speed with smooth transitions
 * - Instant manual zoom control with button feedback
 * - Smart override timeout system (10 seconds default)
 * - Visual feedback for zoom level and mode changes
 * - Settings persistence via NVS storage
 * - Optimized for enduro riding and mature rider vision
 * - Seamless integration with Week 1's map display system
 * 
 * Integration Points:
 * - ButtonManager for user input (zoom in/out buttons)
 * - MapDisplay for zoom level changes
 * - SharedData for GPS speed information
 * - DisplayManager for visual feedback
 * - Preferences for settings persistence
 * 
 * By ProXXi Pty Ltd - "Lost, Never Again"
 */

#include "zoom_controller.h"
#include "shared_data.h"
#include "display_manager.h"
#include "map_display.h"
#include "gps_manager.h"
#include <math.h>

// ===================================================================
// GLOBAL INSTANCES
// ===================================================================
ZoomController zoomController;
ZoomMenuSystem zoomMenu;
ZoomNotification zoomNotification;

// ===================================================================
// ZOOM CONTROLLER IMPLEMENTATION
// ===================================================================

bool ZoomController::initialize(ButtonManager* btnMgr) {
  Serial.println("Zoom Controller: Initializing professional zoom system...");
  
  // Store button manager reference
  buttonManager = btnMgr;
  
  // Initialize NVS storage
  if (!nvs.begin("zoom_ctrl", false)) {
    Serial.println("Zoom Controller: WARNING - NVS storage failed, using defaults");
  }
  
  // Load saved settings or use defaults
  if (!loadSettings()) {
    Serial.println("Zoom Controller: Using default configuration");
    setDefaultSettings();
    saveSettings();
  }
  
  // Initialize menu system
  if (!zoomMenu.initialize(this)) {
    Serial.println("Zoom Controller: WARNING - Menu system initialization failed");
  }
  
  // Set initial state
  currentSpeed = 0.0;
  lastSpeedUpdate = millis();
  lastZoomChange = millis();
  initialized = true;
  
  Serial.println("Zoom Controller: Professional zoom system ready!");
  Serial.printf("  - Initial zoom: %d (range %d-%d)\n", 
                config.currentLevel, config.minLevel, config.maxLevel);
  Serial.printf("  - Mode: %s\n", getZoomModeText().c_str());
  Serial.printf("  - Auto-zoom factor: %.1f\n", config.responseFactor);
  Serial.printf("  - Manual timeout: %lu seconds\n", config.manualOverrideTimeout / 1000);
  
  // Show initialization notification
  zoomNotification.showModeChange(config.mode);
  
  return true;
}

void ZoomController::update(float speed) {
  if (!initialized) return;
  
  unsigned long now = millis();
  currentSpeed = speed;
  lastSpeedUpdate = now;
  
  // Update zoom transition animation
  updateZoomTransition();
  
  // Handle auto-zoom logic
  if (config.mode == ZOOM_AUTO || 
      (config.mode == ZOOM_SMART && !isManualOverrideActive())) {
    updateAutoZoom(speed);
  }
  
  // Check manual override timeout
  if (config.mode == ZOOM_SMART) {
    checkOverrideTimeout();
  }
  
  // Update visual notifications
  zoomNotification.update();
}

void ZoomController::handleButtonInput() {
  if (!initialized || !buttonManager) return;
  
  // Check zoom in button
  if (buttonManager->wasButtonPressed(BTN_ZOOM_IN)) {
    zoomIn();
  }
  
  // Check zoom out button  
  if (buttonManager->wasButtonPressed(BTN_ZOOM_OUT)) {
    zoomOut();
  }
  
  // Check for combination inputs (future expansion)
  // e.g., Hold center + zoom = mode change
  if (buttonManager->isButtonHeld(BTN_CENTER) && 
      buttonManager->wasButtonPressed(BTN_ZOOM_IN)) {
    // Cycle through zoom modes
    ZoomMode newMode = (ZoomMode)((config.mode + 1) % 4);
    setZoomMode(newMode);
    zoomNotification.showModeChange(newMode);
  }
}

// ===================================================================
// ZOOM CONTROL METHODS
// ===================================================================

void ZoomController::setZoomMode(ZoomMode mode) {
  if (config.mode == mode) return;
  
  ZoomMode oldMode = config.mode;
  config.mode = mode;
  
  Serial.printf("Zoom Controller: Mode changed from %s to %s\n",
                getZoomModeText(oldMode).c_str(), 
                getZoomModeText().c_str());
  
  // Handle mode-specific setup
  switch (mode) {
    case ZOOM_AUTO:
      // Disable any active manual override
      disableManualOverride();
      // Immediately apply auto-zoom
      updateAutoZoom(currentSpeed);
      break;
      
    case ZOOM_MANUAL:
      // Disable auto-zoom, keep current level
      disableManualOverride();
      break;
      
    case ZOOM_SMART:
      // Enable auto-zoom with manual override capability
      disableManualOverride();
      updateAutoZoom(currentSpeed);
      break;
      
    case ZOOM_LOCKED:
      // Lock at current level
      disableManualOverride();
      break;
  }
  
  // Save settings
  saveSettings();
}

void ZoomController::setZoomLevel(int level) {
  if (!isValidZoomLevel(level) || level == config.currentLevel) return;
  
  int oldLevel = config.currentLevel;
  
  // Trigger manual override if in smart mode
  if (config.mode == ZOOM_SMART) {
    enableManualOverride(level);
  }
  
  // Start smooth transition
  if (config.smoothTransitions) {
    startZoomTransition(level);
  } else {
    config.currentLevel = level;
    // Apply immediately to map display
    mapDisplay.setZoom(level);
  }
  
  lastZoomChange = millis();
  
  Serial.printf("Zoom Controller: Level changed %d -> %d (%s)\n",
                oldLevel, level, 
                isManualOverrideActive() ? "Manual" : "Auto");
  
  // Show zoom change notification
  zoomNotification.showZoomChange(level);
}

void ZoomController::zoomIn() {
  if (config.currentLevel < config.maxLevel) {
    setZoomLevel(config.currentLevel + 1);
  } else {
    Serial.println("Zoom Controller: Already at maximum zoom");
    zoomNotification.showError("Maximum Zoom");
  }
}

void ZoomController::zoomOut() {
  if (config.currentLevel > config.minLevel) {
    setZoomLevel(config.currentLevel - 1);
  } else {
    Serial.println("Zoom Controller: Already at minimum zoom");
    zoomNotification.showError("Minimum Zoom");
  }
}

void ZoomController::resetToAuto() {
  if (config.mode == ZOOM_SMART && isManualOverrideActive()) {
    disableManualOverride();
    updateAutoZoom(currentSpeed);
    zoomNotification.showModeChange(ZOOM_AUTO);
  }
}

// ===================================================================
// AUTO-ZOOM CALCULATION ENGINE
// ===================================================================

int ZoomController::calculateAutoZoom(float speed) {
  // Clamp speed to valid range
  float clampedSpeed = constrain(speed, curve.walkingSpeed, curve.highwaySpeed);
  
  // Determine zoom level based on speed thresholds
  int targetZoom;
  
  if (clampedSpeed <= curve.walkingSpeed) {
    targetZoom = curve.walkingZoom;
  } else if (clampedSpeed <= curve.trailSpeed) {
    // Interpolate between walking and trail
    float ratio = (clampedSpeed - curve.walkingSpeed) / 
                  (curve.trailSpeed - curve.walkingSpeed);
    targetZoom = curve.walkingZoom - (int)(ratio * (curve.walkingZoom - curve.trailZoom));
  } else if (clampedSpeed <= curve.normalSpeed) {
    // Interpolate between trail and normal
    float ratio = (clampedSpeed - curve.trailSpeed) / 
                  (curve.normalSpeed - curve.trailSpeed);
    targetZoom = curve.trailZoom - (int)(ratio * (curve.trailZoom - curve.normalZoom));
  } else if (clampedSpeed <= curve.fastSpeed) {
    // Interpolate between normal and fast
    float ratio = (clampedSpeed - curve.normalSpeed) / 
                  (curve.fastSpeed - curve.normalSpeed);
    targetZoom = curve.normalZoom - (int)(ratio * (curve.normalZoom - curve.fastZoom));
  } else if (clampedSpeed <= curve.highwaySpeed) {
    // Interpolate between fast and highway
    float ratio = (clampedSpeed - curve.fastSpeed) / 
                  (curve.highwaySpeed - curve.fastSpeed);
    targetZoom = curve.fastZoom - (int)(ratio * (curve.fastZoom - curve.highwayZoom));
  } else {
    targetZoom = curve.highwayZoom;
  }
  
  // Apply response factor
  if (config.responseFactor != 1.0) {
    int centerZoom = (config.minLevel + config.maxLevel) / 2;
    int offset = targetZoom - centerZoom;
    offset = (int)(offset * config.responseFactor);
    targetZoom = centerZoom + offset;
  }
  
  // Ensure within valid range
  return constrain(targetZoom, config.minLevel, config.maxLevel);
}

void ZoomController::updateAutoZoom(float speed) {
  int newTargetZoom = calculateAutoZoom(speed);
  
  if (newTargetZoom != targetZoom) {
    targetZoom = newTargetZoom;
    
    // Apply zoom change
    if (config.smoothTransitions) {
      startZoomTransition(targetZoom);
    } else {
      config.currentLevel = targetZoom;
      mapDisplay.setZoom(targetZoom);
    }
    
    Serial.printf("Auto-Zoom: Speed %.1f km/h -> Zoom %d\n", speed, targetZoom);
  }
}

void ZoomController::startZoomTransition(int newZoom) {
  if (!config.smoothTransitions || newZoom == config.currentLevel) {
    // Immediate change
    config.currentLevel = newZoom;
    mapDisplay.setZoom(newZoom);
    return;
  }
  
  // Setup smooth transition
  transitionStartZoom = config.currentLevel;
  transitionTargetZoom = newZoom;
  transitionStartTime = millis();
  zoomTransitionProgress = 0.0;
  
  Serial.printf("Zoom Transition: %d -> %d (%.1fs)\n", 
                transitionStartZoom, transitionTargetZoom, 
                transitionDuration / 1000.0);
}

void ZoomController::updateZoomTransition() {
  if (zoomTransitionProgress >= 1.0) return;
  
  unsigned long elapsed = millis() - transitionStartTime;
  zoomTransitionProgress = (float)elapsed / transitionDuration;
  
  if (zoomTransitionProgress >= 1.0) {
    // Transition complete
    zoomTransitionProgress = 1.0;
    config.currentLevel = transitionTargetZoom;
    mapDisplay.setZoom(transitionTargetZoom);
    Serial.printf("Zoom Transition: Complete at level %d\n", transitionTargetZoom);
  } else {
    // Calculate intermediate zoom level
    int currentZoom = calculateTransitionZoom();
    if (currentZoom != config.currentLevel) {
      config.currentLevel = currentZoom;
      mapDisplay.setZoom(currentZoom);
    }
  }
}

// ===================================================================
// MANUAL OVERRIDE SYSTEM
// ===================================================================

void ZoomController::enableManualOverride(int zoomLevel) {
  override.isActive = true;
  override.startTime = millis();
  override.overrideLevel = zoomLevel;
  override.previousMode = config.mode;
  
  Serial.printf("Manual Override: ACTIVE at zoom %d (timeout in %lu seconds)\n",
                zoomLevel, config.manualOverrideTimeout / 1000);
  
  zoomNotification.showManualOverride(zoomLevel, config.manualOverrideTimeout);
}

void ZoomController::disableManualOverride() {
  if (!override.isActive) return;
  
  override.isActive = false;
  
  Serial.println("Manual Override: DISABLED - Auto-zoom resumed");
}

bool ZoomController::isManualOverrideActive() {
  if (!override.isActive) return false;
  
  // Check timeout
  unsigned long elapsed = millis() - override.startTime;
  if (elapsed >= config.manualOverrideTimeout) {
    disableManualOverride();
    return false;
  }
  
  return true;
}

unsigned long ZoomController::getOverrideTimeRemaining() {
  if (!override.isActive) return 0;
  
  unsigned long elapsed = millis() - override.startTime;
  if (elapsed >= config.manualOverrideTimeout) {
    return 0;
  }
  
  return config.manualOverrideTimeout - elapsed;
}

void ZoomController::checkOverrideTimeout() {
  if (!override.isActive) return;
  
  unsigned long timeRemaining = getOverrideTimeRemaining();
  
  if (timeRemaining == 0) {
    // Timeout occurred
    disableManualOverride();
    updateAutoZoom(currentSpeed);
    zoomNotification.showOverrideTimeout();
  } else if (timeRemaining <= 3000 && override.showTimeoutWarning) {
    // Show 3-second warning
    zoomNotification.showOverrideTimeout();
    override.showTimeoutWarning = false;
  }
}

// ===================================================================
// CONFIGURATION METHODS
// ===================================================================

void ZoomController::setResponseFactor(float factor) {
  config.responseFactor = constrain(factor, 0.1, 2.0);
  Serial.printf("Zoom Controller: Response factor set to %.1f\n", config.responseFactor);
  saveSettings();
}

void ZoomController::setSpeedRange(float minSpeed, float maxSpeed) {
  config.speedMin = max(0.0f, minSpeed);
  config.speedMax = max(config.speedMin + 1.0f, maxSpeed);
  Serial.printf("Zoom Controller: Speed range set to %.1f - %.1f km/h\n", 
                config.speedMin, config.speedMax);
  saveSettings();
}

void ZoomController::setZoomRange(int minZoom, int maxZoom) {
  config.minLevel = constrain(minZoom, ZOOM_MIN_LEVEL, ZOOM_MAX_LEVEL);
  config.maxLevel = constrain(maxZoom, config.minLevel, ZOOM_MAX_LEVEL);
  Serial.printf("Zoom Controller: Zoom range set to %d - %d\n", 
                config.minLevel, config.maxLevel);
  saveSettings();
}

void ZoomController::setSmoothTransitions(bool enable) {
  config.smoothTransitions = enable;
  Serial.printf("Zoom Controller: Smooth transitions %s\n", 
                enable ? "ENABLED" : "DISABLED");
  saveSettings();
}

void ZoomController::setManualTimeout(unsigned long ms) {
  config.manualOverrideTimeout = constrain(ms, 1000UL, 60000UL);
  Serial.printf("Zoom Controller: Manual timeout set to %lu seconds\n", 
                config.manualOverrideTimeout / 1000);
  saveSettings();
}

// ===================================================================
// SETTINGS PERSISTENCE
// ===================================================================

bool ZoomController::loadSettings() {
  Serial.println("Zoom Controller: Loading saved configuration...");
  
  config.mode = (ZoomMode)nvs.getUChar("mode", ZOOM_SMART);
  config.currentLevel = nvs.getUChar("current", 14);
  config.minLevel = nvs.getUChar("min", ZOOM_MIN_LEVEL);
  config.maxLevel = nvs.getUChar("max", ZOOM_MAX_LEVEL);
  config.speedMin = nvs.getFloat("speed_min", 0.0);
  config.speedMax = nvs.getFloat("speed_max", 100.0);
  config.responseFactor = nvs.getFloat("response", 1.0);
  config.smoothTransitions = nvs.getBool("smooth", true);
  config.manualOverrideTimeout = nvs.getULong("timeout", 10000);
  
  // Load zoom curve
  curve.walkingSpeed = nvs.getFloat("c_walk_spd", 5.0);
  curve.trailSpeed = nvs.getFloat("c_trail_spd", 15.0);
  curve.normalSpeed = nvs.getFloat("c_norm_spd", 30.0);
  curve.fastSpeed = nvs.getFloat("c_fast_spd", 60.0);
  curve.highwaySpeed = nvs.getFloat("c_hwy_spd", 80.0);
  
  curve.walkingZoom = nvs.getUChar("c_walk_zm", 18);
  curve.trailZoom = nvs.getUChar("c_trail_zm", 16);
  curve.normalZoom = nvs.getUChar("c_norm_zm", 14);
  curve.fastZoom = nvs.getUChar("c_fast_zm", 12);
  curve.highwayZoom = nvs.getUChar("c_hwy_zm", 10);
  
  // Validate loaded settings
  if (!validateSettings(config)) {
    Serial.println("Zoom Controller: Invalid settings detected, using defaults");
    return false;
  }
  
  Serial.println("Zoom Controller: Configuration loaded successfully");
  return true;
}

bool ZoomController::saveSettings() {
  if (!config.saveSettings) return true;
  
  Serial.println("Zoom Controller: Saving configuration...");
  
  nvs.putUChar("mode", config.mode);
  nvs.putUChar("current", config.currentLevel);
  nvs.putUChar("min", config.minLevel);
  nvs.putUChar("max", config.maxLevel);
  nvs.putFloat("speed_min", config.speedMin);
  nvs.putFloat("speed_max", config.speedMax);
  nvs.putFloat("response", config.responseFactor);
  nvs.putBool("smooth", config.smoothTransitions);
  nvs.putULong("timeout", config.manualOverrideTimeout);
  
  // Save zoom curve
  nvs.putFloat("c_walk_spd", curve.walkingSpeed);
  nvs.putFloat("c_trail_spd", curve.trailSpeed);
  nvs.putFloat("c_norm_spd", curve.normalSpeed);
  nvs.putFloat("c_fast_spd", curve.fastSpeed);
  nvs.putFloat("c_hwy_spd", curve.highwaySpeed);
  
  nvs.putUChar("c_walk_zm", curve.walkingZoom);
  nvs.putUChar("c_trail_zm", curve.trailZoom);
  nvs.putUChar("c_norm_zm", curve.normalZoom);
  nvs.putUChar("c_fast_zm", curve.fastZoom);
  nvs.putUChar("c_hwy_zm", curve.highwayZoom);
  
  return true;
}

// ===================================================================
// UTILITY AND HELPER METHODS
// ===================================================================

bool ZoomController::validateSettings(const ZoomConfig& cfg) {
  return (cfg.mode >= ZOOM_MANUAL && cfg.mode <= ZOOM_LOCKED) &&
         (cfg.currentLevel >= ZOOM_MIN_LEVEL && cfg.currentLevel <= ZOOM_MAX_LEVEL) &&
         (cfg.minLevel >= ZOOM_MIN_LEVEL && cfg.minLevel <= cfg.maxLevel) &&
         (cfg.maxLevel >= cfg.minLevel && cfg.maxLevel <= ZOOM_MAX_LEVEL) &&
         (cfg.speedMin >= 0.0 && cfg.speedMin < cfg.speedMax) &&
         (cfg.responseFactor >= 0.1 && cfg.responseFactor <= 2.0) &&
         (cfg.manualOverrideTimeout >= 1000 && cfg.manualOverrideTimeout <= 60000);
}

void ZoomController::setDefaultSettings() {
  config.mode = ZOOM_SMART;
  config.currentLevel = 14;
  config.minLevel = ZOOM_MIN_LEVEL;
  config.maxLevel = ZOOM_MAX_LEVEL;
  config.speedMin = 0.0;
  config.speedMax = 100.0;
  config.responseFactor = 1.0;
  config.smoothTransitions = true;
  config.manualOverrideTimeout = 10000;
  config.saveSettings = true;
  
  // Set default zoom curve optimized for enduro riding
  curve.walkingSpeed = 5.0;
  curve.trailSpeed = 15.0;
  curve.normalSpeed = 30.0;
  curve.fastSpeed = 60.0;
  curve.highwaySpeed = 80.0;
  
  curve.walkingZoom = 18;
  curve.trailZoom = 16;
  curve.normalZoom = 14;
  curve.fastZoom = 12;
  curve.highwayZoom = 10;
}

String ZoomController::getZoomModeText() {
  return getZoomModeText(config.mode);
}

String ZoomController::getZoomModeText(ZoomMode mode) {
  switch (mode) {
    case ZOOM_MANUAL: return "Manual";
    case ZOOM_AUTO: return "Auto";
    case ZOOM_SMART: return "Smart";
    case ZOOM_LOCKED: return "Locked";
    default: return "Unknown";
  }
}

String ZoomController::getStatusText() {
  String status = getZoomModeText() + " | Zoom " + String(config.currentLevel);
  
  if (isManualOverrideActive()) {
    unsigned long remaining = getOverrideTimeRemaining() / 1000;
    status += " | Override " + String(remaining) + "s";
  }
  
  if (isTransitioning()) {
    status += " | Transitioning";
  }
  
  return status;
}

bool ZoomController::isValidZoomLevel(int level) {
  return (level >= config.minLevel && level <= config.maxLevel);
}

float ZoomController::easeInOutCubic(float t) {
  return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

int ZoomController::calculateTransitionZoom() {
  float easedProgress = easeInOutCubic(zoomTransitionProgress);
  return transitionStartZoom + 
         (int)(easedProgress * (transitionTargetZoom - transitionStartZoom));
}

// ===================================================================
// GETTERS
// ===================================================================

ZoomMode ZoomController::getZoomMode() { return config.mode; }
int ZoomController::getCurrentZoom() { return config.currentLevel; }
int ZoomController::getTargetZoom() { return targetZoom; }
float ZoomController::getResponseFactor() { return config.responseFactor; }
bool ZoomController::isTransitioning() { return zoomTransitionProgress < 1.0; }
unsigned long ZoomController::getManualTimeout() { return config.manualOverrideTimeout; }
bool ZoomController::getSmoothTransitions() { return config.smoothTransitions; }

// ===================================================================
// ZOOM CURVE CUSTOMIZATION
// ===================================================================

void ZoomController::setZoomCurve(const ZoomCurve& newCurve) {
  curve = newCurve;
  Serial.println("Zoom Controller: Custom curve applied");
  saveSettings();
}

ZoomCurve ZoomController::getZoomCurve() {
  return curve;
}

void ZoomController::resetCurveToDefaults() {
  ZoomCurve defaultCurve;
  defaultCurve.walkingSpeed = 5.0;
  defaultCurve.trailSpeed = 15.0;
  defaultCurve.normalSpeed = 30.0;
  defaultCurve.fastSpeed = 60.0;
  defaultCurve.highwaySpeed = 80.0;
  
  defaultCurve.walkingZoom = 18;
  defaultCurve.trailZoom = 16;
  defaultCurve.normalZoom = 14;
  defaultCurve.fastZoom = 12;
  defaultCurve.highwayZoom = 10;
  
  setZoomCurve(defaultCurve);
}

// ===================================================================
// GLOBAL ACCESS FUNCTIONS FOR CORE 0
// ===================================================================

bool initializeZoomControl() {
  return zoomController.initialize(&buttons);
}

void updateZoomControl(float currentSpeed) {
  zoomController.update(currentSpeed);
  zoomController.handleButtonInput();
}

void handleZoomButtons() {
  zoomController.handleButtonInput();
}

bool isAutoZoomActive() {
  ZoomMode mode = zoomController.getZoomMode();
  return (mode == ZOOM_AUTO || 
          (mode == ZOOM_SMART && !zoomController.isManualOverrideActive()));
}

int getCurrentZoomLevel() {
  return zoomController.getCurrentZoom();
}

void enterZoomMenu() {
  // Integration point for menu system
  Serial.println("Zoom Menu: Opening zoom configuration menu");
  // TODO: Integrate with main menu system
}

void showZoomStatus() {
  Serial.println("=== ZOOM CONTROLLER STATUS ===");
  Serial.printf("Mode: %s\n", zoomController.getZoomModeText().c_str());
  Serial.printf("Current Zoom: %d\n", zoomController.getCurrentZoom());
  Serial.printf("Target Zoom: %d\n", zoomController.getTargetZoom());
  Serial.printf("Manual Override: %s\n", 
                zoomController.isManualOverrideActive() ? "ACTIVE" : "INACTIVE");
  if (zoomController.isManualOverrideActive()) {
    Serial.printf("Override Time Remaining: %lu seconds\n", 
                  zoomController.getOverrideTimeRemaining() / 1000);
  }
  Serial.printf("Transitioning: %s\n", 
                zoomController.isTransitioning() ? "YES" : "NO");
  Serial.printf("Response Factor: %.1f\n", zoomController.getResponseFactor());
  Serial.println("================================");
}

// ===================================================================
// ZOOM MENU SYSTEM IMPLEMENTATION
// ===================================================================

bool ZoomMenuSystem::initialize(ZoomController* zoomCtrl) {
  zoomController = zoomCtrl;
  currentItem = ZOOM_MENU_MODE;
  inSubMenu = false;
  needsRedraw = true;
  
  Serial.println("Zoom Menu: Menu system initialized");
  return true;
}

void ZoomMenuSystem::handleButtonPress(int button) {
  switch (button) {
    case BTN_UP:
      navigateUp();
      break;
    case BTN_DOWN:
      navigateDown();
      break;
    case BTN_CENTER:
      selectCurrent();
      break;
    case BTN_LEFT:
      if (inSubMenu) {
        adjustValue(false); // Decrease
      } else {
        exitMenu();
      }
      break;
    case BTN_RIGHT:
      if (inSubMenu) {
        adjustValue(true); // Increase
      }
      break;
  }
  needsRedraw = true;
}

void ZoomMenuSystem::navigateUp() {
  if (currentItem > ZOOM_MENU_MODE) {
    currentItem = (ZoomMenuItem)(currentItem - 1);
  }
}

void ZoomMenuSystem::navigateDown() {
  if (currentItem < ZOOM_MENU_BACK) {
    currentItem = (ZoomMenuItem)(currentItem + 1);
  }
}

void ZoomMenuSystem::selectCurrent() {
  switch (currentItem) {
    case ZOOM_MENU_MODE:
      handleModeSelection();
      break;
    case ZOOM_MENU_LEVEL:
      inSubMenu = !inSubMenu;
      break;
    case ZOOM_MENU_RESPONSE:
      inSubMenu = !inSubMenu;
      break;
    case ZOOM_MENU_TIMEOUT:
      inSubMenu = !inSubMenu;
      break;
    case ZOOM_MENU_SMOOTH:
      handleSmoothToggle();
      break;
    case ZOOM_MENU_CURVE_SETUP:
      handleCurveSetup();
      break;
    case ZOOM_MENU_RESET:
      handleReset();
      break;
    case ZOOM_MENU_BACK:
      exitMenu();
      break;
  }
}

void ZoomMenuSystem::handleModeSelection() {
  ZoomMode currentMode = zoomController->getZoomMode();
  ZoomMode newMode = (ZoomMode)((currentMode + 1) % 4);
  zoomController->setZoomMode(newMode);
  zoomNotification.showModeChange(newMode);
}

void ZoomMenuSystem::handleSmoothToggle() {
  bool current = zoomController->getSmoothTransitions();
  zoomController->setSmoothTransitions(!current);
  zoomNotification.showModeChange(zoomController->getZoomMode());
}

void ZoomMenuSystem::adjustValue(bool increase) {
  switch (currentItem) {
    case ZOOM_MENU_LEVEL:
      if (increase) {
        zoomController->zoomIn();
      } else {
        zoomController->zoomOut();
      }
      break;
    case ZOOM_MENU_RESPONSE:
      {
        float current = zoomController->getResponseFactor();
        float step = 0.1;
        float newValue = increase ? current + step : current - step;
        zoomController->setResponseFactor(newValue);
      }
      break;
    case ZOOM_MENU_TIMEOUT:
      {
        unsigned long current = zoomController->getManualTimeout();
        unsigned long step = 1000; // 1 second steps
        unsigned long newValue = increase ? current + step : current - step;
        zoomController->setManualTimeout(newValue);
      }
      break;
  }
}

void ZoomMenuSystem::handleCurveSetup() {
  Serial.println("Zoom Menu: Curve setup not yet implemented");
  // TODO: Implement curve setup interface
}

void ZoomMenuSystem::handleReset() {
  zoomController->resetCurveToDefaults();
  zoomController->setDefaultSettings();
  zoomController->saveSettings();
  zoomNotification.showModeChange(zoomController->getZoomMode());
}

void ZoomMenuSystem::exitMenu() {
  inSubMenu = false;
  // TODO: Exit to main menu system
}

void ZoomMenuSystem::draw() {
  if (!needsRedraw) return;
  
  // TODO: Implement menu drawing using display system
  // This would integrate with the main UI renderer
  needsRedraw = false;
}

String ZoomMenuSystem::getMenuItemText(ZoomMenuItem item) {
  switch (item) {
    case ZOOM_MENU_MODE: return "Zoom Mode";
    case ZOOM_MENU_LEVEL: return "Zoom Level";
    case ZOOM_MENU_RESPONSE: return "Response";
    case ZOOM_MENU_TIMEOUT: return "Override Timeout";
    case ZOOM_MENU_SMOOTH: return "Smooth Transitions";
    case ZOOM_MENU_CURVE_SETUP: return "Curve Setup";
    case ZOOM_MENU_RESET: return "Reset to Defaults";
    case ZOOM_MENU_BACK: return "Back";
    default: return "Unknown";
  }
}

// ===================================================================
// ZOOM NOTIFICATION SYSTEM IMPLEMENTATION
// ===================================================================

void ZoomNotification::showModeChange(ZoomMode mode) {
  String modeText;
  switch (mode) {
    case ZOOM_MANUAL: modeText = "Manual"; break;
    case ZOOM_AUTO: modeText = "Auto"; break;
    case ZOOM_SMART: modeText = "Smart"; break;
    case ZOOM_LOCKED: modeText = "Locked"; break;
    default: modeText = "Unknown"; break;
  }
  notificationText = "Mode: " + modeText;
  showingNotification = true;
  showStartTime = millis();
  showDuration = 2000;
  pulsing = false;
  
  Serial.printf("Zoom Notification: Mode change - %s\n", notificationText.c_str());
}

void ZoomNotification::showManualOverride(int level, unsigned long timeoutMs) {
  notificationText = "Manual Zoom " + String(level) + " (" + String(timeoutMs/1000) + "s)";
  showingNotification = true;
  showStartTime = millis();
  showDuration = 3000;
  pulsing = true;
  
  Serial.printf("Zoom Notification: Manual override - %s\n", notificationText.c_str());
}

void ZoomNotification::showOverrideTimeout() {
  notificationText = "Auto-zoom resumed";
  showingNotification = true;
  showStartTime = millis();
  showDuration = 2000;
  pulsing = false;
  
  Serial.println("Zoom Notification: Override timeout");
}

void ZoomNotification::showZoomChange(int level) {
  notificationText = "Zoom " + String(level);
  showingNotification = true;
  showStartTime = millis();
  showDuration = 1500;
  pulsing = false;
  
  Serial.printf("Zoom Notification: Zoom change - Level %d\n", level);
}

void ZoomNotification::showError(const String& error) {
  notificationText = error;
  showingNotification = true;
  showStartTime = millis();
  showDuration = 2000;
  pulsing = true;
  
  Serial.printf("Zoom Notification: Error - %s\n", error.c_str());
}

void ZoomNotification::update() {
  if (!showingNotification) return;
  
  unsigned long elapsed = millis() - showStartTime;
  if (elapsed >= showDuration) {
    hideNotification();
    return;
  }
  
  // Update animation phase for pulsing
  if (pulsing) {
    animationPhase = (millis() / 250) % 4; // 4-phase pulse cycle
  }
}

void ZoomNotification::draw() {
  if (!showingNotification) return;
  
  // This would integrate with the display system
  // For now, we handle notifications via Serial output
  // In a full implementation, this would draw on-screen notifications
}

void ZoomNotification::hideNotification() {
  showingNotification = false;
  notificationText = "";
  animationPhase = 0;
  pulsing = false;
}

bool ZoomNotification::isShowing() {
  return showingNotification;
}
