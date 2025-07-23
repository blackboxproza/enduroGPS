/*
 * E_NAVI EnduroGPS - UI Renderer Header
 * Complete UI rendering system for vision-optimized interface
 * 
 * Features: Menu screens, calibration UI, track manager, connect mode,
 *          QR code display, vision-optimized fonts and colors
 * 
 * MATCHES THE ACTUAL IMPLEMENTATION IN ui_renderer.cpp
 */

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <Arduino.h>
#include "config.h"
#include "shared_data.h"

// Forward declarations to avoid circular includes
class DisplayManager;
class MenuManager;
class BatteryMonitor;
class TrackManager;
class ConnectModeManager;
class WiFiManager;

// ========================================
// MISSING STATE TYPES (COMPILER ERROR FIXES)
// ========================================

struct CalibrationData {
  CalibrationState state = CALIBRATION_STATE_IDLE;
  int samplesCollected = 0;     // Number of samples collected
  int samplesRequired = 100;    // Total samples needed
  float progress = 0.0;         // Progress percentage (0.0 - 1.0)
  float offsetX = 0.0;          // X-axis offset
  float offsetY = 0.0;          // Y-axis offset  
  float offsetZ = 0.0;          // Z-axis offset
  float quality = 0.0;          // Calibration quality (0.0 - 1.0)
  unsigned long startTime = 0;  // Calibration start time
  bool isValid = false;         // True if calibration data is valid
  String errorMessage = "";     // Error message if failed
};

enum TrackManagerState {
  TRACK_MANAGER_STATE_LIST,    // Showing track list
  TRACK_MANAGER_STATE_DETAILS, // Showing track details
  TRACK_MANAGER_STATE_DELETE,  // Confirming track deletion
  TRACK_MANAGER_STATE_EXPORT,  // Exporting track
  TRACK_MANAGER_STATE_ERROR    // Error state
};


struct TrackInfo {
  String name = "";            // Track name
  String filename = "";        // File name
  unsigned long size = 0;      // File size in bytes
  unsigned long created = 0;   // Creation timestamp
  float distance = 0.0;        // Track distance (km)
  unsigned long duration = 0;  // Track duration (ms)
  int points = 0;              // Number of track points
  bool isValid = false;        // Track is valid
};

// ========================================
// UI RENDERER CLASS (from actual implementation)
// ========================================

class UIRenderer {
  private:
    // ========================================
    // CORE STATE (from actual implementation)
    // ========================================
    
    bool initialized = false;
    DisplayManager* display = nullptr;  // Reference to display manager
    
    // ========================================
    // PERFORMANCE TRACKING (from actual implementation)
    // ========================================
    
    unsigned long lastFrameTime = 0;
    unsigned long frameCount = 0;
    float averageFPS = 0.0;
    bool screenDirty = true;
    
    // ========================================
    // LAYOUT CONSTANTS (from actual implementation)
    // ========================================
    
    struct UILayout {
      int contentY = 60;
      int contentHeight = 380;
      // Add other layout constants as needed
    } layout;
    
    // ========================================
    // PRIVATE METHODS (from actual implementation)
    // ========================================
    
    // Performance and state
    void updatePerformanceMetrics();
    bool hasDataChanged(const GPSData& gps, const CompassData& compass, 
                       const TrackingData& tracking, const SystemStatus& system);
    
    // Main screen components
    void renderStatusBar(const GPSData& gps, const SystemStatus& system);
    void renderMainContent(const GPSData& gps, const CompassData& compass, const TrackingData& tracking);
    void renderBottomBar(const GPSData& gps, const CompassData& compass, const SystemStatus& system);
    void renderRecordingIndicators(const TrackingData& tracking);
    
    // Calibration screens
    void renderCalibrationWaiting(int y);
    void renderCalibrationProgress(int y, const CalibrationData& data);
    void renderCalibrationComplete(int y);
    void renderCalibrationFailed(int y);
    
    // Connect mode screens
    void renderWiFiStarting(int y);
    void renderWiFiActive(int y);
    void renderConnectModeError(int y);
    
    // QR Code system (from actual implementation)
    void renderQRCodeDisplay(int y);
    void renderQRPattern(int x, int y, int size);
    void renderQRFinderPattern(int x, int y, uint16_t black, uint16_t white);
    
    // Utility functions (from actual implementation)
    String formatTime(unsigned long seconds);
    String formatBytes(unsigned long bytes);
    
  public:
    // ========================================
    // CONSTRUCTOR & INITIALIZATION (from actual implementation)
    // ========================================
    
