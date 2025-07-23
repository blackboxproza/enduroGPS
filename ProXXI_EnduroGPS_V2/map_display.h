#ifndef MAP_DISPLAY_H
#define MAP_DISPLAY_H

#include "config.h"
#include "display_manager.h"
#include <SD.h>

// ===================================================================
// MAP DISPLAY SYSTEM - PROFESSIONAL PNG TILE MANAGEMENT
// PSRAM-cached tile system with recording indicators
// ===================================================================

// Tile coordinates
struct TileCoord {
  int x = 0;
  int y = 0;
  int zoom = 14;
  
  bool operator==(const TileCoord& other) const {
    return (x == other.x && y == other.y && zoom == other.zoom);
  }
};

// Cached tile data
struct CachedTile {
  TileCoord coord;
  uint8_t* imageData = nullptr;        // PNG data in PSRAM
  uint32_t dataSize = 0;               // Size of PNG data
  unsigned long lastAccess = 0;        // LRU tracking
  bool isLoaded = false;
  bool isValid = false;
  uint32_t checksum = 0;               // Data integrity
};

// Map viewport
struct MapViewport {
  double centerLat = -33.9249;         // Cape Town default
  double centerLon = 18.4241;
  int zoomLevel = 14;
  int screenCenterX = 160;             // Screen center (320/2)
  int screenCenterY = 192;             // Content area center (48+288/2)
  int viewportWidth = 320;
  int viewportHeight = 288;            // Content area height
  float pixelsPerMeter = 1.0;          // Scale factor
};

// Position marker
struct PositionMarker {
  double latitude = 0.0;
  double longitude = 0.0;
  float heading = 0.0;                 // degrees
  float speed = 0.0;                   // km/h
  bool isValid = false;
  bool showTrail = true;               // GPS trail
  unsigned long lastUpdate = 0;
};

// Recording visual state
enum MapRecordingState {
  MAP_REC_OFF,                         // No recording - no indicators
  MAP_REC_RECORDING,                   // Active - thick green border
  MAP_REC_PAUSED,                      // Paused - thick blue border
  MAP_REC_STARTING,                    // Starting - pulsing green
  MAP_REC_ERROR                        // Error - red border
};

class MapDisplay {
private:
  // Tile cache system (using 8MB PSRAM)
  static const int MAX_CACHED_TILES = TILE_CACHE_SIZE; // 50 tiles
  CachedTile tileCache[MAX_CACHED_TILES];
  int cacheUsed = 0;
  unsigned long cacheHits = 0;
  unsigned long cacheMisses = 0;
  
  // Current viewport
  MapViewport viewport;
  PositionMarker position;
  
  // Recording indicators
  MapRecordingState recordingState = MAP_REC_OFF;
  unsigned long recordingBorderPhase = 0;
  
  // Display state
  bool initialized = false;
  bool needsRedraw = true;
  TileCoord lastCenterTile;
  unsigned long lastDrawTime = 0;
  
  // Performance tracking
  unsigned long tilesDrawn = 0;
  unsigned long averageDrawTime = 0;
  
public:
  // Initialization
  bool initialize();
  bool testTileSystem();
  
  // Core rendering (called from Core 0 loop)
  void update();
  void draw();
  void drawTiles();
  void drawPosition();
  void drawRecordingIndicators();
  
  // Map control
  void setCenter(double lat, double lon);
  void setZoom(int zoom);
  void panMap(int deltaX, int deltaY);
  void centerOnPosition();
  
  // Position updates
  void updatePosition(double lat, double lon, float heading, float speed);
  bool isPositionValid();
  
  // Recording state
  void setRecordingState(MapRecordingState state);
  MapRecordingState getRecordingState();
  
  // Tile management
  bool loadTile(const TileCoord& coord);
  CachedTile* findCachedTile(const TileCoord& coord);
  bool cacheTile(const TileCoord& coord, uint8_t* data, uint32_t size);
  void evictOldestTile();
  
  // Coordinate conversion
  TileCoord latLonToTile(double lat, double lon, int zoom);
  void tileToPixel(const TileCoord& tile, int& pixelX, int& pixelY);
  void latLonToPixel(double lat, double lon, int& pixelX, int& pixelY);
  void pixelToLatLon(int pixelX, int pixelY, double& lat, double& lon);
  
