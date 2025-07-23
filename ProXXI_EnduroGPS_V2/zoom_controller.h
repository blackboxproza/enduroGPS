/*
 * FILE LOCATION: ProXXI_EnduroGPS_V2/input/zoom_controller.h
 * 
 * E_NAVI EnduroGPS - Professional Zoom Controller Interface
 * Speed-responsive auto-zoom with intelligent manual override system
 * 
 * Features:
 * - Auto-zoom based on GPS speed with customizable curves
 * - Manual zoom controls with smart timeout system  
 * - Smooth zoom transitions with easing animation
 * - Settings persistence via NVS storage
 * - Visual feedback system with notifications
 * - Menu integration for configuration
 * - Optimized for enduro riding scenarios
 * 
 * Integration:
 * - ButtonManager for user input handling
 * - MapDisplay for zoom level application
 * - SharedData for GPS speed information
 * - DisplayManager for visual feedback
 * - Preferences for settings persistence
 * 
 * By ProXXi Pty Ltd - "Lost, Never Again"
 */

#ifndef ZOOM_CONTROLLER_H
#define ZOOM_CONTROLLER_H

#include "config.h"
#include "button_manager.h"
#include <Preferences.h>

// Forward declarations
class ButtonManager;

// ===================================================================
// ZOOM CONTROLLER - PROFESSIONAL AUTO-ZOOM WITH MANUAL OVERRIDE
// Speed-responsive zoom with user control and timeout system
// ===================================================================

// Zoom operation modes
enum ZoomMode {
  ZOOM_MANUAL,           // Manual control only - no auto-zoom
  ZOOM_AUTO,             // Speed-responsive auto-zoom only
  ZOOM_SMART,            // Auto + manual override with timeout
  ZOOM_LOCKED            // Locked at current level - no changes
};

// Zoom configuration structure
struct ZoomConfig {
  ZoomMode mode = ZOOM_SMART;                   // Current zoom mode
  int currentLevel = 14;                        // Current zoom level (8-18)
  int minLevel = ZOOM_MIN_LEVEL;               // Minimum zoom level (8)
  int maxLevel = ZOOM_MAX_LEVEL;               // Maximum zoom level (18)
  float speedMin = ZOOM_SPEED_MIN;             // Minimum speed for auto-zoom (0.0 km/h)
  float speedMax = ZOOM_SPEED_MAX;             // Maximum speed for auto-zoom (100.0 km/h)
  float responseFactor = 1.0;                  // User adjustable sensitivity (0.1-2.0)
  bool smoothTransitions = true;               // Enable smooth zoom transitions
  unsigned long manualOverrideTimeout = 10000; // Manual override timeout (10 seconds)
  bool saveSettings = true;                    // Persist settings to NVS
};

// Auto-zoom curve configuration for different riding scenarios
struct ZoomCurve {
  // Speed thresholds for different zoom levels (km/h)
  float walkingSpeed = 5.0;       // Pedestrian/technical section speed
  float trailSpeed = 15.0;        // Slow trail riding speed
  float normalSpeed = 30.0;       // Normal enduro riding speed
  float fastSpeed = 60.0;         // Fast trail/fire road speed
  float highwaySpeed = 80.0;      // Highway/road transition speed
  
  // Corresponding zoom levels for each speed threshold
  int walkingZoom = 18;           // Maximum detail for technical sections
  int trailZoom = 16;             // High detail for slow trail riding
  int normalZoom = 14;            // Balanced view for normal riding
  int fastZoom = 12;              // Wide view for fast sections
  int highwayZoom = 10;           // Very wide view for highway
};

// Manual override tracking structure
struct ManualOverride {
  bool isActive = false;                       // Override currently active
  unsigned long startTime = 0;                // When override started
  int overrideLevel = 14;                     // Override zoom level
  ZoomMode previousMode = ZOOM_AUTO;          // Mode before override
  bool showTimeoutWarning = true;             // Show 3-second timeout warning
};

// ===================================================================
// ZOOM CONTROLLER CLASS - MAIN CONTROL SYSTEM
// ===================================================================

class ZoomController {
private:
  // Configuration and state
  ZoomConfig config;                          // Current configuration
  ZoomCurve curve;                           // Auto-zoom speed curve
  ManualOverride override;                   // Manual override state
  Preferences nvs;                           // NVS storage for settings
  
