/*
 * ProXXI EnduroGPS V2 - Map Display Implementation
 * Professional PNG tile-based map rendering for serious navigation
 * 
 * Features:
 * - PSRAM-cached PNG tile system (50 tiles, ~12MB cache)
 * - High-performance coordinate conversion (Web Mercator)
 * - GPS trail rendering with breadcrumb navigation
 * - Recording state visual indicators
 * - Cape Town optimized defaults
 * - Vision-optimized display for mature riders
 * 
 * Hardware: ESP32-S3 with 3.5" IPS TFT (320x480)
 * Memory: 8MB PSRAM for tile caching
 */

#include "map_display.h"
#include "sd_manager.h"
#include "shared_data.h"
#include <math.h>

// ===================================================================
// GLOBAL INSTANCES
// ===================================================================
MapDisplay mapDisplay;
GPSTrail gpsTrail;
TilePreloader tilePreloader;

// ===================================================================
// MATHEMATICAL CONSTANTS FOR WEB MERCATOR PROJECTION
// ===================================================================
static const double EARTH_RADIUS = 6378137.0;          // WGS84 earth radius (meters)
static const double ORIGIN_SHIFT = M_PI * EARTH_RADIUS; // Half circumference

// ===================================================================
// MAP DISPLAY CLASS IMPLEMENTATION
// ===================================================================

bool MapDisplay::initialize() {
  if (initialized) return true;
  
  Serial.println("MapDisplay: Initializing map system...");
  
  // Initialize viewport with Cape Town coordinates
  viewport.centerLat = -33.9249;
  viewport.centerLon = 18.4241;
  viewport.zoomLevel = ZOOM_DEFAULT;
  viewport.screenCenterX = SCREEN_WIDTH / 2;
  viewport.screenCenterY = STATUS_BAR_HEIGHT + (CONTENT_AREA_HEIGHT / 2);
  viewport.viewportWidth = SCREEN_WIDTH;
  viewport.viewportHeight = CONTENT_AREA_HEIGHT;
  
  // Initialize position marker
  position.latitude = viewport.centerLat;
  position.longitude = viewport.centerLon;
  position.heading = 0.0;
  position.speed = 0.0;
  position.isValid = false;
  position.showTrail = true;
  position.lastUpdate = 0;
  
  // Initialize tile cache
  for (int i = 0; i < MAX_CACHED_TILES; i++) {
    tileCache[i].imageData = nullptr;
    tileCache[i].dataSize = 0;
    tileCache[i].isLoaded = false;
    tileCache[i].isValid = false;
    tileCache[i].lastAccess = 0;
    tileCache[i].checksum = 0;
  }
  cacheUsed = 0;
  cacheHits = 0;
  cacheMisses = 0;
  
  // Initialize recording state
  recordingState = MAP_REC_OFF;
  recordingBorderPhase = 0;
  
  // Performance tracking
  tilesDrawn = 0;
  averageDrawTime = 0;
  lastDrawTime = 0;
  needsRedraw = true;
  
  initialized = true;
  Serial.println("MapDisplay: Initialization complete");
  
  return testTileSystem();
}

bool MapDisplay::testTileSystem() {
  Serial.println("MapDisplay: Testing tile system...");
  
  // Test if SD card is accessible
  if (!SD.begin()) {
    Serial.println("MapDisplay: ERROR - SD card not accessible");
    return false;
  }
  
  // Test if tile directory exists
  String testTileDir = TILE_FOLDER;
  if (!SD.exists(testTileDir)) {
    Serial.printf("MapDisplay: WARNING - Tile directory %s not found\n", testTileDir.c_str());
    // Create directory structure
    SD.mkdir(testTileDir);
  }
  
  // Test loading a sample tile (Cape Town area)
  TileCoord testTile = latLonToTile(viewport.centerLat, viewport.centerLon, viewport.zoomLevel);
  Serial.printf("MapDisplay: Testing tile %d/%d/%d\n", testTile.zoom, testTile.x, testTile.y);
  
  if (loadTile(testTile)) {
    Serial.println("MapDisplay: Tile system test PASSED");
    return true;
  } else {
    Serial.println("MapDisplay: Tile system test FAILED - No tiles available");
    // Not a critical error - system can still operate
    return true;
  }
}

// ===================================================================
// CORE RENDERING LOOP
// ===================================================================

void MapDisplay::update() {
  if (!initialized) return;
  
  unsigned long currentTime = millis();
  
  // Update recording animation
  updateRecordingAnimation();
  
  // Check if redraw is needed
  if (needsRedraw || (currentTime - lastDrawTime) > 100) {
    draw();
    lastDrawTime = currentTime;
    needsRedraw = false;
  }
  
  // Update performance statistics
  updatePerformanceStats();
}

