#include "button_manager.h"
#include <Arduino.h>

// ===================================================================
// BUTTON MANAGER IMPLEMENTATION - PROFESSIONAL GRADE INPUT SYSTEM
// Zero-latency button response for serious navigation
// ===================================================================

ButtonManager buttonManager; // Global instance

bool ButtonManager::initialize() {
  Serial.println("Button Manager: Initializing CH423S GPIO Expander...");
  
  // Initialize I2C communication with CH423S
  if (!ch423.begin()) {
    Serial.println("ERROR: CH423S initialization failed!");
    return false;
  }
  
  // Test communication
  if (!testCH423Connection()) {
    Serial.println("ERROR: CH423S communication test failed!");
    return false;
  }
  
  // Configure all 8 buttons as inputs with pullups
  for (int i = 0; i < BUTTON_COUNT; i++) {
    ch423.pinMode(i, INPUT_PULLUP);
    
    // Initialize button states
    buttons[i].currentState = true;  // Pullup = true when not pressed
    buttons[i].lastState = true;
    buttons[i].isPressed = false;
    buttons[i].wasPressed = false;
    buttons[i].pressTime = 0;
    buttons[i].releaseTime = 0;
    buttons[i].pressCount = 0;
    buttons[i].lastPressTime = 0;
  }
  
  Serial.println("Button Manager: All 8 buttons configured as INPUT_PULLUP");
  
  // Initialize zoom system
  zoomConfig.currentZoom = 14; // Good starting zoom for enduro
  zoomConfig.autoZoomEnabled = true;
  zoomConfig.autoZoomFactor = 1.0;
  zoomConfig.lastManualZoom = 0;
  
  Serial.printf("Button Manager: Zoom system initialized (Level %d, Auto: %s)\n", 
                zoomConfig.currentZoom, zoomConfig.autoZoomEnabled ? "ON" : "OFF");
  
  initialized = true;
  lastScanTime = millis();
  
  Serial.println("Button Manager: Professional button system ready!");
  return true;
}

bool ButtonManager::testCH423Connection() {
  Serial.println("Button Manager: Testing CH423S communication...");
  
  // Try to read all GPIO states
  bool testPassed = true;
  
  for (int i = 0; i < BUTTON_COUNT; i++) {
    int state = ch423.digitalRead(i);
    if (state < 0) { // Error reading
      Serial.printf("ERROR: Failed to read GPIO %d\n", i);
      testPassed = false;
    } else {
      Serial.printf("GPIO %d state: %s\n", i, state ? "HIGH" : "LOW");
    }
  }
  
  if (testPassed) {
    Serial.println("Button Manager: CH423S communication test PASSED");
  }
  
  return testPassed;
}

ButtonManager::ButtonEventData ButtonManager::getNextEvent() {
  ButtonID button;
  ButtonEvent event = getNextButtonEvent(button);
  return ButtonEventData(button, event, millis());
}

// ===================================================================
// CORE BUTTON SCANNING - CALLED EVERY 5MS FROM CORE 0
// ===================================================================

void ButtonManager::scanButtons() {
  if (!initialized) return;
  
  unsigned long now = millis();
  if (now - lastScanTime < scanInterval) return; // Maintain 5ms scan rate
  
  // Scan all 8 buttons
  for (int i = 0; i < BUTTON_COUNT; i++) {
    updateButtonState((ButtonID)i);
  }
  
  // Handle zoom buttons specifically
  handleZoomButtons();
  
  // Handle center button multi-function
  handleCenterButton();
  
  lastScanTime = now;
}

void ButtonManager::updateButtonState(ButtonID button) {
  int buttonIndex = (int)button;
  ButtonState& btn = buttons[buttonIndex];
  
  // Read current hardware state
  int hwState = ch423.digitalRead(buttonIndex);
  bool currentPressed = (hwState == 0); // Active low with pullup
  
  unsigned long now = millis();
  
  // Debouncing logic
  if (currentPressed != !btn.currentState) {
    // State change detected
    if (now - btn.releaseTime > debounceTime || 
        now - btn.pressTime > debounceTime) {
      
      btn.lastState = btn.currentState;
      btn.currentState = !currentPressed; // Invert for pullup logic
      
      if (currentPressed && !btn.isPressed) {
        // Button press detected
        btn.isPressed = true;
        btn.wasPressed = true;
        btn.pressTime = now;
        btn.pressCount++;
        btn.lastPressTime = now;
        
        Serial.printf("Button %d PRESSED\n", buttonIndex);
        
      } else if (!currentPressed && btn.isPressed) {
        // Button release detected
        btn.isPressed = false;
        btn.releaseTime = now;
        
        unsigned long pressDuration = now - btn.pressTime;
        Serial.printf("Button %d RELEASED (held %lums)\n", buttonIndex, pressDuration);
      }
    }
  }
  
  // Reset double-click counter after timeout
  if (now - btn.lastPressTime > doubleClickTime) {
    btn.pressCount = 0;
  }
}

