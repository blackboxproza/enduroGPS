/*
 * E_NAVI EnduroGPS - Professional Button Manager
 * Zero-Latency Input System for Motorcycle Navigation
 * 
 * Features: 8-Button Support via CH423S GPIO Expander
 * Events: Press, Hold, Double-Click, Release
 * Zoom: Manual + Auto-Speed Based Control
 * 
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "config.h"  // For timing constants if needed
#include "shared_data.h"  // For GPSData access in auto-zoom

// ========================================
// BUTTON EVENT DATA
// ========================================
struct ButtonEventData {
  ButtonID button;
  ButtonEvent event;
  unsigned long timestamp;
  
  ButtonEventData() : button(BTN_CENTER), event(BTN_EVENT_NONE), timestamp(0) {}
  ButtonEventData(ButtonID b, ButtonEvent e, unsigned long t) : button(b), event(e), timestamp(t) {}
};

// ========================================
// BUTTON MANAGER CLASS
// ========================================
class ButtonManager {
  private:
    // Button States
    struct ButtonState {
      bool currentState = true;  // HIGH = not pressed (pullup)
      bool lastState = true;
      bool isPressed = false;
      bool wasPressed = false;
      unsigned long pressTime = 0;
      unsigned long releaseTime = 0;
      unsigned long lastPressTime = 0;
      int pressCount = 0;
    };
    
    ButtonState buttons[BTN_COUNT];
    
    // Timing Constants
    const unsigned long debounceTime = BUTTON_DEBOUNCE_MS;
    const unsigned long holdTime = BUTTON_HOLD_MS;
    const unsigned long doubleClickTime = BUTTON_DOUBLE_CLICK_MS;
    
    // Scan Control
    unsigned long lastScanTime = 0;
    unsigned long scanInterval = BUTTON_SCAN_INTERVAL_MS;
    
    // Zoom Configuration
    struct ZoomConfig {
      int currentZoom = ZOOM_DEFAULT;
      int minZoom = ZOOM_MIN_LEVEL;
      int maxZoom = ZOOM_MAX_LEVEL;
      bool autoZoomEnabled = true;
      float autoZoomFactor = 1.0;
      unsigned long manualZoomTimeout = ZOOM_MANUAL_TIMEOUT_MS;
      unsigned long lastManualZoom = 0;
    } zoomConfig;
    
    bool initialized = false;
    
    // Internal Helpers
    void updateButtonState(ButtonID button);
    void handleZoomButtons();
    void handleCenterButton();
    int calculateAutoZoom(float speed);
    bool isManualZoomActive();
    
  public:
    // Initialization
    bool initialize();
    bool testCH423Connection();
    
    // Update Functions
    void scanButtons();
    bool hasButtonEvent();
    ButtonEvent getButtonEvent(ButtonID& buttonID);
    ButtonEventData getNextEvent();
    
    // Button State Queries
    bool isButtonPressed(ButtonID button);
    bool wasButtonPressed(ButtonID button);
    bool isButtonHeld(ButtonID button);
    unsigned long getButtonHoldTime(ButtonID button);
    void getButtonStatus(bool states[BUTTON_COUNT]);
    
    // Directional Helpers
    bool isUpPressed() { return isButtonPressed(BTN_UP); }
    bool isDownPressed() { return isButtonPressed(BTN_DOWN); }
    bool isLeftPressed() { return isButtonPressed(BTN_LEFT); }
    bool isRightPressed() { return isButtonPressed(BTN_RIGHT); }
    
    // Center Button Helpers
    bool isCenterPressed() { return isButtonPressed(BTN_CENTER); }
    bool isCenterHeld() { return isButtonHeld(BTN_CENTER); }
    bool isCenterButtonWake();
    bool isCenterButtonMenu();
    bool isCenterButtonSelect();
    
    // Zoom Button Helpers
    bool isZoomInPressed() { return isButtonPressed(BTN_ZOOM_IN); }
    bool isZoomOutPressed() { return isButtonPressed(BTN_ZOOM_OUT); }
    
    // Waypoint Helper
    bool isWaypointPressed() { return isButtonPressed(BTN_WAYPOINT); }
    
    // Zoom Control
    void zoomIn();
    void zoomOut();
    void setZoom(int level);
    int getCurrentZoom();
    void updateAutoZoom(float speed);
    void enableAutoZoom(bool enable);
    void setAutoZoomFactor(float factor);
    bool isAutoZoomActive();
    
    // Configuration
    void setDebounceTime(unsigned long ms);
    void setHoldTime(unsigned long ms);
    
    // Status
    bool isInitialized() const { return initialized; }
};

// ========================================
// GLOBAL INSTANCE
// ========================================
extern ButtonManager buttonManager;

// ========================================
// GLOBAL ACCESS FUNCTIONS
// ========================================
bool initializeButtons();
void scanButtonInputs();
bool getButtonPress(ButtonID button);
void handleZoomControls();
int getCurrentZoomLevel();
void setZoomLevel(int level);

#endif // BUTTON_MANAGER_H