/*
 * E_NAVI EnduroGPS - Connect Mode Implementation
 * Complete WiFi Connect Mode with QR Code Display and File Operations
 * 
 * Features: Connect mode state, QR code display, file re-indexing
 * User Flow: Menu -> Connect -> WiFi Active -> Done -> Re-index -> Ready
 */

#include "connect_mode.h"
#include "display_manager.h"
#include "track_management.h"
#include "wifi_manager.h"
#include "web_server.h"

// ========================================
// CONNECT MODE MANAGER IMPLEMENTATION
// ========================================

bool ConnectModeManager::initialize() {
    Serial.println("Connect Mode Manager: Initializing...");
    
    // Initialize state
    currentState = CONNECT_IDLE;
    modeActive = false;
    needsReindexing = false;
    
    // Reset statistics
    resetSessionStats();
    
    Serial.println("Connect Mode Manager: Ready");
    return true;
}

ConnectModeManager::ConnectModeManager() {
  currentState = CONNECT_IDLE;
  previousState = CONNECT_IDLE;
  stateStartTime = 0;
  modeActive = false;
  
  currentScreen = 0;
  lastScreenSwitch = 0;
  screenSwitchDelay = 8000;
  userRequestedDone = false;
  
  needsReindexing = false;
  reindexingComplete = false;
  reindexStartTime = 0;
  filesFound = 0;
  newFilesDetected = 0;
  
  memset(&sessionStats, 0, sizeof(sessionStats));
  
  Serial.println("Connect Mode: Manager initialized");
}

ConnectModeManager::~ConnectModeManager() {
  if (modeActive) {
    stopConnectMode();
  }
}

bool ConnectModeManager::startConnectMode() {
  if (modeActive) {
    Serial.println("Connect Mode: Already active");
    return true;
  }
  
  Serial.println("Connect Mode: Starting WiFi file transfer mode...");
  
  // Initialize session statistics
  sessionStats.sessionStartTime = millis();
  sessionStats.totalSessionTime = 0;
  sessionStats.clientConnections = 0;
  sessionStats.filesUploaded = 0;
  sessionStats.filesDownloaded = 0;
  sessionStats.bytesTransferred = 0;
  sessionStats.lastClientIP = "";
  sessionStats.wasSuccessful = false;
  
  // Store current file list for comparison
  fileReindexer.storePreviousFileList();
  
  // Start WiFi services
  if (!startWiFiServices()) {
    Serial.println("Connect Mode: Failed to start WiFi services");
    changeState(CONNECT_ERROR);
    return false;
  }
  
  // Enter connect mode
  modeActive = true;
  userRequestedDone = false;
  currentScreen = 0;
  lastScreenSwitch = millis();
  
  changeState(CONNECT_QR_DISPLAY);
  
  Serial.println("Connect Mode: Successfully started");
  Serial.printf("  SSID: %s\n", wifiManager.getSSID().c_str());
  Serial.printf("  URL: %s\n", wifiManager.getURL().c_str());
  
  return true;
}

void ConnectModeManager::stopConnectMode() {
  if (!modeActive) {
    return;
  }
  
  Serial.println("Connect Mode: Stopping...");
  
  // Update session duration
  sessionStats.totalSessionTime = millis() - sessionStats.sessionStartTime;
  sessionStats.wasSuccessful = true;
  
  // Stop WiFi services
  stopWiFiServices();
  
  // Check if file re-indexing is needed
  if (wifiManager.getStatistics().filesUploaded > 0) {
    needsReindexing = true;
    changeState(CONNECT_REINDEXING);
  } else {
    changeState(CONNECT_COMPLETE);
  }
  
  modeActive = false;
  userRequestedDone = false;
  
  Serial.printf("Connect Mode: Session completed (%lu seconds)\n", 
                sessionStats.totalSessionTime / 1000);
  Serial.printf("  Files uploaded: %lu\n", sessionStats.filesUploaded);
  Serial.printf("  Files downloaded: %lu\n", sessionStats.filesDownloaded);
  Serial.printf("  Data transferred: %lu bytes\n", sessionStats.bytesTransferred);
}

void ConnectModeManager::update() {
  if (!modeActive) return;
  
  unsigned long now = millis();
  
  // Update state logic
  updateStateLogic();
  
  // Check for state transitions
  checkForStateTransitions();
  
  // Auto-switch screens in QR/Active states
  if ((currentState == CONNECT_QR_DISPLAY || currentState == CONNECT_ACTIVE) &&
      (now - lastScreenSwitch > screenSwitchDelay)) {
    nextScreen();
    lastScreenSwitch = now;
  }
  
  // Update reindexing progress
  if (currentState == CONNECT_REINDEXING) {
    updateReindexingProgress();
  }
  
  // Update session statistics
  WiFiStats wifiStats = wifiManager.getStatistics();
  sessionStats.filesUploaded = wifiStats.filesUploaded;
  sessionStats.filesDownloaded = wifiStats.filesDownloaded;
  sessionStats.bytesTransferred = wifiStats.bytesUploaded + wifiStats.bytesDownloaded;
  sessionStats.clientConnections = wifiStats.totalConnections;
}

