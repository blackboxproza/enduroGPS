/*
 * E_NAVI EnduroGPS V2 - Enhanced Menu System
 * Vision-Optimized Menu for 35-65 Year Old Riders
 * 
 * FIXED VERSION - Circular dependency issues resolved
 * FIXED: Forward declarations properly handled
 * FIXED: Removed duplicate enum definitions, now uses shared_data.h
 * FIXED: Removed duplicate SettingsManager class, now uses forward declaration
 * FIXED: TrackDisplayUI defined here without conflicts
 * 
 * STREAMLINED DESIGN: V1's simplicity with V2's functionality
 * - Large fonts for aging eyes (48px+ buttons)  
 * - High contrast design
 * - Single-function simplicity
 * - Essential settings only
 * - Professional appearance
 * 
 * Interface: 5-way buttons (up/down/left/right/center) + zoom/waypoint
 */

#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include <Arduino.h>
#include "config.h"
#include "shared_data.h"

// Forward declarations to avoid circular dependencies
struct GPSData;
struct NavigationData;
struct CompassData;
struct TrackingData;
struct SystemStatus;
struct ConnectivityStatus;
struct TrackFileInfo;
struct TrackStatistics;

// Forward declare classes
class TrackManager;
class ButtonManager;
class SettingsManager;

// ========================================
// MENU CONFIGURATION - MOVED TO TOP
// ========================================

// Menu Display Settings (Large for Aging Eyes)
#define MENU_TITLE_HEIGHT       60      // Larger title area
#define MENU_ITEM_HEIGHT        60      // Even larger for easier selection
#define MENU_BORDER_WIDTH        3      // Thicker borders
#define MENU_PADDING            15      // More padding
#define MENU_FONT_SIZE_LARGE    28      // Very large font
#define MENU_FONT_SIZE_MEDIUM   22      // Large medium font
#define MENU_FONT_SIZE_SMALL    18      // Large small font

// Menu Behavior Settings (Aging-Friendly)
#define MENU_TIMEOUT_MS         45000   // Longer timeout (45 seconds)
#define MENU_REPEAT_DELAY_MS     250    // Faster repeat for easier navigation
#define STATUS_MESSAGE_TIME      5000   // Longer message display (5 seconds)
#define CONFIRM_TIMEOUT_MS      10000   // 10 seconds for confirmations

// Maximum Items (Reduced for Better Visibility) - MOVED TO TOP
#define MAX_MENU_ITEMS              6   // Fewer items per screen

// Visual Enhancement Settings
#define HIGH_CONTRAST_RATIO       4.5   // WCAG AA contrast ratio
#define LARGE_TOUCH_TARGET_SIZE    60   // Large touch targets
#define ICON_SIZE                  32   // Large icons
#define BUTTON_MIN_HEIGHT          60   // Minimum button height

// Accessibility Settings
#define ENABLE_LARGE_FONTS       true   // Always use large fonts
#define ENABLE_HIGH_CONTRAST     true   // High contrast by default
#define ENABLE_CLEAR_FEEDBACK    true   // Clear visual feedback
#define ENABLE_SIMPLE_NAVIGATION true   // Simplified navigation

// ========================================
// MENU COLOR CONSTANTS - ADDED
// ========================================

// Menu-specific colors (in addition to config.h colors)
#define MENU_BACKGROUND         COLOR_DAY_BACKGROUND
#define MENU_TEXT               COLOR_DAY_TEXT
#define MENU_SELECTED_BG        COLOR_DAY_ACCENT
#define MENU_SELECTED_TEXT      COLOR_DAY_BACKGROUND
#define MENU_TITLE_BG           COLOR_DAY_ACCENT
#define MENU_TITLE_TEXT         COLOR_DAY_BACKGROUND
#define MENU_BORDER             COLOR_DAY_TEXT
#define MENU_WARNING            COLOR_DAY_GPS_WARN
#define MENU_SUCCESS            COLOR_DAY_GPS_GOOD
#define MENU_ERROR              COLOR_DAY_GPS_BAD

