/*
 * E_NAVI EnduroGPS - Loop-Loop Compass Calibration Header
 * User-Friendly Motorcycle Calibration System
 * 
 * Method: Left circle (30s) + Right circle (30s) = Perfect calibration
 * Perfect for motorcycles - no awkward figure-8 required
 * 
 * MATCHES THE ACTUAL IMPLEMENTATION IN loop_calibration.cpp
 */

#ifndef LOOP_CALIBRATION_H
#define LOOP_CALIBRATION_H

#include <Arduino.h>
#include "config.h"
#include "shared_data.h"

// Forward declarations to avoid circular includes
class DisplayManager;
class ButtonManager;

// ========================================
// COMPASS CALIBRATION STRUCT (Defined here to resolve unknown type)
// ========================================

// struct CompassCalibration {
//   float magOffsetX = 0.0;
//   float magOffsetY = 0.0;
//   float magOffsetZ = 0.0;
//   float magScaleX = 1.0;
//   float magScaleY = 1.0;
//   float magScaleZ = 1.0;
//   float declination = 0.0;
//   float calibrationQuality = 0.0;
//   float dataRange[3] = {0.0, 0.0, 0.0};
//   bool isCalibrated = false;
//   unsigned long calibrationDate = 0;
//   int calibrationPoints = 0;
//   String calibrationMethod = "Unknown";
// };

// ========================================
// LOOP-LOOP CALIBRATION CLASS (matches .cpp implementation)
// ========================================

class LoopLoopCalibration {
  private:
    // ========================================
    // CALIBRATION STATE (from actual implementation)
    // ========================================
    
    CalibrationState currentPhase = CAL_IDLE;  // Made public access via getter
    unsigned long phaseStartTime = 0;
    unsigned long totalStartTime = 0;
    
    // ========================================
    // DATA COLLECTION (from actual implementation)
    // ========================================
    
    static const int MAX_POINTS = 500;
    struct CalibrationPoint {
      float magX, magY, magZ;
      float gpsSpeed;
      unsigned long timestamp;
      bool validPoint;
    };
    
    CalibrationPoint points[MAX_POINTS];
    int pointCount = 0;
    int leftLoopPoints = 0;
    int rightLoopPoints = 0;
    
    // ========================================
    // QUALITY ASSESSMENT (from actual implementation)
    // ========================================
    
    float coverageScore = 0.0;
    float consistencyScore = 0.0;
    float overallQuality = 0.0;
    
    // ========================================
    // USER INTERFACE (from actual implementation)
    // ========================================
    
    bool userConfirmed = false;
    bool cancelRequested = false;
    String currentInstruction = "";
    
    // ========================================
    // RESULTS (from actual implementation)
    // ========================================
    
    CompassCalibration calculatedCalibration;
    bool calibrationSuccessful = false;
    
  public:
    // ========================================
    // CONSTRUCTOR & DESTRUCTOR
    // ========================================
    
    LoopLoopCalibration();
    ~LoopLoopCalibration();
    
    // ========================================
    // CALIBRATION CONTROL (from actual implementation)
    // ========================================
    
    bool startCalibration();
    void updateCalibration();
    void cancelCalibration();
    bool isActive() const { return currentPhase != CAL_IDLE; }
    
    // ========================================
    // DATA COLLECTION (from actual implementation)
    // ========================================
    
    void collectDataPoint();
    bool isDataPointValid();
    
    // ========================================
    // PHASE MANAGEMENT (from actual implementation)
    // ========================================
    
    void advanceToNextPhase();
    void setPhase(CalibrationState phase);
    
    // ========================================
    // QUALITY ASSESSMENT (from actual implementation)
    // ========================================
    
    void calculateQuality();
    float getQualityScore() const { return overallQuality; }
    
    // ========================================
    // RESULTS (from actual implementation)
    // ========================================
    
    bool processCalibrationData();
    CompassCalibration getResults() const;
    
    // ========================================
    // USER INTERFACE (from actual implementation)
    // ========================================
    
    void updateDisplay();
    void handleUserInput(ButtonID button, ButtonEvent event);
    
    // ========================================
    // PHASE-SPECIFIC METHODS (from actual implementation)
    // ========================================
    
    void showIntroduction();
    void showPreparation();
    void processLeftLoop();
    void processRightLoop();
    void processResults();
    void showError();

    // Getter for currentPhase (to resolve private access)
    CalibrationState getCurrentPhase() const { return currentPhase; }
};

// ========================================
    // GLOBAL INSTANCE (from actual implementation)
    // ========================================

extern LoopLoopCalibration loopCalibration;

// ========================================
    // GLOBAL ACCESS FUNCTIONS (if needed for compatibility)
    // ========================================

// These functions can be added if other parts of the system need them
inline bool startCompassCalibration() {
  return loopCalibration.startCalibration();
}

inline void updateCompassCalibration() {
  loopCalibration.updateCalibration();
}

inline void cancelCompassCalibration() {
  loopCalibration.cancelCalibration();
}

inline bool isCompassCalibrationActive() {
  return loopCalibration.isActive();
}

inline CalibrationState getCalibrationPhase() {
  return loopCalibration.getCurrentPhase();
}

inline float getCalibrationQuality() {
  return loopCalibration.getQualityScore();
}

inline CompassCalibration getCalibrationResults() {
  return loopCalibration.getResults();
}

// ========================================
    // CALIBRATION CONSTANTS (from config.h and implementation)
    // ========================================

// Phase timing
#define CALIBRATION_PHASE_DURATION_MS    30000    // 30 seconds per phase
#define CALIBRATION_TOTAL_DURATION_MS    60000    // Total calibration time

// Speed validation
#define CALIBRATION_MIN_SPEED_KMPH       2.0     // Minimum speed for valid data
#define CALIBRATION_MAX_SPEED_KMPH       15.0    // Maximum speed for valid data

// Quality thresholds
#define CALIBRATION_MIN_QUALITY_SCORE    60.0    // Minimum acceptable quality
#define CALIBRATION_MIN_POINTS           50      // Minimum data points required

#endif // LOOP_CALIBRATION_H 