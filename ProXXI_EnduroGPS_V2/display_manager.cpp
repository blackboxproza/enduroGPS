#include "display_manager.h"
#include <time.h>

// ===================================================================
// E_NAVI PROFESSIONAL DISPLAY IMPLEMENTATION  
// High-performance rendering for serious navigation
// ===================================================================

TFT_eSPI tft = TFT_eSPI(); // Create display instance
DisplayManager display;     // Global display manager

// ===================================================================
// INITIALIZATION - FAST BOOT PRIORITY
// ===================================================================

bool DisplayManager::initialize() {
  // Initialize TFT with professional settings
  tft.init();
  tft.setRotation(SCREEN_ROTATION); // Portrait mode
  tft.fillScreen(COLOR_BACKGROUND);
  
  // Configure SPI for maximum speed
  //tft.setSPISpeed(80000000); // 80MHz for smooth updates
  
  // Set initial brightness
  setBacklight(nightSettings.brightness);
  
  // Show professional boot screen
  showBootScreen("E_NAVI", 0);
  showBootScreen("Initializing Systems...", 25);
  
  displayInitialized = true;
  
  // Set initial display mode
  updateDisplayMode();
  
  showBootScreen("Display Ready", 100);
  delay(500); // Brief pause to show completion
  
  return true;
}

void DisplayManager::setBacklight(int brightness) {
  currentBrightness = constrain(brightness, 0, 100);
  
  // Convert percentage to PWM value (0-255)
  int pwmValue = map(currentBrightness, 0, 100, 0, 255);
  analogWrite(TFT_BL, pwmValue);
  
  backlightEnabled = (brightness > 0);
}

// ===================================================================
// DISPLAY MODE MANAGEMENT - DAY/NIGHT SWITCHING
// ===================================================================

void DisplayManager::setDisplayMode(DisplayMode mode) {
  currentDisplayMode = mode;
  
  switch(mode) {
    case DISPLAY_DAY_MODE:
      setBacklight(nightSettings.brightness);
      break;
      
    case DISPLAY_NIGHT_MODE:
      setBacklight(nightSettings.nightBrightness);
      break;
      
    case DISPLAY_AUTO_MODE:
      updateDisplayMode(); // Determine based on time
      break;
  }
}

void DisplayManager::updateDisplayMode() {
  if (nightSettings.autoSwitchEnabled && 
      nightSettings.currentMode == DISPLAY_AUTO_MODE) {
    
    // Get current hour (you'll need to implement getCurrentHour())
    int currentHour = 12; // Placeholder - implement with RTC or GPS time
    
    if (currentHour >= nightSettings.nightModeStartHour || 
        currentHour < nightSettings.dayModeStartHour) {
      if (currentDisplayMode != DISPLAY_NIGHT_MODE) {
        setDisplayMode(DISPLAY_NIGHT_MODE);
      }
    } else {
      if (currentDisplayMode != DISPLAY_DAY_MODE) {
        setDisplayMode(DISPLAY_DAY_MODE);
      }
    }
  }
}

// ===================================================================
// STATUS UPDATES - REAL-TIME FEEDBACK
// ===================================================================

void DisplayManager::updateGPSStatus(int satellites, float hdop, bool hasFix) {
  if (!hasFix || satellites < GPS_MIN_SATELLITES) {
    status.gpsStatus = StatusIndicators::GPS_NO_FIX;
  } else if (hdop <= GPS_EXCELLENT_HDOP) {
    status.gpsStatus = StatusIndicators::GPS_GOOD;
  } else if (hdop <= GPS_GOOD_HDOP) {
    status.gpsStatus = StatusIndicators::GPS_MARGINAL;  
  } else {
    status.gpsStatus = StatusIndicators::GPS_POOR;
  }
}

void DisplayManager::updateRecordingStatus(StatusIndicators::RecordStatus recordStatus) {
  status.recordStatus = recordStatus;
}

void DisplayManager::updateSystemStatus(bool sdOK, bool compassOK, bool batteryOK) {
  status.sdCardOK = sdOK;
  status.compassOK = compassOK;
  status.batteryOK = batteryOK;
}