bool ButtonManager::hasButtonEvent() {
  if (!initialized) return false;
  
  for (int i = 0; i < BUTTON_COUNT; i++) {
    if (buttons[i].wasPressed) {
      return true;
    }
  }
  return false;
}

ButtonEvent ButtonManager::getButtonEvent(ButtonID& buttonID) {
  if (!initialized) return BTN_EVENT_NONE;
  
  unsigned long now = millis();
  
  for (int i = 0; i < BUTTON_COUNT; i++) {
    ButtonState& btn = buttons[i];
    
    // Check for press event
    if (btn.wasPressed) {
      btn.wasPressed = false; // Clear flag
      buttonID = (ButtonID)i;
      return BTN_EVENT_PRESS;
    }
    
    // Check for hold event
    if (btn.isPressed && (now - btn.pressTime) > holdTime) {
      buttonID = (ButtonID)i;
      return BTN_EVENT_HOLD;
    }
    
    // Check for double-click
    if (btn.pressCount >= 2 && (now - btn.lastPressTime) < doubleClickTime) {
      btn.pressCount = 0; // Reset counter
      buttonID = (ButtonID)i;
      return BTN_EVENT_DOUBLE_CLICK;
    }
  }
  
  return BTN_EVENT_NONE;
}

// ===================================================================
// ZOOM CONTROL SYSTEM - PROFESSIONAL FEATURES
// ===================================================================

void ButtonManager::handleZoomButtons() {
  if (!initialized) return;
  
  // Check zoom in button
  if (buttons[BUTTON_ZOOM_IN].wasPressed) {
    zoomIn();
    buttons[BUTTON_ZOOM_IN].wasPressed = false;
  }
  
  // Check zoom out button
  if (buttons[BUTTON_ZOOM_OUT].wasPressed) {
    zoomOut();
    buttons[BUTTON_ZOOM_OUT].wasPressed = false;
  }
}

void ButtonManager::zoomIn() {
  if (zoomConfig.currentZoom < zoomConfig.maxZoom) {
    zoomConfig.currentZoom++;
    zoomConfig.lastManualZoom = millis(); // Disable auto-zoom temporarily
    
    Serial.printf("Zoom Manager: Zoomed IN to level %d (Manual override)\n", 
                  zoomConfig.currentZoom);
  } else {
    Serial.println("Zoom Manager: Already at maximum zoom");
  }
}

void ButtonManager::zoomOut() {
  if (zoomConfig.currentZoom > zoomConfig.minZoom) {
    zoomConfig.currentZoom--;
    zoomConfig.lastManualZoom = millis(); // Disable auto-zoom temporarily
    
    Serial.printf("Zoom Manager: Zoomed OUT to level %d (Manual override)\n", 
                  zoomConfig.currentZoom);
  } else {
    Serial.println("Zoom Manager: Already at minimum zoom");
  }
}

void ButtonManager::setZoom(int level) {
  int newZoom = constrain(level, zoomConfig.minZoom, zoomConfig.maxZoom);
  if (newZoom != zoomConfig.currentZoom) {
    zoomConfig.currentZoom = newZoom;
    Serial.printf("Zoom Manager: Set to level %d\n", newZoom);
  }
}

int ButtonManager::getCurrentZoom() {
  return zoomConfig.currentZoom;
}

void ButtonManager::updateAutoZoom(float speed) {
  if (!zoomConfig.autoZoomEnabled || isManualZoomActive()) {
    return; // Auto-zoom disabled or manual override active
  }
  
  int targetZoom = calculateAutoZoom(speed);
  if (targetZoom != zoomConfig.currentZoom) {
    zoomConfig.currentZoom = targetZoom;
    Serial.printf("Auto-Zoom: Speed %.1f km/h -> Zoom level %d\n", speed, targetZoom);
  }
}

