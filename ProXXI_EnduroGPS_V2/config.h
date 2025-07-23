/*
 * E_NAVI EnduroGPS V2 - Enhanced System Configuration
 * By ProXXi Pty Ltd - "Lost, Never Again"
 * 
 * ENHANCED: Added all missing constants required for compilation
 * FIXED: Added GPS_POWER_NORMAL and other required constants
 * FIXED: Removed conflicting macro definitions that clash with enums
 * Hardware pin definitions and configuration constants for ESP32-S3
 * 
 * Hardware Platform: JC3248W535C ESP32-S3 module
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ========================================
// HARDWARE PLATFORM IDENTIFICATION
// ========================================
#define HARDWARE_VERSION "JC3248W535C"
#define SOFTWARE_VERSION "E_NAVI_v2.1"
#define MANUFACTURER "ProXXi Pty Ltd"
#define PRODUCT_NAME "E_NAVI Enduro Navigation"
#define TAGLINE "Lost, Never Again"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// ========================================
// ESP32-S3 GPIO PIN CONFIGURATION
// ========================================

// TFT Display (AXS15231B Driver) - Hardware SPI
#define TFT_MOSI            11      // Display MOSI (DO)
#define TFT_SCLK            12      // Display SCLK (CLK)  
#define TFT_CS              10      // Display CS (Chip Select)
#define TFT_DC               8      // Display D/C (Data/Command)
#define TFT_RST              9      // Display Reset
#define TFT_BL              46      // Backlight PWM control

// SD Card (Shared SPI with Display)
#define SD_CS               21      // SD Card Chip Select
#define SD_MOSI             11      // Shared with display MOSI
#define SD_SCLK             12      // Shared with display SCLK
#define SD_MISO             13      // SD Card MISO

// GPS Module (NEO-7M) - Hardware UART
#define GPS_TX               1      // ESP32 TX to NEO-7M RX
#define GPS_RX               2      // ESP32 RX from NEO-7M TX
#define GPS_UART_NUM         2      // Hardware UART2
#define GPS_BAUD_RATE    38400      // GPS module baud rate

// I2C Bus (LSM303DLHC + CH423S)
#define I2C_SDA             17      // I2C Data
#define I2C_SCL             18      // I2C Clock
#define I2C_FREQ        400000      // 400kHz I2C speed

// LSM303DLHC Compass/Accelerometer
#define COMPASS_DRDY        15      // Data Ready interrupt
#define COMPASS_INT1        14      // Motion detection interrupt (wake)

// System Monitoring & Control
#define BATTERY_ADC          7      // Battery voltage (220k/47k divider)
#define BATTERY_ADC_PIN      7       // Battery voltage (220k/47k divider)
#define STATUS_LED          45      // System status LED
#define POWER_ENABLE        16      // Power enable control

// Spare GPIO for Future Expansion
#define SPARE_GPIO1          5      // Future expansion pin 1
#define SPARE_GPIO2          6      // Future expansion pin 2
#define SPARE_GPIO3          4      // Future expansion pin 3
#define SPARE_GPIO4          3      // Future expansion pin 4

// ========================================
// I2C DEVICE ADDRESSES
// ========================================
#define LSM303DLHC_ACCEL_ADDR   0x19    // Accelerometer address
#define LSM303DLHC_MAG_ADDR     0x1E    // Magnetometer address
#define CH423S_BASE_ADDR        0x20    // CH423S base address (0x20-0x3F range)

// ========================================
// TILE SYSTEM CONFIGURATION (MISSING FROM YOUR CONFIG.H)
// ========================================

// Map Tile System Settings
#define TILE_SIZE               256      // Tile size in pixels (standard)
#define TILE_FOLDER            "/maps/"  // SD card tile folder
#define TILE_FORMAT            "png"     // Tile file format
#define TILE_CACHE_SIZE         50       // Maximum cached tiles

// Display Constants (if missing)
#define SCREEN_WIDTH           320       // TFT width
#define SCREEN_HEIGHT          480       // TFT height
#define STATUS_BAR_HEIGHT      48        // Top status bar
#define CONTENT_AREA_HEIGHT    (SCREEN_HEIGHT - STATUS_BAR_HEIGHT)

// Color Constants for Display
#define COLOR_DAY_BACKGROUND   0x0000    // Black
#define COLOR_NIGHT_BACKGROUND 0x0841    // Dark blue
#define COLOR_BACKGROUND       COLOR_DAY_BACKGROUND

// System Constants
#define WAKE_MOTION_THRESHOLD  0.3       // Motion threshold (g-force)

// Button Constants
#define testCH423Connection    testI2CConnection  // Fix function name error

// Aliases and missing button timing constants for button_manager.h compatibility
#define BUTTON_DEBOUNCE_MS      BTN_DEBOUNCE_MS     // Debounce time (aliased from existing)
#define BUTTON_HOLD_MS          BTN_LONG_PRESS_MS   // Hold time (aliased from existing long press)
#define BUTTON_DOUBLE_CLICK_MS  300                 // Double-click detection window in milliseconds
#define BUTTON_SCAN_INTERVAL_MS BUTTON_CHECK_INTERVAL_MS  // Scan interval (aliased from existing check interval)


// Button States & Timing
#define BTN_PRESSED             1
#define BTN_RELEASED            0
#define BTN_DEBOUNCE_MS        50       // Debounce time in milliseconds
#define BTN_LONG_PRESS_MS    1000       // Long press threshold
#define BTN_REPEAT_DELAY_MS   500       // Button repeat delay

// ========================================
// DISPLAY CONFIGURATION
// ========================================

// Display Specifications
#define SCREEN_WIDTH          320       // TFT width in pixels
#define SCREEN_HEIGHT         480       // TFT height in pixels
#define SCREEN_ROTATION         0       // Portrait mode (0° rotation)

// Compatibility aliases for existing code
#define DISPLAY_WIDTH         SCREEN_WIDTH
#define DISPLAY_HEIGHT        SCREEN_HEIGHT

// Display Zones (Portrait Layout for Vision-Optimized UI)
#define STATUS_BAR_HEIGHT      48       // Top 10% (48px) - GPS status
#define CONTENT_AREA_HEIGHT   288       // Middle 60% (288px) - Map/GPS
#define BOTTOM_BAR_HEIGHT     144       // Bottom 30% (144px) - Speed/heading

// Display Performance
#define SPI_FREQUENCY    40000000       // 40MHz SPI for fast updates
#define TFT_BUFFER_SIZE       2048      // Buffer size for DMA transfers
#define PWM_FREQ              5000      // 5kHz PWM frequency for backlight
#define PWM_CHANNEL              0      // PWM channel for backlight
#define PWM_RESOLUTION           8      // 8-bit PWM resolution (0-255)

// ========================================
// COLOR DEFINITIONS FOR UI
// ========================================

// Day Mode Colors (High Contrast)
#define COLOR_DAY_BACKGROUND    0x0000  // Black
#define COLOR_DAY_TEXT          0xFFFF  // White
#define COLOR_DAY_TEXT_SEC      0xBDF7  // Light Gray
#define COLOR_DAY_ACCENT        0x07E0  // Bright Green
#define COLOR_DAY_GPS_GOOD      0x07E0  // Bright Green
#define COLOR_DAY_GPS_WARN      0xFFE0  // Yellow
#define COLOR_DAY_GPS_BAD       0xF800  // Red
#define COLOR_DAY_RECORDING     0xF800  // Red
#define COLOR_DAY_PAUSED        0xFFE0  // Yellow

// Night Mode Colors (Red Tinted for Night Vision)
#define COLOR_NIGHT_BACKGROUND  0x0000  // Black
#define COLOR_NIGHT_TEXT        0xF800  // Dim Red
#define COLOR_NIGHT_TEXT_SEC    0x8800  // Darker Red
#define COLOR_NIGHT_ACCENT      0x8800  // Darker Red
#define COLOR_NIGHT_GPS_GOOD    0x8800  // Darker Red
#define COLOR_NIGHT_GPS_WARN    0x8800  // Darker Red
#define COLOR_NIGHT_GPS_BAD     0x4000  // Very Dark Red
#define COLOR_NIGHT_RECORDING   0xF800  // Red
#define COLOR_NIGHT_PAUSED      0x8800  // Darker Red

// ADD these missing color constants (add to appropriate header file)
#define COLOR_STATUS_OK     0x07E0    // Green
#define COLOR_STATUS_WARN   0xFFE0    // Yellow  
#define COLOR_STATUS_ERROR  0xF800    // Red
#define COLOR_TEXT_PRIMARY  0xFFFF    // White
#define COLOR_TEXT_SECOND   0xC618    // Light gray

// ADD these color aliases to config.h (after existing color definitions):

// GPS Colors (aliases for existing)
#define COLOR_GPS_GOOD          COLOR_DAY_GPS_GOOD      // Green
#define COLOR_GPS_MARGINAL      COLOR_DAY_GPS_WARN      // Yellow
#define COLOR_GPS_POOR          COLOR_DAY_GPS_BAD       // Red

// Night Mode Colors (aliases for existing)
#define COLOR_NIGHT_GOOD        COLOR_NIGHT_GPS_GOOD    // Dark red
#define COLOR_NIGHT_WARN        COLOR_NIGHT_GPS_WARN    // Dark red
#define COLOR_NIGHT_ERROR       COLOR_NIGHT_GPS_BAD     // Very dark red
#define COLOR_NIGHT_TEXT        0xF800        // Red

// Additional missing colors
#define TEXT_CENTER_X(text, size)  ((SCREEN_WIDTH - (strlen(text) * (6 * size))) / 2)

// ========================================
// GPS CONFIGURATION
// ========================================
#define GPS_UPDATE_RATE         10       // GPS update rate in Hz
#define GPS_COORDINATE_PRECISION 4       // Decimal places for coordinates
#define GPS_TIMEOUT_MS          5000     // GPS timeout for no data
#define GPS_COLD_START_TIMEOUT  120000   // Cold start timeout (2 minutes)
#define GPS_WARM_START_TIMEOUT  30000    // Warm start timeout (30 seconds)

// GPS Message Configuration
#define GPS_ENABLE_GGA          true     // Enable GGA messages
#define GPS_ENABLE_RMC          true     // Enable RMC messages
#define GPS_ENABLE_GSV          false    // Disable GSV messages (verbose)
#define GPS_ENABLE_GSA          false    // Disable GSA messages

// GPS Quality Thresholds
#define GPS_MIN_SATELLITES      4        // Minimum satellites for fix
#define GPS_MAX_HDOP            5.0      // Maximum HDOP for good fix
#define GPS_EXCELLENT_HDOP      1.0      // HDOP for excellent fix
#define GPS_GOOD_HDOP           2.0      // HDOP for good fix

// ADD this line to config.h (after BTN_COUNT definition):
#define BUTTON_COUNT    BTN_COUNT    // Alias for compatibility

#define BATTERY_GOOD           12.6f    // Good battery voltage
#define BATTERY_LOW            11.8f    // Low battery voltage

// GPS Power Mode - Using enums from gps_manager.h instead of macros
// Values defined in GPSPowerState enum to avoid preprocessor conflicts

// ========================================
// COMPASS CONFIGURATION  
// ========================================
#define COMPASS_I2C_ADDRESS     0x1E     // HMC5883L/QMC5883L address
#define COMPASS_UPDATE_RATE     50       // Compass update rate in Hz
#define COMPASS_DECLINATION     0.0      // Magnetic declination (degrees)

// Compass Filtering & Smoothing
#define COMPASS_FILTER_ALPHA    0.1f     // Low-pass filter coefficient
#define COMPASS_MOTION_THRESHOLD 0.5f    // Motion detection threshold (g)
#define COMPASS_STILLNESS_TIME  3000     // Time to detect stillness (ms)
#define COMPASS_HEADING_SMOOTHING 0.8    // Heading smoothing factor
#define COMPASS_HEADING_HOLD_SPEED 2.0   // Speed threshold for heading hold

// Compass Calibration
#define COMPASS_CAL_TIMEOUT     60000    // Calibration timeout (60 seconds)
#define COMPASS_CAL_MIN_POINTS  100      // Minimum calibration points
#define COMPASS_CAL_QUALITY_MIN 70.0     // Minimum calibration quality (%)

// Compass-GPS Fusion
#define COMPASS_GPS_BLEND_SPEED 5.0      // Speed threshold for GPS heading blend (km/h)
#define COMPASS_HEADING_FILTER  0.2f     // Heading smoothing filter

// ========================================
// TRACKING CONFIGURATION
// ========================================

// Track Mode Constants (for use in code - NOT conflicting with enums)
#define TRACK_MODE_AUTO         2        // Auto tracking mode constant
#define TRACK_MODE_MANUAL       1        // Manual tracking mode constant
#define TRACK_MODE_OFF          0        // Off tracking mode constant
#define TRACK_MODE_CONTINUOUS   3        // Continuous tracking mode constant

// Track Filtering Settings
#define TRACK_FILTER_ENABLED    true     // Enable intelligent filtering
#define TRACK_MIN_DISTANCE_M    5.0      // Minimum distance between points (meters)
#define TRACK_MIN_SPEED_KMH     2.0      // Minimum speed for recording (km/h)
#define TRACK_MAX_INTERVAL_MS   5000     // Maximum time between points (ms)
#define TRACK_MIN_MOTION_G      0.2f     // Minimum motion for recording (g)

// Track File Management
#define TRACK_AUTO_SAVE_POINTS  100      // Auto-save every N points
#define TRACK_MAX_POINTS_PER_FILE 5000   // Maximum points per GPX file
#define TRACK_MAX_FILENAME_LEN  32       // Maximum track filename length

// ========================================
// POWER MANAGEMENT CONFIGURATION
// ========================================

// Battery Monitoring - Physical Constants
#define BATTERY_R1              220000.0 // 220kΩ high-side resistor
#define BATTERY_R2              47000.0  // 47kΩ low-side resistor  
#define BATTERY_VREF            3.3      // 3.3V ADC reference
#define BATTERY_ADC_BITS        4095     // 4095 for 12-bit ADC

// Battery Voltage Levels (for 12V motorcycle battery)
#define BATTERY_VOLTAGE_DIVIDER 4.68f    // Voltage divider ratio (220k/47k)
#define BATTERY_ADC_RESOLUTION  4095.0   // 12-bit ADC resolution
#define BATTERY_REFERENCE_V     3.3f     // ADC reference voltage
#define BATTERY_FULL_VOLTAGE    13.2f    // Full battery voltage (charging)
#define BATTERY_GOOD_VOLTAGE    12.6f    // Good battery voltage (engine off)
#define BATTERY_LOW_VOLTAGE     12.0f    // Low battery warning voltage
#define BATTERY_CRITICAL_VOLTAGE 11.0f   // Critical battery voltage
#define BATTERY_EMPTY_VOLTAGE   10.5f    // Empty battery voltage

// Power Management Timeouts
#define DISPLAY_TIMEOUT_MS      30000    // Display sleep timeout (30 seconds)
#define SYSTEM_SLEEP_TIMEOUT_MS 300000   // System sleep timeout (5 minutes)
#define DEEP_SLEEP_TIMEOUT_MS   1800000  // Deep sleep timeout (30 minutes)

// Motion Detection
#define MOTION_TIMEOUT_MS       300000   // 5 minutes motion timeout
#define MOTION_THRESHOLD_G      0.1f     // Motion detection sensitivity

// Wake-on-Motion Constants (ADDED)
#define WAKE_MOTION_THRESH      0.3f     // Motion threshold for wake (g)
#define WAKE_MOTION_DURATION    500      // Motion duration for wake (ms)

// ========================================
// STORAGE CONFIGURATION
// ========================================

// SD Card Configuration
#define SD_CARD_SPEED           80000000 // 80MHz SPI speed
#define SD_RETRY_COUNT          3        // Number of retry attempts
#define SD_TIMEOUT_MS           5000     // SD operation timeout

// Directory Structure
#define DIR_TRACKS              "/tracks"
#define DIR_WAYPOINTS           "/waypoints"  
#define DIR_MAPS                "/maps"
#define DIR_SETTINGS            "/settings"
#define DIR_LOGS                "/logs"
#define DIR_BACKUP              "/backup"
#define DIR_TEMP                "/temp"

// ========================================
// CONNECTIVITY CONFIGURATION
// ========================================

// WiFi Settings
#define WIFI_AP_SSID            "E_NAVI_GPS"
#define WIFI_AP_PASSWORD        "enduro123"
#define WIFI_AP_CHANNEL         6
#define WIFI_MAX_CONNECTIONS    4
#define WIFI_TIMEOUT_MS         30000    // 30 seconds

// Web Server Settings
#define WEB_SERVER_PORT         80
#define MAX_UPLOAD_SIZE         10485760 // 10MB max file size
#define MAX_FILENAME_LENGTH     64
#define SESSION_TIMEOUT_MS      1800000  // 30 minutes

// Web Server File Operations (ADDED)
#define MAX_SESSIONS            10       // Maximum concurrent sessions
#define UPLOAD_TEMP_DIR         "/temp"  // Temporary upload directory
#define DOWNLOAD_BUFFER_SIZE    8192     // Download buffer size

// ========================================
// UI AND DISPLAY CONSTANTS
// ========================================

// Font Sizes for Vision-Optimized UI
#define FONT_SIZE_LARGE         4        // 48pt - Status, speed, heading
#define FONT_SIZE_MEDIUM        2        // 24pt - Primary information
#define FONT_SIZE_SMALL         1        // 12pt - Secondary information
#define FONT_SIZE_TINY          1        // 8pt - Debug/details only

// UI Refresh and Display Constants
#define UI_REFRESH_INTERVAL_MS  16       // ~60fps (16.67ms)
#define UI_BUTTON_DEBOUNCE_MS   50       // Button debounce time
#define UI_LONG_PRESS_MS        1000     // Long press detection

// System Check Intervals
#define BUTTON_CHECK_INTERVAL_MS    5    // Check buttons every 5ms
#define SYSTEM_CHECK_INTERVAL_MS  100    // Check system every 100ms
#define DISPLAY_UPDATE_INTERVAL_MS 16    // Update display every 16ms (60fps)

// Tile and Zoom Constants  
#define TILE_CACHE_SIZE         50       // Maximum cached map tiles
#define TILE_CACHE_COUNT        50       // Same as TILE_CACHE_SIZE
#define ZOOM_DEFAULT            14       // Default zoom level
#define ZOOM_MIN_LEVEL          8        // Minimum zoom level
#define ZOOM_MAX_LEVEL          18       // Maximum zoom level
#define ZOOM_SPEED_MIN          0.0      // Minimum speed for auto-zoom (km/h)
#define ZOOM_SPEED_MAX          100.0    // Maximum speed for auto-zoom (km/h)
#define ZOOM_MANUAL_TIMEOUT_MS  5000    // Manual zoom override timeout in milliseconds (30 seconds)

// ========================================
// MENU SYSTEM CONFIGURATION
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

// Maximum Items (Reduced for Better Visibility)
#define MAX_MENU_ITEMS              6   // Fewer items per screen
#define MAX_TRACKS_DISPLAYED       15   // Fewer tracks in list
#define MAX_FILENAME_DISPLAY_LENGTH 20  // Shorter filenames for clarity

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
// MEMORY AND PERFORMANCE
// ========================================

// Task Stack Sizes
#define STACK_SIZE_GPS          4096     // GPS task stack
#define STACK_SIZE_DISPLAY      8192     // Display task stack
#define STACK_SIZE_TRACKING     4096     // Tracking task stack
#define STACK_SIZE_COMPASS      2048     // Compass task stack

// Task Priorities
#define PRIORITY_GPS            3        // GPS highest priority
#define PRIORITY_DISPLAY        2        // Display medium priority
#define PRIORITY_TRACKING       2        // Tracking medium priority
#define PRIORITY_COMPASS        1        // Compass lower priority

// Memory Allocation
#define MAX_GPX_BUFFER_SIZE     32768    // 32KB for GPX processing
#define MAX_TRACK_POINTS_CONFIG 2000     // Maximum track points in memory (renamed to avoid conflict)
#define MAX_WAYPOINTS_MEMORY    200      // Maximum waypoints in memory

// ========================================
// DEBUG AND LOGGING CONFIGURATION
// ========================================

// Debug Settings
#define DEBUG_ENABLED           true     // Enable debug output
#define DEBUG_GPS               true     // Enable GPS debug
#define DEBUG_COMPASS           true     // Enable compass debug
#define DEBUG_TRACKING          true     // Enable tracking debug
#define DEBUG_DISPLAY           false    // Enable display debug
#define DEBUG_WIFI              true     // Enable WiFi debug
#define DEBUG_POWER             true     // Enable power debug

// Serial Debug Settings
#define DEBUG_SERIAL_BAUD       115200   // Debug serial baud rate
#define DEBUG_BUFFER_SIZE       1024     // Debug message buffer size

// Performance Monitoring
#define ENABLE_PERFORMANCE_MONITOR true  // Enable performance monitoring
#define PERFORMANCE_LOG_INTERVAL 10000   // Performance log interval (ms)

// ========================================
// VERSION & BUILD INFORMATION
// ========================================

// Build configuration
#ifdef DEBUG_ENABLED
  #define BUILD_TYPE "DEBUG"
#else
  #define BUILD_TYPE "RELEASE"
#endif

// Feature flags for conditional compilation
#define FEATURE_WIFI_ENABLED        true
#define FEATURE_GPS_ENABLED         true
#define FEATURE_COMPASS_ENABLED     true
#define FEATURE_TRACKING_ENABLED    true
#define FEATURE_POWER_MGMT_ENABLED  true
#define FEATURE_SD_CARD_ENABLED     true

// ========================================
// HARDWARE VALIDATION MACROS
// ========================================

// Validate critical pins are defined
#if !defined(TFT_MOSI) || !defined(TFT_SCLK) || !defined(TFT_CS) || !defined(TFT_DC) || !defined(TFT_RST)
  #error "Critical TFT pins not properly defined"
#endif

#if !defined(GPS_TX) || !defined(GPS_RX) || !defined(GPS_UART_NUM)
  #error "GPS UART pins not properly defined"
#endif

#if !defined(I2C_SDA) || !defined(I2C_SCL)
  #error "I2C pins not properly defined"
#endif

// Validate pin conflicts
#if TFT_MOSI == SD_MOSI && TFT_SCLK == SD_SCLK
  // Shared SPI is OK
#else
  #error "TFT and SD card must share SPI pins"
#endif

#endif // CONFIG_H