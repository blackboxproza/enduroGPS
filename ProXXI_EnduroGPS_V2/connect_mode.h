/*
 * E_NAVI EnduroGPS - Connect Mode Integration
 * Complete WiFi Connect Mode with QR Code Display and File Operations
 * 
 * Features: Connect mode state, QR code display, file re-indexing
 * User Flow: Menu -> Connect -> WiFi Active -> Done -> Re-index -> Ready
 */

#ifndef CONNECT_MODE_H
#define CONNECT_MODE_H

#include <Arduino.h>
#include "config.h"
#include "shared_data.h"
#include "display_manager.h"
#include "button_manager.h"
#include "wifi_manager.h"
#include "web_server.h"


// ========================================
// CONNECT MODE STATISTICS
// ========================================
struct ConnectModeStats {
  unsigned long sessionStartTime = 0;    // When connect mode started
  unsigned long totalSessionTime = 0;    // Total time in connect mode
  unsigned long clientConnections = 0;   // Number of client connections
  unsigned long filesUploaded = 0;       // Files uploaded this session
  unsigned long filesDownloaded = 0;     // Files downloaded this session
  unsigned long bytesTransferred = 0;    // Total bytes transferred
  String lastClientIP = "";              // Last connected client
  bool wasSuccessful = false;            // Session completed successfully
};

// ========================================
// CONNECT MODE MANAGER CLASS
// ========================================
class ConnectModeManager {
  private:
    // State Management
    ConnectModeState currentState = CONNECT_IDLE;
    ConnectModeState previousState = CONNECT_IDLE;
    unsigned long stateStartTime = 0;
    bool modeActive = false;
    
    // User Interface
    int currentScreen = 0;              // 0=QR, 1=Status, 2=Transfer
    unsigned long lastScreenSwitch = 0;
    unsigned long screenSwitchDelay = 8000; // 8 seconds per screen
    bool userRequestedDone = false;
    
    // File Operations
    bool needsReindexing = false;
    bool reindexingComplete = false;
    unsigned long reindexStartTime = 0;
    int filesFound = 0;
    int newFilesDetected = 0;
    
    // Statistics
    ConnectModeStats sessionStats;
    
    // Display Management
    void drawQRCodeScreen();
    void drawStatusScreen();
    void drawTransferScreen();
    void drawReindexingScreen();
    void drawCompleteScreen();
    void drawErrorScreen(const String& error);
    
    // State Management
    void changeState(ConnectModeState newState);
    void updateStateLogic();
    void checkForStateTransitions();
    
    // File Management
    void startFileReindexing();
    void updateReindexingProgress();
    int performFileReindexing();
    void detectNewFiles();
    
    // WiFi Management
    bool startWiFiServices();
    void stopWiFiServices();
    void updateWiFiStatus();
    
  public:
    ConnectModeManager();
    ~ConnectModeManager();

    bool initialize();                      // ADD THIS LINE ← NEW
    
    // Connect Mode Control
    bool startConnectMode();            // Enter connect mode
    void stopConnectMode();             // User requested stop
    void update();                      // Call in main loop
    bool isActive() const { return modeActive; }
    ConnectModeState getState() const { return currentState; }
    
    // User Interface
    void handleInput(ButtonID button, ButtonEvent event);
    void nextScreen();                  // Manual screen switching
    void showDoneButton();              // Show "Done" option
    bool isDoneRequested() const { return userRequestedDone; }
    
    // File Operations
    bool hasNewFiles() const { return newFilesDetected > 0; }
    int getNewFileCount() const { return newFilesDetected; }
    void markReindexingNeeded() { needsReindexing = true; }
    bool isReindexingComplete() const { return reindexingComplete; }
    
    // Statistics
    ConnectModeStats getSessionStats() const { return sessionStats; }
    String getSessionSummary();
    void resetSessionStats();
    
    // Status Information
    String getStatusText();
    String getConnectionInfo();
    float getSessionProgress();         // 0-100% progress through session
    
    // Error Handling
    bool hasError() const;
    String getLastError();
    void clearError();
};

// ========================================
// QR CODE DISPLAY SYSTEM
// ========================================
class QRCodeDisplay {
  private:
    bool qrGenerated = false;
    bool qrVisible = false;
    String qrData = "";
    
    // QR Code Generation (Simplified for MVP)
    void generateSimpleQR(const String& text);
    void drawQRPattern(int x, int y, int size);
    
  public:
    QRCodeDisplay();
    ~QRCodeDisplay();
    
    // QR Code Management
    bool generateWiFiQR(const String& ssid, const String& password);
    bool generateURLQR(const String& url);
    void showQRCode();
    void hideQRCode();
    bool isVisible() const { return qrVisible; }
    
    // Display Functions
    void drawQRCode(int x, int y, int size = 120);
    void drawQRWithInfo(int x, int y, const String& ssid, 
                       const String& password, const String& url);
    
    // Utility
    void clearQR();
    String getQRData() const { return qrData; }
};

// ========================================
// FILE REINDEXING SYSTEM
// ========================================
class FileReindexer {
  private:
    bool indexingActive = false;
    unsigned long indexStartTime = 0;
    int totalFiles = 0;
    int processedFiles = 0;
    int newFiles = 0;
    int updatedFiles = 0;
    String currentFile = "";

    
    // File Tracking
    String previousFileList[100];       // Store previous file list
    int previousFileCount = 0;
    bool hasStoredPreviousList = false;
    
  public:
    FileReindexer();
    ~FileReindexer();
    
    // Indexing Control
    bool startReindexing();
    void stopReindexing();
    bool isActive() const { return indexingActive; }
    
    // Progress Monitoring
    float getProgress();                // 0-100% complete
    int getTotalFiles() const { return totalFiles; }
    int getProcessedFiles() const { return processedFiles; }
    int getNewFilesFound() const { return newFiles; }
    String getCurrentFile() const { return currentFile; }
    
    // File Detection
    void storePreviousFileList();       // Call before connect mode
    void detectChanges();               // Detect new/changed files
    bool hasNewFiles() const { return newFiles > 0; }
    
    // Results
    String getIndexingSummary();
    void clearResults();

    bool performFileReindexing();  // ADD THIS DECLARATION
};

// ========================================
// CONNECT MODE UI SCREENS
// ========================================

// Screen 1: QR Code Display
void drawConnectQRScreen(const String& ssid, const String& password, const String& url);

// Screen 2: Connection Status
void drawConnectStatusScreen();

// Screen 3: File Reindexing
void drawReindexingScreen(float progress, const String& currentFile, int newFiles);

// Session Complete Screen
void drawSessionCompleteScreen(const ConnectModeStats& stats);

// ========================================
// GLOBAL INSTANCES
// ========================================
extern ConnectModeManager connectMode;
extern QRCodeDisplay qrDisplay;
extern FileReindexer fileReindexer;

// ========================================
// UTILITY FUNCTIONS
// ========================================

// Format helpers
String formatFileSize(unsigned long bytes);
String formatDuration(unsigned long milliseconds);
String formatTransferRate(unsigned long bytesPerSecond);

// Connection helpers
String generateRandomSSID();
bool isValidWiFiPassword(const String& password);
String getConnectionURL(const String& ip);

// File operation helpers
bool needsFileReindexing();
void triggerFileReindexing();
int countFilesInDirectory(const String& directory);

#endif // CONNECT_MODE_H 