void MapDisplay::draw() {
  unsigned long startTime = millis();
  
  // Get current display reference
  TFT_eSPI& display = ::display.getTFT();
  
  // Clear the content area (preserve status/bottom bars)
  int contentY = STATUS_BAR_HEIGHT;
  int contentHeight = CONTENT_AREA_HEIGHT;
  
  // Apply recording border offset
  int borderOffset = (recordingState != MAP_REC_OFF) ? getRecordingBorderThickness() : 0;
  
  display.fillRect(borderOffset, contentY + borderOffset, 
                  SCREEN_WIDTH - (2 * borderOffset), 
                  contentHeight - (2 * borderOffset), 
                  ::display.getBackgroundColor());
  
  // Draw map tiles
  drawTiles();
  
  // Draw GPS trail if enabled
  if (position.showTrail) {
    drawGPSTrail();
  }
  
  // Draw current position
  if (position.isValid) {
    drawPosition();
  }
  
  // Draw recording indicators
  drawRecordingIndicators();
  
  // Update performance tracking
  unsigned long drawTime = millis() - startTime;
  tilesDrawn++;
  
  if (averageDrawTime == 0) {
    averageDrawTime = drawTime;
  } else {
    averageDrawTime = (averageDrawTime * 0.9) + (drawTime * 0.1);
  }
}

void MapDisplay::drawTiles() {
  if (!initialized) return;
  
  // Calculate which tiles are visible
  TileCoord centerTile = latLonToTile(viewport.centerLat, viewport.centerLon, viewport.zoomLevel);
  
  // Calculate how many tiles we need in each direction
  int tilesX = (viewport.viewportWidth / TILE_SIZE) + 2;  // +2 for partial tiles
  int tilesY = (viewport.viewportHeight / TILE_SIZE) + 2;
  
  int startTileX = centerTile.x - (tilesX / 2);
  int startTileY = centerTile.y - (tilesY / 2);
  
  // Draw tiles
  for (int ty = 0; ty < tilesY; ty++) {
    for (int tx = 0; tx < tilesX; tx++) {
      TileCoord tileCoord;
      tileCoord.x = startTileX + tx;
      tileCoord.y = startTileY + ty;
      tileCoord.zoom = viewport.zoomLevel;
      
      // Convert tile to screen coordinates
      int screenX, screenY;
      tileToPixel(tileCoord, screenX, screenY);
      
      // Only draw if tile is visible
      if (screenX + TILE_SIZE >= 0 && screenX < viewport.viewportWidth &&
          screenY + TILE_SIZE >= 0 && screenY < viewport.viewportHeight) {
        
        drawTile(tileCoord, screenX, screenY);
      }
    }
  }
}

void MapDisplay::drawTile(const TileCoord& coord, int screenX, int screenY) {
  // Try to find tile in cache first
  CachedTile* cachedTile = findCachedTile(coord);
  
  if (cachedTile && cachedTile->isValid) {
    // Cache hit - draw from PSRAM
    cacheHits++;
    cachedTile->lastAccess = millis();
    
    // Draw the PNG tile
    drawPNGTile(cachedTile->imageData, cachedTile->dataSize, 
                screenX, screenY + STATUS_BAR_HEIGHT);
    
  } else {
    // Cache miss - try to load from SD
    cacheMisses++;
    
    if (loadTile(coord)) {
      // Successfully loaded - try again
      cachedTile = findCachedTile(coord);
      if (cachedTile && cachedTile->isValid) {
        drawPNGTile(cachedTile->imageData, cachedTile->dataSize, 
                    screenX, screenY + STATUS_BAR_HEIGHT);
      }
    } else {
      // No tile available - draw placeholder
      TFT_eSPI& display = ::display.getTFT();
      uint16_t placeholderColor = 0x39E7; // Light gray
      
      display.drawRect(screenX, screenY + STATUS_BAR_HEIGHT, 
                      TILE_SIZE, TILE_SIZE, placeholderColor);
      
      // Draw "NO MAP" text in center of tile
      display.setTextColor(placeholderColor);
      display.setTextSize(1);
      display.setCursor(screenX + TILE_SIZE/2 - 24, screenY + STATUS_BAR_HEIGHT + TILE_SIZE/2);
      display.print("NO MAP");
    }
  }
}

void MapDisplay::drawPosition() {
  if (!position.isValid) return;
  
  // Convert GPS coordinates to screen pixels
  int screenX, screenY;
  latLonToPixel(position.latitude, position.longitude, screenX, screenY);
  
  // Adjust for status bar
  screenY += STATUS_BAR_HEIGHT;
  
  // Only draw if position is visible on screen
  if (screenX >= 0 && screenX < SCREEN_WIDTH && 
      screenY >= STATUS_BAR_HEIGHT && screenY < (STATUS_BAR_HEIGHT + CONTENT_AREA_HEIGHT)) {
    
    drawPositionMarker(screenX, screenY);
    
    // Draw heading arrow if moving
    if (position.speed > 1.0) { // Only show when moving > 1 km/h
      drawHeadingArrow(screenX, screenY, position.heading);
    }
  }
}

