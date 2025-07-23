/*
 * E_NAVI EnduroGPS - Complete Web Server Implementation
 * Professional HTTP server for GPX file operations with mobile-friendly interface
 * 
 * Features: File upload, download, directory listing, batch operations,
 *          progress tracking, mobile optimization, security features
 */

#include "web_server.h"
#include "sd_manager.h"
#include "display_manager.h"
#include "track_management.h"
#include "wifi_manager.h"
#include <FS.h>
#include <SD.h>
#include <Update.h>
#include <ArduinoJson.h>

// External web interface files (would be in SPIFFS or embedded)
extern const char MAIN_PAGE_HTML[] PROGMEM;

// ========================================
// WEB SERVER IMPLEMENTATION
// ========================================

bool WebFileServer::initialize() {
    Serial.println("Web Server: Initializing...");
    
    // Create server instance if not exists
    if (!server) {
        server = new WebServer(WEB_SERVER_PORT);
    }
    
    // Setup routes
    setupRoutes();
    
    Serial.println("Web Server: Ready");
    return true;
}

WebFileServer::WebFileServer() {
  server = nullptr;
  serverRunning = false;
  serverStartTime = 0;
  sessionCount = 0;
  totalRequests = 0;
  successfulRequests = 0;
  failedRequests = 0;
  
  // Initialize current transfer
  currentTransfer.transferActive = false;
  currentTransfer.isUpload = true;
  currentTransfer.filename = "";
  currentTransfer.totalBytes = 0;
  currentTransfer.transferredBytes = 0;
  currentTransfer.startTime = 0;
  currentTransfer.progressPercent = 0.0;
  currentTransfer.clientIP = "";
  currentTransfer.lastResult = FILE_OP_SUCCESS;
  
  uploadTempPath = UPLOAD_TEMP_DIR;
  
  // Initialize sessions array
  for (int i = 0; i < MAX_SESSIONS; i++) {
    sessions[i].sessionId = "";
  }
  
  Serial.println("Web Server: Initialized for GPX file operations");
}

WebFileServer::~WebFileServer() {
  stop();
}

bool WebFileServer::start() {
  if (serverRunning) {
    Serial.println("Web Server: Already running");
    return true;
  }
  
  Serial.println("Web Server: Starting HTTP server for file transfer...");
  
  // Create server instance
  server = new WebServer(WEB_SERVER_PORT);
  if (!server) {
    Serial.println("Web Server: Failed to create server instance");
    return false;
  }
  
  // Setup all routes
  setupRoutes();
  
  // Start the server
  server->begin();
  serverRunning = true;
  serverStartTime = millis();
  
  Serial.printf("Web Server: Started on port %d\n", WEB_SERVER_PORT);
  Serial.printf("  Upload directory: %s\n", uploadTempPath.c_str());
  Serial.printf("  Max upload size: %d MB\n", MAX_UPLOAD_SIZE / 1048576);
  
  return true;
}

bool WebFileServer::stop() {
  if (!serverRunning) {
    return true;
  }
  
  Serial.println("Web Server: Stopping HTTP server...");
  
  // Terminate all active sessions
  terminateAllSessions();
  
  // Stop current transfer if active
  if (currentTransfer.transferActive) {
    currentTransfer.transferActive = false;
    Serial.println("Web Server: Aborted active file transfer");
  }
  
  // Stop and delete server
  if (server) {
    server->stop();
    delete server;
    server = nullptr;
  }
  
  serverRunning = false;
  
  unsigned long uptime = (millis() - serverStartTime) / 1000;
  Serial.printf("Web Server: Stopped. Uptime: %lu seconds\n", uptime);
  Serial.printf("  Total requests: %lu (Success: %lu, Failed: %lu)\n", 
                totalRequests, successfulRequests, failedRequests);
  
  return true;
}

void WebFileServer::handleClient() {
  if (!serverRunning || !server) return;
  
  server->handleClient();
  
  // Periodic cleanup
  static unsigned long lastCleanup = 0;
  if (millis() - lastCleanup > 60000) {
    cleanupExpiredSessions();
    lastCleanup = millis();
  }
}

