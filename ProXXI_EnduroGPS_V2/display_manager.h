/*
 * E_NAVI EnduroGPS - Display Manager
 * TFT Display Initialization and Control for Vision-Optimized UI
 * 
 * Hardware: 3.5" IPS TFT 320x480 (AXS15231B driver)
 * Layout: Portrait mode optimized for mature riders (35-65 years)
 * Features: Day/Night mode, High contrast, Large fonts
 * FIXED: Removed duplicate function declarations
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "shared_data.h" // Move this before any DisplayMode usage
#include "config.h"

// ========================================
// DISPLAY CONFIGURATION
// ========================================


// Display Zones (Portrait 320x480 layout)
struct DisplayZones {
  // Status Bar (Top 10% - 48px)
  int statusY = 0;
  int statusHeight = STATUS_BAR_HEIGHT;
  
  // Content Area (Middle 60% - 288px) 
  int contentY = STATUS_BAR_HEIGHT;
  int contentHeight = CONTENT_AREA_HEIGHT;
  
  // Bottom Bar (Bottom 30% - 144px)
  int bottomY = STATUS_BAR_HEIGHT + CONTENT_AREA_HEIGHT;
  int bottomHeight = BOTTOM_BAR_HEIGHT;
};

// Color Schemes for Day/Night Vision
struct ColorScheme {
  uint16_t background;     // Primary background
  uint16_t text;          // Primary text color
  uint16_t textSecondary; // Secondary text color
  uint16_t gpsGood;       // GPS good status
  uint16_t gpsWarn;       // GPS warning status
  uint16_t gpsBad;        // GPS poor/no signal
  uint16_t accent;        // Highlight/accent color
  uint16_t recording;     // Recording indicator
  uint16_t paused;        // Paused indicator
};

// Font Sizes for Vision-Optimized UI
struct FontSizes {
  int large = FONT_SIZE_LARGE;    // 48pt - Status, speed, heading
  int medium = FONT_SIZE_MEDIUM;  // 24pt - Primary information  
  int small = FONT_SIZE_SMALL;    // 12pt - Secondary information
  int tiny = FONT_SIZE_TINY;      // 8pt - Debug only
};

// ========================================
// DISPLAY MANAGER CLASS
// ========================================
class DisplayManager {
  private:
    TFT_eSPI tft;
    DisplayMode currentMode = DISPLAY_AUTO_MODE;
    ColorScheme colors;
    FontSizes fonts;
    DisplayZones zones;
    
    // Display State
    bool initialized = false;
    bool backlightEnabled = true;
    int brightness = 80;           // 0-100%
    unsigned long lastUpdate = 0;
    int currentBrightness = 80;
    bool displayInitialized = false;
    
    // NEW: Missing Display Mode and Update Interval
    DisplayMode currentDisplayMode = DISPLAY_DAY_MODE;
    unsigned long updateInterval = 16;  // 60fps update rate
    
    // Night Mode Configuration
    bool autoModeEnabled = true;
    int dayModeStartHour = 6;      // 6 AM
    int nightModeStartHour = 18;   // 6 PM
    int nightBrightness = 20;      // 20% in night mode
    struct NightSettings {
        int brightness = 20;
        int nightBrightness = 20;           // ADD this
        bool autoSwitchEnabled = true;      // ADD this  
        DisplayMode currentMode = DISPLAY_AUTO_MODE;  // ADD this
        int nightModeStartHour = 18;        // ADD this
        int dayModeStartHour = 6;           // ADD this
    } nightSettings;
    
    // NEW: Layout Configuration Structure
    struct LayoutConfig {
        int statusY = 0;
        int statusHeight = 48;
        int contentY = 48;
        int contentHeight = 380;
        int dataY = 428;
        int dataHeight = 52;
    } layout;
    
    // NEW: Status Data Structure
    struct StatusData {
        StatusIndicators::GPSStatus gpsStatus = StatusIndicators::GPS_NO_FIX;
        StatusIndicators::RecordStatus recordStatus = StatusIndicators::REC_OFF;
        bool sdCardOK = false;
        bool batteryOK = true;
        bool motionDetected = false;
        bool compassOK = true;              // ADD this
        int sleepCountdown = 0;             // ADD this
        int signalStrength = 0;
    } status;
    
    // Performance Tracking
    unsigned long frameCount = 0;
    unsigned long totalFrameTime = 0;
    float averageFPS = 0.0;
    
    // Internal Methods
    void initializeTFT();
    void setupColorSchemes();
    void updateColorScheme();
    void setBrightness(int level);
    bool shouldUseNightMode();
    
  public:
    DisplayManager();
    ~DisplayManager();
    
    // Initialization
    bool initialize();
    bool isInitialized() const { return initialized; }
    
    // Display Mode Control
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return currentMode; }
    void toggleDayNightMode();
    void setAutoMode(bool enabled);
    
    // Brightness Control
    void setBrightnessLevel(int level);  // 0-100%
    int getBrightnessLevel() const { return brightness; }
    void enableBacklight(bool enable);
    bool isBacklightEnabled() const { return backlightEnabled; }
    
    // Color Access
    ColorScheme getCurrentColors() const { return colors; }
    uint16_t getBackgroundColor() { return colors.background; }  // FIXED: Removed const
    uint16_t getTextColor() const { return colors.text; }
    uint16_t getGPSStatusColor(bool gpsValid, int satellites, float hdop) const;
    uint16_t getRecordingColor(bool recording, bool paused) const;
    uint16_t getStatusColor(StatusIndicators::GPSStatus gpsStatus);
    uint16_t getRecordingColor(StatusIndicators::RecordStatus recordStatus);
    
    // Zone Access
    DisplayZones getZones() const { return zones; }
    
    // Screen Management
    void clearScreen();
    void clearZone(int y, int height);
    void fillZone(int y, int height, uint16_t color);
    
    // Basic Drawing (TFT_eSPI wrapper with vision optimization)
    void drawPixel(int x, int y, uint16_t color);
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color);
    void drawRect(int x, int y, int w, int h, uint16_t color);
    void fillRect(int x, int y, int w, int h, uint16_t color);
    void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color);
    void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color);
    void drawCircle(int x, int y, int r, uint16_t color);
    void fillCircle(int x, int y, int r, uint16_t color);
    
    // Vision-Optimized Text Drawing
    void setTextSize(int size);
    void setTextColor(uint16_t color);
    void setTextColor(uint16_t foreground, uint16_t background);
    void setCursor(int x, int y);
    void print(const String& text);
    void println(const String& text);
    void printf(const char* format, ...);
    
    // Large Text for Aging Eyes
    void drawLargeText(int x, int y, const String& text, uint16_t color);
    void drawCenteredText(int y, const String& text, uint16_t color, int fontSize);
    void drawRightAlignedText(int x, int y, const String& text, uint16_t color);
    
    // Status Indicators (High Contrast, Large Size)
    void drawGPSStatus(int x, int y, bool valid, int satellites, float hdop);
    void drawCompassStatus(int x, int y, bool valid, bool calibrated);
    void drawRecordingStatus(int x, int y, bool recording, bool paused);
    void drawBatteryStatus(int x, int y, float voltage, bool charging);
    void drawSystemStatus(int x, int y, bool healthy);
    
    // Recording Visual Feedback (Border System)
    void drawRecordingBorder(bool recording, bool paused);
    void clearRecordingBorder();
    
    // Progress Indicators
    void drawProgressBar(int x, int y, int width, int height, float progress, uint16_t color);
    void drawSpeedometer(int x, int y, int radius, float speed, float maxSpeed);
    void drawCompass(int x, int y, int radius, float heading);
    
    // Boot Sequence Display
    void showBootSplash();
    void showBootProgress(const String& message, int progress);
    void showBootComplete();
    void showSystemReady();
    
    // Error Display
    void showError(const String& title, const String& message);
    void showCriticalError(const String& message);
    void clearError();
    
    // Performance Information
    float getAverageFPS() const { return averageFPS; }
    unsigned long getFrameCount() const { return frameCount; }
    void updatePerformanceStats();
    
    // Low-Level TFT Access (when needed)
    TFT_eSPI& getTFT() { return tft; }
    
    // Screen Capture/Debug
    void dumpScreenToSerial();  // For debugging
    bool saveScreenToSD(const String& filename);

    // System Integration Functions
    void setBacklight(int brightness);
    void updateDisplayMode();
    void update();
    void updateGPSStatus(int satellites, float hdop, bool hasFix);
    void updateSystemStatus(bool sdOK, bool compassOK, bool batteryOK);
    void updateMotionStatus(bool motion, int sleepTimer);
    void drawStatusBar();
    void drawGPSContent(double lat, double lon, float speed, float heading);
    void drawDataBar(float speed, float heading, float temp, float battery);
    void showBootScreen(const char* message, int progress);
    void showMessage(const char* title, const char* message, int timeoutMs);
    void updateScreen();
    void drawText(int x, int y, const String& text, uint16_t color, int fontSize);
    
    // NEW: Missing Recording Status Update Function
    void updateRecordingStatus(StatusIndicators::RecordStatus recordStatus);
    
    // Color functions
    uint16_t getPrimaryTextColor();
    uint16_t getSecondaryTextColor();
};

// ========================================
// GLOBAL DISPLAY MANAGER INSTANCE
// ========================================
extern DisplayManager display;

// ========================================
// UTILITY FUNCTIONS FOR VISION OPTIMIZATION
// ========================================

// Color Conversion Utilities
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
void rgb565ToRGB(uint16_t color, uint8_t& r, uint8_t& g, uint8_t& b);
uint16_t adjustBrightness(uint16_t color, float factor);
uint16_t blendColors(uint16_t color1, uint16_t color2, float ratio);

// Vision-Optimized Color Selection
uint16_t getHighContrastColor(uint16_t background);
uint16_t getReadableTextColor(uint16_t background);
bool isColorContrastSufficient(uint16_t foreground, uint16_t background);
float calculateColorContrast(uint16_t color1, uint16_t color2);

// Text Measurement
int getTextWidth(const String& text, int fontSize);
int getTextHeight(int fontSize);
int getCenteredX(const String& text, int fontSize, int containerWidth = SCREEN_WIDTH);
int getRightAlignedX(const String& text, int fontSize, int rightEdge);

// Icon Drawing (Large, High Contrast)
void drawGPSIcon(int x, int y, int size, uint16_t color, bool filled = true);
void drawCompassIcon(int x, int y, int size, uint16_t color, float heading = 0);
void drawBatteryIcon(int x, int y, int size, uint16_t color, float level);
void drawRecordIcon(int x, int y, int size, uint16_t color, bool recording);
void drawWaypointIcon(int x, int y, int size, uint16_t color);
void drawErrorIcon(int x, int y, int size, uint16_t color);

// Special Effects for Status Indication
void flashIcon(int x, int y, int size, uint16_t color, int duration = 500);
void pulseIcon(int x, int y, int size, uint16_t color, float frequency = 1.0);
void animateIcon(int x, int y, int size, uint16_t color, int animationType);

// ========================================
// DISPLAY TIMING & PERFORMANCE
// ========================================

// Frame Rate Management
class FrameRateManager {
  private:
    unsigned long lastFrameTime = 0;
    unsigned long targetFrameTime = UI_REFRESH_INTERVAL_MS; // 16.67ms for 60fps
    bool vsyncEnabled = true;
    
  public:
    void setTargetFPS(int fps);
    bool shouldUpdateFrame();
    void waitForVSync();
    void frameCompleted();
    float getCurrentFPS();
};

extern FrameRateManager frameManager;

// ========================================
// DISPLAY CONFIGURATION VALIDATION
// ========================================

// Hardware Validation
bool validateDisplayHardware();
bool testDisplayFunctionality();
bool calibrateDisplayColors();

// Performance Testing
bool testDisplayPerformance();
float measureDrawingSpeed();
bool validateRefreshRate();

// Configuration Validation
bool validateColorScheme(const ColorScheme& scheme);
bool validateDisplayMode(DisplayMode mode); 
bool validateBrightness(int brightness);

#endif // DISPLAY_MANAGER_H