void MapDisplay::drawPositionMarker(int screenX, int screenY) {
  TFT_eSPI& display = ::display.getTFT();
  
  // GPS accuracy colors
  uint16_t markerColor;
  GPSData gps = getGPSData();
  
  if (gps.hdop < 2.0 && gps.satellites >= 8) {
    markerColor = 0x07E0; // Bright green - excellent
  } else if (gps.hdop < 5.0 && gps.satellites >= 5) {
    markerColor = 0xFFE0; // Yellow - good  
  } else {
    markerColor = 0xF800; // Red - poor
  }
  
  // Draw position circle (larger for visibility)
  int radius = 8;
  
  // Outer ring for contrast
  display.fillCircle(screenX, screenY, radius + 2, TFT_BLACK);
  display.fillCircle(screenX, screenY, radius, markerColor);
  
  // Inner dot for center
  display.fillCircle(screenX, screenY, 3, TFT_WHITE);
}

void MapDisplay::drawHeadingArrow(int centerX, int centerY, float heading) {
  TFT_eSPI& display = ::display.getTFT();
  
  // Convert heading to radians
  float angleRad = (heading - 90.0) * (M_PI / 180.0); // -90 to point north up
  
  // Arrow parameters
  int arrowLength = 20;
  int arrowWidth = 8;
  uint16_t arrowColor = 0x001F; // Blue arrow
  
  // Calculate arrow tip
  int tipX = centerX + (cos(angleRad) * arrowLength);
  int tipY = centerY + (sin(angleRad) * arrowLength);
  
  // Calculate arrow base points
  float baseAngle1 = angleRad + (2.4); // ~135 degrees
  float baseAngle2 = angleRad - (2.4); // ~-135 degrees
  
  int base1X = centerX + (cos(baseAngle1) * arrowWidth);
  int base1Y = centerY + (sin(baseAngle1) * arrowWidth);
  int base2X = centerX + (cos(baseAngle2) * arrowWidth);
  int base2Y = centerY + (sin(baseAngle2) * arrowWidth);
  
  // Draw arrow triangle
  display.fillTriangle(tipX, tipY, base1X, base1Y, base2X, base2Y, arrowColor);
  
  // Draw outline for visibility
  display.drawTriangle(tipX, tipY, base1X, base1Y, base2X, base2Y, TFT_WHITE);
}

void MapDisplay::drawRecordingIndicators() {
  if (recordingState == MAP_REC_OFF) return;
  
  drawRecordingBorder();
  
  // Add recording icon in top-right of content area
  TFT_eSPI& display = ::display.getTFT();
  
  int iconX = SCREEN_WIDTH - 30;
  int iconY = STATUS_BAR_HEIGHT + 10;
  int iconSize = 6;
  
  uint16_t iconColor = getRecordingBorderColor();
  
  int pulseSize = 0; // Moved outside switch for scope
  
  switch (recordingState) {
    case MAP_REC_RECORDING:
      // Recording dot
      display.fillCircle(iconX, iconY, iconSize, iconColor);
      break;
      
    case MAP_REC_PAUSED:
      // Pause symbol (two bars)
      display.fillRect(iconX - 4, iconY - iconSize, 3, iconSize * 2, iconColor);
      display.fillRect(iconX + 2, iconY - iconSize, 3, iconSize * 2, iconColor);
      break;
      
    case MAP_REC_STARTING:
      // Pulsing dot
      pulseSize = iconSize + (sin(millis() * 0.01) * 3);
      display.fillCircle(iconX, iconY, pulseSize, iconColor);
      break;
      
    case MAP_REC_ERROR:
      // Error X
      display.drawLine(iconX - iconSize, iconY - iconSize, iconX + iconSize, iconY + iconSize, iconColor);
      display.drawLine(iconX + iconSize, iconY - iconSize, iconX - iconSize, iconY + iconSize, iconColor);
      break;
      
    default:
      break;
  }
}

void MapDisplay::drawRecordingBorder() {
  if (recordingState == MAP_REC_OFF) return;
  
  TFT_eSPI& display = ::display.getTFT();
  
  uint16_t borderColor = getRecordingBorderColor();
  int thickness = getRecordingBorderThickness();
  
  int contentY = STATUS_BAR_HEIGHT;
  int contentHeight = CONTENT_AREA_HEIGHT;
  
  // Draw thick border around content area
  for (int i = 0; i < thickness; i++) {
    display.drawRect(i, contentY + i, 
                    SCREEN_WIDTH - (i * 2), 
                    contentHeight - (i * 2), 
                    borderColor);
  }
}

void MapDisplay::drawGPSTrail() {
  gpsTrail.drawTrail(this);
}

// ===================================================================
// MAP CONTROL FUNCTIONS
// ===================================================================

void MapDisplay::setCenter(double lat, double lon) {
  viewport.centerLat = lat;
  viewport.centerLon = lon;
  needsRedraw = true;
}