  // Display helpers
  void drawTile(const TileCoord& coord, int screenX, int screenY);
  void drawPositionMarker(int screenX, int screenY);
  void drawHeadingArrow(int centerX, int centerY, float heading);
  void drawRecordingBorder();
  void drawGPSTrail();
  
  // Performance
  void updatePerformanceStats();
  float getCacheHitRatio();
  unsigned long getAverageDrawTime();
  
  // Status
  MapViewport getViewport();
  PositionMarker getPosition();
  String getStatusText();
  
private:
  // Internal tile operations
  String getTilePath(const TileCoord& coord);
  bool loadTileFromSD(const TileCoord& coord, uint8_t*& data, uint32_t& size);
  bool validateTileData(uint8_t* data, uint32_t size);
  uint32_t calculateChecksum(uint8_t* data, uint32_t size);
  
  // Cache management
  int findLRUTile();
  void updateTileAccess(CachedTile* tile);
  void clearTileCache();
  
  // Drawing primitives
  void drawPNGTile(uint8_t* pngData, uint32_t size, int x, int y);
  void drawCircle(int centerX, int centerY, int radius, uint16_t color);
  void drawTriangle(int centerX, int centerY, int size, float angle, uint16_t color);
  void drawThickLine(int x1, int y1, int x2, int y2, int thickness, uint16_t color);
  
  // Recording border effects
  uint16_t getRecordingBorderColor();
  int getRecordingBorderThickness();
  void updateRecordingAnimation();
};

// ===================================================================
// GPS TRAIL SYSTEM - BREADCRUMB NAVIGATION
// ===================================================================

struct TrailPoint {
  double latitude;
  double longitude;
  unsigned long timestamp;
  bool isValid;
};

class GPSTrail {
private:
  static const int MAX_TRAIL_POINTS = 200;
  TrailPoint trailPoints[MAX_TRAIL_POINTS];
  int trailCount = 0;
  int trailIndex = 0;              // Circular buffer index
  
  // Trail configuration
  bool enabled = true;
  float minDistance = 10.0;        // meters between points
  unsigned long maxAge = 3600000;  // 1 hour trail
  uint16_t trailColor = 0xFD20;    // Orange
  
public:
  // Trail management
  void addPoint(double lat, double lon);
  void clearTrail();
  void setEnabled(bool enable);
  
  // Drawing
  void drawTrail(MapDisplay* mapDisplay);
  
  // Configuration
  void setMinDistance(float meters);
  void setMaxAge(unsigned long ms);
  void setTrailColor(uint16_t color);
  
  // Status
  int getTrailPointCount();
  bool isEnabled();
  
private:
  // Internal helpers
  bool shouldAddPoint(double lat, double lon);
  void removeOldPoints();
  float calculateDistance(double lat1, double lon1, double lat2, double lon2);
};

// ===================================================================
// MAP TILE PRELOADER - BACKGROUND LOADING
// ===================================================================

class TilePreloader {
private:
  MapDisplay* mapDisplay;
  
  // Preload queue
  static const int MAX_PRELOAD_QUEUE = 20;
  TileCoord preloadQueue[MAX_PRELOAD_QUEUE];
  int queueSize = 0;
  int queueIndex = 0;
  
  // Preloader state
  bool enabled = true;
  unsigned long lastPreload = 0;
  const unsigned long preloadInterval = 100; // ms between loads
  
public:
  // Initialization
  void initialize(MapDisplay* mapDisp);
  
  // Queue management
  void queueSurroundingTiles(const TileCoord& center);
  void queueTile(const TileCoord& coord);
  void clearQueue();
  
  // Background processing
  void update();
  void preloadNextTile();
  
  // Configuration
  void setEnabled(bool enable);
  bool isEnabled();
  
private:
  // Internal helpers
  bool isTileInQueue(const TileCoord& coord);
  void addToQueue(const TileCoord& coord);
};

// Global instances
extern MapDisplay mapDisplay;
extern GPSTrail gpsTrail;
extern TilePreloader tilePreloader;

// Quick access functions for Core 0
bool initializeMapSystem();
void updateMapDisplay();
void setMapPosition(double lat, double lon, float heading, float speed);
void setMapZoom(int zoom);
void setMapRecording(bool recording, bool paused);
void centerMapOnPosition();

#endif // MAP_DISPLAY_H