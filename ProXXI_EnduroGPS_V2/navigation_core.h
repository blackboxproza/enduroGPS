#ifndef NAVIGATION_CORE_H
#define NAVIGATION_CORE_H

#include "config.h"
#include "shared_data.h"
#include "gps_manager.h"
#include "compass_manager.h"
#include "gpx_parser.h"

// ===================================================================
// NAVIGATION CORE - INTELLIGENT NAVIGATION WITH SMART FILTERING
// Core 1 Task Implementation - Priority 1 (Normal)
// 80-90% file size reduction through intelligent point filtering
// ===================================================================

// Forward declarations to avoid circular includes
class GPSManager;
class CompassManager;

// Navigation state
enum NavigationState {
  NAV_IDLE,                    // No active navigation
  NAV_FOLLOWING_TRACK,         // Following loaded GPX track
  NAV_FOLLOWING_ROUTE,         // Following route to destination
  NAV_RECORDING_TRACK,         // Recording track with filtering
  NAV_WAYPOINT_NAVIGATION      // Navigating to single waypoint
};

enum GPSPowerState {
  GPS_STATE_FULL_POWER,
  GPS_STATE_LIGHT_SLEEP,
  GPS_STATE_DEEP_SLEEP,
  GPS_STATE_OFF
};

// Waypoint information
struct NavigationWaypoint {
  double latitude = 0.0;
  double longitude = 0.0;
  String name = "";
  String description = "";
  float distanceFromStart = 0.0;   // km from track start
  int trackPointIndex = -1;        // Index in original track
  bool isActive = false;
  bool isReached = false;
};

// NOTE: NavigationData struct is now defined in shared_data.h to avoid duplication

// ===================================================================
// INTELLIGENT GPS POINT FILTERING - 80-90% REDUCTION
// ===================================================================

// Filtering algorithms
enum FilterAlgorithm {
  FILTER_NONE,                 // No filtering (100% of points)
  FILTER_DISTANCE_BASIC,       // Simple distance-based
  FILTER_DOUGLAS_PEUCKER,      // Line simplification
  FILTER_ADAPTIVE_SMART,       // Speed/direction/time adaptive
  FILTER_PROFESSIONAL          // All algorithms combined
};

// Point filtering configuration
struct FilterConfig {
  FilterAlgorithm algorithm = FILTER_PROFESSIONAL;
  
  // Distance filtering
  float minDistance = 5.0;             // meters - minimum point spacing
  float maxDistance = 100.0;           // meters - maximum point spacing
  
  // Speed-based filtering
  float minSpeed = 2.0;                // km/h - minimum speed to record
  float speedThreshold = 10.0;         // km/h - different rules above/below
  
  // Direction filtering
  float minDirectionChange = 15.0;     // degrees - minimum heading change
  float significantTurn = 45.0;        // degrees - always record sharp turns
  
  // Time filtering
  unsigned long minInterval = 1000;    // ms - minimum time between points
  unsigned long maxInterval = 30000;   // ms - maximum time between points
  
  // Adaptive parameters
  float lowSpeedDetail = 2.0;          // Higher detail at low speeds
  float highSpeedSimplify = 3.0;       // More simplification at high speeds
  bool preserveStops = true;           // Always record start/stop points
  bool preserveTurns = true;           // Always record significant turns
  bool preserveSpeed = true;           // Always record speed changes
  
  // Quality settings
  float qualityFactor = 1.0;           // 0.1=max compression, 2.0=max detail
  bool enableAdaptive = true;          // Enable adaptive filtering
  bool enableSmoothing = true;         // Enable track smoothing
};

// Filter statistics
struct FilterStats {
  unsigned long totalPointsReceived = 0;
  unsigned long pointsFiltered = 0;
  unsigned long pointsSaved = 0;
  float compressionRatio = 0.0;        // 0.1 = 90% reduction
  float averageDistance = 0.0;         // Average point spacing
  unsigned long processingTime = 0;    // microseconds per point
  bool isActive = false;
};

class NavigationCore {
private:
  // Core components
  GPSManager* gpsManager;
  CompassManager* compassManager;
  
  // Navigation state
  NavigationState currentState = NAV_IDLE;
  NavigationData navData;               // Uses NavigationData from shared_data.h
  
  // Loaded track/route
  GPXTrack currentTrack;
  bool hasLoadedTrack = false;
  int currentTrackPoint = 0;
  
  // Point filtering system
  FilterConfig filterConfig;
  FilterStats filterStats;
  GPXTrackPoint lastRecordedPoint;
  bool hasLastPoint = false;
  
  // Navigation calculations
  unsigned long lastNavigationUpdate = 0;
  const unsigned long navigationUpdateInterval = 100; // 100ms = 10Hz
  
  // Performance tracking
  unsigned long calculationsPerSecond = 0;
  unsigned long lastPerformanceUpdate = 0;
  
public:
  // Initialization
  bool initialize(GPSManager* gps, CompassManager* compass);
  
  // Core navigation processing (called from Core 1 task)
  void update();
  void processGPSPoint();
  void calculateNavigation();
  void updateTrackProgress();
  
