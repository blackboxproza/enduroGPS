 /*
 * TFT_eSPI User_Setup.h for E_NAVI EnduroGPS
 * Configuration for JC3248W535C ESP32-S3 with AXS15231B driver
 * 
 * IMPORTANT: This file must be placed in:
 * Arduino/libraries/TFT_eSPI/User_Setup.h
 * (Replace the existing User_Setup.h)
 */

#ifndef USER_SETUP_H
#define USER_SETUP_H

// ========================================
// DRIVER SELECTION - AXS15231B Compatible
// ========================================
#define ST7796_DRIVER    // AXS15231B is compatible with ST7796 driver

// ========================================
// ESP32-S3 PIN CONFIGURATION
// ========================================
// Exact pins for JC3248W535C module

// SPI Interface (Hardware SPI)
#define TFT_MOSI   11    // ESP32-S3 GPIO11 → Display MOSI (SDA)
#define TFT_SCLK   12    // ESP32-S3 GPIO12 → Display SCLK (SCL)
#define TFT_CS     10    // ESP32-S3 GPIO10 → Display CS
#define TFT_DC      8    // ESP32-S3 GPIO8  → Display D/C (Data/Command)
#define TFT_RST     9    // ESP32-S3 GPIO9  → Display Reset

// Backlight Control
#define TFT_BL     46    // ESP32-S3 GPIO46 → Backlight PWM control

// No touch screen pins defined (using hardware buttons instead)
// #define TOUCH_CS    // Not used - using CH423S buttons for weather reliability

// ========================================
// DISPLAY SPECIFICATIONS
// ========================================
#define TFT_WIDTH  320   // Physical width in pixels
#define TFT_HEIGHT 480   // Physical height in pixels

// Display Orientation
#define TFT_ROTATION 0   // Portrait mode (0° rotation)
                        // 0 = Portrait, 1 = Landscape, 2 = Portrait flipped, 3 = Landscape flipped

// ========================================
// SPI PERFORMANCE CONFIGURATION
// ========================================
// High-speed SPI for 60fps updates with large fonts

#define SPI_FREQUENCY       80000000   // 80MHz - Maximum safe speed for drawing
#define SPI_READ_FREQUENCY  20000000   // 20MHz - For reading display data
#define SPI_TOUCH_FREQUENCY  2500000   // 2.5MHz - Not used (no touch screen)

// SPI Port Selection (ESP32-S3 has multiple SPI ports)
#define USE_HSPI_PORT       // Use high-speed SPI port for better performance

// ========================================
// FONT CONFIGURATION FOR AGING EYES
// ========================================
// Load specific fonts optimized for outdoor visibility

#define LOAD_GLCD      // Font 1: Original Adafruit 8 pixel font (debug only)
#define LOAD_FONT2     // Font 2: Small 16 pixel high font (secondary info)
#define LOAD_FONT4     // Font 4: Medium 26 pixel high font (primary info)
#define LOAD_FONT6     // Font 6: Large 48 pixel high font (speed, heading)
#define LOAD_FONT7     // Font 7: 7 segment 48 pixel high font (numbers)
#define LOAD_FONT8     // Font 8: Large 75 pixel high font (status)

// Anti-aliased fonts for smoother appearance
#define SMOOTH_FONT    // Enable anti-aliased fonts (better readability)

// Custom fonts for outdoor use (if available)
//#define LOAD_GFXFF   // FreeFonts - can be loaded if needed

// ========================================
// COLOR DEPTH & PIXEL FORMAT
// ========================================
#define TFT_RGB_ORDER TFT_BGR  // Color order for AXS15231B (Blue-Green-Red)
                              // Some displays use RGB, others BGR - test both if colors are wrong

// Color depth
// #define TFT_PARALLEL_8_BIT  // Not used - using SPI interface
// #define TFT_PARALLEL_16_BIT // Not used - using SPI interface

// ========================================
// PERFORMANCE OPTIMIZATIONS
// ========================================

// DMA for faster transfers (ESP32-S3 specific)
#define SPI_DMA_CHANNEL  1     // Use DMA channel 1 for SPI transfers

// Buffer optimizations
#define TFT_BUFFER_SIZE  2048  // Buffer size for DMA transfers

// Hardware acceleration
#define USE_DMA_TO_TFT         // Use DMA for faster screen updates

// ========================================
// POWER MANAGEMENT
// ========================================

// Backlight PWM configuration
#define TFT_BACKLIGHT_ON  HIGH // Backlight enable logic level
#define PWM_FREQ         5000  // 5kHz PWM frequency for backlight
#define PWM_CHANNEL      0     // PWM channel for backlight control
#define PWM_RESOLUTION   8     // 8-bit PWM resolution (0-255)