    UIRenderer();
    ~UIRenderer();
    
    bool initialize();  // Initialize with display manager from global
    bool isInitialized() const { return initialized; }
    
    // ========================================
    // MAIN RENDERING FUNCTIONS (from actual implementation)
    // ========================================
    
    void renderMainScreen();                    // Main navigation screen
    void renderMenuScreen();                    // Menu system
    void renderCalibrationScreen();             // Compass calibration
    void renderTrackManagerScreen();            // Track management
    void renderConnectModeScreen();             // WiFi connect mode
    
    // ========================================
    // SCREEN STATE MANAGEMENT (from actual implementation)
    // ========================================
    
    void setScreenDirty(bool dirty = true) { screenDirty = dirty; }
    bool isScreenDirty() const { return screenDirty; }
    
    // ========================================
    // PERFORMANCE METRICS (from actual implementation)
    // ========================================
    
    float getAverageFPS() const { return averageFPS; }
    unsigned long getFrameCount() const { return frameCount; }
    
    // ========================================
    // UTILITY METHODS (from actual implementation)
    // ========================================
    
    // These might be called by other components for formatting
    static String formatTimeString(unsigned long seconds);
    static String formatBytesString(unsigned long bytes);
};

// ========================================
// GLOBAL INSTANCE (implied from usage in .cpp files)
// ========================================

extern UIRenderer uiRenderer;

// ========================================
// GLOBAL ACCESS FUNCTIONS (for compatibility)
// ========================================

// These functions provide access to the renderer for other components
bool initializeUIRenderer();                   // Initialize UI renderer
void updateUI();                              // Update UI rendering
void renderCurrentScreen();                   // Render current screen

// Screen-specific rendering functions
void renderMainScreen();                      // Render main navigation screen
void renderMenuScreen();                      // Render menu system
void renderCalibrationScreen();               // Render calibration screen
void renderTrackManagerScreen();              // Render track manager
void renderConnectModeScreen();               // Render connect mode

// Utility functions
String formatUITime(unsigned long seconds);   // Format time for UI display
String formatUIBytes(unsigned long bytes);    // Format bytes for UI display

// Screen state management
void setUIScreenDirty(bool dirty = true);     // Mark screen as needing redraw
bool isUIScreenDirty();                       // Check if screen needs redraw

// ========================================
// UI CONSTANTS (from actual implementation and config.h)
// ========================================

// Screen dimensions
#define SCREEN_WIDTH                320       // Screen width in pixels
#define SCREEN_HEIGHT               480       // Screen height in pixels

// Font sizes (from display system)
#ifndef FONT_SIZE_SMALL
#define FONT_SIZE_SMALL             1         // Small font size
#endif
#ifndef FONT_SIZE_MEDIUM
#define FONT_SIZE_MEDIUM            2         // Medium font size
#endif
#ifndef FONT_SIZE_LARGE
#define FONT_SIZE_LARGE             3         // Large font size
#endif

// QR Code constants (from actual implementation)
#define QR_SIZE                     120       // QR code size in pixels

// Layout constants
#define UI_HEADER_HEIGHT            60        // Header area height
#define UI_CONTENT_START_Y          60        // Content area start
#define UI_CONTENT_HEIGHT           380       // Content area height
#define UI_MENU_ITEM_HEIGHT         50        // Menu item height
#define UI_VISIBLE_MENU_ITEMS       7         // Maximum visible menu items

// Colors would be defined in display_manager.h or config.h

// ========================================
// STATE ACCESS FUNCTIONS (used by renderer)
// ========================================

// These functions are expected to be available from other components
MenuState getMenuState();                     // Get current menu state
int getMenuSelectedIndex();                   // Get selected menu item
int getMenuScrollOffset();                    // Get menu scroll offset

CalibrationState getCalibrationState();       // Get calibration state
CalibrationData getCalibrationData();         // Get calibration data

TrackManagerState getTrackManagerState();     // Get track manager state
int getSelectedTrackIndex();                  // Get selected track
int getTrackScrollOffset();                   // Get track scroll offset
int getTrackCount();                         // Get total track count
String getStorageInfo();                      // Get storage information
TrackInfo getTrackInfo(int index);           // Get track information

ConnectModeState getConnectModeState();       // Get connect mode state
WiFiManager& getWiFiManager();               // Get WiFi manager reference

// System data access (from shared_data.h)
GPSData getGPSData();
CompassData getCompassData();
TrackingData getTrackingData();
SystemStatus getSystemStatus();

#endif // UI_RENDERER_H