int ButtonManager::calculateAutoZoom(float speed) {
  // Clamp speed to configured range
  float clampedSpeed = constrain(speed, ZOOM_MIN_SPEED, ZOOM_MAX_SPEED);
  
  // Linear interpolation with user factor
  float normalizedSpeed = clampedSpeed / ZOOM_MAX_SPEED;
  float targetZoom = zoomConfig.maxZoom - 
    (normalizedSpeed * (zoomConfig.maxZoom - zoomConfig.minZoom)) * 
    zoomConfig.autoZoomFactor;
    
  return constrain((int)targetZoom, zoomConfig.minZoom, zoomConfig.maxZoom);
}

bool ButtonManager::isManualZoomActive() {
  return (millis() - zoomConfig.lastManualZoom) < zoomConfig.manualZoomTimeout;
}

void ButtonManager::enableAutoZoom(bool enable) {
  zoomConfig.autoZoomEnabled = enable;
  Serial.printf("Auto-Zoom: %s\n", enable ? "ENABLED" : "DISABLED");
}

void ButtonManager::setAutoZoomFactor(float factor) {
  zoomConfig.autoZoomFactor = constrain(factor, 0.1, 2.0);
  Serial.printf("Auto-Zoom Factor: %.1f\n", zoomConfig.autoZoomFactor);
}

bool ButtonManager::isAutoZoomActive() {
  return zoomConfig.autoZoomEnabled && !isManualZoomActive();
}

// ===================================================================
// CENTER BUTTON MULTI-FUNCTION SYSTEM
// ===================================================================

void ButtonManager::handleCenterButton() {
  if (!initialized) return;
  
  ButtonState& centerBtn = buttons[BUTTON_5WAY_CENTER];
  unsigned long now = millis();
  
  if (centerBtn.wasPressed) {
    unsigned long pressDuration = now - centerBtn.pressTime;
    
    if (pressDuration < 100) {
      // Quick press = SELECT/MENU
      Serial.println("Center Button: MENU/SELECT function");
    } else if (pressDuration > holdTime) {
      // Long hold = WAKE function
      Serial.println("Center Button: WAKE function");
    }
    
    centerBtn.wasPressed = false;
  }
}

bool ButtonManager::isCenterButtonWake() {
  return buttons[BUTTON_5WAY_CENTER].isPressed && 
         (millis() - buttons[BUTTON_5WAY_CENTER].pressTime) > holdTime;
}

bool ButtonManager::isCenterButtonMenu() {
  return buttons[BUTTON_5WAY_CENTER].wasPressed;
}

bool ButtonManager::isCenterButtonSelect() {
  return buttons[BUTTON_5WAY_CENTER].wasPressed;
}

// ===================================================================
// BUTTON STATE QUERIES
// ===================================================================

bool ButtonManager::isButtonPressed(ButtonID button) {
  if (!initialized || button >= BUTTON_COUNT) return false;
  return buttons[button].isPressed;
}

bool ButtonManager::wasButtonPressed(ButtonID button) {
  if (!initialized || button >= BUTTON_COUNT) return false;
  return buttons[button].wasPressed;
}

bool ButtonManager::isButtonHeld(ButtonID button) {
  if (!initialized || button >= BUTTON_COUNT) return false;
  return buttons[button].isPressed && 
         (millis() - buttons[button].pressTime) > holdTime;
}

unsigned long ButtonManager::getButtonHoldTime(ButtonID button) {
  if (!initialized || button >= BUTTON_COUNT || !buttons[button].isPressed) {
    return 0;
  }
  return millis() - buttons[button].pressTime;
}

void ButtonManager::getButtonStatus(bool states[BUTTON_COUNT]) {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    states[i] = buttons[i].isPressed;
  }
}

// ===================================================================
// CONFIGURATION
// ===================================================================

void ButtonManager::setDebounceTime(unsigned long ms) {
  // Note: debounceTime is const, but this could be made configurable
  Serial.printf("Debounce time: %lu ms (fixed)\n", debounceTime);
}

void ButtonManager::setHoldTime(unsigned long ms) {
  // Note: holdTime is const, but this could be made configurable  
  Serial.printf("Hold time: %lu ms (fixed)\n", holdTime);
}

// ===================================================================
// GLOBAL ACCESS FUNCTIONS
// ===================================================================

bool initializeButtons() {
  return buttonManager.initialize();
}

void scanButtonInputs() {
  buttonManager.scanButtons();
}

bool getButtonPress(ButtonID button) {
  return buttonManager.wasButtonPressed(button);
}

void handleZoomControls() {
  buttonManager.handleZoomButtons();
}

int getCurrentZoomLevel() {
  return buttonManager.getCurrentZoom();
}

void setZoomLevel(int level) {
  buttonManager.setZoom(level);
} 
