 /*
 * E_NAVI EnduroGPS - Track Management Implementation
 * Complete file management system for recorded tracks
 */

#include "track_management.h"
#include "display_manager.h"

// ========================================
// TRACK MANAGER IMPLEMENTATION
// ========================================

TrackManager::TrackManager() {
  trackCount = 0;
  listLoaded = false;
  selectedTrack = -1;
  currentTrackFile = "";
  displayOffset = 0;
  statisticsValid = false;
  
  // Initialize statistics
  memset(&statistics, 0, sizeof(statistics));
  statistics.shortestTrack = 999999.0;
  statistics.shortestDuration = 999999;
  statistics.lowestPoint = 999999.0;
}

bool TrackManager::initialize() {
  Serial.println("TrackManager: Initializing...");
  
  // Ensure track directory exists
  if (!sdManager.directoryExists(trackDirectory)) {
    if (!sdManager.createDirectory(trackDirectory)) {
      Serial.println("TrackManager: Failed to create track directory");
      return false;
    }
  }
  
  // Load track list
  if (!loadTrackList()) {
    Serial.println("TrackManager: Failed to load track list");
    return false;
  }
  
  // Calculate statistics
  calculateStatistics();
  
  Serial.printf("TrackManager: Initialized with %d tracks\n", trackCount);
  return true;
}

bool TrackManager::loadTrackList() {
  trackCount = 0;
  
  // Get list of GPX files in track directory
  String files[MAX_TRACKS];
  int fileCount = 0;
  
  if (!sdManager.listDirectory(trackDirectory, files, MAX_TRACKS, fileCount)) {
    Serial.println("TrackManager: Failed to list track directory");
    return false;
  }
  
  Serial.printf("TrackManager: Found %d files in track directory\n", fileCount);
  
  // Analyze each GPX file
  for (int i = 0; i < fileCount && trackCount < MAX_TRACKS; i++) {
    if (files[i].endsWith(".gpx")) {
      String fullPath = trackDirectory + "/" + files[i];
      
      if (analyzeTrackFile(fullPath, trackList[trackCount])) {
        trackCount++;
      } else {
        Serial.println("TrackManager: Failed to analyze " + files[i]);
      }
    }
  }
  
  // Sort by date (newest first)
  sortByDate(false);
  
  listLoaded = true;
  return true;
}

bool TrackManager::analyzeTrackFile(const String& filename, TrackFileInfo& info) {
  // Initialize track info
  info.filename = filename;
  info.displayName = getTrackNameFromFile(filename);
  info.fileSize = sdManager.getFileSize(filename);
  info.isValid = false;
  info.isCorrupted = false;
  
  // Get file date/time
  String dateTime = sdManager.getFileDate(filename);
  // Parse date/time (simplified for MVP)
  info.creationDate = dateTime.substring(0, 10);  // DD/MM/YYYY
  info.creationTime = dateTime.substring(11, 16); // HH:MM
  
  // Basic file validation
  if (info.fileSize == 0) {
    info.isCorrupted = true;
    return false;
  }
  
  if (info.fileSize > MAX_TRACK_SIZE_MB * 1048576) {
    Serial.println("TrackManager: Track file too large: " + filename);
    return false;
  }
  
  // Parse GPX content
  if (parseGPXFile(filename, info)) {
    info.isValid = true;
    return true;
  }
  
  return false;
}

bool TrackManager::parseGPXFile(const String& filename, TrackFileInfo& info) {
  // For MVP, do basic parsing without full XML parser
  String content = sdManager.readFile(filename);
  if (content.length() == 0) {
    return false;
  }
  
  // Count track points
  info.pointCount = 0;
  int pos = 0;
  while ((pos = content.indexOf("<trkpt", pos)) != -1) {
    info.pointCount++;
    pos++;
  }
  
  if (info.pointCount == 0) {
    return false;
  }
  
  // Extract basic statistics (simplified parsing)
  info.totalDistance = extractDistanceFromGPX(content);
  info.duration = extractDurationFromGPX(content);
  info.maxSpeed = extractMaxSpeedFromGPX(content);
  info.avgSpeed = (info.duration > 0) ? (info.totalDistance / info.duration * 3600) : 0;
  
  // Extract elevation data
  info.maxAltitude = extractMaxElevationFromGPX(content);
  info.minAltitude = extractMinElevationFromGPX(content);
  info.elevationGain = extractElevationGainFromGPX(content);
  info.elevationLoss = extractElevationLossFromGPX(content);
  
  return true;
}