  // Runtime state
  bool initialized = false;                   // Initialization complete
  float currentSpeed = 0.0;                  // Last GPS speed reading
  int targetZoom = 14;                       // Target zoom level for auto-zoom
  unsigned long lastSpeedUpdate = 0;         // Last GPS speed update time
  unsigned long lastZoomChange = 0;          // Last zoom level change time
  
  // Smooth transition system
  float zoomTransitionProgress = 1.0;        // Transition progress (0.0-1.0)
  int transitionStartZoom = 14;              // Zoom level at transition start
  int transitionTargetZoom = 14;             // Target zoom level for transition
  unsigned long transitionStartTime = 0;     // Transition start time
  const unsigned long transitionDuration = 1000; // Transition duration (1 second)
  
  // Integration references
  ButtonManager* buttonManager = nullptr;     // Button manager reference
  
public:
  // ===================================================================
  // INITIALIZATION AND SETUP
  // ===================================================================
  
  // Initialize zoom controller with button manager
  bool initialize(ButtonManager* btnMgr);
  
  // Load settings from NVS storage
  bool loadSettings();
  
  // Save current settings to NVS storage
  bool saveSettings();
  
  // ===================================================================
  // CORE UPDATE METHODS (CALLED FROM MAIN LOOP)
  // ===================================================================
  
  // Main update method - call with current GPS speed
  void update(float speed);
  
  // Handle button input events
  void handleButtonInput();
  
  // ===================================================================
  // ZOOM CONTROL METHODS
  // ===================================================================
  
  // Set zoom operation mode
  void setZoomMode(ZoomMode mode);
  
  // Set specific zoom level (triggers manual override in SMART mode)
  void setZoomLevel(int level);
  
  // Zoom in one level
  void zoomIn();
  
  // Zoom out one level
  void zoomOut();
  
  // Reset to auto-zoom (disable manual override)
  void resetToAuto();
  
  // ===================================================================
  // AUTO-ZOOM CALCULATION ENGINE
  // ===================================================================
  
  // Calculate auto-zoom level for given speed
  int calculateAutoZoom(float speed);
  
  // Update auto-zoom based on current speed
  void updateAutoZoom(float speed);
  
  // Start smooth zoom transition to new level
  void startZoomTransition(int newZoom);
  
  // Update ongoing zoom transition
  void updateZoomTransition();
  
  // ===================================================================
  // MANUAL OVERRIDE SYSTEM
  // ===================================================================
  
  // Enable manual override at specific zoom level
  void enableManualOverride(int zoomLevel);
  
  // Disable manual override (return to auto-zoom)
  void disableManualOverride();
  
  // Check if manual override is currently active
  bool isManualOverrideActive();
  
  // Get remaining time for manual override (milliseconds)
  unsigned long getOverrideTimeRemaining();
  
  // Check and handle manual override timeout
  void checkOverrideTimeout();
  
  // ===================================================================
  // CONFIGURATION METHODS
  // ===================================================================
  
  // Set zoom response factor (0.1-2.0)
  void setResponseFactor(float factor);
  
  // Set speed range for auto-zoom
  void setSpeedRange(float minSpeed, float maxSpeed);
  
  // Set zoom level range
  void setZoomRange(int minZoom, int maxZoom);
  
  // Enable/disable smooth transitions
  void setSmoothTransitions(bool enable);
  
  // Set manual override timeout (milliseconds)
  void setManualTimeout(unsigned long ms);
  
  // ===================================================================
  // ZOOM CURVE CUSTOMIZATION
  // ===================================================================
  
  // Set custom zoom curve
  void setZoomCurve(const ZoomCurve& newCurve);
  
  // Get current zoom curve
  ZoomCurve getZoomCurve();
  
  // Reset zoom curve to optimized defaults
  void resetCurveToDefaults();
  
  // ===================================================================
  // GETTER METHODS
  // ===================================================================
  
  // Get current zoom operation mode
  ZoomMode getZoomMode();
  
  // Get current zoom level
  int getCurrentZoom();
  
  // Get target zoom level (for auto-zoom)
  int getTargetZoom();
  
  // Get zoom response factor
  float getResponseFactor();
  
  // Check if zoom transition is in progress
  bool isTransitioning();
  
  // Get manual override timeout setting
  unsigned long getManualTimeout();
  
  // Get smooth transitions setting
  bool getSmoothTransitions();
  