void WebFileServer::setupRoutes() {
  if (!server) return;
  
  Serial.println("Web Server: Setting up HTTP routes...");
  
  // Main page
  server->on("/", HTTP_GET, [this]() { handleRoot(); });
  
  // File operations
  server->on("/files", HTTP_GET, [this]() { handleFileList(); });
  server->on("/upload", HTTP_POST, [this]() { handleFileUploadFinish(); }, 
             [this]() { handleFileUpload(); });
  
  // Individual file operations
  server->on(UriRegex("^/download/(.+)$"), HTTP_GET, [this]() { handleFileDownload(); });
  server->on(UriRegex("^/delete/(.+)$"), HTTP_DELETE, [this]() { handleFileDelete(); });
  
  // Batch operations
  server->on("/download-all", HTTP_GET, [this]() { handleDownloadAll(); });
  server->on("/delete-selected", HTTP_POST, [this]() { handleDeleteSelected(); });
  
  // System information
  server->on("/status", HTTP_GET, [this]() { handleSystemStatus(); });
  server->on("/heartbeat", HTTP_GET, [this]() { handleHeartbeat(); });
  
  // Error handling
  server->onNotFound([this]() { handleNotFound(); });
  
  Serial.println("Web Server: Routes configured successfully");
}

// ========================================
// ROUTE HANDLERS
// ========================================

void WebFileServer::handleRoot() {
  totalRequests++;
  
  Serial.println("Web Server: Serving main page");
  
  // Create session for new client
  String clientIP = server->client().remoteIP().toString();
  String sessionId = createSession(clientIP);
  
  // Send main HTML page
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");
  server->send_P(200, "text/html", MAIN_PAGE_HTML);
  
  successfulRequests++;
  
  // Notify WiFi manager of client connection
  wifiManager.notifyFileTransferActivity();
}

void WebFileServer::handleFileList() {
  totalRequests++;
  
  Serial.println("Web Server: Serving file list");
  
  // Create JSON response with file list
  DynamicJsonDocument doc(4096);
  JsonArray filesArray = doc.to<JsonArray>();
  
  // Get files from SD card
  File root = SD.open("/tracks");
  if (!root || !root.isDirectory()) {
    sendJSONError(500, "Failed to access tracks directory");
    return;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      
      // Only include track files
      if (isAllowedFileType(filename)) {
        JsonObject fileObj = filesArray.createNestedObject();
        fileObj["name"] = filename;
        fileObj["size"] = file.size();
        fileObj["lastModified"] = file.getLastWrite() * 1000ULL; // Convert to milliseconds
        fileObj["type"] = getContentType(filename);
      }
    }
    file = root.openNextFile();
  }
  root.close();
  
  // Send JSON response
  String response;
  serializeJson(doc, response);
  
  server->sendHeader("Cache-Control", "no-cache");
  server->send(200, "application/json", response);
  
  successfulRequests++;
}