void TrackManager::calculateStatistics() {
  // Reset statistics
  memset(&statistics, 0, sizeof(statistics));
  statistics.shortestTrack = 999999.0;
  statistics.shortestDuration = 999999;
  statistics.lowestPoint = 999999.0;
  
  if (trackCount == 0) {
    statisticsValid = true;
    return;
  }
  
  statistics.totalTracks = trackCount;
  
  // Calculate combined statistics
  for (int i = 0; i < trackCount; i++) {
    const TrackFileInfo& track = trackList[i];
    
    if (!track.isValid) continue;
    
    // File statistics
    statistics.totalSize += track.fileSize;
    
    // Distance statistics
    statistics.totalDistance += track.totalDistance;
    if (track.totalDistance > statistics.longestTrack) {
      statistics.longestTrack = track.totalDistance;
    }
    if (track.totalDistance < statistics.shortestTrack) {
      statistics.shortestTrack = track.totalDistance;
    }
    
    // Time statistics
    statistics.totalTime += track.duration;
    if (track.duration > statistics.longestDuration) {
      statistics.longestDuration = track.duration;
    }
    if (track.duration < statistics.shortestDuration) {
      statistics.shortestDuration = track.duration;
    }
    
    // Speed statistics
    if (track.maxSpeed > statistics.maxSpeedOverall) {
      statistics.maxSpeedOverall = track.maxSpeed;
    }
    
    // Elevation statistics
    if (track.maxAltitude > statistics.highestPoint) {
      statistics.highestPoint = track.maxAltitude;
    }
    if (track.minAltitude < statistics.lowestPoint) {
      statistics.lowestPoint = track.minAltitude;
    }
    statistics.totalElevationGain += track.elevationGain;
    statistics.totalElevationLoss += track.elevationLoss;
  }
  
  // Calculate averages
  if (trackCount > 0) {
    statistics.avgTrackDistance = statistics.totalDistance / trackCount;
    statistics.avgDuration = statistics.totalTime / trackCount;
    statistics.avgSpeedOverall = (statistics.totalTime > 0) ? 
                                (statistics.totalDistance / statistics.totalTime * 3600) : 0;
  }
  
  statisticsValid = true;
  
  Serial.printf("TrackManager: Statistics - %d tracks, %.1f km total, %lu MB storage\n",
               statistics.totalTracks, statistics.totalDistance, 
               statistics.totalSize / 1048576);
}

TrackFileInfo TrackManager::getTrackInfo(int index) {
  if (index >= 0 && index < trackCount) {
    return trackList[index];
  }
  
  // Return empty track info
  TrackFileInfo empty;
  memset(&empty, 0, sizeof(empty));
  return empty;
}

bool TrackManager::deleteTrack(int index) {
  if (index < 0 || index >= trackCount) {
    return false;
  }
  
  String filename = trackList[index].filename;
  
  // Delete file
  if (sdManager.deleteFile(filename) != FILE_SUCCESS) {
    Serial.println("TrackManager: Failed to delete " + filename);
    return false;
  }
  
  // Remove from list
  for (int i = index; i < trackCount - 1; i++) {
    trackList[i] = trackList[i + 1];
  }
  trackCount--;
  
  // Recalculate statistics
  calculateStatistics();
  
  Serial.println("TrackManager: Deleted track " + filename);
  return true;
}

String TrackManager::getTrackSummary(int index) {
  if (index < 0 || index >= trackCount) {
    return "Invalid track";
  }
  
  const TrackFileInfo& track = trackList[index];
  
  String summary = track.displayName + "\n";
  summary += "Date: " + track.creationDate + " " + track.creationTime + "\n";
  summary += "Distance: " + formatDistance(track.totalDistance) + "\n";
  summary += "Duration: " + formatDuration(track.duration) + "\n";
  summary += "Max Speed: " + formatSpeed(track.maxSpeed) + "\n";
  summary += "Points: " + String(track.pointCount) + "\n";
  summary += "Size: " + formatFileSize(track.fileSize);
  
  return summary;
}