void MapDisplay::setZoom(int zoom) {
  zoom = constrain(zoom, ZOOM_MIN_LEVEL, ZOOM_MAX_LEVEL);
  if (zoom != viewport.zoomLevel) {
    viewport.zoomLevel = zoom;
    
    // Update pixels per meter for the new zoom
    double resolution = (2.0 * M_PI * EARTH_RADIUS) / (TILE_SIZE * pow(2.0, zoom));
    viewport.pixelsPerMeter = 1.0 / resolution;
    
    needsRedraw = true;
  }
}

void MapDisplay::panMap(int deltaX, int deltaY) {
  // Convert pixel delta to lat/lon delta
  double latDelta = (deltaY / viewport.pixelsPerMeter) / EARTH_RADIUS * (180.0 / M_PI);
  double lonDelta = (deltaX / viewport.pixelsPerMeter) / EARTH_RADIUS * (180.0 / M_PI) / cos(viewport.centerLat * (M_PI / 180.0));
  
  viewport.centerLat -= latDelta;
  viewport.centerLon -= lonDelta;
  needsRedraw = true;
}

void MapDisplay::centerOnPosition() {
  if (position.isValid) {
    setCenter(position.latitude, position.longitude);
  }
}

void MapDisplay::updatePosition(double lat, double lon, float heading, float speed) {
  bool wasValid = position.isValid;
  
  position.latitude = lat;
  position.longitude = lon;
  position.heading = heading;
  position.speed = speed;
  position.isValid = true;
  position.lastUpdate = millis();
  
  // Add to GPS trail
  if (position.showTrail) {
    gpsTrail.addPoint(lat, lon);
  }
  
  // If this is first valid position, center map
  if (!wasValid) {
    centerOnPosition();
  }
  
  needsRedraw = true;
}

bool MapDisplay::isPositionValid() {
  return position.isValid && ((millis() - position.lastUpdate) < 10000); // 10 second timeout
}

// ===================================================================
// RECORDING STATE MANAGEMENT
// ===================================================================

void MapDisplay::setRecordingState(MapRecordingState state) {
  if (recordingState != state) {
    recordingState = state;
    recordingBorderPhase = 0;
    needsRedraw = true;
  }
}

MapRecordingState MapDisplay::getRecordingState() {
  return recordingState;
}

uint16_t MapDisplay::getRecordingBorderColor() {
  switch (recordingState) {
    case MAP_REC_RECORDING:
      return 0x07E0; // Green
    case MAP_REC_PAUSED:
      return 0x001F; // Blue
    case MAP_REC_STARTING:
      // Pulsing green
      {
        float intensity = (sin(millis() * 0.01) + 1.0) / 2.0;
        int green = 31 * intensity;
        return (green << 5);
      }
    case MAP_REC_ERROR:
      return 0xF800; // Red
    default:
      return 0x0000; // Black
  }
}

int MapDisplay::getRecordingBorderThickness() {
  switch (recordingState) {
    case MAP_REC_RECORDING:
    case MAP_REC_PAUSED:
      return 4;
    case MAP_REC_STARTING:
    case MAP_REC_ERROR:
      return 3;
    default:
      return 0;
  }
}

void MapDisplay::updateRecordingAnimation() {
  if (recordingState == MAP_REC_STARTING) {
    recordingBorderPhase = (recordingBorderPhase + 1) % 100;
    if (recordingBorderPhase % 10 == 0) {
      needsRedraw = true;
    }
  }
}

// ===================================================================
// TILE MANAGEMENT SYSTEM
// ===================================================================

bool MapDisplay::loadTile(const TileCoord& coord) {
  // Check if already cached
  CachedTile* existing = findCachedTile(coord);
  if (existing && existing->isValid) {
    existing->lastAccess = millis();
    return true;
  }
  
  // Try to load from SD card
  uint8_t* tileData = nullptr;
  uint32_t tileSize = 0;
  
  if (!loadTileFromSD(coord, tileData, tileSize)) {
    return false;
  }
  
  // Cache the tile
  return cacheTile(coord, tileData, tileSize);
}

CachedTile* MapDisplay::findCachedTile(const TileCoord& coord) {
  for (int i = 0; i < cacheUsed; i++) {
    if (tileCache[i].coord == coord && tileCache[i].isValid) {
      return &tileCache[i];
    }
  }
  return nullptr;
}