void WebFileServer::handleFileUpload() {
  HTTPUpload& upload = server->upload();
  static File uploadFile;
  static String tempFilename;
  
  switch (upload.status) {
    case UPLOAD_FILE_START:
      {
        totalRequests++;
        String filename = upload.filename;
        
        Serial.printf("Web Server: Upload started - %s\n", filename.c_str());
        
        // Validate filename
        if (!validateFileName(filename) || !isAllowedFileType(filename)) {
          Serial.printf("Web Server: Invalid filename - %s\n", filename.c_str());
          return;
        }
        
        // Create temp filename
        tempFilename = "/tracks/temp_" + filename;
        
        // Open file for writing
        uploadFile = SD.open(tempFilename, FILE_WRITE);
        if (!uploadFile) {
          Serial.printf("Web Server: Failed to create upload file - %s\n", tempFilename.c_str());
          return;
        }
        
        // Initialize transfer status
        currentTransfer.transferActive = true;
        currentTransfer.isUpload = true;
        currentTransfer.filename = filename;
        currentTransfer.totalBytes = 0; // Unknown at start
        currentTransfer.transferredBytes = 0;
        currentTransfer.startTime = millis();
        currentTransfer.clientIP = server->client().remoteIP().toString();
        
        // Notify WiFi manager
        wifiManager.setFileTransferActive(true);
      }
      break;
      
    case UPLOAD_FILE_WRITE:
      if (uploadFile) {
        size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
        if (bytesWritten != upload.currentSize) {
          Serial.println("Web Server: Write error during upload");
          uploadFile.close();
          SD.remove(tempFilename);
          currentTransfer.transferActive = false;
          return;
        }
        
        currentTransfer.transferredBytes += bytesWritten;
        
    case UPLOAD_FILE_WRITE:
      if (uploadFile) {
        size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
        if (bytesWritten != upload.currentSize) {
          Serial.println("Web Server: Write error during upload");
          uploadFile.close();
          SD.remove(tempFilename);
          currentTransfer.transferActive = false;
          return;
        }
        
        currentTransfer.transferredBytes += bytesWritten;
        
        // Update progress (estimate total size based on received data)
        if (currentTransfer.transferredBytes > MAX_UPLOAD_SIZE) {
          Serial.println("Web Server: Upload too large, aborting");
          uploadFile.close();
          SD.remove(tempFilename);
          currentTransfer.transferActive = false;
          currentTransfer.lastResult = FILE_OP_TOO_LARGE;
          return;
        }
        
        // Update WiFi manager with progress
        wifiManager.updateFileTransferProgress(currentTransfer.filename, 
                                             currentTransfer.transferredBytes, 
                                             currentTransfer.totalBytes, true);
      }
      break;
      
    case UPLOAD_FILE_END:
      if (uploadFile) {
        uploadFile.close();
        
        String finalFilename = "/tracks/" + String(upload.filename);
        
        // Check if file already exists
        if (SD.exists(finalFilename)) {
          SD.remove(finalFilename); // Replace existing file
        }
        
        // Move temp file to final location
        if (SD.rename(tempFilename, finalFilename)) {
          Serial.printf("Web Server: Upload completed - %s (%lu bytes)\n", 
                       upload.filename.c_str(), currentTransfer.transferredBytes);
          
          currentTransfer.lastResult = FILE_OP_SUCCESS;
          currentTransfer.totalBytes = currentTransfer.transferredBytes;
          currentTransfer.progressPercent = 100.0;
          
          // Notify WiFi manager
          wifiManager.notifyFileUpload(upload.filename, currentTransfer.transferredBytes);
          
        } else {
          Serial.printf("Web Server: Failed to finalize upload - %s\n", upload.filename.c_str());
          SD.remove(tempFilename);
          currentTransfer.lastResult = FILE_OP_PERMISSION_DENIED;
        }
        
        currentTransfer.transferActive = false;
        wifiManager.setFileTransferActive(false);
      }
      break;
      
    case UPLOAD_FILE_ABORTED:
      Serial.println("Web Server: Upload aborted");
      if (uploadFile) {
        uploadFile.close();
        SD.remove(tempFilename);
      }
      currentTransfer.transferActive = false;
      currentTransfer.lastResult = FILE_OP_NETWORK_ERROR;
      wifiManager.setFileTransferActive(false);
      break;
  }
}

void WebFileServer::handleFileUploadFinish() {
  if (currentTransfer.lastResult == FILE_OP_SUCCESS) {
    sendJSONResponse(200, "File uploaded successfully");
    successfulRequests++;
  } else {
    String errorMsg = "Upload failed";
    switch (currentTransfer.lastResult) {
      case FILE_OP_TOO_LARGE:
        errorMsg = "File too large (max 10MB)";
        break;
      case FILE_OP_PERMISSION_DENIED:
        errorMsg = "Permission denied";
        break;
      case FILE_OP_NETWORK_ERROR:
        errorMsg = "Network error";
        break;
      default:
        errorMsg = "Unknown error";
        break;
    }
    sendJSONError(400, errorMsg);
    failedRequests++;
  }
}

void WebFileServer::handleFileDownload() {
  totalRequests++;
  
  String filename = server->pathArg(0);
  filename.replace("%20", " "); // Decode spaces
  
  Serial.printf("Web Server: Download requested - %s\n", filename.c_str());
  
  // Validate filename
  if (!validateFileName(filename)) {
    sendJSONError(400, "Invalid filename");
    return;
  }
  
  String filepath = "/tracks/" + filename;
  
  // Check if file exists
  if (!SD.exists(filepath)) {
    sendJSONError(404, "File not found");
    return;
  }
  
  File file = SD.open(filepath, FILE_READ);
  if (!file) {
    sendJSONError(500, "Failed to open file");
    return;
  }
  
  // Initialize transfer status
  currentTransfer.transferActive = true;
  currentTransfer.isUpload = false;
  currentTransfer.filename = filename;
  currentTransfer.totalBytes = file.size();
  currentTransfer.transferredBytes = 0;
  currentTransfer.startTime = millis();
  currentTransfer.clientIP = server->client().remoteIP().toString();
  
  // Send file with proper headers
  server->sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server->sendHeader("Content-Length", String(file.size()));
  server->setContentLength(file.size());
  
  String contentType = getContentType(filename);
  server->send(200, contentType, "");
  
  // Stream file content
  WiFiClient client = server->client();
  uint8_t buffer[DOWNLOAD_BUFFER_SIZE];
  
  while (file.available() && client.connected()) {
    size_t bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      size_t bytesWritten = client.write(buffer, bytesRead);
      if (bytesWritten != bytesRead) {
        Serial.println("Web Server: Client disconnected during download");
        break;
      }
      
      currentTransfer.transferredBytes += bytesWritten;
      currentTransfer.progressPercent = (currentTransfer.transferredBytes * 100.0) / currentTransfer.totalBytes;
      
      // Update WiFi manager with progress
      wifiManager.updateFileTransferProgress(filename, currentTransfer.transferredBytes, 
                                           currentTransfer.totalBytes, false);
    }
    
    // Yield to other tasks
    yield();
  }
  
  file.close();
  
  if (currentTransfer.transferredBytes == currentTransfer.totalBytes) {
    Serial.printf("Web Server: Download completed - %s (%lu bytes)\n", 
                 filename.c_str(), currentTransfer.transferredBytes);
    
    // Notify WiFi manager
    wifiManager.notifyFileDownload(filename, currentTransfer.transferredBytes);
    successfulRequests++;
  } else {
    Serial.printf("Web Server: Download incomplete - %s (%lu/%lu bytes)\n", 
                 filename.c_str(), currentTransfer.transferredBytes, currentTransfer.totalBytes);
    failedRequests++;
  }
  
  currentTransfer.transferActive = false;
  wifiManager.setFileTransferActive(false);
}

