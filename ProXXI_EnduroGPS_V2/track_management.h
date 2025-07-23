/*
 * E_NAVI EnduroGPS - Track Management System
 * File management and statistics for recorded tracks
 * 
 * FIXED VERSION - Removed extern TrackDisplayUI declaration to prevent conflicts
 * FIXED: Removed circular dependency with shared_data.h
 * 
 * Features: Track listing, statistics, file operations
 * Integration: Works with smart_tracker.h and sd_manager.h
 */

#ifndef TRACK_MANAGEMENT_H
#define TRACK_MANAGEMENT_H

#include <Arduino.h>
#include "config.h"
// Forward declare structures from shared_data.h to avoid circular dependency
struct TrackFileInfo;
struct TrackStatistics;

// ========================================
// TRACK FILE INFORMATION
// ========================================
struct TrackFileInfo {
  String filename;              // Full filename with path
  String displayName;           // User-friendly name
  String creationDate;          // Date created (DD/MM/YYYY)
  String creationTime;          // Time created (HH:MM)
  unsigned long fileSize;       // File size in bytes
  unsigned long duration;       // Track duration in seconds
  float totalDistance;          // Total distance in km
  int pointCount;              // Number of GPS points
  float maxSpeed;              // Maximum speed recorded
  float avgSpeed;              // Average speed
  float maxAltitude;           // Highest elevation
  float minAltitude;           // Lowest elevation
  float elevationGain;         // Total ascent
  float elevationLoss;         // Total descent
  bool isValid;                // File is valid GPX
  bool isCorrupted;            // File appears corrupted
};

struct TrackStatistics {
  // File Statistics
  int totalTracks = 0;
  unsigned long totalSize = 0;        // Total storage used
  unsigned long oldestTrack = 0;      // Timestamp of oldest track
  unsigned long newestTrack = 0;      // Timestamp of newest track
  
  // Distance Statistics
  float totalDistance = 0.0;          // All tracks combined
  float longestTrack = 0.0;           // Longest single track
  float shortestTrack = 999999.0;     // Shortest single track
  float avgTrackDistance = 0.0;       // Average track length
  
  // Time Statistics
  unsigned long totalTime = 0;        // All recording time
  unsigned long longestDuration = 0;  // Longest single track time
  unsigned long shortestDuration = 999999; // Shortest track time
  unsigned long avgDuration = 0;      // Average track duration
  
  // Speed Statistics
  float maxSpeedOverall = 0.0;        // Highest speed ever recorded
  float avgSpeedOverall = 0.0;        // Average speed across all tracks
  
  // Elevation Statistics
  float highestPoint = -999.0;        // Highest elevation reached
  float lowestPoint = 999999.0;       // Lowest elevation
  float totalElevationGain = 0.0;     // All climbs combined
  float totalElevationLoss = 0.0;     // All descents combined
};

// ========================================
// FORWARD DECLARATIONS TO AVOID CIRCULAR DEPENDENCIES
// ========================================
class SDManager;

// ========================================
// TRACK MANAGEMENT CLASS
// ========================================
class TrackManager {
  private:
    // File Management
    String trackDirectory = "/tracks";   // Default directory, will be set from config
    static const int MAX_TRACKS = 100;
    TrackFileInfo trackList[MAX_TRACKS];
    int trackCount = 0;
    bool listLoaded = false;
    
    // Statistics
    TrackStatistics statistics;
    bool statisticsValid = false;
    
    // Current Selection
    int selectedTrack = -1;
    String currentTrackFile = "";
    
    // Display State
    int displayOffset = 0;           // For scrolling through long lists
    int itemsPerPage = 8;            // Tracks visible per screen
    
    // Internal Methods
    bool loadTrackList();
    bool analyzeTrackFile(const String& filename, TrackFileInfo& info);
    bool parseGPXFile(const String& filename, TrackFileInfo& info);
    void calculateStatistics();
    void sortTrackList(const String& sortBy = "date");
    bool isValidGPXFile(const String& filename);
    
  public:
    TrackManager();
    ~TrackManager();
    
    // Initialization
    bool initialize();
    bool refreshTrackList();
    bool isInitialized() const { return listLoaded; }
    
    // Track List Access
    int getTrackCount() const { return trackCount; }
    TrackFileInfo getTrackInfo(int index);
    TrackFileInfo* getTrackList() { return trackList; }
    void setSelectedTrack(int index);
    int getSelectedTrack() const { return selectedTrack; }
    
    // Track Operations
    bool deleteTrack(int index);
    bool deleteTrack(const String& filename);
    bool renameTrack(int index, const String& newName);
    bool exportTrack(int index, const String& format = "gpx");
    bool duplicateTrack(int index);
    
    // Track Information
    TrackFileInfo getCurrentTrackInfo();
    String getTrackSummary(int index);
    String getDetailedTrackInfo(int index);
    bool validateTrack(int index);
    
    // File Operations
    unsigned long getTotalStorageUsed();
    unsigned long getAvailableStorage();
    float getStorageUsagePercent();
    bool cleanupOldTracks(int maxTracks = 50);
    bool compactTrackFiles();
    
    // Statistics
    TrackStatistics getStatistics();
    void updateStatistics();
    String getStatisticsSummary();
    void resetStatistics();
    
