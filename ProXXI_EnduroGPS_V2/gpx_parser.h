#ifndef GPX_PARSER_H
#define GPX_PARSER_H

#include "config.h"
#include "shared_data.h"  // For GPX struct definitions
#include <SD.h>
#include <FS.h>
#include <ArduinoJson.h>

// ===================================================================
// GPX PARSER - PROFESSIONAL GPX FILE SYSTEM
// Complete GPX reading/writing/validation for serious navigation
// Core 0 Implementation - File Operations
// ===================================================================

// GPX element types
enum GPXElementType {
  GPX_WAYPOINT,
  GPX_TRACK,
  GPX_ROUTE,
  GPX_TRACK_SEGMENT,
  GPX_TRACK_POINT,
  GPX_METADATA
};

// GPX structs removed - using definitions from shared_data.h
// GPXWaypoint and GPXTrackPoint are defined in shared_data.h

// Track segment
// struct GPXTrackSegment {
//   GPXTrackPoint* points = nullptr;
//   int pointCount = 0;
//   int maxPoints = 0;
//   String name = "";
//   bool isLoaded = false;
// };

// Complete track structure
// Remove struct GPXTrack (lines 39–50)
// Rely on GPXTrack from shared_data.h

// GPX file metadata
struct GPXMetadata {
  String name = "";
  String description = "";
  String creator = "E_NAVI Enduro GPS - ProXXi Pty Ltd";
  String version = "1.1";
  unsigned long timestamp = 0;
  float minLat = 90.0, maxLat = -90.0;
  float minLon = 180.0, maxLon = -180.0;
  int totalWaypoints = 0;
  int totalTracks = 0;
  bool isValid = false;
};

// Complete GPX file structure
struct GPXFile {
  GPXMetadata metadata;
  GPXWaypoint* waypoints = nullptr;
  int waypointCount = 0;
  int maxWaypoints = 0;
  GPXTrack* tracks = nullptr;
  int trackCount = 0;
  int maxTracks = 0;
  String filename = "";
  bool isLoaded = false;
  bool isValid = false;
};

// File browser item
struct GPXFileInfo {
  String filename = "";
  String displayName = "";
  unsigned long fileSize = 0;
  String dateCreated = "";
  String description = "";
  int waypointCount = 0;
  int trackCount = 0;
  float distance = 0.0;        // km
  bool isValid = false;
};

class GPXParser {
private:
  // Parser state
  bool initialized = false;
  String lastError = "";
  
  // Memory management
  static const int MAX_WAYPOINTS = 100;
  static const int MAX_TRACKS = 10;
  static const int MAX_TRACK_POINTS = 5000;
  static const int MAX_SEGMENTS = 20;
  
  // Parsing configuration
  bool validateOnLoad = true;
  bool loadFullData = true;
  bool optimizeMemory = true;
  
public:
  // Initialization
  bool initialize();
  
  // File operations
  bool loadGPXFile(const String& filename, GPXFile& gpxFile);
  bool saveGPXFile(const String& filename, const GPXFile& gpxFile);
  bool validateGPXFile(const String& filename);
  bool parseGPXHeader(const String& filename, GPXMetadata& metadata);
  
  // Track operations
  bool loadTrack(const String& filename, int trackIndex, GPXTrack& track);
  bool saveTrack(const String& filename, const GPXTrack& track);
  bool appendTrackPoint(const String& filename, const GPXTrackPoint& point);
  
  // Waypoint operations
  bool loadWaypoints(const String& filename, GPXWaypoint waypoints[], int& count);
  bool saveWaypoint(const String& filename, const GPXWaypoint& waypoint);
  bool createWaypoint(double lat, double lon, const String& name);
  
  // File analysis
  bool analyzeGPXFile(const String& filename, GPXFileInfo& info);
  bool getFileStatistics(const String& filename, float& distance, 
                        unsigned long& duration, float& maxSpeed);
  
  // Memory management
  bool allocateGPXFile(GPXFile& gpxFile, int waypoints, int tracks);
  void deallocateGPXFile(GPXFile& gpxFile);
  bool optimizeGPXFile(GPXFile& gpxFile);
  
  // Validation
  bool isValidGPXFile(const String& filename);
  bool isValidCoordinate(double lat, double lon);
  bool isValidTrackPoint(const GPXTrackPoint& point);
  
  // Configuration
  void setValidateOnLoad(bool validate);
  void setLoadFullData(bool loadFull);
  void setOptimizeMemory(bool optimize);