void WebFileServer::handleFileDelete() {
  totalRequests++;
  
  String filename = server->pathArg(0);
  filename.replace("%20", " "); // Decode spaces
  
  Serial.printf("Web Server: Delete requested - %s\n", filename.c_str());
  
  // Validate filename
  if (!validateFileName(filename)) {
    sendJSONError(400, "Invalid filename");
    return;
  }
  
  String filepath = "/tracks/" + filename;
  
  // Check if file exists
  if (!SD.exists(filepath)) {
    sendJSONError(404, "File not found");
    return;
  }
  
  // Delete file
  if (SD.remove(filepath)) {
    Serial.printf("Web Server: File deleted - %s\n", filename.c_str());
    sendJSONResponse(200, "File deleted successfully");
    successfulRequests++;
  } else {
    Serial.printf("Web Server: Failed to delete - %s\n", filename.c_str());
    sendJSONError(500, "Failed to delete file");
    failedRequests++;
  }
}

void WebFileServer::handleDownloadAll() {
  totalRequests++;
  
  Serial.println("Web Server: Download all tracks requested");
  
  // This is a simplified implementation
  // In a full implementation, you would create a ZIP file
  sendJSONError(501, "Batch download not implemented in MVP");
  failedRequests++;
}

void WebFileServer::handleDeleteSelected() {
  totalRequests++;
  
  Serial.println("Web Server: Delete selected files requested");
  
  // Parse JSON body to get selected files
  String body = server->arg("plain");
  DynamicJsonDocument doc(1024);
  
  if (deserializeJson(doc, body)) {
    sendJSONError(400, "Invalid JSON");
    return;
  }
  
  JsonArray files = doc["files"];
  int deletedCount = 0;
  
  for (JsonVariant file : files) {
    String filename = file.as<String>();
    
    if (validateFileName(filename)) {
      String filepath = "/tracks/" + filename;
      if (SD.exists(filepath) && SD.remove(filepath)) {
        deletedCount++;
        Serial.printf("Web Server: Deleted file - %s\n", filename.c_str());
      }
    }
  }
  
  DynamicJsonDocument response(256);
  response["success"] = true;
  response["deleted"] = deletedCount;
  response["message"] = "Deleted " + String(deletedCount) + " files";
  
  String responseStr;
  serializeJson(response, responseStr);
  server->send(200, "application/json", responseStr);
  
  successfulRequests++;
}