void DisplayManager::updateMotionStatus(bool motion, int sleepTimer) {
  status.motionDetected = motion;
  status.sleepCountdown = sleepTimer;
}

// ===================================================================
// MAIN UPDATE FUNCTION - 60FPS RENDERING
// ===================================================================

void DisplayManager::update() {
  if (!displayInitialized) return;
  
  unsigned long now = millis();
  if (now - lastUpdate < updateInterval) return; // Maintain 60fps
  
  // Update display mode if auto-switching
  if (nightSettings.currentMode == DISPLAY_AUTO_MODE) {
    updateDisplayMode();
  }
  
  // Render all screen components
  drawStatusBar();
  drawRecordingBorder();
  // GPS content and data bar will be updated by main application
  
  lastUpdate = now;
}

// ===================================================================
// STATUS BAR - CRITICAL INFORMATION DISPLAY
// ===================================================================

void DisplayManager::drawStatusBar() {
  // Clear status area
  tft.fillRect(0, layout.statusY, SCREEN_WIDTH, layout.statusHeight, getBackgroundColor());
  
  // GPS Status Icon (Large, clear)
  int iconSize = 32;
  int iconY = layout.statusY + 8;
  
  // GPS Status
  uint16_t gpsColor = getStatusColor(status.gpsStatus);
  tft.fillCircle(40, iconY + iconSize/2, iconSize/2, gpsColor);
  
  // GPS Text
  tft.setTextColor(getPrimaryTextColor(), getBackgroundColor());
  tft.setTextSize(2);
  tft.setCursor(80, iconY + 8);
  
  switch(status.gpsStatus) {
    case StatusIndicators::GPS_GOOD:
      tft.print("GPS OK");
      break;
    case StatusIndicators::GPS_MARGINAL:
      tft.print("GPS FAIR");
      break;
    case StatusIndicators::GPS_POOR:
      tft.print("GPS WEAK");
      break;
    default:
      tft.print("NO GPS");
      break;
  }
  
  // System Status Icons (Right side)
  int rightX = SCREEN_WIDTH - 120;
  
  // SD Card Status
  uint16_t sdColor = status.sdCardOK ? COLOR_STATUS_OK : COLOR_STATUS_ERROR;
  tft.fillRect(rightX, iconY, 15, 20, sdColor);
  
  // Compass Status  
  uint16_t compassColor = status.compassOK ? COLOR_STATUS_OK : COLOR_STATUS_ERROR;
  tft.fillCircle(rightX + 30, iconY + 10, 8, compassColor);
  
  // Battery Status
  uint16_t batteryColor = status.batteryOK ? COLOR_STATUS_OK : COLOR_STATUS_WARN;
  tft.drawRect(rightX + 50, iconY + 5, 25, 15, batteryColor);
  tft.fillRect(rightX + 75, iconY + 8, 3, 9, batteryColor);
  
  // Sleep Timer (if counting down)
  if (!status.motionDetected && status.sleepCountdown > 0) {
    tft.setTextColor(COLOR_STATUS_WARN, getBackgroundColor());
    tft.setTextSize(1);
    tft.setCursor(rightX + 85, iconY + 25);
    tft.printf("Sleep:%ds", status.sleepCountdown);
  }
}

// ===================================================================
// RECORDING BORDER - VISUAL TRACKING INDICATOR
// ===================================================================

void DisplayManager::drawRecordingBorder() {
  uint16_t borderColor = getRecordingColor(status.recordStatus);
  int borderWidth = 4;
  
  if (status.recordStatus != StatusIndicators::REC_OFF) {
    // Draw thick border around GPS content area
    tft.drawRect(0, layout.contentY, SCREEN_WIDTH, layout.contentHeight, borderColor);
    tft.drawRect(1, layout.contentY + 1, SCREEN_WIDTH - 2, layout.contentHeight - 2, borderColor);
    tft.drawRect(2, layout.contentY + 2, SCREEN_WIDTH - 4, layout.contentHeight - 4, borderColor);
    tft.drawRect(3, layout.contentY + 3, SCREEN_WIDTH - 6, layout.contentHeight - 6, borderColor);
  }
}