// Night mode menu colors
#define MENU_NIGHT_BACKGROUND   COLOR_NIGHT_BACKGROUND
#define MENU_NIGHT_TEXT         COLOR_NIGHT_TEXT
#define MENU_NIGHT_SELECTED_BG  COLOR_NIGHT_ACCENT
#define MENU_NIGHT_SELECTED_TEXT COLOR_NIGHT_BACKGROUND
#define MENU_NIGHT_BORDER       COLOR_NIGHT_TEXT

// ========================================
// MENU DEFINITION STRUCTURE
// ========================================

struct MenuDefinition {
  const char* title;
  const char* items[8];   // Maximum 8 items per menu (aging-friendly)
  int itemCount;
  bool hasBack;           // Has back option
  bool hasIcons;          // Use icons for better visibility
};

// ========================================
// MENU COLORS FOR HIGH CONTRAST
// ========================================

struct MenuColors {
  uint16_t background;
  uint16_t text;
  uint16_t selectedBg;
  uint16_t selectedText;
  uint16_t titleBg;
  uint16_t titleText;
  uint16_t border;
  uint16_t accent;
  uint16_t warning;
  uint16_t success;
};

// ========================================
// COMPATIBILITY FORWARD DECLARATIONS
// ========================================

// For compatibility with existing code that expects these
struct ButtonEventData;  // Forward declaration

// ========================================
// MENU MANAGER CLASS - STREAMLINED
// ========================================

class MenuManager {
  private:
    // Menu State
    MenuState currentMenu = MENU_MAIN;
    int selectedItem = 0;
    int topDisplayItem = 0;
    bool menuActive = false;
    bool inSubMenu = false;
    bool largeFont = true;  // Add missing member variable
    int itemsPerScreen = 6; // Add missing member variable
    
    // Menu Definitions
    MenuDefinition menuDefs[8];     // Reduced from complex V2 structure
    
    // Status & Feedback
    bool settingsChanged = false;
    String statusMessage = "";
    unsigned long statusMessageTime = 0;
    unsigned long lastInput = 0;
    
    // Menu Colors
    MenuColors dayColors;
    MenuColors nightColors;
    
    // Internal Methods
    void initializeMenuDefinitions();
    void initializeMenuColors();
    void drawMenu();
    void drawMenuItem(int index, int yPos, bool selected);
    void drawMenuTitle(const String& title);
    void drawStatusMessage();
    void drawScrollIndicators();
    void drawMenuBorder();
    
    // Menu Navigation
    void navigateUp();
    void navigateDown();
    void navigateLeft();      // Back functionality
    void navigateRight();     // Forward/Enter functionality
    void selectItem();
    void goBack();
    
    // Menu Handlers
    void handleMainMenu();
    void handleTrackingMenu();
    void handleConnectMenu();    // NEW: WiFi file transfer
    void handleDisplayMenu();
    void handleCompassMenu();
    void handleGPSMenu();
    void handleSystemMenu();
    
    // Setting Adjusters (Simplified) - FIXED: Use correct GPS constants
    void adjustTrackMode();
    void toggleTrackFilter();
    void toggleAutoSave();
    void adjustBrightness();
    void toggleNightMode();
    void toggleAutoNightMode();
    void adjustGPSPowerMode();   // FIXED: Now uses GPS_POWER_NORMAL, etc.
    
    // Connect Mode Functions (V2 Features)
    void startConnectMode();
    void showQRCode();
    void showConnectStatus();
    void finishConnectMode();
    
    // Information Displays (Large, Clear)
    void showCurrentTrackInfo();
    void showCompassStatus();
    void showGPSStatus();
    void showSystemInfo();
    void showConnectionInfo();
    void runSystemDiagnostics();
    void startCompassCalibration();
    
    // Display Helpers
    String getMenuItemValue(int index);
    String getMenuItemIcon(int index);
    void showInfoDialog(const String& info, uint16_t color = COLOR_DAY_TEXT);
    void showConfirmDialog(const String& message, bool& result);
    void saveSettings();
    