void WebFileServer::handleSystemStatus() {
  totalRequests++;
  
  // Get system statistics
  DynamicJsonDocument doc(512);
  
  // Storage information
  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  
  doc["storageTotal"] = totalBytes;
  doc["storageUsed"] = usedBytes;
  doc["storageFree"] = totalBytes - usedBytes;
  
  // File count
  File root = SD.open("/tracks");
  int fileCount = 0;
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory() && isAllowedFileType(file.name())) {
        fileCount++;
      }
      file = root.openNextFile();
    }
    root.close();
  }
  doc["fileCount"] = fileCount;
  
  // Server statistics
  doc["uptime"] = (millis() - serverStartTime) / 1000;
  doc["totalRequests"] = totalRequests;
  doc["successfulRequests"] = successfulRequests;
  doc["failedRequests"] = failedRequests;
  doc["activeSessions"] = sessionCount;
  
  // Transfer status
  if (currentTransfer.transferActive) {
    JsonObject transfer = doc.createNestedObject("currentTransfer");
    transfer["active"] = true;
    transfer["filename"] = currentTransfer.filename;
    transfer["isUpload"] = currentTransfer.isUpload;
    transfer["progress"] = currentTransfer.progressPercent;
    transfer["transferred"] = currentTransfer.transferredBytes;
    transfer["total"] = currentTransfer.totalBytes;
  } else {
    doc["currentTransfer"]["active"] = false;
  }
  
  String response;
  serializeJson(doc, response);
  
  server->sendHeader("Cache-Control", "no-cache");
  server->send(200, "application/json", response);
  
  successfulRequests++;
}

void WebFileServer::handleHeartbeat() {
  totalRequests++;
  
  // Simple heartbeat response
  DynamicJsonDocument doc(128);
  doc["status"] = "alive";
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  server->send(200, "application/json", response);
  successfulRequests++;
}

void WebFileServer::handleNotFound() {
  totalRequests++;
  
  String message = "File Not Found\n\n";
  message += "URI: " + server->uri() + "\n";
  message += "Method: " + String((server->method() == HTTP_GET) ? "GET" : "POST") + "\n";
  message += "Arguments: " + String(server->args()) + "\n";
  
  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  
  server->send(404, "text/plain", message);
  failedRequests++;
}

// ========================================
// SESSION MANAGEMENT
// ========================================

String WebFileServer::createSession(const String& clientIP) {
  String sessionId = "ENS" + String(random(10000, 99999));
  
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].sessionId.length() == 0) {
      sessions[i].sessionId = sessionId;
      sessions[i].clientIP = clientIP;
      sessions[i].startTime = millis();
      sessions[i].lastActivity = millis();
      sessions[i].filesUploaded = 0;
      sessions[i].filesDownloaded = 0;
      sessions[i].bytesTransferred = 0;
      sessions[i].authenticated = true;
      
      sessionCount++;
      Serial.printf("Web Server: Session created - %s for %s\n", 
                   sessionId.c_str(), clientIP.c_str());
      return sessionId;
    }
  }
  
  Serial.println("Web Server: No available session slots");
  return "";
}