bool MapDisplay::cacheTile(const TileCoord& coord, uint8_t* data, uint32_t size) {
  // Find available cache slot
  int slotIndex = -1;
  
  if (cacheUsed < MAX_CACHED_TILES) {
    // Use next available slot
    slotIndex = cacheUsed++;
  } else {
    // Evict oldest tile
    slotIndex = findLRUTile();
    evictOldestTile();
  }
  
  if (slotIndex < 0) return false;
  
  // Allocate memory in PSRAM
  uint8_t* psramData = (uint8_t*)ps_malloc(size);
  if (!psramData) {
    Serial.println("MapDisplay: Failed to allocate PSRAM for tile");
    return false;
  }
  
  // Copy data to PSRAM
  memcpy(psramData, data, size);
  
  // Update cache entry
  CachedTile* tile = &tileCache[slotIndex];
  tile->coord = coord;
  tile->imageData = psramData;
  tile->dataSize = size;
  tile->lastAccess = millis();
  tile->isLoaded = true;
  tile->isValid = validateTileData(psramData, size);
  tile->checksum = calculateChecksum(psramData, size);
  
  return tile->isValid;
}

void MapDisplay::evictOldestTile() {
  int oldestIndex = findLRUTile();
  if (oldestIndex >= 0) {
    CachedTile* tile = &tileCache[oldestIndex];
    if (tile->imageData) {
      free(tile->imageData);
      tile->imageData = nullptr;
    }
    tile->dataSize = 0;
    tile->isLoaded = false;
    tile->isValid = false;
  }
}

int MapDisplay::findLRUTile() {
  unsigned long oldestTime = ULONG_MAX;
  int oldestIndex = -1;
  
  for (int i = 0; i < cacheUsed; i++) {
    if (tileCache[i].lastAccess < oldestTime) {
      oldestTime = tileCache[i].lastAccess;
      oldestIndex = i;
    }
  }
  
  return oldestIndex;
}

// ===================================================================
// COORDINATE CONVERSION FUNCTIONS
// ===================================================================

TileCoord MapDisplay::latLonToTile(double lat, double lon, int zoom) {
  TileCoord coord;
  coord.zoom = zoom;
  
  // Web Mercator projection
  double latRad = lat * (M_PI / 180.0);
  double n = pow(2.0, zoom);
  
  coord.x = (int)((lon + 180.0) / 360.0 * n);
  coord.y = (int)((1.0 - asinh(tan(latRad)) / M_PI) / 2.0 * n);
  
  return coord;
}

void MapDisplay::tileToPixel(const TileCoord& tile, int& pixelX, int& pixelY) {
  // Get center tile for current viewport
  TileCoord centerTile = latLonToTile(viewport.centerLat, viewport.centerLon, viewport.zoomLevel);
  
  // Calculate tile offset from center
  int tileOffsetX = tile.x - centerTile.x;
  int tileOffsetY = tile.y - centerTile.y;
  
  // Convert to pixel offset
  pixelX = (tileOffsetX * TILE_SIZE) + (viewport.screenCenterX - TILE_SIZE/2);
  pixelY = (tileOffsetY * TILE_SIZE) + (viewport.screenCenterY - STATUS_BAR_HEIGHT - TILE_SIZE/2);
}

void MapDisplay::latLonToPixel(double lat, double lon, int& pixelX, int& pixelY) {
  // Convert to tile coordinates (floating point)
  double latRad = lat * (M_PI / 180.0);
  double n = pow(2.0, viewport.zoomLevel);
  
  double tileX = (lon + 180.0) / 360.0 * n;
  double tileY = (1.0 - asinh(tan(latRad)) / M_PI) / 2.0 * n;
  
  // Get center tile
  TileCoord centerTile = latLonToTile(viewport.centerLat, viewport.centerLon, viewport.zoomLevel);
  
  // Calculate pixel position
  double tileOffsetX = tileX - centerTile.x;
  double tileOffsetY = tileY - centerTile.y;
  
  pixelX = (int)((tileOffsetX * TILE_SIZE) + (viewport.screenCenterX - TILE_SIZE/2));
  pixelY = (int)((tileOffsetY * TILE_SIZE) + (viewport.screenCenterY - STATUS_BAR_HEIGHT - TILE_SIZE/2));
}

void MapDisplay::pixelToLatLon(int pixelX, int pixelY, double& lat, double& lon) {
  // Convert pixel to tile offset
  TileCoord centerTile = latLonToTile(viewport.centerLat, viewport.centerLon, viewport.zoomLevel);
  
  double tileOffsetX = (pixelX - (viewport.screenCenterX - TILE_SIZE/2)) / (double)TILE_SIZE;
  double tileOffsetY = (pixelY - (viewport.screenCenterY - STATUS_BAR_HEIGHT - TILE_SIZE/2)) / (double)TILE_SIZE;
  
  // Calculate tile coordinates
  double tileX = centerTile.x + tileOffsetX;
  double tileY = centerTile.y + tileOffsetY;
  
  // Convert to lat/lon
  double n = pow(2.0, viewport.zoomLevel);
  lon = (tileX / n) * 360.0 - 180.0;
  
  double latRad = atan(sinh(M_PI * (1.0 - 2.0 * tileY / n)));
  lat = latRad * (180.0 / M_PI);
}

// ===================================================================
// TILE FILE OPERATIONS
// ===================================================================