    // Visual Enhancements for Aging Eyes
    void drawLargeText(const String& text, int x, int y, uint16_t color);
    void drawHighContrastButton(const String& text, int x, int y, int w, int h, bool selected);
    void drawProgressBar(int x, int y, int w, int h, float progress, uint16_t color);
    void drawIcon(const String& icon, int x, int y, uint16_t color);
    
  public:
    MenuManager();
    ~MenuManager();
    
    // Initialization
    bool initialize();
    
    // Menu Control
    void enterMenu();
    void exitMenu();
    bool isActive() const { return menuActive; }
    void update();                    // Call in main loop
    
    // Navigation (5-way button interface)
    void handleInput(ButtonID button, ButtonEvent event);
    void handleUpButton();
    void handleDownButton();
    void handleLeftButton();
    void handleRightButton();
    void handleCenterButton();
    void handleZoomInButton();        // Alternative navigation
    void handleZoomOutButton();       // Alternative navigation
    void handleWaypointButton();      // Quick access features
    
    // Menu State
    MenuState getCurrentMenu() const { return currentMenu; }
    int getSelectedItem() const { return selectedItem; }
    bool hasUnsavedChanges() const { return settingsChanged; }
    bool isInSubMenu() const { return inSubMenu; }
    
    // Status Messages (Large, Clear)
    void showMessage(const String& message, unsigned long duration = 3000);
    void showSuccessMessage(const String& message);
    void showWarningMessage(const String& message);
    void showErrorMessage(const String& message);
    void clearMessage();
    
    // Visual Settings
    void setNightMode(bool enabled);
    void setBrightness(int level);
    void setLargeFont(bool enabled) { largeFont = enabled; }
    
    // Accessibility Features
    void increaseTextSize();
    void decreaseTextSize();
    void toggleHighContrast();
    void enableVoiceFeedback(bool enabled);  // Future feature
    
    // Quick Access Functions
    void quickTrackStart();           // One-button track start
    void quickConnectMode();          // One-button connect mode
    void quickBrightnessAdjust();     // Quick brightness cycle
    void quickInfoDisplay();          // Quick system info

    void resetSettings();  // ADD THIS DECLARATION
};

// ========================================
// COMPASS CALIBRATION UI - SIMPLIFIED
// ========================================

class CompassCalibrationUI {
  private:
    bool calibrationActive = false;
    bool calibrationComplete = false;
    float calibrationProgress = 0.0;
    String currentInstruction = "";
    unsigned long calibrationStartTime = 0;
    int calibrationStep = 0;
    
  public:
    CompassCalibrationUI();
    ~CompassCalibrationUI();
    
    // Calibration Control
    void startCalibration();
    void stopCalibration();
    void updateCalibration();
    bool isActive() const { return calibrationActive; }
    bool isComplete() const { return calibrationComplete; }
    
    // Display
    void drawCalibrationScreen();
    void drawProgress();
    void drawInstructions();
    void drawLoopLoopDiagram();
    
    // Status
    float getProgress() const { return calibrationProgress; }
    String getCurrentInstruction() const { return currentInstruction; }
    int getCurrentStep() const { return calibrationStep; }
};

// ========================================
// TRACK DISPLAY UI - PROPERLY DEFINED HERE
// ========================================

class TrackDisplayUI {
  private:
    TrackManager* trackManager;
    bool managerActive = false;
    bool visible = false;
    bool showingList = false;
    bool showingDetails = false;
    int selectedTrack = 0;
    int displayOffset = 0;
    String trackList[MAX_TRACKS_DISPLAYED];
    int trackCount = 0;
    
    // Display Configuration
    int itemHeight = 40;
    int itemsPerScreen = 10;
    int listStartY = 60;
    unsigned long lastUpdate = 0;
    String statusMessage = "";

    // Internal Methods
    void drawTrackItem(int index, int yPos, bool selected);
    void drawTrackDetails(const TrackFileInfo& info);
    void drawStatistics(const TrackStatistics& stats);
    void drawScrollIndicator();
    void updateScrollPosition();