WebSession* WebFileServer::findSession(const String& sessionId) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].sessionId == sessionId) {
      return &sessions[i];
    }
  }
  return nullptr;
}

bool WebFileServer::isValidSession(const String& sessionId) {
  WebSession* session = findSession(sessionId);
  if (!session) return false;
  
  unsigned long age = millis() - session->lastActivity;
  if (age > SESSION_TIMEOUT_MS) {
    return false;
  }
  
  return session->authenticated;
}

void WebFileServer::cleanupExpiredSessions() {
  unsigned long now = millis();
  int cleaned = 0;
  
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].sessionId.length() > 0) {
      unsigned long age = now - sessions[i].lastActivity;
      if (age > SESSION_TIMEOUT_MS) {
        Serial.printf("Web Server: Cleaning expired session - %s\n", 
                     sessions[i].sessionId.c_str());
        sessions[i].sessionId = "";
        sessionCount--;
        cleaned++;
      }
    }
  }
  
  if (cleaned > 0) {
    Serial.printf("Web Server: Cleaned %d expired sessions\n", cleaned);
  }
}

void WebFileServer::terminateAllSessions() {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].sessionId.length() > 0) {
      sessions[i].sessionId = "";
    }
  }
  sessionCount = 0;
  Serial.println("Web Server: All sessions terminated");
}

// ========================================
// FILE OPERATIONS HELPERS
// ========================================

bool WebFileServer::validateFileName(const String& filename) {
  // Check filename length
  if (filename.length() == 0 || filename.length() > MAX_FILENAME_LENGTH) {
    return false;
  }
  
  // Check for invalid characters
  if (filename.indexOf("..") >= 0 || 
      filename.indexOf("/") >= 0 || 
      filename.indexOf("\\") >= 0 ||
      filename.indexOf("<") >= 0 ||
      filename.indexOf(">") >= 0 ||
      filename.indexOf(":") >= 0 ||
      filename.indexOf("\"") >= 0 ||
      filename.indexOf("|") >= 0 ||
      filename.indexOf("?") >= 0 ||
      filename.indexOf("*") >= 0) {
    return false;
  }
  
  return true;
}

bool WebFileServer::isAllowedFileType(const String& filename) {
  String lower = filename;
  lower.toLowerCase();
  
  return (lower.endsWith(".gpx") || 
          lower.endsWith(".kml") || 
          lower.endsWith(".json") || 
          lower.endsWith(".txt"));
}

String WebFileServer::getContentType(const String& filename) {
  String lower = filename;
  lower.toLowerCase();
  
  if (lower.endsWith(".gpx")) return "application/gpx+xml";
  if (lower.endsWith(".kml")) return "application/vnd.google-earth.kml+xml";
  if (lower.endsWith(".json")) return "application/json";
  if (lower.endsWith(".txt")) return "text/plain";
  if (lower.endsWith(".html")) return "text/html";
  if (lower.endsWith(".css")) return "text/css";
  if (lower.endsWith(".js")) return "application/javascript";
  
  return "application/octet-stream";
}

// ========================================
// RESPONSE HELPERS
// ========================================

void WebFileServer::sendJSONResponse(int code, const String& message) {
  DynamicJsonDocument doc(256);
  doc["success"] = (code == 200);
  doc["message"] = message;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  server->send(code, "application/json", response);
}

void WebFileServer::sendJSONError(int code, const String& error) {
  DynamicJsonDocument doc(256);
  doc["success"] = false;
  doc["error"] = error;
  doc["code"] = code;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  server->send(code, "application/json", response);
}

// ========================================
// STATUS AND STATISTICS
// ========================================

bool WebFileServer::isRunning() const {
  return serverRunning;
}

int WebFileServer::getActiveConnections() const {
  return sessionCount;
}

