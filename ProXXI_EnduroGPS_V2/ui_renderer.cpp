/*
 * E_NAVI EnduroGPS - Complete UI Renderer Implementation
 * Vision-optimized UI system with all screen renderers completed
 * 
 * Features: Menu screens, calibration UI, track manager, connect mode,
 *          QR code display, vision-optimized fonts and colors
 */

#include "ui_renderer.h"
#include "display_manager.h"
#include "shared_data.h"
#include "battery_monitor.h"
#include "menu_system.h"
#include "loop_calibration.h"
#include "track_management.h"
#include "connect_mode.h"
#include "wifi_manager.h"

// ========================================
// COMPLETE MENU SCREEN RENDERER
// ========================================

void UIRenderer::renderMenuScreen() {
  if (!initialized) return;
  
  // Get current menu state
  MenuState menuState = getMenuState();
  int selectedIndex = getMenuSelectedIndex();
  int scrollOffset = getMenuScrollOffset();
  
  // Clear screen with dark background
  display.clearScreen();
  
  // Header section
  display.fillRect(0, 0, SCREEN_WIDTH, 60, display.getCurrentColors().headerBg);
  display.setTextSize(FONT_SIZE_LARGE);
  display.drawCenteredText(30, "E_NAVI MENU", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // Menu items area
  int menuY = 70;
  int itemHeight = 50;
  int visibleItems = 7;
  
  // Get menu items based on current state
  const char* menuItems[10];
  int itemCount = 0;
  
  switch (menuState) {
    case MENU_MAIN:
      menuItems[0] = "Navigate";
      menuItems[1] = "Track Manager";
      menuItems[2] = "Calibrate GPS";
      menuItems[3] = "Connect Mode";
      menuItems[4] = "Settings";
      menuItems[5] = "System Info";
      itemCount = 6;
      break;
      
    case MENU_SETTINGS:
      menuItems[0] = "Display Settings";
      menuItems[1] = "GPS Settings";
      menuItems[2] = "Track Settings";
      menuItems[3] = "Power Settings";
      menuItems[4] = "Back to Main";
      itemCount = 5;
      break;
      
    case MENU_SYSTEM:
      menuItems[0] = "Storage Info";
      menuItems[1] = "GPS Status";
      menuItems[2] = "WiFi Status";
      menuItems[3] = "Battery Info";
      menuItems[4] = "Reset Settings";
      menuItems[5] = "Back to Main";
      itemCount = 6;
      break;
  }
  
  // Render menu items with vision-optimized styling
  for (int i = 0; i < min(itemCount, visibleItems); i++) {
    int itemIndex = i + scrollOffset;
    if (itemIndex >= itemCount) break;
    
    int itemY = menuY + (i * itemHeight);
    bool isSelected = (itemIndex == selectedIndex);
    
    // Item background
    uint16_t bgColor = isSelected ? display.getCurrentColors().accent : display.getCurrentColors().menuBg;
    uint16_t textColor = isSelected ? display.getCurrentColors().background : display.getCurrentColors().text;
    
    display.fillRect(10, itemY, SCREEN_WIDTH - 20, itemHeight - 5, bgColor);
    
    // Item text - large and clear for vision optimization
    display.setTextSize(FONT_SIZE_MEDIUM);
    display.drawText(25, itemY + 15, menuItems[itemIndex], textColor, FONT_SIZE_MEDIUM);
    
    // Selection indicator
    if (isSelected) {
      display.fillTriangle(SCREEN_WIDTH - 30, itemY + 15, 
                          SCREEN_WIDTH - 30, itemY + 35,
                          SCREEN_WIDTH - 15, itemY + 25, textColor);
    }
  }
  
  // Scroll indicators
  if (scrollOffset > 0) {
    display.fillTriangle(SCREEN_WIDTH/2 - 10, 65, SCREEN_WIDTH/2 + 10, 65, 
                        SCREEN_WIDTH/2, 55, display.getCurrentColors().accent);
  }
  
  if (scrollOffset + visibleItems < itemCount) {
    int bottomY = menuY + (visibleItems * itemHeight) + 10;
    display.fillTriangle(SCREEN_WIDTH/2 - 10, bottomY, SCREEN_WIDTH/2 + 10, bottomY, 
                        SCREEN_WIDTH/2, bottomY + 10, display.getCurrentColors().accent);
  }
  
  // Navigation hint
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(SCREEN_HEIGHT - 25, "SELECT: OK  EXIT: BACK", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

// ========================================
// COMPLETE CALIBRATION SCREEN RENDERER
// ========================================

void UIRenderer::renderCalibrationScreen() {
  if (!initialized) return;
  
  CalibrationState calState = getCalibrationState();
  CalibrationData calData = getCalibrationData();
  
  display.clearScreen();
  
  // Header
  display.fillRect(0, 0, SCREEN_WIDTH, 60, display.getCurrentColors().headerBg);
  display.setTextSize(FONT_SIZE_LARGE);
  display.drawCenteredText(30, "GPS CALIBRATION", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  int contentY = 80;
  
  switch (calState) {
    case CAL_WAITING_FOR_GPS:
      renderCalibrationWaiting(contentY);
      break;
      
    case CAL_COLLECTING_DATA:
      renderCalibrationProgress(contentY, calData);
      break;
      
    case CAL_ANALYZING:
      renderCalibrationAnalyzing(contentY);
      break;
      
    case CAL_COMPLETE:
      renderCalibrationResults(contentY, calData);
      break;
      
    case CAL_ERROR:
      renderCalibrationError(contentY);
      break;
  }
  
  // Instructions at bottom
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(SCREEN_HEIGHT - 40, "Stay still in open area", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  display.drawCenteredText(SCREEN_HEIGHT - 20, "BACK: Exit calibration", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

void UIRenderer::renderCalibrationWaiting(int y) {
  // GPS waiting animation
  static unsigned long lastBlink = 0;
  static bool showDot = true;
  
  if (millis() - lastBlink > 500) {
    showDot = !showDot;
    lastBlink = millis();
  }
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 50, "Waiting for GPS signal", 
                          display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  String dots = showDot ? "..." : "   ";
  display.drawCenteredText(y + 80, dots, display.getCurrentColors().accent, FONT_SIZE_MEDIUM);
  
  // GPS status
  GPSData gps = getGPSData();
  String statusText = "Satellites: " + String(gps.satellites) + " / 4 required";
  display.drawCenteredText(y + 120, statusText, display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

void UIRenderer::renderCalibrationProgress(int y, const CalibrationData& calData) {
  // Progress circle
  int centerX = SCREEN_WIDTH / 2;
  int centerY = y + 80;
  int radius = 40;
  
  // Background circle
  display.drawCircle(centerX, centerY, radius, display.getCurrentColors().border);
  
  // Progress arc
  float progressAngle = (calData.samplesCollected * 360.0) / calData.targetSamples;
  for (int i = 0; i < progressAngle; i += 5) {
    float angle = (i - 90) * PI / 180.0;
    int x1 = centerX + (radius - 5) * cos(angle);
    int y1 = centerY + (radius - 5) * sin(angle);
    int x2 = centerX + radius * cos(angle);
    int y2 = centerY + radius * sin(angle);
    display.drawLine(x1, y1, x2, y2, display.getCurrentColors().accent);
  }
  
  // Progress text
  display.setTextSize(FONT_SIZE_MEDIUM);
  String progressText = String(calData.samplesCollected) + "/" + String(calData.targetSamples);
  display.drawCenteredText(centerY - 5, progressText, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Current accuracy
  display.setTextSize(FONT_SIZE_SMALL);
  String accuracyText = "Accuracy: " + String(calData.currentAccuracy, 1) + "m";
  display.drawCenteredText(y + 150, accuracyText, display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  
  // Quality indicator
  uint16_t qualityColor = calData.currentAccuracy < 3.0 ? 
                         display.getCurrentColors().gpsGood : 
                         display.getCurrentColors().gpsWarn;
  
  display.fillRect(50, y + 170, (SCREEN_WIDTH - 100) * calData.samplesCollected / calData.targetSamples, 
                  10, qualityColor);
  display.drawRect(50, y + 170, SCREEN_WIDTH - 100, 10, display.getCurrentColors().border);
}

void UIRenderer::renderCalibrationAnalyzing(int y) {
  // Analyzing animation
  static unsigned long lastSpin = 0;
  static int spinAngle = 0;
  
  if (millis() - lastSpin > 50) {
    spinAngle = (spinAngle + 15) % 360;
    lastSpin = millis();
  }
  
  int centerX = SCREEN_WIDTH / 2;
  int centerY = y + 80;
  
  // Spinning arc
  for (int i = 0; i < 90; i += 10) {
    float angle = (spinAngle + i) * PI / 180.0;
    int x = centerX + 30 * cos(angle);
    int y_pos = centerY + 30 * sin(angle);
    display.fillCircle(x, y_pos, 3, display.getCurrentColors().accent);
  }
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 130, "Analyzing GPS data...", 
                          display.getCurrentColors().text, FONT_SIZE_MEDIUM);
}

void UIRenderer::renderCalibrationResults(int y, const CalibrationData& calData) {
  // Success indicator
  display.fillCircle(SCREEN_WIDTH/2, y + 50, 25, display.getCurrentColors().gpsGood);
  display.setTextSize(FONT_SIZE_LARGE);
  display.drawCenteredText(y + 55, "✓", display.getCurrentColors().background, FONT_SIZE_LARGE);
  
  // Results
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 90, "Calibration Complete!", 
                          display.getCurrentColors().gpsGood, FONT_SIZE_MEDIUM);
  
  display.setTextSize(FONT_SIZE_SMALL);
  String avgAccuracy = "Average accuracy: " + String(calData.averageAccuracy, 1) + "m";
  display.drawCenteredText(y + 120, avgAccuracy, display.getCurrentColors().text, FONT_SIZE_SMALL);
  
  String maxAccuracy = "Best accuracy: " + String(calData.bestAccuracy, 1) + "m";
  display.drawCenteredText(y + 140, maxAccuracy, display.getCurrentColors().text, FONT_SIZE_SMALL);
  
  // Quality assessment
  String qualityText;
  uint16_t qualityColor;
  
  if (calData.averageAccuracy < 2.0) {
    qualityText = "EXCELLENT GPS SIGNAL";
    qualityColor = display.getCurrentColors().gpsGood;
  } else if (calData.averageAccuracy < 5.0) {
    qualityText = "GOOD GPS SIGNAL";
    qualityColor = display.getCurrentColors().gpsWarn;
  } else {
    qualityText = "FAIR GPS SIGNAL";
    qualityColor = display.getCurrentColors().gpsBad;
  }
  
  display.drawCenteredText(y + 170, qualityText, qualityColor, FONT_SIZE_SMALL);
}

void UIRenderer::renderCalibrationError(int y) {
  // Error indicator
  display.fillCircle(SCREEN_WIDTH/2, y + 50, 25, display.getCurrentColors().gpsBad);
  display.setTextSize(FONT_SIZE_LARGE);
  display.drawCenteredText(y + 55, "!", display.getCurrentColors().background, FONT_SIZE_LARGE);
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 90, "Calibration Failed", 
                          display.getCurrentColors().gpsBad, FONT_SIZE_MEDIUM);
  
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(y + 120, "Poor GPS signal", display.getCurrentColors().text, FONT_SIZE_SMALL);
  display.drawCenteredText(y + 140, "Move to open area", display.getCurrentColors().text, FONT_SIZE_SMALL);
  display.drawCenteredText(y + 160, "Try again", display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

// ========================================
// COMPLETE TRACK MANAGER SCREEN RENDERER
// ========================================

void UIRenderer::renderTrackManagerScreen() {
  if (!initialized) return;
  
  TrackManagerState tmState = getTrackManagerState();
  int selectedTrack = getSelectedTrackIndex();
  int scrollOffset = getTrackScrollOffset();
  
  display.clearScreen();
  
  // Header
  display.fillRect(0, 0, SCREEN_WIDTH, 60, display.getCurrentColors().headerBg);
  display.setTextSize(FONT_SIZE_LARGE);
  display.drawCenteredText(30, "TRACK MANAGER", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // Track count and storage info
  int trackCount = getTrackCount();
  String infoText = String(trackCount) + " tracks | " + getStorageInfo();
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawText(10, 65, infoText, display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  
  int listY = 80;
  int itemHeight = 45;
  int visibleItems = 6;
  
  if (trackCount == 0) {
    // No tracks message
    display.setTextSize(FONT_SIZE_MEDIUM);
    display.drawCenteredText(SCREEN_HEIGHT/2 - 20, "No tracks found", 
                            display.getCurrentColors().textDim, FONT_SIZE_MEDIUM);
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawCenteredText(SCREEN_HEIGHT/2 + 10, "Go riding to create tracks!", 
                            display.getCurrentColors().accent, FONT_SIZE_SMALL);
  } else {
    // Render track list
    for (int i = 0; i < min(trackCount, visibleItems); i++) {
      int trackIndex = i + scrollOffset;
      if (trackIndex >= trackCount) break;
      
      TrackInfo track = getTrackInfo(trackIndex);
      int itemY = listY + (i * itemHeight);
      bool isSelected = (trackIndex == selectedTrack);
      
      // Item background
      if (isSelected) {
        display.fillRect(5, itemY, SCREEN_WIDTH - 10, itemHeight - 2, 
                        display.getCurrentColors().accent);
      }
      
      uint16_t textColor = isSelected ? display.getCurrentColors().background : 
                                       display.getCurrentColors().text;
      uint16_t dimColor = isSelected ? display.getCurrentColors().background : 
                                      display.getCurrentColors().textDim;
      
      // Track name
      display.setTextSize(FONT_SIZE_MEDIUM);
      display.drawText(15, itemY + 5, track.name, textColor, FONT_SIZE_MEDIUM);
      
      // Track details
      display.setTextSize(FONT_SIZE_SMALL);
      String details = track.date + " | " + String(track.distanceKm, 1) + "km | " + 
                      track.duration + " | " + String(track.fileSizeKB) + "KB";
      display.drawText(15, itemY + 25, details, dimColor, FONT_SIZE_SMALL);
      
      // Status indicator
      if (track.isActive) {
        display.fillCircle(SCREEN_WIDTH - 20, itemY + 15, 5, display.getCurrentColors().gpsGood);
      }
    }
  }
  
  // Action buttons area
  if (trackCount > 0 && selectedTrack >= 0) {
    int buttonY = SCREEN_HEIGHT - 80;
    display.fillRect(0, buttonY - 5, SCREEN_WIDTH, 85, display.getCurrentColors().menuBg);
    
    // Action buttons
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawText(10, buttonY + 5, "OK: Load track", display.getCurrentColors().accent, FONT_SIZE_SMALL);
    display.drawText(10, buttonY + 20, "MENU: Track options", display.getCurrentColors().text, FONT_SIZE_SMALL);
    display.drawText(10, buttonY + 35, "ZOOM+: Export", display.getCurrentColors().text, FONT_SIZE_SMALL);
    display.drawText(10, buttonY + 50, "ZOOM-: Delete", display.getCurrentColors().gpsBad, FONT_SIZE_SMALL);
  }
  
  // Navigation hint
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(SCREEN_HEIGHT - 10, "BACK: Exit", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

// ========================================
// COMPLETE CONNECT MODE SCREEN RENDERER
// ========================================

void UIRenderer::renderConnectModeScreen() {
  if (!initialized) return;
  
  ConnectModeState connectState = getConnectModeState();
  WiFiManager& wifi = getWiFiManager();
  
  display.clearScreen();
  
  // Header
  display.fillRect(0, 0, SCREEN_WIDTH, 60, display.getCurrentColors().headerBg);
  display.setTextSize(FONT_SIZE_LARGE);
  display.drawCenteredText(30, "CONNECT MODE", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  int contentY = 80;
  
  switch (connectState) {
    case CONNECT_STARTING:
      renderConnectStarting(contentY);
      break;
      
    case CONNECT_WIFI_ACTIVE:
      renderConnectActive(contentY, wifi);
      break;
      
    case CONNECT_CLIENT_CONNECTED:
      renderConnectClientActive(contentY, wifi);
      break;
      
    case CONNECT_FILE_TRANSFER:
      renderConnectFileTransfer(contentY, wifi);
      break;
      
    case CONNECT_REINDEXING:
      renderConnectReindexing(contentY);
      break;
      
    case CONNECT_ERROR:
      renderConnectError(contentY);
      break;
  }
}

void UIRenderer::renderConnectStarting(int y) {
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 50, "Starting WiFi hotspot...", 
                          display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Starting animation
  static unsigned long lastBlink = 0;
  static bool showDot = true;
  
  if (millis() - lastBlink > 300) {
    showDot = !showDot;
    lastBlink = millis();
  }
  
  String dots = showDot ? "●●●" : "○○○";
  display.drawCenteredText(y + 80, dots, display.getCurrentColors().accent, FONT_SIZE_MEDIUM);
}

void UIRenderer::renderConnectActive(int y, WiFiManager& wifi) {
  // WiFi status
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 20, "WiFi Hotspot Active", 
                          display.getCurrentColors().gpsGood, FONT_SIZE_MEDIUM);
  
  // Connection details - large and clear for easy reading
  display.setTextSize(FONT_SIZE_SMALL);
  String ssid = "Network: " + wifi.getSSID();
  display.drawCenteredText(y + 50, ssid, display.getCurrentColors().text, FONT_SIZE_SMALL);
  
  String password = "Password: " + wifi.getPassword();
  display.drawCenteredText(y + 70, password, display.getCurrentColors().text, FONT_SIZE_SMALL);
  
  String url = "URL: http://" + wifi.getLocalIP();
  display.drawCenteredText(y + 90, url, display.getCurrentColors().accent, FONT_SIZE_SMALL);
  
  // QR Code area
  renderQRCodeDisplay(y + 120);
  
  // Instructions
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(y + 260, "1. Connect device to WiFi", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  display.drawCenteredText(y + 275, "2. Open browser to URL", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  display.drawCenteredText(y + 290, "3. Upload/Download files", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  
  // Exit option
  display.drawCenteredText(SCREEN_HEIGHT - 20, "BACK: Done", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void UIRenderer::renderConnectClientActive(int y, WiFiManager& wifi) {
  // Client connected status
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 20, "Device Connected!", 
                          display.getCurrentColors().gpsGood, FONT_SIZE_MEDIUM);
  
  // Client info
  String clientIP = "Client: " + wifi.getLastClientIP();
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(y + 50, clientIP, display.getCurrentColors().text, FONT_SIZE_SMALL);
  
  // Session stats
  WiFiStats stats = wifi.getStatistics();
  String sessionTime = "Session: " + formatTime(stats.totalSessionTime / 1000);
  display.drawCenteredText(y + 70, sessionTime, display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  
  // File transfer status
  display.fillRect(20, y + 100, SCREEN_WIDTH - 40, 80, display.getCurrentColors().menuBg);
  display.drawRect(20, y + 100, SCREEN_WIDTH - 40, 80, display.getCurrentColors().border);
  
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawText(30, y + 110, "Files uploaded: " + String(stats.filesUploaded), 
                  display.getCurrentColors().text, FONT_SIZE_SMALL);
  display.drawText(30, y + 125, "Files downloaded: " + String(stats.filesDownloaded), 
                  display.getCurrentColors().text, FONT_SIZE_SMALL);
  display.drawText(30, y + 140, "Data transferred: " + formatBytes(stats.bytesUploaded + stats.bytesDownloaded), 
                  display.getCurrentColors().text, FONT_SIZE_SMALL);
  
  // Activity indicator
  static unsigned long lastPulse = 0;
  static bool showPulse = true;
  
  if (millis() - lastPulse > 1000) {
    showPulse = !showPulse;
    lastPulse = millis();
  }
  
  if (showPulse) {
    display.fillCircle(SCREEN_WIDTH - 30, y + 30, 8, display.getCurrentColors().accent);
  }
  
  display.drawCenteredText(SCREEN_HEIGHT - 20, "BACK: Done", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void UIRenderer::renderConnectFileTransfer(int y, WiFiManager& wifi) {
  // Transfer in progress
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 20, "File Transfer Active", 
                          display.getCurrentColors().accent, FONT_SIZE_MEDIUM);
  
  // Get current transfer status
  FileTransferStatus transfer = wifi.getCurrentTransfer();
  
  if (transfer.transferActive) {
    // Transfer details
    display.setTextSize(FONT_SIZE_SMALL);
    String operation = transfer.isUpload ? "Uploading: " : "Downloading: ";
    display.drawCenteredText(y + 50, operation + transfer.filename, 
                            display.getCurrentColors().text, FONT_SIZE_SMALL);
    
    // Progress bar
    int progressWidth = SCREEN_WIDTH - 40;
    int progressX = 20;
    int progressY = y + 80;
    
    display.fillRect(progressX, progressY, progressWidth, 20, display.getCurrentColors().background);
    display.drawRect(progressX, progressY, progressWidth, 20, display.getCurrentColors().border);
    
    int fillWidth = (progressWidth * transfer.progressPercent) / 100;
    display.fillRect(progressX + 1, progressY + 1, fillWidth, 18, display.getCurrentColors().accent);
    
    // Progress text
    String progressText = String(transfer.progressPercent, 1) + "%";
    display.drawCenteredText(y + 90, progressText, display.getCurrentColors().text, FONT_SIZE_SMALL);
    
    // Transfer speed and ETA
    String speedText = formatBytes(transfer.transferredBytes) + " / " + formatBytes(transfer.totalBytes);
    display.drawCenteredText(y + 110, speedText, display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  }
  
  display.drawCenteredText(SCREEN_HEIGHT - 20, "Transfer in progress...", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

void UIRenderer::renderConnectReindexing(int y) {
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 50, "Re-indexing files...", 
                          display.getCurrentColors().accent, FONT_SIZE_MEDIUM);
  
  // Scanning animation
  static unsigned long lastScan = 0;
  static int scanPos = 0;
  
  if (millis() - lastScan > 100) {
    scanPos = (scanPos + 10) % (SCREEN_WIDTH - 40);
    lastScan = millis();
  }
  
  // Scanning line
  display.fillRect(20, y + 100, SCREEN_WIDTH - 40, 3, display.getCurrentColors().border);
  display.fillRect(20 + scanPos, y + 100, 20, 3, display.getCurrentColors().accent);
  
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(y + 130, "Detecting new files...", 
                          display.getCurrentColors().text, FONT_SIZE_SMALL);
}

void UIRenderer::renderConnectError(int y) {
  // Error indicator
  display.fillCircle(SCREEN_WIDTH/2, y + 50, 25, display.getCurrentColors().gpsBad);
  display.setTextSize(FONT_SIZE_LARGE);
  display.drawCenteredText(y + 55, "!", display.getCurrentColors().background, FONT_SIZE_LARGE);
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y + 90, "Connection Error", 
                          display.getCurrentColors().gpsBad, FONT_SIZE_MEDIUM);
  
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(y + 120, "Failed to start WiFi", display.getCurrentColors().text, FONT_SIZE_SMALL);
  display.drawCenteredText(y + 140, "Check antenna connection", display.getCurrentColors().text, FONT_SIZE_SMALL);
  
  display.drawCenteredText(SCREEN_HEIGHT - 20, "BACK: Return to menu", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

// ========================================
// QR CODE DISPLAY SYSTEM
// ========================================

void UIRenderer::renderQRCodeDisplay(int y) {
  // QR Code placeholder area (120x120 pixels)
  int qrSize = 120;
  int qrX = (SCREEN_WIDTH - qrSize) / 2;
  int qrY = y;
  
  // QR Code border
  display.drawRect(qrX - 2, qrY - 2, qrSize + 4, qrSize + 4, display.getCurrentColors().accent);
  display.fillRect(qrX, qrY, qrSize, qrSize, display.getCurrentColors().background);
  
  // For MVP: Simple QR code pattern representation
  // In production, this would render actual QR code data
  renderQRPattern(qrX, qrY, qrSize);
  
  // QR Code instructions
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(y + qrSize + 15, "Scan to connect automatically", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

void UIRenderer::renderQRPattern(int x, int y, int size) {
  // Create a recognizable QR code pattern for visual purposes
  // This is a simplified representation - real implementation would use proper QR library
  
  uint16_t black = display.getCurrentColors().text;
  uint16_t white = display.getCurrentColors().background;
  
  // Corner markers (finder patterns)
  renderQRFinderPattern(x + 10, y + 10, black, white);
  renderQRFinderPattern(x + size - 35, y + 10, black, white);
  renderQRFinderPattern(x + 10, y + size - 35, black, white);
  
  // Timing patterns
  for (int i = 25; i < size - 25; i += 4) {
    display.fillRect(x + i, y + 30, 2, 2, (i % 8 == 1) ? black : white);
    display.fillRect(x + 30, y + i, 2, 2, (i % 8 == 1) ? black : white);
  }
  
  // Data pattern (simplified)
  for (int dy = 45; dy < size - 20; dy += 6) {
    for (int dx = 45; dx < size - 20; dx += 6) {
      if ((dx + dy) % 12 < 6) {
        display.fillRect(x + dx, y + dy, 3, 3, black);
      }
    }
  }
  
  // Alignment pattern (center)
  int centerX = x + size/2 - 10;
  int centerY = y + size/2 - 10;
  display.fillRect(centerX, centerY, 20, 20, black);
  display.fillRect(centerX + 2, centerY + 2, 16, 16, white);
  display.fillRect(centerX + 6, centerY + 6, 8, 8, black);
}

void UIRenderer::renderQRFinderPattern(int x, int y, uint16_t black, uint16_t white) {
  // 7x7 finder pattern
  display.fillRect(x, y, 25, 25, black);
  display.fillRect(x + 2, y + 2, 21, 21, white);
  display.fillRect(x + 4, y + 4, 17, 17, black);
  display.fillRect(x + 6, y + 6, 13, 13, white);
  display.fillRect(x + 8, y + 8, 9, 9, black);
}

// ========================================
// UTILITY FUNCTIONS FOR UI RENDERING
// ========================================

String UIRenderer::formatTime(unsigned long seconds) {
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  
  if (hours > 0) {
    return String(hours) + "h " + String(minutes) + "m";
  } else {
    return String(minutes) + "m " + String(secs) + "s";
  }
}

String UIRenderer::formatBytes(unsigned long bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < 1048576) {
    return String(bytes / 1024.0, 1) + "KB";
  } else {
    return String(bytes / 1048576.0, 1) + "MB";
  }
}

// ========================================
// ENHANCED PERFORMANCE TRACKING
// ========================================

void UIRenderer::updatePerformanceMetrics() {
  unsigned long now = millis();
  unsigned long frameDuration = now - lastFrameTime;
  
  frameCount++;
  
  // Calculate average FPS over last 30 frames
  static float frameTimes[30];
  static int frameIndex = 0;
  static bool bufferFilled = false;
  
  frameTimes[frameIndex] = frameDuration;
  frameIndex = (frameIndex + 1) % 30;
  
  if (frameIndex == 0) bufferFilled = true;
  
  if (bufferFilled) {
    float totalTime = 0;
    for (int i = 0; i < 30; i++) {
      totalTime += frameTimes[i];
    }
    averageFPS = 30000.0 / totalTime; // Convert to FPS
  }
  
  lastFrameTime = now;
}

// ========================================
// ENHANCED MAIN SCREEN RENDERING
// ========================================

void UIRenderer::renderMainScreen() {
  if (!initialized) return;
  
  GPSData gps = getGPSData();
  CompassData compass = getCompassData();
  TrackingData tracking = getTrackingData();
  SystemStatus system = getSystemStatus();
  
  // Only render if screen is dirty or data has changed
  if (!screenDirty && !hasDataChanged(gps, compass, tracking, system)) {
    return;
  }
  
  // Render all main screen components
  renderStatusBar(gps, system);
  renderMainContent(gps, compass, tracking);
  renderBottomBar(gps, compass, system);
  renderRecordingIndicators(tracking);
  
  screenDirty = false;
}

void UIRenderer::renderMainContent(const GPSData& gps, const CompassData& compass, 
                                  const TrackingData& tracking) {
  // Clear content area
  display.fillRect(0, layout.contentY, SCREEN_WIDTH, layout.contentHeight, 
                   display.getCurrentColors().background);
  
  // Main navigation data area
  int contentX = 10;
  int contentY = layout.contentY + 10;
  int contentW = SCREEN_WIDTH - 20;
  int contentH = layout.contentHeight - 20;
  
  if (gps.isValid) {
    renderNavigationData(contentX, contentY, contentW, contentH, gps, compass);
  } else {
    renderNoGPSScreen(contentX, contentY, contentW, contentH);
  }
  
  // Map integration (if available)
  if (gps.isValid && isMapDisplayAvailable()) {
    renderMapOverlay(contentX + contentW - 100, contentY, 90, 90);
  }
}

void UIRenderer::renderNavigationData(int x, int y, int w, int h, 
                                     const GPSData& gps, const CompassData& compass) {
  // Speed display (large, center top)
  int speedCenterX = x + w/2;
  int speedCenterY = y + 60;
  renderSpeedDisplay(speedCenterX, speedCenterY, gps.speedKph);
  
  // Heading display (below speed)
  int headingCenterY = speedCenterY + 80;
  renderHeadingDisplay(speedCenterX, headingCenterY, compass.heading);
  
  // Coordinates (bottom of content area)
  renderCoordinateDisplay(x + 10, y + h - 40, gps.latitude, gps.longitude);
  
  // Altitude and accuracy (right side)
  renderAltitudeDisplay(x + w - 100, y + 20, gps.altitude);
  renderAccuracyDisplay(x + w - 100, y + 60, gps.hdop, gps.satellites);
  
  // Track info (left side if recording)
  if (getTrackingData().isRecording) {
    renderTrackInfo(x + 10, y + h - 80);
  }
}

void UIRenderer::renderSpeedDisplay(int centerX, int centerY, float speed) {
  // Large speed number - optimized for vision
  display.setTextSize(FONT_SIZE_XLARGE);
  String speedText = String(speed, 1);
  
  // Speed value
  uint16_t speedColor = (speed > 50) ? display.getCurrentColors().gpsWarn : 
                       display.getCurrentColors().text;
  display.drawCenteredText(centerY - 10, speedText, speedColor, FONT_SIZE_XLARGE);
  
  // Units
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(centerY + 25, "km/h", display.getCurrentColors().textDim, FONT_SIZE_MEDIUM);
  
  // Speed history bars (small speed trend)
  static float speedHistory[10];
  static int historyIndex = 0;
  static unsigned long lastSpeedUpdate = 0;
  
  if (millis() - lastSpeedUpdate > 1000) {
    speedHistory[historyIndex] = speed;
    historyIndex = (historyIndex + 1) % 10;
    lastSpeedUpdate = millis();
  }
  
  // Draw speed trend bars
  for (int i = 0; i < 10; i++) {
    int barX = centerX - 50 + (i * 10);
    int barHeight = constrain(speedHistory[i] * 2, 0, 30);
    int barY = centerY + 35 - barHeight;
    
    uint16_t barColor = (i == (historyIndex - 1 + 10) % 10) ? 
                       display.getCurrentColors().accent : 
                       display.getCurrentColors().border;
    
    display.fillRect(barX, barY, 8, barHeight, barColor);
  }
}

void UIRenderer::renderHeadingDisplay(int centerX, int centerY, float heading) {
  // Compass rose background
  int radius = 35;
  display.drawCircle(centerX, centerY, radius, display.getCurrentColors().border);
  display.drawCircle(centerX, centerY, radius - 5, display.getCurrentColors().border);
  
  // Cardinal directions
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(centerY - radius + 8, "N", display.getCurrentColors().accent, FONT_SIZE_SMALL);
  display.drawText(centerX + radius - 12, centerY - 5, "E", display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  display.drawCenteredText(centerY + radius - 8, "S", display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  display.drawText(centerX - radius + 5, centerY - 5, "W", display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  
  // Heading needle
  float headingRad = (heading - 90) * PI / 180.0;
  int needleX = centerX + (radius - 10) * cos(headingRad);
  int needleY = centerY + (radius - 10) * sin(headingRad);
  
  // Draw needle
  display.drawLine(centerX, centerY, needleX, needleY, display.getCurrentColors().accent);
  display.fillCircle(centerX, centerY, 3, display.getCurrentColors().accent);
  
  // Heading text
  display.setTextSize(FONT_SIZE_MEDIUM);
  String headingText = String(heading, 0) + "°";
  display.drawCenteredText(centerY + 50, headingText, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Cardinal direction
  String cardinalDir = getCardinalDirection(heading);
  display.drawCenteredText(centerY + 70, cardinalDir, display.getCurrentColors().accent, FONT_SIZE_MEDIUM);
}

void UIRenderer::renderCoordinateDisplay(int x, int y, double lat, double lon) {
  display.setTextSize(FONT_SIZE_SMALL);
  
  // Format coordinates for easy reading
  String latText = "Lat: " + String(lat, 6) + "°";
  String lonText = "Lon: " + String(lon, 6) + "°";
  
  display.drawText(x, y, latText, display.getCurrentColors().textDim, FONT_SIZE_SMALL);
  display.drawText(x, y + 15, lonText, display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

void UIRenderer::renderAltitudeDisplay(int x, int y, float altitude) {
  display.setTextSize(FONT_SIZE_MEDIUM);
  String altText = String(altitude, 0) + "m";
  display.drawText(x, y, altText, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawText(x, y + 20, "Altitude", display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

void UIRenderer::renderAccuracyDisplay(int x, int y, float hdop, int satellites) {
  display.setTextSize(FONT_SIZE_SMALL);
  
  String satText = String(satellites) + " sats";
  display.drawText(x, y, satText, display.getCurrentColors().text, FONT_SIZE_SMALL);
  
  String hdopText = "±" + String(hdop, 1) + "m";
  uint16_t accuracyColor = (hdop < 3.0) ? display.getCurrentColors().gpsGood : 
                          (hdop < 10.0) ? display.getCurrentColors().gpsWarn : 
                                         display.getCurrentColors().gpsBad;
  display.drawText(x, y + 15, hdopText, accuracyColor, FONT_SIZE_SMALL);
}

void UIRenderer::renderNoGPSScreen(int x, int y, int w, int h) {
  // GPS searching animation
  static unsigned long lastPulse = 0;
  static int pulseRadius = 10;
  static bool expanding = true;
  
  if (millis() - lastPulse > 100) {
    if (expanding) {
      pulseRadius += 5;
      if (pulseRadius > 40) expanding = false;
    } else {
      pulseRadius -= 5;
      if (pulseRadius < 10) expanding = true;
    }
    lastPulse = millis();
  }
  
  int centerX = x + w/2;
  int centerY = y + h/2;
  
  // Pulsing GPS icon
  display.drawCircle(centerX, centerY, pulseRadius, display.getCurrentColors().gpsWarn);
  display.fillCircle(centerX, centerY, 8, display.getCurrentColors().gpsBad);
  
  // Status text
  display.setTextSize(FONT_SIZE_LARGE);
  display.drawCenteredText(centerY + 60, "Searching GPS...", 
                          display.getCurrentColors().gpsWarn, FONT_SIZE_LARGE);
  
  // Satellite count
  GPSData gps = getGPSData();
  display.setTextSize(FONT_SIZE_MEDIUM);
  String satText = String(gps.satellites) + " satellites found";
  display.drawCenteredText(centerY + 90, satText, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Instructions
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(centerY + 120, "Move to open area for better signal", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

void UIRenderer::renderTrackInfo(int x, int y) {
  TrackingData tracking = getTrackingData();
  
  display.setTextSize(FONT_SIZE_SMALL);
  
  // Recording time
  String timeText = "Rec: " + formatTime(tracking.recordingTime / 1000);
  display.drawText(x, y, timeText, display.getCurrentColors().accent, FONT_SIZE_SMALL);
  
  // Distance
  String distText = "Dist: " + String(tracking.totalDistance / 1000.0, 2) + "km";
  display.drawText(x, y + 15, distText, display.getCurrentColors().text, FONT_SIZE_SMALL);
}

void UIRenderer::renderRecordingIndicators(const TrackingData& tracking) {
  if (!tracking.isRecording) return;
  
  // Recording indicator (top right)
  static unsigned long lastBlink = 0;
  static bool showRec = true;
  
  if (millis() - lastBlink > 500) {
    showRec = !showRec;
    lastBlink = millis();
  }
  
  if (showRec) {
    display.fillCircle(SCREEN_WIDTH - 20, 30, 8, display.getCurrentColors().gpsBad);
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawText(SCREEN_WIDTH - 50, 25, "REC", display.getCurrentColors().gpsBad, FONT_SIZE_SMALL);
  }
}

void UIRenderer::renderBottomBar(const GPSData& gps, const CompassData& compass, 
                                const SystemStatus& system) {
  // Clear bottom bar
  display.fillRect(0, layout.bottomBarY, SCREEN_WIDTH, layout.bottomBarHeight, 
                   display.getCurrentColors().bottomBarBg);
  
  int barY = layout.bottomBarY + 10;
  
  // System status icons
  renderSystemStatusIcons(10, barY, system);
  
  // Current time (if GPS has time)
  if (gps.isValid) {
    renderCurrentTime(SCREEN_WIDTH/2, barY + 15, gps);
  }
  
  // Battery status
  renderBatteryStatus(SCREEN_WIDTH - 80, barY, system);
  
  // Button hints
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(layout.bottomBarY + layout.bottomBarHeight - 15, 
                          "MENU: Options  OK: Action  ZOOM: Map", 
                          display.getCurrentColors().textDim, FONT_SIZE_SMALL);
}

void UIRenderer::renderSystemStatusIcons(int x, int y, const SystemStatus& system) {
  int iconSpacing = 25;
  int iconX = x;
  
  // SD Card icon
  renderSDCardIcon(iconX, y, system.sdCardOK);
  iconX += iconSpacing;
  
  // Compass icon
  renderCompassIcon(iconX, y, system.compassOK);
  iconX += iconSpacing;
  
  // WiFi icon (if active)
  if (system.wifiActive) {
    renderWiFiIcon(iconX, y, system.wifiConnected);
  }
}

void UIRenderer::renderCurrentTime(int x, int y, const GPSData& gps) {
  if (!gps.timeValid) return;
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  String timeText = formatGPSTime(gps.hour, gps.minute, gps.second);
  display.drawCenteredText(y, timeText, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
}

void UIRenderer::renderBatteryStatus(int x, int y, const SystemStatus& system) {
  // Battery outline
  display.drawRect(x, y, 30, 15, display.getCurrentColors().border);
  display.fillRect(x + 30, y + 4, 3, 7, display.getCurrentColors().border);
  
  // Battery fill
  int fillWidth = (26 * getBatteryFillLevel(system.batteryVoltage)) / 16;
  uint16_t fillColor = getBatteryColor(system.batteryVoltage);
  display.fillRect(x + 2, y + 2, fillWidth, 11, fillColor);
  
  // Battery percentage
  display.setTextSize(FONT_SIZE_SMALL);
  String battText = String(system.batteryPercent) + "%";
  display.drawText(x, y + 20, battText, fillColor, FONT_SIZE_SMALL);
}

void UIRenderer::renderSDCardIcon(int x, int y, bool sdOK) {
  uint16_t color = sdOK ? display.getCurrentColors().gpsGood : display.getCurrentColors().gpsBad;
  
  // SD card shape
  display.drawRect(x, y + 2, 15, 12, color);
  display.drawRect(x + 2, y, 8, 4, color);
  
  if (sdOK) {
    display.fillRect(x + 1, y + 3, 13, 10, color);
  }
}

void UIRenderer::renderCompassIcon(int x, int y, bool compassOK) {
  uint16_t color = compassOK ? display.getCurrentColors().gpsGood : display.getCurrentColors().gpsBad;
  
  // Compass circle
  display.drawCircle(x + 8, y + 8, 7, color);
  
  if (compassOK) {
    // Needle
    display.drawLine(x + 8, y + 3, x + 8, y + 13, color);
    display.fillTriangle(x + 6, y + 5, x + 10, y + 5, x + 8, y + 2, color);
  }
}

void UIRenderer::renderWiFiIcon(int x, int y, bool connected) {
  uint16_t color = connected ? display.getCurrentColors().gpsGood : display.getCurrentColors().gpsWarn;
  
  // WiFi waves
  display.drawCircle(x + 8, y + 12, 3, color);
  display.drawCircle(x + 8, y + 12, 6, color);
  display.drawCircle(x + 8, y + 12, 9, color);
  
  // Base point
  display.fillCircle(x + 8, y + 12, 2, color);
}

// ========================================
// HELPER FUNCTIONS
// ========================================

String UIRenderer::formatGPSTime(int hour, int minute, int second) {
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hour, minute, second);
  return String(timeStr);
}

uint16_t UIRenderer::getGPSStatusColor(const GPSData& gps) {
  if (!gps.isValid) return display.getCurrentColors().gpsBad;
  if (gps.hdop < 3.0) return display.getCurrentColors().gpsGood;
  if (gps.hdop < 10.0) return display.getCurrentColors().gpsWarn;
  return display.getCurrentColors().gpsBad;
}

uint16_t UIRenderer::getBatteryColor(float voltage) {
  if (voltage > BATTERY_GOOD) return display.getCurrentColors().gpsGood;
  if (voltage > BATTERY_LOW) return display.getCurrentColors().gpsWarn;
  return display.getCurrentColors().gpsBad;
}

int UIRenderer::getBatteryFillLevel(float voltage) {
  float range = BATTERY_FULL - BATTERY_CRITICAL;
  float position = (voltage - BATTERY_CRITICAL) / range;
  return (int)(position * 16);
}

String UIRenderer::getCardinalDirection(float heading) {
  const char* cardinals[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  int index = (int)((heading + 22.5) / 45.0) % 8;
  return String(cardinals[index]);
}