  // ===================================================================
  // STATUS AND DIAGNOSTICS
  // ===================================================================
  
  // Get zoom mode as text string
  String getZoomModeText();
  String getZoomModeText(ZoomMode mode);
  
  // Get complete status text
  String getStatusText();
  
  // Check if settings have been modified
  bool hasSettingsChanged();
  
private:
  // ===================================================================
  // INTERNAL HELPER METHODS
  // ===================================================================
  
  // Auto-zoom calculation helpers
  int interpolateZoom(float speed);
  float calculateSpeedFactor(float speed);
  void applyResponseFactor(float& factor);
  
  // Validation and safety
  bool isValidZoomLevel(int level);
  bool validateSettings(const ZoomConfig& cfg);
  
  // Smooth transition helpers
  float easeInOutCubic(float t);              // Easing function
  int calculateTransitionZoom();               // Calculate intermediate zoom
  
  // Settings management
  void setDefaultSettings();                   // Load default configuration
};

// ===================================================================
// ZOOM MENU SYSTEM - INTEGRATED CONFIGURATION INTERFACE
// ===================================================================

// Menu item enumeration
enum ZoomMenuItem {
  ZOOM_MENU_MODE,          // Zoom mode selection (Auto/Manual/Smart/Locked)
  ZOOM_MENU_LEVEL,         // Current zoom level adjustment
  ZOOM_MENU_RESPONSE,      // Response factor adjustment
  ZOOM_MENU_TIMEOUT,       // Manual override timeout setting
  ZOOM_MENU_SMOOTH,        // Smooth transitions toggle
  ZOOM_MENU_CURVE_SETUP,   // Customize zoom curve (advanced)
  ZOOM_MENU_RESET,         // Reset all settings to defaults
  ZOOM_MENU_BACK           // Return to main menu
};

class ZoomMenuSystem {
private:
  ZoomController* zoomController = nullptr;   // Reference to zoom controller
  
  // Menu navigation state
  ZoomMenuItem currentItem = ZOOM_MENU_MODE;  // Currently selected item
  bool inSubMenu = false;                     // In sub-menu for value editing
  bool needsRedraw = true;                    // Screen needs redraw
  
  // Curve setup state (for advanced configuration)
  int curveEditIndex = 0;                     // Which curve point being edited
  bool editingSpeed = true;                   // Editing speed vs zoom level
  
public:
  // ===================================================================
  // MENU SYSTEM METHODS
  // ===================================================================
  
  // Initialize menu system with zoom controller reference
  bool initialize(ZoomController* zoomCtrl);
  
  // Handle button press events
  void handleButtonPress(int button);
  
  // Navigation methods
  void navigateUp();                          // Move selection up
  void navigateDown();                        // Move selection down
  void selectCurrent();                       // Select current item
  void adjustValue(bool increase);            // Adjust current value
  void exitMenu();                           // Exit menu system
  
  // ===================================================================
  // DISPLAY METHODS
  // ===================================================================
  
  // Main menu drawing
  void draw();
  void drawMainMenu();
  void drawCurveSetup();
  void drawConfirmDialog();
  
  // ===================================================================
  // HELPER METHODS
  // ===================================================================
  
  // Get text for menu items
  String getMenuItemText(ZoomMenuItem item);
  String getValueText(ZoomMenuItem item);
  void showQuickHelp();
  
private:
  // ===================================================================
  // MENU ITEM HANDLERS
  // ===================================================================
  
  void handleModeSelection();                 // Cycle through zoom modes
  void handleLevelAdjustment(bool increase);  // Adjust zoom level
  void handleResponseAdjustment(bool increase); // Adjust response factor
  void handleTimeoutAdjustment(bool increase); // Adjust timeout
  void handleSmoothToggle();                  // Toggle smooth transitions
  void handleCurveSetup();                    // Enter curve setup
  void handleReset();                         // Reset to defaults
  
  // ===================================================================
  // CURVE SETUP HELPERS (ADVANCED CONFIGURATION)
  // ===================================================================
  
  void enterCurveSetup();                     // Enter curve setup mode
  void exitCurveSetup();                      // Exit curve setup mode
  void adjustCurveValue(bool increase);       // Adjust curve parameters
  void saveCurveChanges();                    // Save modified curve
};