  bool parseGPXContent(File& file, GPXFile& gpxFile);
  bool parseWaypoint(const String& line, File& file, GPXFile& gpxFile);
  bool parseTrackPoint(const String& line, File& file, GPXTrack& track, int segmentIndex);
  String extractElementContent(const String& line, const String& elementName);
  void calculateTrackStatistics(GPXTrack& track);
  bool validateGPXData(const GPXFile& gpxFile);
  
  // Error handling
  String getLastError();
  
private:
  // XML parsing helpers
  bool parseXMLElement(const String& line, String& element, String& attributes);
  bool extractAttribute(const String& attributes, const String& name, String& value);
  bool parseCoordinates(const String& lat, const String& lon, double& latitude, double& longitude);
  bool parseTimestamp(const String& timeStr, unsigned long& timestamp);
  
  // File I/O helpers
  bool readFileLine(File& file, String& line);
  bool writeGPXHeader(File& file, const GPXMetadata& metadata);
  bool writeGPXFooter(File& file);
  bool writeWaypoint(File& file, const GPXWaypoint& waypoint);
  bool writeTrackPoint(File& file, const GPXTrackPoint& point);
  
  // Analysis helpers
  float calculateDistance(const GPXTrackPoint& p1, const GPXTrackPoint& p2);
  float calculateTotalDistance(const GPXTrack& track);
  unsigned long calculateDuration(const GPXTrack& track);
  float calculateMaxSpeed(const GPXTrack& track);
  float calculateElevationGain(const GPXTrack& track);
  
  // Memory helpers
  bool allocateTrack(GPXTrack& track, int segments, int pointsPerSegment);
  void deallocateTrack(GPXTrack& track);
  bool compactTrack(GPXTrack& track);
};

// ===================================================================
// FILE BROWSER SYSTEM - 5-WAY NAVIGATION
// ===================================================================

// Browser state
enum BrowserState {
  BROWSER_FILE_LIST,      // Main file listing
  BROWSER_FILE_DETAILS,   // File details view
  BROWSER_TRACK_LIST,     // Track selection
  BROWSER_WAYPOINT_LIST,  // Waypoint selection
  BROWSER_OPTIONS         // File options menu
};

// File browser configuration
struct BrowserConfig {
  int maxFilesPerPage = 8;        // Files visible per screen
  bool showFileSize = true;
  bool showFileDate = true;
  bool showPreview = true;
  bool sortByDate = true;         // vs alphabetical
  bool showHiddenFiles = false;
  String currentDirectory = "/tracks";
};

class GPXFileBrowser {
private:
  GPXParser* parser;
  
  // Browser state
  BrowserState currentState = BROWSER_FILE_LIST;
  BrowserConfig config;
  
  // File listing
  GPXFileInfo* fileList = nullptr;
  int fileCount = 0;
  int maxFiles = 50;
  int currentFile = 0;
  int pageStart = 0;
  
  // Current selection
  String selectedFilename = "";
  GPXFile currentGPX;
  int selectedTrack = 0;
  int selectedWaypoint = 0;
  
  // Display state
  bool needsRedraw = true;
  unsigned long lastUpdate = 0;
  
public:
  // Initialization
  bool initialize(GPXParser* gpxParser);
  
  // Navigation control (5-way + zoom buttons)
  void handleButtonPress(int button);
  void navigateUp();
  void navigateDown();
  void navigateLeft();
  void navigateRight();
  void selectCurrent();
  
  // Display management
  void update();
  void draw();
  void drawFileList();
  void drawFileDetails();
  void drawTrackList();
  void drawWaypointList();
  void drawOptionsMenu();
  
  // File operations
  bool refreshFileList();
  bool loadSelectedFile();
  bool deleteSelectedFile();
  bool copySelectedFile();
  bool renameSelectedFile();
  
  // Navigation
  bool enterDirectory(const String& dirname);
  bool exitToParent();
  void changeState(BrowserState newState);
  
  // Getters
  String getSelectedFilename();
  GPXFile getCurrentGPX();
  BrowserState getCurrentState();
  
private:
  // File system operations
  bool scanDirectory(const String& path);
  bool sortFileList();
  void updateFileInfo(GPXFileInfo& info);
  
  // Display helpers
  void drawHeader(const String& title);
  void drawFileItem(int index, bool selected);
  void drawScrollBar();
  void drawStatusBar();
  
  // Memory management
  bool allocateFileList();
  void deallocateFileList();
};

// Global instances
extern GPXParser gpxParser;
extern GPXFileBrowser gpxBrowser;

// Quick access functions
bool initializeGPXSystem();
bool loadGPXTrack(const String& filename);
bool saveCurrentTrack();
bool createNewWaypoint(double lat, double lon, const String& name);
void updateFileBrowser();
void handleFileBrowserInput(int button);

#endif // GPX_PARSER_H