bool ConnectModeManager::startWiFiServices() {
  Serial.println("Connect Mode: Starting WiFi hotspot...");
  
  // Start WiFi hotspot
  if (!wifiManager.startHotspot()) {
    Serial.println("Connect Mode: Failed to start WiFi hotspot");
    return false;
  }
  
  // Start web server
  if (!webServer.start()) {
    Serial.println("Connect Mode: Failed to start web server");
    wifiManager.stopHotspot();
    return false;
  }
  
  // Generate and show QR code
  qrDisplay.generateWiFiQR(wifiManager.getSSID(), wifiManager.getPassword());
  
  Serial.println("Connect Mode: WiFi services started successfully");
  return true;
}

void ConnectModeManager::stopWiFiServices() {
  Serial.println("Connect Mode: Stopping WiFi services...");
  
  // Stop web server
  webServer.stop();
  
  // Stop WiFi hotspot
  wifiManager.stopHotspot();
  
  // Hide QR code
  qrDisplay.hideQRCode();
  
  Serial.println("Connect Mode: WiFi services stopped");
}

void ConnectModeManager::changeState(ConnectModeState newState) {
  if (currentState == newState) return;
  
  previousState = currentState;
  currentState = newState;
  stateStartTime = millis();
  
  Serial.printf("Connect Mode: State %d -> %d\n", previousState, currentState);
  
  // State entry actions
  switch (newState) {
    case CONNECT_QR_DISPLAY:
      currentScreen = 0; // Start with QR screen
      break;
      
    case CONNECT_ACTIVE:
      currentScreen = 1; // Switch to status screen
      break;
      
    case CONNECT_REINDEXING:
      startFileReindexing();
      break;
      
    case CONNECT_COMPLETE:
      reindexingComplete = true;
      break;
      
    case CONNECT_ERROR:
      stopWiFiServices();
      break;
  }
}

void ConnectModeManager::updateStateLogic() {
  unsigned long now = millis();
  unsigned long stateTime = now - stateStartTime;
  
  switch (currentState) {
    case CONNECT_QR_DISPLAY:
      // Check if clients have connected
      if (wifiManager.getConnectedClients() > 0) {
        changeState(CONNECT_ACTIVE);
      }
      break;
      
    case CONNECT_ACTIVE:
      // Monitor file transfers
      if (webServer.isTransferActive()) {
        changeState(CONNECT_FILE_TRANSFER);
      }
      break;
      
    case CONNECT_FILE_TRANSFER:
      // Return to active when transfer completes
      if (!webServer.isTransferActive()) {
        changeState(CONNECT_ACTIVE);
      }
      break;
      
    case CONNECT_REINDEXING:
      // Check if reindexing is complete
      if (!fileReindexer.isActive()) {
        detectNewFiles();
        changeState(CONNECT_COMPLETE);
      }
      break;
      
    case CONNECT_COMPLETE:
      // Auto-exit after showing results
      if (stateTime > 5000) { // 5 seconds
        modeActive = false;
      }
      break;
      
    case CONNECT_ERROR:
      // Auto-exit after showing error
      if (stateTime > 3000) { // 3 seconds
        modeActive = false;
      }
      break;
  }
}

void ConnectModeManager::checkForStateTransitions() {
  // Check for user requesting "Done"
  if (userRequestedDone && (currentState == CONNECT_QR_DISPLAY || 
                           currentState == CONNECT_ACTIVE ||
                           currentState == CONNECT_FILE_TRANSFER)) {
    changeState(CONNECT_STOPPING);
    stopConnectMode();
  }
  
  // Check for WiFi errors
  if (wifiManager.hasError() && currentState != CONNECT_ERROR) {
    changeState(CONNECT_ERROR);
  }
  
}

void ConnectModeManager::handleInput(ButtonID button, ButtonEvent event) {
  if (event != BTN_EVENT_CLICK) return;
  
  switch (button) {
    case BTN_CENTER:
      // "Done" button
      userRequestedDone = true;
      break;
      
    case BTN_UP:
    case BTN_DOWN:
      // Manual screen switching
      nextScreen();
      lastScreenSwitch = millis();
      break;
      
    case BTN_LEFT:
      // Cancel/Exit
      userRequestedDone = true;
      break;
  }
}

void ConnectModeManager::nextScreen() {
  currentScreen = (currentScreen + 1) % 3; // 0=QR, 1=Status, 2=Transfer
  
  // Update display based on current screen
  switch (currentScreen) {
    case 0:
      drawQRCodeScreen();
      break;
    case 1:
      drawStatusScreen();
      break;
    case 2:
      drawTransferScreen();
      break;
  }
}

// ========================================
// DISPLAY MANAGEMENT
// ========================================