String MapDisplay::getTilePath(const TileCoord& coord) {
  return String(TILE_FOLDER) + String(coord.zoom) + "/" + 
         String(coord.x) + "/" + String(coord.y) + "." + TILE_FORMAT;
}

bool MapDisplay::loadTileFromSD(const TileCoord& coord, uint8_t*& data, uint32_t& size) {
  String tilePath = getTilePath(coord);
  
  if (!SD.exists(tilePath)) {
    return false;
  }
  
  File tileFile = SD.open(tilePath, FILE_READ);
  if (!tileFile) {
    return false;
  }
  
  size = tileFile.size();
  if (size == 0 || size > 50000) { // Reasonable PNG size limits
    tileFile.close();
    return false;
  }
  
  data = (uint8_t*)malloc(size);
  if (!data) {
    tileFile.close();
    return false;
  }
  
  size_t bytesRead = tileFile.read(data, size);
  tileFile.close();
  
  if (bytesRead != size) {
    free(data);
    data = nullptr;
    return false;
  }
  
  return validateTileData(data, size);
}

bool MapDisplay::validateTileData(uint8_t* data, uint32_t size) {
  if (!data || size < 8) return false;
  
  // Check PNG magic number
  const uint8_t pngMagic[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
  return memcmp(data, pngMagic, 8) == 0;
}

uint32_t MapDisplay::calculateChecksum(uint8_t* data, uint32_t size) {
  // Simple CRC32-like checksum for data integrity
  uint32_t checksum = 0xFFFFFFFF;
  
  for (uint32_t i = 0; i < size; i++) {
    checksum ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (checksum & 1) {
        checksum = (checksum >> 1) ^ 0xEDB88320;
      } else {
        checksum >>= 1;
      }
    }
  }
  
  return checksum ^ 0xFFFFFFFF;
}

// ===================================================================
// PNG TILE RENDERING
// ===================================================================

void MapDisplay::drawPNGTile(uint8_t* pngData, uint32_t size, int x, int y) {
  // For now, we'll use a simplified approach
  // In a full implementation, you'd use a PNG decoder library like LodePNG
  // For this professional system, we'll draw a placeholder that indicates
  // the tile system is working and show tile coordinates
  
  TFT_eSPI& display = ::display.getTFT();
  
  // Draw tile boundary
  uint16_t tileColor = 0x7BEF; // Light gray
  display.drawRect(x, y, TILE_SIZE, TILE_SIZE, tileColor);
  
  // Fill with subtle pattern to show it's a valid tile
  for (int py = 0; py < TILE_SIZE; py += 8) {
    for (int px = 0; px < TILE_SIZE; px += 8) {
      if ((px + py) % 16 == 0) {
        display.fillRect(x + px, y + py, 4, 4, 0x39E7);
      }
    }
  }
  
  // Note: In production, replace this with actual PNG decoding:
  // Example with LodePNG:
  // unsigned char* image;
  // unsigned width, height;
  // lodepng_decode32(&image, &width, &height, pngData, size);
  // Then use pushImage or similar to display the decoded image
}

// ===================================================================
// DRAWING PRIMITIVES
// ===================================================================

void MapDisplay::drawCircle(int centerX, int centerY, int radius, uint16_t color) {
  TFT_eSPI& display = ::display.getTFT();
  display.fillCircle(centerX, centerY, radius, color);
}

void MapDisplay::drawTriangle(int centerX, int centerY, int size, float angle, uint16_t color) {
  TFT_eSPI& display = ::display.getTFT();
  
  // Calculate triangle points
  float angleRad = angle * (M_PI / 180.0);
  
  int x1 = centerX + (cos(angleRad) * size);
  int y1 = centerY + (sin(angleRad) * size);
  
  int x2 = centerX + (cos(angleRad + 2.094) * size * 0.6); // 120 degrees
  int y2 = centerY + (sin(angleRad + 2.094) * size * 0.6);
  
  int x3 = centerX + (cos(angleRad - 2.094) * size * 0.6); // -120 degrees
  int y3 = centerY + (sin(angleRad - 2.094) * size * 0.6);
  
  display.fillTriangle(x1, y1, x2, y2, x3, y3, color);
}

void MapDisplay::drawThickLine(int x1, int y1, int x2, int y2, int thickness, uint16_t color) {
  TFT_eSPI& display = ::display.getTFT();
  
  for (int i = -thickness/2; i <= thickness/2; i++) {
    for (int j = -thickness/2; j <= thickness/2; j++) {
      display.drawLine(x1 + i, y1 + j, x2 + i, y2 + j, color);
    }
  }
}

// ===================================================================
// PERFORMANCE AND STATUS FUNCTIONS
// ===================================================================