// ===================================================================
// ZOOM NOTIFICATION SYSTEM - VISUAL FEEDBACK
// ===================================================================

class ZoomNotification {
private:
  // Notification display state
  bool showingNotification = false;           // Currently showing notification
  String notificationText = "";               // Text to display
  unsigned long showStartTime = 0;            // When notification started
  unsigned long showDuration = 3000;         // How long to show (3 seconds)
  
  // Animation state
  int animationPhase = 0;                     // Current animation frame
  bool pulsing = false;                       // Use pulsing animation
  
public:
  // ===================================================================
  // NOTIFICATION DISPLAY METHODS
  // ===================================================================
  
  // Show zoom mode change notification
  void showModeChange(ZoomMode mode);
  
  // Show manual override activation
  void showManualOverride(int level, unsigned long timeoutMs);
  
  // Show manual override timeout
  void showOverrideTimeout();
  
  // Show zoom level change
  void showZoomChange(int level);
  
  // Show error message
  void showError(const String& error);
  
  // ===================================================================
  // UPDATE AND CONTROL METHODS
  // ===================================================================
  
  // Update notification system (call from main loop)
  void update();
  
  // Draw notification on screen
  void draw();
  
  // Hide current notification
  void hideNotification();
  
  // Check if notification is currently showing
  bool isShowing();
  
private:
  // ===================================================================
  // INTERNAL DRAWING HELPERS
  // ===================================================================
  
  void drawNotificationBox();                 // Draw notification background
  void drawNotificationText();                // Draw notification text
  uint16_t getNotificationColor();            // Get notification color
};

// ===================================================================
// GLOBAL INSTANCES AND ACCESS FUNCTIONS
// ===================================================================

// Global instances (defined in zoom_controller.cpp)
extern ZoomController zoomController;
extern ZoomMenuSystem zoomMenu;
extern ZoomNotification zoomNotification;

// ===================================================================
// QUICK ACCESS FUNCTIONS FOR MAIN SYSTEM INTEGRATION
// ===================================================================

// Initialize zoom control system (call from setup())
bool initializeZoomControl();

// Update zoom control system (call from main loop with GPS speed)
void updateZoomControl(float currentSpeed);

// Handle zoom button presses (call from button event handler)
void handleZoomButtons();

// Check if auto-zoom is currently active
bool isAutoZoomActive();

// Get current zoom level
int getCurrentZoomLevel();

// Enter zoom configuration menu
void enterZoomMenu();

// Show detailed zoom status (for debugging)
void showZoomStatus();

// ===================================================================
// INTEGRATION CONSTANTS AND DEFINITIONS
// ===================================================================

// Default zoom curve values (optimized for enduro riding)
#define DEFAULT_WALKING_SPEED   5.0f     // km/h
#define DEFAULT_TRAIL_SPEED     15.0f    // km/h  
#define DEFAULT_NORMAL_SPEED    30.0f    // km/h
#define DEFAULT_FAST_SPEED      60.0f    // km/h
#define DEFAULT_HIGHWAY_SPEED   80.0f    // km/h

#define DEFAULT_WALKING_ZOOM    18       // Maximum detail
#define DEFAULT_TRAIL_ZOOM      16       // High detail
#define DEFAULT_NORMAL_ZOOM     14       // Balanced view
#define DEFAULT_FAST_ZOOM       12       // Wide view
#define DEFAULT_HIGHWAY_ZOOM    10       // Very wide view

// Manual override timeout options (milliseconds)
#define TIMEOUT_SHORT           5000     // 5 seconds
#define TIMEOUT_NORMAL          10000    // 10 seconds (default)
#define TIMEOUT_LONG            15000    // 15 seconds
#define TIMEOUT_VERY_LONG       30000    // 30 seconds

// Response factor limits
#define RESPONSE_FACTOR_MIN     0.1f     // Least responsive
#define RESPONSE_FACTOR_MAX     2.0f     // Most responsive
#define RESPONSE_FACTOR_DEFAULT 1.0f     // Balanced response

// Transition timing
#define TRANSITION_FAST         500      // 0.5 seconds
#define TRANSITION_NORMAL       1000     // 1.0 seconds (default)
#define TRANSITION_SLOW         2000     // 2.0 seconds

// NVS storage namespace
#define ZOOM_NVS_NAMESPACE      "zoom_ctrl"

#endif // ZOOM_CONTROLLER_H