  // Track/Route loading
  bool loadTrack(const GPXTrack& track);
  bool loadRoute(const GPXWaypoint waypoints[], int count);
  bool setDestination(double lat, double lon, const String& name);
  void clearNavigation();
  
  // Navigation control
  void startNavigation();
  void stopNavigation();
  void pauseNavigation();
  void resumeNavigation();
  
  // Intelligent point filtering - FIXED: Single consistent declarations
  bool shouldRecordPoint(const GPXTrackPoint& point);
  bool applyProfessionalFilter(const GPXTrackPoint& point);  // FIXED: Only bool version
  bool applyDistanceFilter(const GPXTrackPoint& point);
  bool applyAdaptiveSmartFilter(const GPXTrackPoint& point); 
  bool applyBasicDistanceFilter(const GPXTrackPoint& point);
  void applyDouglasPeuckerFilter();
  
  // Navigation calculations
  float calculateDistance(double lat1, double lon1, double lat2, double lon2);
  float calculateBearing(double lat1, double lon1, double lat2, double lon2);
  float calculateCrossTrackError(double lat, double lon);
  NavigationWaypoint findNearestWaypoint(double lat, double lon);
  void calculateWaypointNavigation(double currentLat, double currentLon);
  
  // Track following
  void updateCurrentWaypoint();
  void checkWaypointReached();
  void calculateTrackProgress();
  void estimateArrivalTimes();
  
  // Getters
  NavigationState getNavigationState();
  NavigationData getNavigationData();  // Returns NavigationData from shared_data.h
  FilterStats getFilterStatistics();
  bool isNavigationActive();
  
  // Configuration
  void setFilterConfig(const FilterConfig& config);
  FilterConfig getFilterConfig();
  void setFilterAlgorithm(FilterAlgorithm algorithm);
  void setQualityFactor(float factor);
  
private:
  // Internal calculations
  bool isSignificantMovement(const GPXTrackPoint& point);
  bool isSignificantDirectionChange(const GPXTrackPoint& point);
  bool isSignificantSpeedChange(const GPXTrackPoint& point);
  bool shouldPreservePoint(const GPXTrackPoint& point);
  
  // Adaptive filtering helpers
  float calculateAdaptiveMinDistance(float speed);
  float calculateAdaptiveDirectionThreshold(float speed);
  unsigned long calculateAdaptiveTimeInterval(float speed);
  
  // Douglas-Peucker algorithm
  float perpendicularDistance(const GPXTrackPoint& point, 
                             const GPXTrackPoint& lineStart, 
                             const GPXTrackPoint& lineEnd);
  void douglasPeuckerRecursive(GPXTrackPoint points[], int start, int end, 
                              float epsilon, bool keep[]);
  
  // Performance optimization
  void updatePerformanceStats();
  void optimizeUpdateInterval();
};

// ===================================================================
// SMART GPS POWER MANAGEMENT - INTEGRATED WITH NAVIGATION
// ===================================================================

class SmartGPSPowerManager {
private:
  NavigationCore* navigation;
  GPSManager* gpsManager;
  
  // Power state tracking
  GPSPowerMode currentPowerState = GPS_POWER_FULL;
  unsigned long lastPowerChange = 0;
  unsigned long lastMotionTime = 0;
  unsigned long lastNavigationActivity = 0;
  
  // Power management configuration
  struct PowerConfig {
    unsigned long motionTimeout = 30000;      // 30s → Light sleep
    unsigned long deepSleepTimeout = 120000;  // 2min → Deep sleep
    unsigned long sleepTimeout = 300000;      // 5min → Sleep mode
    bool enableAdaptivePower = true;
    bool maintainDuringNavigation = true;
    bool motionBasedWake = true;
  } powerConfig;
  
public:
  // Initialization
  bool initialize(NavigationCore* nav, GPSManager* gps);
  
  // Power management (called from Core 1 task)
  void update();
  void handleMotionEvent(bool motionDetected);
  void handleNavigationStateChange(NavigationState state);
  
  // Power state control - FIXED: Only GPSPowerMode version
  void transitionToPowerState(GPSPowerMode newState);
  GPSPowerMode getCurrentPowerState();
  bool shouldTransitionPower();
  
  // Configuration
  void setPowerConfig(const PowerConfig& config);
  PowerConfig getPowerConfig();
  void setAdaptivePower(bool enable);
  
  // Status
  unsigned long getTimeSinceLastMotion();
  bool isInPowerSaveMode();
  String getPowerStateText();

private:
  // Power decision logic - FIXED: Only GPSPowerMode version
  GPSPowerMode calculateOptimalPowerState();
  bool isNavigationActive();
  bool hasRecentMotion();
  bool shouldMaintainFullPower();
  
  // State transition helpers
  void transitionToLightSleep();
  void transitionToDeepSleep();
  void transitionToSleep();
  void transitionToFull();
};

// Global instances
extern NavigationCore navigationCore;
extern SmartGPSPowerManager gpsPowerManager;

// Quick access functions for Core 1 task
bool initializeNavigationCore();
void updateNavigationCore();
void processNavigationPoint();
bool loadNavigationTrack(const GPXTrack& track);
NavigationData getCurrentNavigation();   // Returns NavigationData from shared_data.h
FilterStats getFilteringStats();

#endif // NAVIGATION_CORE_H