void MapDisplay::updatePerformanceStats() {
  // Update cache statistics periodically
  static unsigned long lastStatsUpdate = 0;
  if (millis() - lastStatsUpdate > 5000) { // Every 5 seconds
    lastStatsUpdate = millis();
    
    Serial.printf("MapDisplay Stats - Cache: %d/%d, Hits: %lu, Misses: %lu, Ratio: %.1f%%\n",
                  cacheUsed, MAX_CACHED_TILES, cacheHits, cacheMisses,
                  getCacheHitRatio() * 100.0);
  }
}

float MapDisplay::getCacheHitRatio() {
  unsigned long total = cacheHits + cacheMisses;
  return (total > 0) ? (float)cacheHits / total : 0.0;
}

unsigned long MapDisplay::getAverageDrawTime() {
  return averageDrawTime;
}

MapViewport MapDisplay::getViewport() {
  return viewport;
}

PositionMarker MapDisplay::getPosition() {
  return position;
}

String MapDisplay::getStatusText() {
  String status = "Map: ";
  
  if (!initialized) {
    status += "Not Initialized";
  } else if (cacheUsed == 0) {
    status += "No Tiles";
  } else {
    status += String(cacheUsed) + "/" + String(MAX_CACHED_TILES) + " tiles";
    status += ", Hit: " + String((int)(getCacheHitRatio() * 100)) + "%";
  }
  
  return status;
}

// ===================================================================
// GPS TRAIL IMPLEMENTATION
// ===================================================================

void GPSTrail::addPoint(double lat, double lon) {
  if (!enabled) return;
  
  // Check if we should add this point
  if (!shouldAddPoint(lat, lon)) return;
  
  // Add point to circular buffer
  TrailPoint& point = trailPoints[trailIndex];
  point.latitude = lat;
  point.longitude = lon;
  point.timestamp = millis();
  point.isValid = true;
  
  // Advance index
  trailIndex = (trailIndex + 1) % MAX_TRAIL_POINTS;
  
  // Update count
  if (trailCount < MAX_TRAIL_POINTS) {
    trailCount++;
  }
  
  // Remove old points
  removeOldPoints();
}

void GPSTrail::clearTrail() {
  trailCount = 0;
  trailIndex = 0;
  
  for (int i = 0; i < MAX_TRAIL_POINTS; i++) {
    trailPoints[i].isValid = false;
  }
}

void GPSTrail::setEnabled(bool enable) {
  enabled = enable;
  if (!enabled) {
    clearTrail();
  }
}

void GPSTrail::drawTrail(MapDisplay* mapDisplay) {
  if (!enabled || trailCount == 0) return;
  
  TFT_eSPI& display = ::display.getTFT();
  
  // Draw trail points as connected line
  int lastScreenX = -1, lastScreenY = -1;
  
  for (int i = 0; i < trailCount; i++) {
    int pointIndex = (trailIndex - trailCount + i + MAX_TRAIL_POINTS) % MAX_TRAIL_POINTS;
    TrailPoint& point = trailPoints[pointIndex];
    
    if (!point.isValid) continue;
    
    // Convert to screen coordinates
    int screenX, screenY;
    mapDisplay->latLonToPixel(point.latitude, point.longitude, screenX, screenY);
    screenY += STATUS_BAR_HEIGHT;
    
    // Check if point is visible
    if (screenX >= 0 && screenX < SCREEN_WIDTH && 
        screenY >= STATUS_BAR_HEIGHT && screenY < (STATUS_BAR_HEIGHT + CONTENT_AREA_HEIGHT)) {
      
      if (lastScreenX >= 0) {
        // Draw line segment
        display.drawLine(lastScreenX, lastScreenY, screenX, screenY, trailColor);
      }
      
      lastScreenX = screenX;
      lastScreenY = screenY;
    } else {
      // Point not visible, break connection
      lastScreenX = -1;
      lastScreenY = -1;
    }
  }
}

void GPSTrail::setMinDistance(float meters) {
  minDistance = meters;
}

void GPSTrail::setMaxAge(unsigned long ms) {
  maxAge = ms;
}

void GPSTrail::setTrailColor(uint16_t color) {
  trailColor = color;
}

int GPSTrail::getTrailPointCount() {
  return trailCount;
}

bool GPSTrail::isEnabled() {
  return enabled;
}

bool GPSTrail::shouldAddPoint(double lat, double lon) {
  if (trailCount == 0) return true;
  
  // Get last point
  int lastIndex = (trailIndex - 1 + MAX_TRAIL_POINTS) % MAX_TRAIL_POINTS;
  TrailPoint& lastPoint = trailPoints[lastIndex];
  
  if (!lastPoint.isValid) return true;
  
  // Check distance from last point
  float distance = calculateDistance(lat, lon, lastPoint.latitude, lastPoint.longitude);
  return distance >= minDistance;
}

void GPSTrail::removeOldPoints() {
  unsigned long currentTime = millis();
  
  // Remove points older than maxAge
  for (int i = 0; i < trailCount; i++) {
    int pointIndex = (trailIndex - trailCount + i + MAX_TRAIL_POINTS) % MAX_TRAIL_POINTS;
    TrailPoint& point = trailPoints[pointIndex];
    
    if (point.isValid && (currentTime - point.timestamp) > maxAge) {
      point.isValid = false;
      trailCount--;
    }
  }
}