void ConnectModeManager::drawQRCodeScreen() {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(30, "📱 CONNECT MODE", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // QR Code Area (simplified for now)
  qrDisplay.drawQRWithInfo(100, 70, wifiManager.getSSID(), 
                          wifiManager.getPassword(), wifiManager.getURL());
  
  // Instructions
  display.setTextSize(FONT_SIZE_SMALL);
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.drawCenteredText(350, "1. Connect phone to WiFi", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(370, "2. Open browser to URL", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(390, "3. Upload/Download files", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  
  // Footer
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(450, "[CENTER] Done  [UP/DOWN] Screens", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void ConnectModeManager::drawStatusScreen() {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(30, "📡 STATUS", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // Connection Status
  int y = 80;
  int lineHeight = 30;
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  
  display.setCursor(20, y);
  display.printf("WiFi: %s", wifiManager.hasClients() ? "CONNECTED" : "WAITING");
  y += lineHeight;
  
  display.setCursor(20, y);
  display.printf("Clients: %d", wifiManager.getConnectedClients());
  y += lineHeight;
  
  display.setCursor(20, y);
  display.printf("SSID: %s", wifiManager.getSSID().c_str());
  y += lineHeight;
  
  display.setCursor(20, y);
  display.printf("URL: %s", wifiManager.getURL().c_str());
  y += lineHeight;
  
  // Session Statistics
  y += 20;
  display.setTextSize(FONT_SIZE_SMALL);
  display.setTextColor(display.getCurrentColors().textSecondary);
  
  display.setCursor(20, y);
  display.printf("Uploads: %lu files", sessionStats.filesUploaded);
  y += 20;
  
  display.setCursor(20, y);
  display.printf("Downloads: %lu files", sessionStats.filesDownloaded);
  y += 20;
  
  display.setCursor(20, y);
  display.printf("Data: %s", formatFileSize(sessionStats.bytesTransferred).c_str());
  
  // Footer
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(450, "[CENTER] Done  [UP/DOWN] Screens", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void ConnectModeManager::drawTransferScreen() {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(30, "📁 TRANSFERS", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // Transfer Status
  FileTransferStatus transfer = webServer.getCurrentTransfer();

  if (transfer.transferActive) {
    // Active transfer
    display.setTextSize(FONT_SIZE_MEDIUM);
    display.setTextColor(display.getCurrentColors().text);
    display.drawCenteredText(120, transfer.isUpload ? "⬆️ Uploading" : "⬇️ Downloading", 
                            display.getCurrentColors().text, FONT_SIZE_MEDIUM);
    
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawCenteredText(150, transfer.filename, 
                            display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
    
    // Progress bar
    int barWidth = 280;
    int barX = (SCREEN_WIDTH - barWidth) / 2;
    int barY = 180;
    
    display.drawRect(barX, barY, barWidth, 20, display.getCurrentColors().text);
    int fillWidth = (transfer.progressPercent * barWidth) / 100.0;
    display.fillRect(barX + 2, barY + 2, fillWidth - 4, 16, display.getCurrentColors().accent);
    
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawCenteredText(210, String((int)transfer.progressPercent) + "%", 
                            display.getCurrentColors().text, FONT_SIZE_SMALL);
  } else {
    // No active transfer
    display.setTextSize(FONT_SIZE_MEDIUM);
    display.setTextColor(display.getCurrentColors().textSecondary);
    display.drawCenteredText(150, "No active transfers", 
                            display.getCurrentColors().textSecondary, FONT_SIZE_MEDIUM);
    
    // Show recent activity
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawCenteredText(200, "Ready for file operations", 
                            display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  }
  
  // Footer
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(450, "[CENTER] Done  [UP/DOWN] Screens", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void ConnectModeManager::drawReindexingScreen() {
  display.clearScreen();
  
  float progress = fileReindexer.getProgress();
  String currentFile = fileReindexer.getCurrentFile();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(50, "🔄 UPDATING", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // Progress Bar
  int barWidth = 280;
  int barX = (SCREEN_WIDTH - barWidth) / 2;
  int barY = 120;
  
  display.drawRect(barX, barY, barWidth, 25, display.getCurrentColors().text);
  int fillWidth = (progress * barWidth) / 100.0;
  display.fillRect(barX + 2, barY + 2, fillWidth - 4, 21, display.getCurrentColors().recording);
  
  // Progress Text
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(barY + 35, String((int)progress) + "% Complete", 
                          display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Current File
  if (currentFile.length() > 0) {
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawCenteredText(180, "Processing: " + currentFile, 
                            display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  }
  
  // Status message
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(220, "Updating file index...", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
}

void ConnectModeManager::drawCompleteScreen() {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().recording);
  display.drawCenteredText(50, "✅ COMPLETE", display.getCurrentColors().recording, FONT_SIZE_LARGE);
  
  // Summary
  int y = 120;
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  display.drawCenteredText(y, "Transfer Session Summary:", display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  y += 40;
  display.setTextSize(FONT_SIZE_SMALL);
  display.setCursor(50, y);
  display.printf("Uploads: %lu files", sessionStats.filesUploaded);
  y += 25;
  
  display.setCursor(50, y);
  display.printf("Downloads: %lu files", sessionStats.filesDownloaded);
  y += 25;
  
  display.setCursor(50, y);
  display.printf("Data: %s", formatFileSize(sessionStats.bytesTransferred).c_str());
  y += 25;
  
  if (newFilesDetected > 0) {
    y += 10;
    display.setTextColor(display.getCurrentColors().recording);
    display.setCursor(50, y);
    display.printf("New files detected: %d", newFilesDetected);
  }
  
  // Ready message
  y += 50;
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y, "🏍️ Ready for Adventure!", 
                          display.getCurrentColors().accent, FONT_SIZE_MEDIUM);
}

void ConnectModeManager::drawErrorScreen(const String& error) {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().gpsBad);
  display.drawCenteredText(50, "❌ ERROR", display.getCurrentColors().gpsBad, FONT_SIZE_LARGE);
  
  // Error message
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  display.drawCenteredText(150, error, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Instructions
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(200, "Connect mode failed to start", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(220, "Returning to main menu...", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
}

// ========================================
// FILE REINDEXING MANAGEMENT
// ========================================

void ConnectModeManager::startFileReindexing() {
  Serial.println("Connect Mode: Starting file reindexing...");
  
  reindexStartTime = millis();
  needsReindexing = true;
  reindexingComplete = false;
  
  // Start the reindexing process
  fileReindexer.startReindexing();
}

void ConnectModeManager::updateReindexingProgress() {
  // The file reindexer handles its own progress
  // This function can add any additional logic needed
}

void ConnectModeManager::detectNewFiles() {
  // Compare with stored file list to detect new files
  fileReindexer.detectChanges();
  newFilesDetected = fileReindexer.getNewFilesFound();
  
  Serial.printf("Connect Mode: Detected %d new files\n", newFilesDetected);
}

// ========================================
// STATUS AND INFORMATION
// ========================================

String ConnectModeManager::getStatusText() {
  switch (currentState) {
    case CONNECT_IDLE:
      return "Idle";
    case CONNECT_STARTING:
      return "Starting...";
    case CONNECT_QR_DISPLAY:
      return "Showing QR Code";
    case CONNECT_ACTIVE:
      return "WiFi Active";
    case CONNECT_FILE_TRANSFER:
      return "File Transfer";
    case CONNECT_STOPPING:
      return "Stopping...";
    case CONNECT_REINDEXING:
      return "Updating Files";
    case CONNECT_COMPLETE:
      return "Complete";
    case CONNECT_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

String ConnectModeManager::getConnectionInfo() {
  if (!modeActive) return "Not active";
  
  String info = "SSID: " + wifiManager.getSSID() + "\n";
  info += "Password: " + wifiManager.getPassword() + "\n";
  info += "URL: " + wifiManager.getURL() + "\n";
  info += "Clients: " + String(wifiManager.getConnectedClients()) + "\n";
  
  return info;
}

float ConnectModeManager::getSessionProgress() {
  // Return progress based on current state
  switch (currentState) {
    case CONNECT_IDLE:
      return 0.0;
    case CONNECT_STARTING:
      return 10.0;
    case CONNECT_QR_DISPLAY:
      return 25.0;
    case CONNECT_ACTIVE:
      return 50.0;
    case CONNECT_FILE_TRANSFER:
      return 75.0;
    case CONNECT_STOPPING:
      return 85.0;
    case CONNECT_REINDEXING:
      return 90.0 + (fileReindexer.getProgress() * 0.1);
    case CONNECT_COMPLETE:
      return 100.0;
    default:
      return 0.0;
  }
}

String ConnectModeManager::getSessionSummary() {
  String summary = "Connect Mode Session:\n";
  summary += "Duration: " + formatDuration(sessionStats.totalSessionTime) + "\n";
  summary += "Files uploaded: " + String(sessionStats.filesUploaded) + "\n";
  summary += "Files downloaded: " + String(sessionStats.filesDownloaded) + "\n";
  summary += "Data transferred: " + formatFileSize(sessionStats.bytesTransferred) + "\n";
  summary += "Client connections: " + String(sessionStats.clientConnections) + "\n";
  summary += "New files detected: " + String(newFilesDetected) + "\n";
  summary += "Status: " + String(sessionStats.wasSuccessful ? "Success" : "Failed");
  
  return summary;
}

void ConnectModeManager::resetSessionStats() {
  memset(&sessionStats, 0, sizeof(sessionStats));
  newFilesDetected = 0;
  needsReindexing = false;
  reindexingComplete = false;
}

bool ConnectModeManager::hasError() const {
  return currentState == CONNECT_ERROR;
}

String ConnectModeManager::getLastError() {
  if (wifiManager.hasError()) {
    return wifiManager.getLastError();
  }
  return "No error";
}

void ConnectModeManager::clearError() {
  if (currentState == CONNECT_ERROR) {
    changeState(CONNECT_IDLE);
  }
  wifiManager.clearError();
}

// ========================================
// QR CODE DISPLAY IMPLEMENTATION
// ========================================

QRCodeDisplay::QRCodeDisplay() {
  qrGenerated = false;
  qrVisible = false;
  qrData = "";
}

QRCodeDisplay::~QRCodeDisplay() {
  clearQR();
}

bool QRCodeDisplay::generateWiFiQR(const String& ssid, const String& password) {
  qrData = "WIFI:T:WPA;S:" + ssid + ";P:" + password + ";;";
  qrGenerated = true;
  
  Serial.printf("QR Code: Generated WiFi QR for %s\n", ssid.c_str());
  return true;
}

bool QRCodeDisplay::generateURLQR(const String& url) {
  qrData = url;
  qrGenerated = true;
  
  Serial.printf("QR Code: Generated URL QR for %s\n", url.c_str());
  return true;
}

void QRCodeDisplay::showQRCode() {
  qrVisible = true;
}

void QRCodeDisplay::hideQRCode() {
  qrVisible = false;
}

void QRCodeDisplay::drawQRCode(int x, int y, int size) {
  if (!qrGenerated) return;
  
  // For MVP, draw a simple pattern representing QR code
  display.drawRect(x, y, size, size, display.getCurrentColors().text);
  display.fillRect(x + 2, y + 2, size - 4, size - 4, display.getCurrentColors().background);
  
  // Draw QR pattern simulation (simple grid)
  for (int i = 8; i < size - 8; i += 8) {
    for (int j = 8; j < size - 8; j += 8) {
      if ((i + j) % 16 == 0) {
        display.fillRect(x + i, y + j, 6, 6, display.getCurrentColors().text);
      }
    }
  }
  
  // Corner markers
  display.fillRect(x + 4, y + 4, 16, 16, display.getCurrentColors().text);
  display.fillRect(x + size - 20, y + 4, 16, 16, display.getCurrentColors().text);
  display.fillRect(x + 4, y + size - 20, 16, 16, display.getCurrentColors().text);
}

void QRCodeDisplay::drawQRWithInfo(int x, int y, const String& ssid, 
                                  const String& password, const String& url) {
  int qrSize = 120;
  
  // Draw QR code
  drawQRCode(x, y, qrSize);
  
  // Draw connection information below QR code
  int infoY = y + qrSize + 20;
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  display.drawCenteredText(infoY, "WiFi: " + ssid, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(infoY + 25, "Password: " + password, 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(infoY + 45, "URL: " + url, 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
}

void QRCodeDisplay::clearQR() {
  qrGenerated = false;
  qrVisible = false;
  qrData = "";
}

// ========================================
// FILE REINDEXER IMPLEMENTATION
// ========================================

FileReindexer::FileReindexer() {
  indexingActive = false;
  indexStartTime = 0;
  totalFiles = 0;
  processedFiles = 0;
  newFiles = 0;
  updatedFiles = 0;
  currentFile = "";
  previousFileCount = 0;
  hasStoredPreviousList = false;
}

FileReindexer::~FileReindexer() {
  stopReindexing();
}

bool FileReindexer::startReindexing() {
  if (indexingActive) {
    Serial.println("File Reindexer: Already active");
    return true;
  }
  
  Serial.println("File Reindexer: Starting file reindexing...");
  
  indexingActive = true;
  indexStartTime = millis();
  processedFiles = 0;
  newFiles = 0;
  updatedFiles = 0;
  currentFile = "";
  
  // Count total files to process
  totalFiles = countFilesInDirectory(DIR_TRACKS);
  
  Serial.printf("File Reindexer: Processing %d files\n", totalFiles);
  
  // Start actual reindexing process (simplified for MVP)
  bool success = performFileReindexing();
  
  indexingActive = false;
  
  Serial.printf("File Reindexer: Completed in %lu ms\n", 
                millis() - indexStartTime);
  
  return success;
}

void FileReindexer::stopReindexing() {
  indexingActive = false;
}

float FileReindexer::getProgress() {
  if (totalFiles == 0) return 100.0;
  return (processedFiles * 100.0) / totalFiles;
}

void FileReindexer::storePreviousFileList() {
  Serial.println("File Reindexer: Storing previous file list...");
  
  previousFileCount = 0;
  hasStoredPreviousList = false;
  
  // Open tracks directory
  File root = SD.open(DIR_TRACKS);
  if (!root) {
    Serial.println("File Reindexer: Failed to open tracks directory");
    return;
  }
  
  // Store current file list
  File file = root.openNextFile();
  while (file && previousFileCount < 100) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".gpx") || filename.endsWith(".kml")) {
        previousFileList[previousFileCount] = filename;
        previousFileCount++;
      }
    }
    file = root.openNextFile();
  }
  root.close();
  
  hasStoredPreviousList = true;
  
  Serial.printf("File Reindexer: Stored %d previous files\n", previousFileCount);
}

void FileReindexer::detectChanges() {
  if (!hasStoredPreviousList) {
    Serial.println("File Reindexer: No previous file list stored");
    return;
  }
  
  Serial.println("File Reindexer: Detecting file changes...");
  
  newFiles = 0;
  updatedFiles = 0;
  
  // Get current file list
  File root = SD.open(DIR_TRACKS);
  if (!root) {
    Serial.println("File Reindexer: Failed to open tracks directory");
    return;
  }
  
  // Check each current file against previous list
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".gpx") || filename.endsWith(".kml")) {
        
        // Check if this file was in previous list
        bool foundInPrevious = false;
        for (int i = 0; i < previousFileCount; i++) {
          if (previousFileList[i] == filename) {
            foundInPrevious = true;
            break;
          }
        }
        
        if (!foundInPrevious) {
          newFiles++;
          Serial.printf("File Reindexer: New file detected - %s\n", filename.c_str());
        }
      }
    }
    file = root.openNextFile();
  }
  root.close();
  
  Serial.printf("File Reindexer: Detected %d new files, %d updated files\n", 
                newFiles, updatedFiles);
}

bool FileReindexer::performFileReindexing() {
  Serial.println("File Reindexer: Performing file system reindex...");
  
  // Open tracks directory
  File root = SD.open(DIR_TRACKS);
  if (!root) {
    Serial.println("File Reindexer: Failed to open tracks directory");
    return false;
  }
  
  processedFiles = 0;
  
  // Process each file
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".gpx") || filename.endsWith(".kml")) {
        
        currentFile = filename;
        
        // Simulate processing time
        delay(50); // 50ms per file
        
        processedFiles++;
        
        // Log progress every 10 files
        if (processedFiles % 10 == 0) {
          Serial.printf("File Reindexer: Processed %d/%d files\n\n", 
                        processedFiles, totalFiles);
        }
      }
    }
    file = root.openNextFile();
  }
  root.close();
  
  currentFile = "";
  
  Serial.printf("File Reindexer: Reindexing complete - processed %d files\n", 
                processedFiles);
  
  return true;
}

String FileReindexer::getIndexingSummary() {
  String summary = "File Reindexing Results:\n";
  summary += "Files processed: " + String(processedFiles) + "/" + String(totalFiles) + "\n";
  summary += "New files found: " + String(newFiles) + "\n";
  summary += "Updated files: " + String(updatedFiles) + "\n";
  
  if (indexStartTime > 0) {
    unsigned long duration = millis() - indexStartTime;
    summary += "Processing time: " + String(duration) + "ms\n";
  }
  
  return summary;
}

void FileReindexer::clearResults() {
  totalFiles = 0;
  processedFiles = 0;
  newFiles = 0;
  updatedFiles = 0;
  currentFile = "";
  indexStartTime = 0;
}

// ========================================
// GLOBAL INSTANCES
// ========================================

ConnectModeManager connectMode;
QRCodeDisplay qrDisplay;
FileReindexer fileReindexer;

// ========================================
// UTILITY FUNCTIONS
// ========================================

String formatFileSize(unsigned long bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < 1048576) return String(bytes / 1024.0, 1) + " KB";
  else if (bytes < 1073741824) return String(bytes / 1048576.0, 1) + " MB";
  else return String(bytes / 1073741824.0, 1) + " GB";
}

String formatDuration(unsigned long milliseconds) {
  unsigned long seconds = milliseconds / 1000;
  if (seconds < 60) return String(seconds) + "s";
  if (seconds < 3600) return String(seconds / 60) + "m " + String(seconds % 60) + "s";
  return String(seconds / 3600) + "h " + String((seconds % 3600) / 60) + "m";
}

String formatTransferRate(unsigned long bytesPerSecond) {
  if (bytesPerSecond < 1024) return String(bytesPerSecond) + " B/s";
  if (bytesPerSecond < 1048576) return String(bytesPerSecond / 1024) + " KB/s";
  return String(bytesPerSecond / 1048576) + " MB/s";
}

String generateRandomSSID() {
  String ssid = "E_NAVI_";
  for (int i = 0; i < 4; i++) {
    ssid += String(random(0, 16), HEX);
  }
  ssid.toUpperCase();
  return ssid;
}

bool isValidWiFiPassword(const String& password) {
  return password.length() >= 8 && password.length() <= 63;
}

String getConnectionURL(const String& ip) {
  return "http://" + ip + "/";
}

bool needsFileReindexing() {
  // Check if new files have been uploaded
  WiFiStats stats = wifiManager.getStatistics();
  return stats.filesUploaded > 0;
}

void triggerFileReindexing() {
  fileReindexer.startReindexing();
}

int countFilesInDirectory(const String& directory) {
  int count = 0;
  
  File root = SD.open(directory);
  if (!root) {
    Serial.printf("Failed to open directory: %s\n", directory.c_str());
    return 0;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".gpx") || filename.endsWith(".kml") || 
          filename.endsWith(".json") || filename.endsWith(".txt")) {
        count++;
      }
    }
    file = root.openNextFile();
  }
  root.close();
  
  return count;
}

// ========================================
// CONNECT MODE UI SCREEN IMPLEMENTATIONS
// ========================================

void drawConnectQRScreen(const String& ssid, const String& password, const String& url) {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(30, "📱 CONNECT MODE", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // QR Code Area
  qrDisplay.drawQRWithInfo(100, 70, ssid, password, url);
  
  // Instructions
  int infoY = 250;
  display.setTextSize(FONT_SIZE_SMALL);
  display.setTextColor(display.getCurrentColors().textSecondary);
  
  display.drawCenteredText(infoY, "1. Connect phone to WiFi network", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(infoY + 20, "2. Open browser to URL shown", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(infoY + 40, "3. Upload/Download GPX files", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(infoY + 60, "4. Press [CENTER] when done", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  
  // Footer
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(450, "[CENTER] Done  [UP/DOWN] Screens", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void drawConnectStatusScreen() {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(30, "📡 CONNECTION STATUS", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // WiFi Status
  WiFiStats stats = wifiManager.getStatistics();
  int y = 80;
  int lineHeight = 25;
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  
  // Connection Info
  display.setCursor(20, y);
  display.print("WiFi: ");
  display.setTextColor(wifiManager.hasClients() ? display.getCurrentColors().recording : 
                                                  display.getCurrentColors().gpsWarn);
  display.print(wifiManager.hasClients() ? "CONNECTED" : "WAITING");
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(20, y);
  display.printf("Clients: %d/%d", wifiManager.getConnectedClients(), 4);
  y += lineHeight;
  
  display.setCursor(20, y);
  display.printf("SSID: %s", wifiManager.getSSID().c_str());
  y += lineHeight;
  
  display.setCursor(20, y);
  display.printf("URL: %s", wifiManager.getURL().c_str());
  y += lineHeight;
  
  // Session Statistics
  y += 20;
  display.setTextSize(FONT_SIZE_SMALL);
  display.setTextColor(display.getCurrentColors().textSecondary);
  
  display.setCursor(20, y);
  display.print("Session Statistics:");
  y += 20;
  
  display.setCursor(20, y);
  display.printf("Uploads: %lu files", stats.filesUploaded);
  y += 18;
  
  display.setCursor(20, y);
  display.printf("Downloads: %lu files", stats.filesDownloaded);
  y += 18;
  
  display.setCursor(20, y);
  display.printf("Data: %s", formatFileSize(stats.bytesUploaded + stats.bytesDownloaded).c_str());
  y += 18;
  
  unsigned long uptime = (millis() - stats.lastConnectionTime) / 1000;
  display.setCursor(20, y);
  display.printf("Uptime: %s", formatDuration(uptime * 1000).c_str());
  
  // Footer
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(450, "[CENTER] Done  [UP/DOWN] Screens", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void drawReindexingScreen(float progress, const String& currentFile, int newFiles) {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(50, "🔄 UPDATING FILES", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // Progress Bar
  int barWidth = 280;
  int barX = (SCREEN_WIDTH - barWidth) / 2;
  int barY = 120;
  int barHeight = 25;
  
  display.drawRect(barX, barY, barWidth, barHeight, display.getCurrentColors().text);
  
  int fillWidth = (progress * barWidth) / 100.0;
  display.fillRect(barX + 2, barY + 2, fillWidth - 4, barHeight - 4, 
                  display.getCurrentColors().recording);
  
  // Progress Text
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  display.drawCenteredText(barY + 35, String((int)progress) + "% Complete", 
                          display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Current File
  if (currentFile.length() > 0) {
    display.setTextSize(FONT_SIZE_SMALL);
    display.drawCenteredText(180, "Processing: " + currentFile, 
                            display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  }
  
  // Results
  int y = 220;
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  
  if (newFiles > 0) {
    display.drawCenteredText(y, "✅ " + String(newFiles) + " new files found!", 
                            display.getCurrentColors().recording, FONT_SIZE_MEDIUM);
  } else {
    display.drawCenteredText(y, "No new files detected", 
                            display.getCurrentColors().textSecondary, FONT_SIZE_MEDIUM);
  }
  
  // Instructions
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(y + 40, "Updating fileindex...", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  display.drawCenteredText(y + 60, "Please wait...", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
}

void drawSessionCompleteScreen(const ConnectModeStats& stats) {
  display.clearScreen();
  
  // Header
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().recording);
  display.drawCenteredText(50, "✅ SESSION COMPLETE", display.getCurrentColors().recording, FONT_SIZE_LARGE);
  
  // Summary
  int y = 120;
  int lineHeight = 22;
  
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  
  display.drawCenteredText(y, "File Transfer Summary:", display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  y += 35;
  
  display.setTextSize(FONT_SIZE_SMALL);
  display.setCursor(40, y);
  display.printf("• Files Uploaded: %lu", stats.filesUploaded);
  y += lineHeight;
  
  display.setCursor(40, y);
  display.printf("• Files Downloaded: %lu", stats.filesDownloaded);
  y += lineHeight;
  
  display.setCursor(40, y);
  display.printf("• Data Transferred: %s", formatFileSize(stats.bytesTransferred).c_str());
  y += lineHeight;
  
  display.setCursor(40, y);
  display.printf("• Session Duration: %s", formatDuration(stats.totalSessionTime).c_str());
  y += lineHeight;
  
  display.setCursor(40, y);
  display.printf("• Connections: %lu", stats.clientConnections);
  
  // Ready message
  y += 50;
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.drawCenteredText(y, "🏍️ Ready for Adventure!", 
                          display.getCurrentColors().accent, FONT_SIZE_MEDIUM);
  
  // Continue instruction
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(450, "[CENTER] Continue", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

// ========================================
// ADDITIONAL HELPER FUNCTIONS
// ========================================

bool validateConnectModeState() {
  // Validate that all required required systems are available
  if (!wifiManager.isActive()) {
    Serial.println("Connect Mode: WiFi manager not ready");
    return false;
  }
  
  if (!webServer.isRunning()) {
    Serial.println("Connect Mode: Web server not running");
    return false;
  }
  
  if (!SD.begin()) {
    Serial.println("Connect Mode: SD card not available");
    return false;
  }
  
  return true;
}

void cleanupConnectModeResources() {
  // Clean up any temporary files or resources
  File tempDir = SD.open(UPLOAD_TEMP_DIR);
  if (tempDir) {
    File file = tempDir.openNextFile();
    while (file) {
      String filename = file.name();
      file.close();
      
      String fullPath = String(UPLOAD_TEMP_DIR) + "/" + filename;
      if (SD.remove(fullPath)) {
        Serial.printf("Cleaned temp file: %s\n", filename.c_str());
      }
      
      file = tempDir.openNextFile();
    }
    tempDir.close();
  }
}

bool createRequiredDirectories() {
  // Ensure all required directories exist
  if (!SD.exists(DIR_TRACKS)) {
    if (SD.mkdir(DIR_TRACKS)) {
      Serial.printf("Created directory: %s\n", DIR_TRACKS);
    } else {
      Serial.printf("Failed to create directory: %s\n", DIR_TRACKS);
      return false;
    }
  }
  
  if (!SD.exists(UPLOAD_TEMP_DIR)) {
    if (SD.mkdir(UPLOAD_TEMP_DIR)) {
      Serial.printf("Created directory: %s\n", UPLOAD_TEMP_DIR);
    } else {
      Serial.printf("Failed to create directory: %s\n", UPLOAD_TEMP_DIR);
      return false;
    }
  }
  
  return true;
}

String getConnectModeStatusReport() {
  String report = "=== CONNECT MODE STATUS REPORT ===\n";
  
  report += "Connect Mode: " + String(connectMode.isActive() ? "ACTIVE" : "INACTIVE") + "\n";
  report += "Current State: " + connectMode.getStatusText() + "\n";
  
  if (connectMode.isActive()) {
    report += "Session Progress: " + String(connectMode.getSessionProgress(), 1) + "%\n";
    report += "\nWiFi Status:\n";
    report += "  SSID: " + wifiManager.getSSID() + "\n";
    report += "  Clients: " + String(wifiManager.getConnectedClients()) + "\n";
    report += "  URL: " + wifiManager.getURL() + "\n";
    
    report += "\nWeb Server:\n";
    report += "  Running: " + String(webServer.isRunning() ? "YES" : "NO") + "\n";
    report += "  Active Sessions: " + String(webServer.getActiveSessionCount()) + "\n";
    report += "  Total Requests: " + String(webServer.getTotalRequests()) + "\n";
    
    if (webServer.isTransferActive()) {
      FileTransferStatus transfer = webServer.getCurrentTransfer();
      report += "\nActive Transfer:\n";
      report += "  File: " + transfer.filename + "\n";
      report += "  Type: " + String(transfer.isUpload ? "Upload" : "Download") + "\n";
      report += "  Progress: " + String(transfer.progressPercent, 1) + "%\n";
    }
    
    report += "\nFile Reindexing:\n";
    report += "  Active: " + String(fileReindexer.isActive() ? "YES" : "NO") + "\n";
    if (fileReindexer.isActive()) {
      report += "  Progress: " + String(fileReindexer.getProgress(), 1) + "%\n";
      report += "  Current File: " + fileReindexer.getCurrentFile() + "\n";
    }
    report += "  New Files Found: " + String(fileReindexer.getNewFilesFound()) + "\n";
  }
  
  report += "\nSession Statistics:\n";
  ConnectModeStats stats = connectMode.getSessionStats();
  report += "  Duration: " + formatDuration(stats.totalSessionTime) + "\n";
  report += "  Files Uploaded: " + String(stats.filesUploaded) + "\n";
  report += "  Files Downloaded: " + String(stats.filesDownloaded) + "\n";
  report += "  Data Transferred: " + formatFileSize(stats.bytesTransferred) + "\n";
  
  report += "================================\n";
  
  return report;
}

void printConnectModeDebugInfo() {
  Serial.println(getConnectModeStatusReport());
}

// Initialize connect mode system
bool initializeConnectModeSystem() {
  Serial.println("Initializing Connect Mode System...");
  
  // Create required directories
  if (!createRequiredDirectories()) {
    Serial.println("Failed to create required directories");
    return false;
  }
  
  // Initialize components
  if (!wifiManager.initialize()) {
    Serial.println("Failed to initialize WiFi manager");
    return false;
  }
  
  if (!webServer.initialize()) {
    Serial.println("Failed to initialize web server");
    return false;
  }
  
  Serial.println("Connect Mode System initialized successfully");
  return true;
}

// Shutdown connect mode system
void shutdownConnectModeSystem() {
  Serial.println("Shutting down Connect Mode System...");
  
  if (connectMode.isActive()) {
    connectMode.stopConnectMode();
  }
  
  cleanupConnectModeResources();
  
  Serial.println("Connect Mode System shutdown complete");
}