// ========================================
// COMPATIBILITY SETTINGS
// ========================================

// ESP32-S3 specific settings
#define ESP32_PARALLEL_NONE    // No parallel interface used

// Disable touch if accidentally enabled
#define DISABLE_ALL_LIBRARY_WARNINGS  // Reduce compilation warnings

// Memory allocation
#define PSRAM_ENABLE    // Enable PSRAM usage for large buffers if available

// ========================================
// DEBUGGING & TESTING
// ========================================

// Uncomment for debugging display issues
// #define TFT_DEBUG     // Enable debug output
// #define TFT_DEBUG_PIN 2  // Debug pin for oscilloscope testing

// Performance monitoring
// #define BENCHMARK_MODE  // Enable performance benchmarks

// ========================================
// VALIDATION MACROS
// ========================================

// Ensure critical pins are defined
#ifndef TFT_MOSI
  #error "TFT_MOSI pin not defined"
#endif

#ifndef TFT_SCLK  
  #error "TFT_SCLK pin not defined"
#endif

#ifndef TFT_CS
  #error "TFT_CS pin not defined"
#endif

#ifndef TFT_DC
  #error "TFT_DC pin not defined"
#endif

#ifndef TFT_RST
  #error "TFT_RST pin not defined"
#endif

// Validate display dimensions
#if TFT_WIDTH != 320 || TFT_HEIGHT != 480
  #warning "Display dimensions may not match hardware"
#endif

// Validate SPI frequency
#if SPI_FREQUENCY > 80000000
  #warning "SPI frequency may be too high for stable operation"
#endif

// ========================================
// ADVANCED CONFIGURATION
// ========================================

// Color inversion (test both if colors are wrong)
// #define TFT_INVERSION_ON   // Uncomment if colors are inverted
// #define TFT_INVERSION_OFF  // Default - normal colors

// Display initialization delay
#define TFT_INIT_DELAY 150   // ms delay after reset before initialization

// Custom initialization sequence (if needed)
// #define TFT_INIT_SEQUENCE  // Define custom init sequence if display doesn't work

// ========================================
// LIBRARY VERSION COMPATIBILITY
// ========================================

// Minimum TFT_eSPI version required: 2.5.43+
// ESP32 Core version required: 2.0.14+

// Check library version (if possible)
// #ifdef TFT_ESPI_VERSION
//   #if TFT_ESPI_VERSION < 25043
//     #error "TFT_eSPI version 2.5.43 or higher required"
//   #endif
// #endif

// ========================================
// HARDWARE-SPECIFIC WORKAROUNDS
// ========================================

// Some AXS15231B displays may need specific timing
#define TFT_CS_DELAY    0    // Delay after CS assertion (nanoseconds)
#define TFT_DC_DELAY    0    // Delay after D/C change (nanoseconds)
#define TFT_RST_DELAY   5    // Reset pulse width (milliseconds)

// Power-on sequence timing
#define TFT_POWER_DELAY 20   // Delay after power-on (milliseconds)

// ========================================
// NOTES FOR TROUBLESHOOTING
// ========================================

/*
 * If display doesn't work, try these steps:
 * 
 * 1. Verify pin connections match config.h exactly
 * 2. Try different SPI frequencies (40MHz, 20MHz, 10MHz)
 * 3. Test with TFT_INVERSION_ON if colors are wrong
 * 4. Check if TFT_RGB_ORDER should be TFT_RGB instead of TFT_BGR
 * 5. Verify 3.3V power supply is stable and adequate
 * 6. Test basic TFT_eSPI examples first before E_NAVI code
 * 7. Use oscilloscope to verify SPI signals if available
 * 8. Check for loose connections on breadboard/PCB
 * 9. Ensure ESP32-S3 has adequate power (USB may not be enough)
 * 10. Try different ESP32 core versions if problems persist
 * 
 * Common Issues:
 * - White screen: Usually pin configuration or power issue
 * - Wrong colors: Try TFT_INVERSION_ON or change RGB/BGR order
 * - Flickering: Reduce SPI frequency or check power supply
 * - No backlight: Verify TFT_BL pin and PWM configuration
 * - Slow updates: Ensure DMA is enabled and frequency is high enough
 * 
 * For E_NAVI specific issues:
 * - Verify this User_Setup.h matches exactly with config.h pin definitions
 * - Test display with simple text before complex UI elements
 * - Check that fonts load correctly (LOAD_FONT2, LOAD_FONT4, etc.)
 * - Ensure adequate power for both ESP32-S3 and display under load
 */

#endif // USER_SETUP_H