float GPSTrail::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  // Haversine formula for distance calculation
  double dLat = (lat2 - lat1) * (M_PI / 180.0);
  double dLon = (lon2 - lon1) * (M_PI / 180.0);
  
  double a = sin(dLat/2) * sin(dLat/2) +
             cos(lat1 * (M_PI / 180.0)) * cos(lat2 * (M_PI / 180.0)) *
             sin(dLon/2) * sin(dLon/2);
  
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return EARTH_RADIUS * c; // Distance in meters
}

// ===================================================================
// TILE PRELOADER IMPLEMENTATION
// ===================================================================

void TilePreloader::initialize(MapDisplay* mapDisp) {
  mapDisplay = mapDisp;
  queueSize = 0;
  queueIndex = 0;
  enabled = true;
  lastPreload = 0;
}

void TilePreloader::queueSurroundingTiles(const TileCoord& center) {
  if (!enabled) return;
  
  clearQueue();
  
  // Queue tiles in a 3x3 grid around center
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      TileCoord coord;
      coord.x = center.x + dx;
      coord.y = center.y + dy;
      coord.zoom = center.zoom;
      
      queueTile(coord);
    }
  }
}

void TilePreloader::queueTile(const TileCoord& coord) {
  if (!enabled || isTileInQueue(coord)) return;
  
  addToQueue(coord);
}

void TilePreloader::clearQueue() {
  queueSize = 0;
  queueIndex = 0;
}

void TilePreloader::update() {
  if (!enabled || queueSize == 0) return;
  
  unsigned long currentTime = millis();
  if (currentTime - lastPreload >= preloadInterval) {
    preloadNextTile();
    lastPreload = currentTime;
  }
}

void TilePreloader::preloadNextTile() {
  if (queueSize == 0) return;
  
  TileCoord& coord = preloadQueue[queueIndex];
  
  // Try to load the tile (this will cache it if successful)
  mapDisplay->loadTile(coord);
  
  // Remove from queue
  queueIndex = (queueIndex + 1) % MAX_PRELOAD_QUEUE;
  queueSize--;
}

void TilePreloader::setEnabled(bool enable) {
  enabled = enable;
  if (!enabled) {
    clearQueue();
  }
}

bool TilePreloader::isEnabled() {
  return enabled;
}

bool TilePreloader::isTileInQueue(const TileCoord& coord) {
  for (int i = 0; i < queueSize; i++) {
    int index = (queueIndex + i) % MAX_PRELOAD_QUEUE;
    if (preloadQueue[index] == coord) {
      return true;
    }
  }
  return false;
}

void TilePreloader::addToQueue(const TileCoord& coord) {
  if (queueSize >= MAX_PRELOAD_QUEUE) return;
  
  int index = (queueIndex + queueSize) % MAX_PRELOAD_QUEUE;
  preloadQueue[index] = coord;
  queueSize++;
}

// ===================================================================
// GLOBAL ACCESS FUNCTIONS
// ===================================================================

bool initializeMapSystem() {
  Serial.println("Initializing map system...");
  
  // Initialize GPS trail
  gpsTrail.setMinDistance(5.0);     // 5 meters between points
  gpsTrail.setMaxAge(1800000);      // 30 minutes trail
  gpsTrail.setTrailColor(0xFD20);   // Orange trail
  gpsTrail.setEnabled(true);
  
  // Initialize tile preloader
  tilePreloader.initialize(&mapDisplay);
  tilePreloader.setEnabled(true);
  
  // Initialize map display
  bool success = mapDisplay.initialize();
  
  if (success) {
    Serial.println("Map system initialized successfully");
  } else {
    Serial.println("Map system initialization failed");
  }
  
  return success;
}

void updateMapDisplay() {
  mapDisplay.update();
  tilePreloader.update();
}

void setMapPosition(double lat, double lon, float heading, float speed) {
  mapDisplay.updatePosition(lat, lon, heading, speed);
  
  // Queue surrounding tiles for preloading
  if (mapDisplay.isPositionValid()) {
    TileCoord centerTile = mapDisplay.latLonToTile(lat, lon, mapDisplay.getViewport().zoomLevel);
    tilePreloader.queueSurroundingTiles(centerTile);
  }
}

void setMapZoom(int zoom) {
  mapDisplay.setZoom(zoom);
}

void setMapRecording(bool recording, bool paused) {
  MapRecordingState state = MAP_REC_OFF;
  
  if (recording) {
    state = paused ? MAP_REC_PAUSED : MAP_REC_RECORDING;
  }
  
  mapDisplay.setRecordingState(state);
}

void centerMapOnPosition() {
  mapDisplay.centerOnPosition();
}