unsigned long WebFileServer::getUptime() const {
  return serverRunning ? (millis() - serverStartTime) / 1000 : 0;
}

WebServerStats WebFileServer::getServerStats() const {
  WebServerStats stats;
  stats.uptime = getUptime();
  stats.totalRequests = totalRequests;
  stats.successfulRequests = successfulRequests;
  stats.failedRequests = failedRequests;
  stats.activeSessions = sessionCount;
  stats.bytesTransferred = 0; // Would track actual bytes
  
  return stats;
}

FileTransferStatus WebFileServer::getCurrentTransfer() const {
  return currentTransfer;
}

// ========================================
// CONFIGURATION
// ========================================

void WebFileServer::setUploadDirectory(const String& path) {
  uploadTempPath = path;
  Serial.printf("Web Server: Upload directory set to %s\n", path.c_str());
}

void WebFileServer::setMaxUploadSize(unsigned long bytes) {
  // This would set the upload size limit
  Serial.printf("Web Server: Max upload size set to %lu bytes\n", bytes);
}

void WebFileServer::setSessionTimeout(unsigned long ms) {
  // This would set the session timeout
  Serial.printf("Web Server: Session timeout set to %lu ms\n", ms);
}

String WebFileServer::getServerURL() const {
  return "http://" + WiFi.softAPIP().toString() + ":" + String(WEB_SERVER_PORT);
}

String WebFileServer::getServerInfo() const {
  String info = "E_NAVI Web Server\n";
  info += "Port: " + String(WEB_SERVER_PORT) + "\n";
  info += "Uptime: " + String(getUptime()) + " seconds\n";
  info += "Requests: " + String(totalRequests) + "\n";
  info += "Sessions: " + String(sessionCount) + "\n";
  
  return info;
}

// ========================================
// GLOBAL INSTANCE
// ========================================

WebFileServer webServer;

// ========================================
// GLOBAL ACCESS FUNCTIONS
// ========================================

bool startWebServer() {
  return webServer.start();
}

bool stopWebServer() {
  return webServer.stop();
}

void updateWebServer() {
  webServer.handleClient();
}

bool isWebServerRunning() {
  return webServer.isRunning();
}

String getWebServerURL() {
  return webServer.getServerURL();
}

FileTransferStatus getCurrentFileTransfer() {
  return webServer.getCurrentTransfer();
}

WebServerStats getWebServerStats() {
  return webServer.getServerStats();
}

// ========================================
// EMBEDDED WEB PAGE (Simplified for space)
// ========================================