// ===================================================================
// GPS CONTENT AREA - MAIN NAVIGATION DISPLAY  
// ===================================================================

void DisplayManager::drawGPSContent(double lat, double lon, float speed, float heading) {
  // Clear GPS content area (inside recording border if active)
  int clearX = (status.recordStatus != StatusIndicators::REC_OFF) ? 4 : 0;
  int clearY = layout.contentY + ((status.recordStatus != StatusIndicators::REC_OFF) ? 4 : 0);
  int clearW = SCREEN_WIDTH - (clearX * 2);
  int clearH = layout.contentHeight - ((status.recordStatus != StatusIndicators::REC_OFF) ? 8 : 0);
  
  tft.fillRect(clearX, clearY, clearW, clearH, getBackgroundColor());
  
  // Large, clear coordinate display
  tft.setTextColor(getPrimaryTextColor(), getBackgroundColor());
  tft.setTextSize(2);
  
  // Latitude
  tft.setCursor(20, clearY + 20);
  tft.printf("LAT: %.6f", lat);
  
  // Longitude  
  tft.setCursor(20, clearY + 50);
  tft.printf("LON: %.6f", lon);
  
  // Large speed display (center)
  tft.setTextSize(4);
  tft.setCursor(80, clearY + 100);
  tft.printf("%.0f", speed);
  
  tft.setTextSize(2);
  tft.setCursor(200, clearY + 120);
  tft.print("km/h");
  
  // Heading display
  tft.setTextSize(3);
  tft.setCursor(100, clearY + 160);
  tft.printf("%.0f°", heading);
  
  // Cardinal direction
  const char* cardinals[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  int cardinalIndex = (int)((heading + 22.5) / 45.0) % 8;
  
  tft.setTextSize(2);
  tft.setCursor(100, clearY + 200);
  tft.print(cardinals[cardinalIndex]);
}

// ===================================================================
// DATA BAR - SECONDARY INFORMATION
// ===================================================================

void DisplayManager::drawDataBar(float speed, float heading, float temp, float battery) {
  // Clear data area
  tft.fillRect(0, layout.dataY, SCREEN_WIDTH, layout.dataHeight, getBackgroundColor());
  
  // Professional data layout
  tft.setTextColor(getPrimaryTextColor(), getBackgroundColor());
  tft.setTextSize(2);
  
  // Speed (left)
  tft.setCursor(10, layout.dataY + 20);
  tft.printf("SPD: %.1f", speed);
  
  // Heading (center)
  tft.setCursor(120, layout.dataY + 20);
  tft.printf("HDG: %.0f", heading);
  
  // Temperature (right)
  tft.setCursor(220, layout.dataY + 20);
  tft.printf("%.0fC", temp);
  
  // Battery voltage (bottom center)
  uint16_t batteryColor = (battery > BATTERY_GOOD) ? COLOR_STATUS_OK :
                         (battery > BATTERY_LOW) ? COLOR_STATUS_WARN : COLOR_STATUS_ERROR;
  
  tft.setTextColor(batteryColor, getBackgroundColor());
  tft.setCursor(120, layout.dataY + 60);
  tft.printf("%.1fV", battery);
  
  // Time display (bottom right)
  tft.setTextColor(getSecondaryTextColor(), getBackgroundColor());
  tft.setTextSize(1);
  tft.setCursor(220, layout.dataY + 80);
  tft.printf("12:34:56"); // Placeholder - implement with GPS time
}

// ===================================================================
// UTILITY FUNCTIONS
// ===================================================================

void DisplayManager::clearScreen() {
  tft.fillScreen(getBackgroundColor());
}

void DisplayManager::showBootScreen(const char* message, int progress) {
  tft.fillScreen(COLOR_BACKGROUND);
  
  // E_NAVI Logo
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setTextSize(4);
  int titleX = TEXT_CENTER_X("E_NAVI", 4);
  tft.setCursor(titleX, 100);
  tft.print("E_NAVI");
  
  // Tagline
  tft.setTextSize(2);
  int taglineX = TEXT_CENTER_X("Lost, Never Again", 2);
  tft.setCursor(taglineX, 150);
  tft.print("Lost, Never Again");
  
  // Message
  tft.setTextSize(2);
  int msgX = TEXT_CENTER_X(message, 2);
  tft.setCursor(msgX, 220);
  tft.print(message);
  
  // Progress bar
  int barWidth = 200;
  int barX = (SCREEN_WIDTH - barWidth) / 2;
  int barY = 280;
  
  tft.drawRect(barX, barY, barWidth, 20, COLOR_TEXT_PRIMARY);
  
  int fillWidth = (progress * barWidth) / 100;
  tft.fillRect(barX + 1, barY + 1, fillWidth, 18, COLOR_GPS_GOOD);
  
  // Progress percentage
  tft.setTextSize(1);
  tft.setCursor(barX + 85, barY + 30);
  tft.printf("%d%%", progress);
}

void DisplayManager::showMessage(const char* title, const char* message, int timeoutMs) {
  // Save current screen (simplified - just clear for now)
  clearScreen();
  
  // Show message
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
  tft.setTextSize(3);
  int titleX = TEXT_CENTER_X(title, 3);
  tft.setCursor(titleX, 150);
  tft.print(title);
  
  tft.setTextSize(2);
  int msgX = TEXT_CENTER_X(message, 2);
  tft.setCursor(msgX, 200);
  tft.print(message);
  
  delay(timeoutMs);
  clearScreen();
}

// ===================================================================
// COLOR MANAGEMENT
// ===================================================================

uint16_t DisplayManager::getStatusColor(StatusIndicators::GPSStatus status) {
  if (currentDisplayMode == DISPLAY_NIGHT_MODE) {
    switch(status) {
      case StatusIndicators::GPS_GOOD: return COLOR_NIGHT_GOOD;
      case StatusIndicators::GPS_MARGINAL: return COLOR_NIGHT_WARN;
      default: return COLOR_NIGHT_ERROR;
    }
  } else {
    switch(status) {
      case StatusIndicators::GPS_GOOD: return COLOR_GPS_GOOD;
      case StatusIndicators::GPS_MARGINAL: return COLOR_GPS_MARGINAL;
      default: return COLOR_GPS_POOR;
    }
  }
}

uint16_t DisplayManager::getRecordingColor(StatusIndicators::RecordStatus status) {
  uint16_t baseColor = (currentDisplayMode == DISPLAY_NIGHT_MODE) ? 
                       COLOR_NIGHT_GOOD : COLOR_GPS_GOOD;
  
  switch(status) {
    case StatusIndicators::REC_RECORDING: return baseColor;
    case StatusIndicators::REC_PAUSED: return COLOR_GPS_MARGINAL;
    default: return COLOR_BACKGROUND;
  }
}

uint16_t DisplayManager::getPrimaryTextColor() {
  return (currentDisplayMode == DISPLAY_NIGHT_MODE) ? 
         COLOR_NIGHT_TEXT : COLOR_TEXT_PRIMARY;
}

uint16_t DisplayManager::getSecondaryTextColor() {
  return (currentDisplayMode == DISPLAY_NIGHT_MODE) ? 
         COLOR_NIGHT_TEXT : COLOR_TEXT_SECOND;
}

uint16_t DisplayManager::getBackgroundColor() {
  return COLOR_BACKGROUND; // Always black for maximum contrast
}

// ===================================================================
// GLOBAL ACCESS FUNCTIONS
// ===================================================================

void initializeDisplay() {
  display.initialize();
}

void updateDisplay() {
  display.update();
}

void setDayMode() {
  display.setDisplayMode(DISPLAY_DAY_MODE);
}

void setNightMode() {
  display.setDisplayMode(DISPLAY_NIGHT_MODE);
}

void setBrightness(int level) {
  display.setBacklight(level);
}