TrackStatistics TrackManager::getStatistics() {
  if (!statisticsValid) {
    calculateStatistics();
  }
  return statistics;
}

void TrackManager::sortByDate(bool ascending) {
  // Simple bubble sort for MVP (good enough for <100 tracks)
  for (int i = 0; i < trackCount - 1; i++) {
    for (int j = 0; j < trackCount - i - 1; j++) {
      bool shouldSwap = false;
      
      // Compare creation dates (simplified)
      if (ascending) {
        shouldSwap = (trackList[j].creationDate > trackList[j + 1].creationDate);
      } else {
        shouldSwap = (trackList[j].creationDate < trackList[j + 1].creationDate);
      }
      
      if (shouldSwap) {
        TrackFileInfo temp = trackList[j];
        trackList[j] = trackList[j + 1];
        trackList[j + 1] = temp;
      }
    }
  }
}

// ========================================
// TRACK DISPLAY UI IMPLEMENTATION
// ========================================

TrackDisplayUI::TrackDisplayUI(TrackManager* manager) {
  trackManager = manager;
  showingList = false;
  showingDetails = false;
  selectedIndex = 0;
  scrollOffset = 0;
  lastUpdate = 0;
  statusMessage = "";
}

void TrackDisplayUI::showTrackList() {
  showingList = true;
  showingDetails = false;
  selectedIndex = 0;
  scrollOffset = 0;
  
  // Refresh track list
  trackManager->refreshTrackList();
  
  drawTrackList();
}