    // Search & Filter
    int findTrackByName(const String& name);
    int findTrackByDate(const String& date);
    int* findTracksByDistance(float minKm, float maxKm, int& count);
    int* findTracksByDuration(unsigned long minSeconds, unsigned long maxSeconds, int& count);
    
    // Sorting
    void sortByDate(bool ascending = false);       // Newest first by default
    void sortByName(bool ascending = true);
    void sortByDistance(bool ascending = false);   // Longest first
    void sortByDuration(bool ascending = false);   // Longest first
    
    // Current Track Integration
    void setCurrentTrack(const String& filename);
    String getCurrentTrackFilename() const { return currentTrackFile; }
    bool hasCurrentTrack() const { return currentTrackFile.length() > 0; }
    
    // Backup & Recovery
    bool backupAllTracks(const String& backupDir);
    bool restoreFromBackup(const String& backupDir);
    bool verifyTrackIntegrity();
    int repairCorruptedTracks();
    
    // Maintenance
    bool optimizeStorage();
    void scheduleCleanup();
    bool performMaintenance();
};

// ========================================
// TRACK ANALYSIS UTILITIES
// ========================================
class TrackAnalyzer {
  private:
    struct AnalysisPoint {
      double lat, lon;
      float alt, speed;
      unsigned long timestamp;
    };
    
    static const int MAX_ANALYSIS_POINTS = 1000;
    AnalysisPoint points[MAX_ANALYSIS_POINTS];
    int pointCount = 0;
    
  public:
    TrackAnalyzer();
    ~TrackAnalyzer();
    
    // Analysis Operations
    bool loadTrackForAnalysis(const String& filename);
    void clearAnalysisData();
    
    // Distance Calculations
    float calculateTotalDistance();
    float calculateStraightLineDistance();
    float calculateAverageSpeed();
    float calculateMaxSpeed();
    
    // Elevation Analysis
    float calculateElevationGain();
    float calculateElevationLoss();
    float calculateMaxElevation();
    float calculateMinElevation();
    float calculateAverageGradient();
    
    // Time Analysis
    unsigned long calculateTotalTime();
    unsigned long calculateMovingTime();
    unsigned long calculateStoppedTime();
    int countStops(float speedThreshold = 1.0);
    
    // Route Characteristics
    float calculateRouteComplexity();     // Based on direction changes
    float calculateTerrainRoughness();    // Based on elevation changes
    String classifyRouteType();           // Road, trail, mixed, etc.
    String assessDifficultyLevel();       // Easy, moderate, hard, extreme
    
    // Quality Assessment
    float assessTrackQuality();           // GPS accuracy, point density
    bool detectGPSErrors();               // Outliers, jumps, etc.
    int countSuspiciousPoints();
    
    // Export Analysis
    String generateAnalysisReport();
    bool saveAnalysisToFile(const String& filename);
};

// ========================================
// GLOBAL INSTANCES - REMOVED TrackDisplayUI extern
// ========================================
extern TrackManager trackManager;
extern TrackAnalyzer trackAnalyzer;
// TrackDisplayUI is declared extern in menu_system.h where it's defined

// ========================================
// UTILITY FUNCTIONS
// ========================================

// File Name Utilities
String generateTrackName();                    // Auto-generate track name
String sanitizeTrackName(const String& name);  // Remove invalid characters
bool isValidTrackName(const String& name);
String getTrackNameFromFile(const String& filename);

// Date/Time Formatting
String formatTrackDate(unsigned long timestamp);
String formatTrackTime(unsigned long timestamp);
String formatDuration(unsigned long seconds);
String formatFileSize(unsigned long bytes);

// Distance/Speed Formatting
String formatDistance(float km);
String formatSpeed(float kmh);
String formatElevation(float meters);

// File Operations
bool copyTrackFile(const String& source, const String& destination);
bool moveTrackFile(const String& source, const String& destination);
bool validateGPXStructure(const String& filename);
unsigned long getTrackFileTimestamp(const String& filename);

// Statistics Helpers
float calculatePercentile(float* values, int count, float percentile);
float calculateMedian(float* values, int count);
float calculateStandardDeviation(float* values, int count);

// Search Utilities
bool matchesSearchCriteria(const TrackFileInfo& track, const String& criteria);
String highlightSearchTerms(const String& text, const String& searchTerm);

// ========================================
// TRACK MANAGEMENT CONFIGURATION
// ========================================

// File Limits
#define MAX_TRACK_FILES         100
#define MAX_TRACK_NAME_LENGTH   32
#define MAX_TRACK_SIZE_MB       10      // Maximum individual track size
#define MAX_TOTAL_SIZE_MB       500     // Maximum total track storage

// Cleanup Thresholds
#define AUTO_CLEANUP_THRESHOLD  80      // Cleanup when 80% full
#define OLD_TRACK_DAYS          90      // Tracks older than 90 days
#define MIN_TRACK_DISTANCE      0.1     // Minimum 100m to keep track

// Display Settings
#define TRACKS_PER_PAGE         8       // Tracks visible per screen
#define TRACK_ITEM_HEIGHT       40      // Height per list item
#define DETAILS_SCROLL_SPEED    3       // Lines per scroll action

// Analysis Settings
#define ANALYSIS_SAMPLE_RATE    10      // Analyze every 10th point for speed
#define MIN_MOVING_SPEED        1.0     // km/h minimum for "moving"
#define STOP_DURATION_MIN       30      // Seconds minimum for a "stop"

#endif // TRACK_MANAGEMENT_H