  public:
    TrackDisplayUI(TrackManager* manager = nullptr);
    ~TrackDisplayUI();

    // Track Manager Control
    void enterTrackManager();
    void exitTrackManager();
    void refreshTrackList();
    bool isActive() const { return managerActive; }
    bool isVisible() const { return visible || showingList || showingDetails; }

    // Navigation
    void handleInput(ButtonID button, ButtonEvent event);
    void selectNextTrack();
    void selectPreviousTrack();
    void selectTrack();
    void deleteSelectedTrack();
    void viewTrackInfo();

    // Display
    void drawTrackList();
    void drawTrackInfo(const String& filename);
    void drawTrackStats();
    void showTrackList();
    void showTrackDetails(int index);
    void showStatistics();
    void hideDisplay();

    // Display State
    int getSelectedIndex() const { return selectedTrack; }
    void setSelectedIndex(int index);
    bool isShowingList() const { return showingList; }
    bool isShowingDetails() const { return showingDetails; }

    // Status Messages
    void showMessage(const String& message);
    void clearMessage();

    void update();
};

// ========================================
// GLOBAL INSTANCES
// ========================================

extern MenuManager menuManager;
extern SettingsManager settingsManager;
extern CompassCalibrationUI calibrationUI;
extern TrackDisplayUI trackUI;
extern TrackDisplayUI trackDisplayUI;    // Consistent type

// ========================================
// MENU UTILITY FUNCTIONS - AGING-FRIENDLY
// ========================================

// Text Formatting for Large, Clear Display
String formatTrackMode(int mode);
String formatGPSPowerMode(int mode);      // Handle int instead of enum for compatibility
String formatFileSize(unsigned long bytes);
String formatDuration(unsigned long seconds);
String formatTrackDistance(float km);
String formatBatteryLevel(int percentage);
String formatGPSQuality(int satellites, float hdop);

// Visual Helpers for Aging Eyes
void drawLargeMenuBackground();
void drawHighContrastBorder();
void drawLargeScrollIndicator(int currentItem, int totalItems, int visibleItems);
void drawLargeBatteryIcon(int x, int y, float voltage);
void drawLargeGPSIcon(int x, int y, bool hasSignal);
void drawLargeWiFiIcon(int x, int y, bool connected);

// Input Validation (Simplified)
bool isValidBrightness(int value);
bool isValidSensitivity(float value);
bool isValidTrackMode(int mode);
bool isValidGPSPowerMode(int mode);

// Menu Color Management
MenuColors getDayMenuColors();
MenuColors getNightMenuColors();
MenuColors getCurrentMenuColors(bool nightMode);

// ========================================
// COMPATIBILITY FUNCTIONS - ADDED
// ========================================

// For existing code that might use different function names
inline String getGPSQualityText(int satellites, float hdop) {
    return formatGPSQuality(satellites, hdop);
}

inline String formatSpeed(float speedKmh) {
    return String(speedKmh, 1) + " km/h";
}

inline String formatUptime(unsigned long seconds) {
    return formatDuration(seconds);
}

// GPS Power Mode Helper Functions - Updated to use int
inline String getGPSPowerModeText(int mode) {
    switch (mode) {
        case 0: return "Full Power";     // GPS_POWER_FULL
        case 1: return "Power Save";     // GPS_POWER_LIGHT
        case 2: return "Deep Save";      // GPS_POWER_DEEP
        case 3: return "Sleep";          // GPS_POWER_SLEEP
        default: return "Unknown";
    }
}

inline int getNextGPSPowerMode(int currentMode) {
    switch (currentMode) {
        case 0: return 1;  // GPS_POWER_FULL -> GPS_POWER_LIGHT
        case 1: return 2;  // GPS_POWER_LIGHT -> GPS_POWER_DEEP
        case 2: return 3;  // GPS_POWER_DEEP -> GPS_POWER_SLEEP
        case 3: return 0; // GPS_POWER_SLEEP -> GPS_POWER_FULL
        default: return 0; // Default to full power
    }
}

#endif // MENU_SYSTEM_H