void TrackDisplayUI::drawTrackList() {
  display.clearScreen();
  
  // Title
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(30, "TRACK LIST", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  int trackCount = trackManager->getTrackCount();
  
  if (trackCount == 0) {
    display.setTextSize(FONT_SIZE_MEDIUM);
    display.drawCenteredText(200, "No tracks recorded", 
                            display.getCurrentColors().text, FONT_SIZE_MEDIUM);
    display.drawCenteredText(240, "Start recording to see tracks here", 
                            display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
    return;
  }
  
  // Track count
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(55, String(trackCount) + " tracks", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
  
  // Draw tracks
  int y = listStartY;
  int endIndex = min(scrollOffset + itemsPerScreen, trackCount);
  
  for (int i = scrollOffset; i < endIndex; i++) {
    bool selected = (i == selectedIndex);
    drawTrackItem(i, y, selected);
    y += itemHeight;
  }
  
  // Draw scroll indicator if needed
  if (trackCount > itemsPerScreen) {
    drawScrollIndicator();
  }
  
  // Controls
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(450, "[↑↓] Navigate [CENTER] Select [LEFT] Back", 
                          display.getCurrentColors().textSecondary, FONT_SIZE_SMALL);
}

void TrackDisplayUI::drawTrackItem(int index, int yPos, bool selected) {
  TrackFileInfo track = trackManager->getTrackInfo(index);
  
  // Background for selected item
  if (selected) {
    display.fillRect(5, yPos - 2, SCREEN_WIDTH - 10, itemHeight - 2, 
                    display.getCurrentColors().selectedBg);
  }
  
  // Track name
  display.setTextSize(FONT_SIZE_MEDIUM);
  uint16_t nameColor = selected ? display.getCurrentColors().selectedText : 
                                 display.getCurrentColors().text;
  display.setTextColor(nameColor);
  display.setCursor(10, yPos + 2);
  
  String displayName = track.displayName;
  if (displayName.length() > 25) {
    displayName = displayName.substring(0, 22) + "...";
  }
  display.print(displayName);
  
  // Track info (distance, date)
  display.setTextSize(FONT_SIZE_SMALL);
  uint16_t infoColor = selected ? display.getCurrentColors().selectedText : 
                                 display.getCurrentColors().textSecondary;
  display.setTextColor(infoColor);
  
  display.setCursor(10, yPos + 20);
  display.print(formatDistance(track.totalDistance) + " • " + track.creationDate);
  
  // Status indicators
  display.setCursor(250, yPos + 2);
  if (!track.isValid) {
    display.setTextColor(display.getCurrentColors().gpsBad);
    display.print("ERROR");
  } else if (track.isCorrupted) {
    display.setTextColor(display.getCurrentColors().gpsWarn);
    display.print("CORRUPT");
  } else {
    display.setTextColor(infoColor);
    display.print(formatFileSize(track.fileSize));
  }
}

void TrackDisplayUI::drawScrollIndicator() {
  int trackCount = trackManager->getTrackCount();
  int indicatorHeight = (itemsPerScreen * 200) / trackCount;
  int indicatorY = 80 + (scrollOffset * 200) / trackCount;
  
  // Scroll track background
  display.drawRect(310, 80, 8, 200, display.getCurrentColors().text);
  
  // Scroll indicator
  display.fillRect(312, indicatorY, 4, indicatorHeight, 
                  display.getCurrentColors().accent);
}

void TrackDisplayUI::handleInput(ButtonID button, ButtonEvent event) {
  if (event != BTN_EVENT_CLICK) return;
  
  if (showingList) {
    switch (button) {
      case BTN_UP:
        navigateUp();
        break;
      case BTN_DOWN:
        navigateDown();
        break;
      case BTN_CENTER:
        selectCurrentTrack();
        break;
      case BTN_LEFT:
        goBack();
        break;
    }
  } else if (showingDetails) {
    switch (button) {
      case BTN_LEFT:
        showTrackList();  // Back to list
        break;
    }
  }
}

void TrackDisplayUI::navigateUp() {
  if (selectedIndex > 0) {
    selectedIndex--;
    
    // Update scroll if needed
    if (selectedIndex < scrollOffset) {
      scrollOffset = selectedIndex;
    }
    
    drawTrackList();
  }
}

void TrackDisplayUI::navigateDown() {
  int trackCount = trackManager->getTrackCount();
  
  if (selectedIndex < trackCount - 1) {
    selectedIndex++;
    
    // Update scroll if needed
    if (selectedIndex >= scrollOffset + itemsPerScreen) {
      scrollOffset = selectedIndex - itemsPerScreen + 1;
    }
    
    drawTrackList();
  }
}

void TrackDisplayUI::selectCurrentTrack() {
  showTrackDetails(selectedIndex);
}

void TrackDisplayUI::showTrackDetails(int index) {
  TrackFileInfo track = trackManager->getTrackInfo(index);
  
  if (!track.isValid) {
    showMessage("Track file is invalid");
    return;
  }
  
  showingList = false;
  showingDetails = true;
  
  drawTrackDetails(track);
}

void TrackDisplayUI::drawTrackDetails(const TrackFileInfo& track) {
  display.clearScreen();
  
  // Title
  display.setTextSize(FONT_SIZE_LARGE);
  display.setTextColor(display.getCurrentColors().accent);
  display.drawCenteredText(25, "TRACK DETAILS", display.getCurrentColors().accent, FONT_SIZE_LARGE);
  
  // Track name
  display.setTextSize(FONT_SIZE_MEDIUM);
  display.setTextColor(display.getCurrentColors().text);
  display.drawCenteredText(55, track.displayName, display.getCurrentColors().text, FONT_SIZE_MEDIUM);
  
  // Details in two columns
  int leftX = 20;
  int rightX = 170;
  int y = 90;
  int lineHeight = 25;
  
  display.setTextSize(FONT_SIZE_SMALL);
  
  // Left column
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(leftX, y);
  display.print("Date:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(leftX + 50, y);
  display.print(track.creationDate);
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(leftX, y);
  display.print("Time:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(leftX + 50, y);
  display.print(track.creationTime);
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(leftX, y);
  display.print("Distance:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(leftX + 70, y);
  display.print(formatDistance(track.totalDistance));
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(leftX, y);
  display.print("Duration:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(leftX + 70, y);
  display.print(formatDuration(track.duration));
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(leftX, y);
  display.print("Avg Speed:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(leftX + 80, y);
  display.print(formatSpeed(track.avgSpeed));
  
  // Right column
  y = 90;
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(rightX, y);
  display.print("Max Speed:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(rightX + 80, y);
  display.print(formatSpeed(track.maxSpeed));
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(rightX, y);
  display.print("Max Alt:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(rightX + 60, y);
  display.print(formatElevation(track.maxAltitude));
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(rightX, y);
  display.print("Min Alt:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(rightX + 60, y);
  display.print(formatElevation(track.minAltitude));
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(rightX, y);
  display.print("Climb:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(rightX + 50, y);
  display.print(formatElevation(track.elevationGain));
  y += lineHeight;
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(rightX, y);
  display.print("Descent:");
  display.setTextColor(display.getCurrentColors().text);
  display.setCursor(rightX + 60, y);
  display.print(formatElevation(track.elevationLoss));
  
  // File info
  y = 250;
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(leftX, y);
  display.print("Points: ");
  display.setTextColor(display.getCurrentColors().text);
  display.print(String(track.pointCount));
  
  display.setTextColor(display.getCurrentColors().textSecondary);
  display.setCursor(rightX, y);
  display.print("Size: ");
  display.setTextColor(display.getCurrentColors().text);
  display.print(formatFileSize(track.fileSize));
  
  // Controls
  display.setTextSize(FONT_SIZE_SMALL);
  display.drawCenteredText(450, "[LEFT] Back to List", 
                          display.getCurrentColors().accent, FONT_SIZE_SMALL);
}

void TrackDisplayUI::goBack() {
  if (showingDetails) {
    showTrackList();
  } else {
    hideDisplay();
  }
}

void TrackDisplayUI::hideDisplay() {
  showingList = false;
  showingDetails = false;
}

void TrackDisplayUI::showMessage(const String& message) {
  statusMessage = message;
  // Show message for 3 seconds
  // Implementation would overlay message on current display
}

// ========================================
// UTILITY FUNCTION IMPLEMENTATIONS
// ========================================

String generateTrackName() {
  // Generate name based on current date/time
  GPSData gps = getGPSData();
  
  char trackName[32];
  sprintf(trackName, "Track_%02d%02d_%02d%02d", 
          gps.day, gps.month, gps.localHour, gps.minute);
  
  return String(trackName);
}

String getTrackNameFromFile(const String& filename) {
  // Extract name from filename, remove path and extension
  int lastSlash = filename.lastIndexOf('/');
  int lastDot = filename.lastIndexOf('.');
  
  if (lastSlash >= 0 && lastDot > lastSlash) {
    return filename.substring(lastSlash + 1, lastDot);
  } else if (lastDot > 0) {
    return filename.substring(0, lastDot);
  } else {
    return filename;
  }
}

String formatDistance(float km) {
  if (km < 1.0) {
    return String((int)(km * 1000)) + " m";
  } else if (km < 10.0) {
    return String(km, 2) + " km";
  } else {
    return String(km, 1) + " km";
  }
}

String formatSpeed(float kmh) {
  return String(kmh, 1) + " km/h";
}

String formatElevation(float meters) {
  return String((int)meters) + " m";
}

String formatDuration(unsigned long seconds) {
  if (seconds < 60) {
    return String(seconds) + "s";
  } else if (seconds < 3600) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    return String(minutes) + "m " + String(secs) + "s";
  } else {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    return String(hours) + "h " + String(minutes) + "m";
  }
}

String formatFileSize(unsigned long bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < 1048576) {
    return String(bytes / 1024) + " KB";
  } else {
    return String(bytes / 1048576) + " MB";
  }
}

// ========================================
// SIMPLIFIED GPX PARSING FOR MVP
// ========================================

float extractDistanceFromGPX(const String& content) {
  // Look for distance metadata in GPX
  int distPos = content.indexOf("<distance>");
  if (distPos >= 0) {
    int endPos = content.indexOf("</distance>", distPos);
    if (endPos > distPos) {
      String distStr = content.substring(distPos + 10, endPos);
      return distStr.toFloat() / 1000.0; // Convert m to km
    }
  }
  
  // If no metadata, estimate from track points (simplified)
  return estimateDistanceFromPoints(content);
}

unsigned long extractDurationFromGPX(const String& content) {
  // Look for duration metadata
  int durPos = content.indexOf("<duration>");
  if (durPos >= 0) {
    int endPos = content.indexOf("</duration>", durPos);
    if (endPos > durPos) {
      String durStr = content.substring(durPos + 10, endPos);
      return durStr.toInt();
    }
  }
  
  // Estimate from first and last timestamps
  return estimateDurationFromTimestamps(content);
}

float extractMaxSpeedFromGPX(const String& content) {
  // Look for speed metadata
  int speedPos = content.indexOf("<maxspeed>");
  if (speedPos >= 0) {
    int endPos = content.indexOf("</maxspeed>", speedPos);
    if (endPos > speedPos) {
      String speedStr = content.substring(speedPos + 10, endPos);
      return speedStr.toFloat() * 3.6; // Convert m/s to km/h
    }
  }
  
  // Default estimate
  return 0.0;
}

float extractMaxElevationFromGPX(const String& content) {
  float maxEle = -999.0;
  int pos = 0;
  
  while ((pos = content.indexOf("<ele>", pos)) != -1) {
    int endPos = content.indexOf("</ele>", pos);
    if (endPos > pos) {
      String eleStr = content.substring(pos + 5, endPos);
      float ele = eleStr.toFloat();
      if (ele > maxEle) {
        maxEle = ele;
      }
    }
    pos = endPos;
  }
  
  return (maxEle > -999.0) ? maxEle : 0.0;
}

float extractMinElevationFromGPX(const String& content) {
  float minEle = 9999.0;
  int pos = 0;
  
  while ((pos = content.indexOf("<ele>", pos)) != -1) {
    int endPos = content.indexOf("</ele>", pos);
    if (endPos > pos) {
      String eleStr = content.substring(pos + 5, endPos);
      float ele = eleStr.toFloat();
      if (ele < minEle) {
        minEle = ele;
      }
    }
    pos = endPos;
  }
  
  return (minEle < 9999.0) ? minEle : 0.0;
}

float extractElevationGainFromGPX(const String& content) {
  // Look for gain metadata
  int gainPos = content.indexOf("<gain>");
  if (gainPos >= 0) {
    int endPos = content.indexOf("</gain>", gainPos);
    if (endPos > gainPos) {
      String gainStr = content.substring(gainPos + 6, endPos);
      return gainStr.toFloat();
    }
  }
  
  // Estimate from elevation profile
  return estimateElevationGain(content);
}

float extractElevationLossFromGPX(const String& content) {
  // Look for loss metadata
  int lossPos = content.indexOf("<loss>");
  if (lossPos >= 0) {
    int endPos = content.indexOf("</loss>", lossPos);
    if (endPos > lossPos) {
      String lossStr = content.substring(lossPos + 6, endPos);
      return lossStr.toFloat();
    }
  }
  
  // Estimate from elevation profile
  return estimateElevationLoss(content);
}

// ========================================
// ESTIMATION HELPERS (SIMPLIFIED FOR MVP)
// ========================================

float estimateDistanceFromPoints(const String& content) {
  // Very simplified distance estimation
  // Count track points and assume average 10m spacing
  int pointCount = 0;
  int pos = 0;
  
  while ((pos = content.indexOf("<trkpt", pos)) != -1) {
    pointCount++;
    pos++;
  }
  
  // Rough estimate: 10 meters per point average
  return (pointCount * 10.0) / 1000.0; // Convert to km
}

unsigned long estimateDurationFromTimestamps(const String& content) {
  // Find first and last timestamps
  int firstTime = content.indexOf("<time>");
  int lastTime = content.lastIndexOf("<time>");
  
  if (firstTime >= 0 && lastTime > firstTime) {
    // For MVP, return a reasonable estimate
    // Real implementation would parse ISO timestamps
    return 3600; // 1 hour default
  }
  
  return 0;
}

float estimateElevationGain(const String& content) {
  // Simplified gain estimation
  float maxEle = extractMaxElevationFromGPX(content);
  float minEle = extractMinElevationFromGPX(content);
  
  // Rough estimate: 60% of total elevation difference
  return (maxEle - minEle) * 0.6;
}

float estimateElevationLoss(const String& content) {
  // Simplified loss estimation
  float maxEle = extractMaxElevationFromGPX(content);
  float minEle = extractMinElevationFromGPX(content);
  
  // Rough estimate: 60% of total elevation difference
  return (maxEle - minEle) * 0.6;
}

// ========================================
// GLOBAL INSTANCES
// ========================================
TrackManager trackManager;
TrackDisplayUI trackDisplayUI(&trackManager);