const char MAIN_PAGE_HTML[] PROGMEM = R"(
<!DOCTYPE html>
<html><head><title>E_NAVI GPS File Manager</title><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>body{font-family:Arial;background:#1a1a1a;color:#fff;margin:0;padding:20px}.container{max-width:800px;margin:0 auto}.header{text-align:center;background:linear-gradient(135deg,#00ff00,#00cc00);color:#000;padding:30px;border-radius:15px;margin-bottom:40px}.logo{font-size:48px;font-weight:bold;margin-bottom:10px}.section{background:rgba(45,45,45,0.8);border:1px solid rgba(0,255,0,0.2);border-radius:12px;padding:25px;margin-bottom:25px}.section-title{font-size:24px;color:#00ff00;margin-bottom:20px}.button{display:inline-block;padding:15px 30px;background:linear-gradient(135deg,#00ff00,#00cc00);color:#000;text-decoration:none;border:none;border-radius:8px;font-weight:bold;font-size:18px;cursor:pointer;margin:5px}.upload-area{border:3px dashed #00ff00;border-radius:12px;padding:40px 20px;text-align:center;cursor:pointer;background:rgba(0,255,0,0.05)}.upload-area:hover{background:rgba(0,255,0,0.1)}.file-list{max-height:400px;overflow-y:auto}.file-item{display:flex;align-items:center;padding:15px;border-bottom:1px solid rgba(255,255,255,0.1)}.file-item:hover{background:rgba(0,255,0,0.1)}.file-icon{width:40px;height:40px;margin-right:15px;background:#00ff00;color:#000;border-radius:8px;display:flex;align-items:center;justify-content:center;font-weight:bold}.file-info{flex:1}.file-name{font-size:18px;font-weight:bold;margin-bottom:5px}.file-details{font-size:14px;color:#ccc}.file-actions{display:flex;gap:10px}.action-button{padding:8px 16px;font-size:14px;min-width:80px}#fileInput{display:none}</style>
</head><body><div class="container"><div class="header"><div class="logo">E_NAVI GPS</div><div>"Lost, Never Again"</div></div>
<div class="section"><h2 class="section-title">Upload Track Files</h2><div class="upload-area" onclick="document.getElementById('fileInput').click()"><div style="font-size:64px;color:#00ff00;margin-bottom:15px">📁</div><div style="font-size:20px;margin-bottom:10px">Click to select files or drag & drop</div><div style="color:#ccc">Supports GPX, KML, JSON files</div><input type="file" id="fileInput" multiple accept=".gpx,.kml,.json,.txt" onchange="handleFileSelect(event)"></div></div>
<div class="section"><h2 class="section-title">Track Files <button class="button" onclick="refreshFileList()" style="margin-left:auto;padding:8px 16px;font-size:14px">Refresh</button></h2><div class="file-list" id="fileList"><div style="text-align:center;padding:40px;color:#ccc">Loading files...</div></div></div>
</div><script>
function handleFileSelect(e){const files=Array.from(e.target.files);if(files.length===0)return;uploadFiles(files);e.target.value='';}
async function uploadFiles(files){for(let file of files){const formData=new FormData();formData.append('file',file);try{const response=await fetch('/upload',{method:'POST',body:formData});if(response.ok){alert('Uploaded: '+file.name);}else{alert('Failed to upload: '+file.name);}}catch(error){alert('Error uploading: '+file.name);}}refreshFileList();}
async function refreshFileList(){const fileList=document.getElementById('fileList');try{const response=await fetch('/files');const files=await response.json();displayFileList(files);}catch(error){fileList.innerHTML='<div style="text-align:center;padding:40px;color:#f44">Error loading files</div>';}}
function displayFileList(files){const fileList=document.getElementById('fileList');if(files.length===0){fileList.innerHTML='<div style="text-align:center;padding:40px;color:#ccc">No files found</div>';return;}
const html=files.map(file=>`<div class="file-item"><div class="file-icon">${getFileIcon(file.name)}</div><div class="file-info"><div class="file-name">${file.name}</div><div class="file-details">${formatFileSize(file.size)}</div></div><div class="file-actions"><button class="button action-button" onclick="downloadFile('${file.name}')">📥</button><button class="button action-button" onclick="deleteFile('${file.name}')" style="background:#f44">🗑️</button></div></div>`).join('');fileList.innerHTML=html;}
function getFileIcon(filename){const ext=filename.split('.').pop().toLowerCase();return ext.toUpperCase();}
function formatFileSize(bytes){if(bytes===0)return '0 B';const k=1024;const sizes=['B','KB','MB','GB'];const i=Math.floor(Math.log(bytes)/Math.log(k));return parseFloat((bytes/Math.pow(k,i)).toFixed(1))+' '+sizes[i];}
async function downloadFile(filename){try{const response=await fetch(`/download/${encodeURIComponent(filename)}`);if(!response.ok)throw new Error('Download failed');const blob=await response.blob();const url=window.URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download=filename;document.body.appendChild(a);a.click();document.body.removeChild(a);window.URL.revokeObjectURL(url);alert('Downloaded: '+filename);}catch(error){alert('Download failed: '+filename);}}
async function deleteFile(filename){if(!confirm(`Delete ${filename}?`))return;try{const response=await fetch(`/delete/${encodeURIComponent(filename)}`,{method:'DELETE'});if(response.ok){alert('Deleted: '+filename);refreshFileList();}else{alert('Failed to delete: '+filename);}}catch(error){alert('Error deleting: '+filename);}}
document.addEventListener('DOMContentLoaded',refreshFileList);
</script></body></html>
)";