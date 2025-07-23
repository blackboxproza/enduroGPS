/*
 * E_NAVI EnduroGPS - Web Server for File Transfer
 * HTTP Server for GPX Upload/Download Operations
 * 
 * FIXED: JavaScript parsing issues - properly embedded in raw string literals
 * Features: File upload, download, directory listing, mobile-friendly UI
 * Security: Session management, file validation, size limits
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <FS.h>
#include "WebServerPatch.h"
#include <WebServer.h>
#include "config.h"
#include "shared_data.h"
#include "sd_manager.h"

// ========================================
// WEB SERVER CONFIGURATION
// ========================================

// Server Settings
#define WEB_SERVER_PORT         80
#define MAX_UPLOAD_SIZE         10485760    // 10MB max file size
#define MAX_FILENAME_LENGTH     64
#define SESSION_TIMEOUT_MS      1800000     // 30 minutes
#define MAX_SESSIONS            8           // Maximum concurrent sessions

// Supported File Types
#define ALLOWED_EXTENSIONS      ".gpx,.kml,.json,.txt"
#define UPLOAD_TEMP_DIR         "/temp"
#define DOWNLOAD_BUFFER_SIZE    2048

// HTTP Response Codes
#define HTTP_OK                 200
#define HTTP_CREATED            201
#define HTTP_BAD_REQUEST        400
#define HTTP_NOT_FOUND          404
#define HTTP_REQUEST_TOO_LARGE  413
#define HTTP_INTERNAL_ERROR     500

// ========================================
// FILE OPERATION RESULTS
// ========================================
enum FileOperationResult {
  FILE_OP_SUCCESS,
  FILE_OP_NOT_FOUND,
  FILE_OP_TOO_LARGE,
  FILE_OP_INVALID_TYPE,
  FILE_OP_DISK_FULL,
  FILE_OP_PERMISSION_DENIED,
  FILE_OP_CORRUPTED,
  FILE_OP_NETWORK_ERROR
};

// ========================================
// WEB SESSION MANAGEMENT
// ========================================
struct WebSession {
  String sessionId;
  String clientIP;
  unsigned long startTime;
  unsigned long lastActivity;
  int filesUploaded;
  int filesDownloaded;
  unsigned long bytesTransferred;
  bool authenticated;
};

// ========================================
// FILE TRANSFER STATUS
// ========================================
struct FileTransferStatus {
  bool transferActive = false;
  bool isUpload = true;                    // true=upload, false=download
  String filename = "";
  unsigned long totalBytes = 0;
  unsigned long transferredBytes = 0;
  unsigned long startTime = 0;
  float progressPercent = 0.0;
  String clientIP = "";
  FileOperationResult lastResult = FILE_OP_SUCCESS;
};

// ========================================
// WEB SERVER STATISTICS
// ========================================
struct WebServerStats {
  unsigned long totalRequests = 0;
  unsigned long successfulRequests = 0;
  unsigned long failedRequests = 0;
  unsigned long bytesTransferred = 0;
  unsigned long uptime = 0;
  unsigned long activeSessions = 0;
  unsigned long peakSessions = 0;
  String lastError = "";
  unsigned long lastErrorTime = 0;
};

// ========================================
// WEB FILE SERVER CLASS
// ========================================
class WebFileServer {
  private:
    // Core Server Components
    WebServer* server = nullptr;
    bool serverRunning = false;
    unsigned long serverStartTime = 0;

    bool initialize();  // ADD THIS
    FileTransferStatus getTransferStatus() const;  // ADD THIS
    WebServerStats getServerStats() const;  // ADD THIS (return type fix)
    
    // Session Management
    WebSession sessions[MAX_SESSIONS];
    int sessionCount = 0;
    unsigned long sessionTimeout = SESSION_TIMEOUT_MS;
    
    // Transfer Management
    FileTransferStatus currentTransfer;
    String uploadTempPath;
    unsigned long maxUploadSize = MAX_UPLOAD_SIZE;
    
    // Statistics
    unsigned long totalRequests = 0;
    unsigned long successfulRequests = 0;
    unsigned long failedRequests = 0;
    unsigned long bytesTransferred = 0;
    
    // Route Handlers
    void handleRoot();
    void handleFileList();
    void handleFileUpload();
    void handleFileUploadFinish();
    void handleFileDownload();
    void handleFileDelete();
    void handleDownloadAll();
    void handleDeleteSelected();
    void handleSystemStatus();
    void handleHeartbeat();
    void handleNotFound();
    
    // Helper Functions
    void setupRoutes();
    bool validateFilename(const String& filename);
    bool isAllowedFileType(const String& filename);
    String getContentType(const String& filename);
    void sendJSONResponse(int code, const String& message);
    void sendJSONError(int code, const String& error);
    
    // Session Management
    String createSession(const String& clientIP);
    bool validateSession(const String& sessionId);
    void updateSessionActivity(const String& sessionId);
    void cleanupExpiredSessions();
    void terminateSession(const String& sessionId);
    
  public:
    WebFileServer();
    ~WebFileServer();
    
    // Server Control
    bool start();
    bool stop();
    void handleClient();
    bool restart();
    
    // Status and Information
    bool isRunning() const;
    int getActiveConnections() const;
    String getServerURL() const;
    unsigned long getUptime() const;
    String getServerInfo();
    
    // Transfer Management
    FileTransferStatus getCurrentTransfer() const { return currentTransfer; }
    bool isTransferActive() const { return currentTransfer.transferActive; }
    void cancelCurrentTransfer();
    
    // Session Management
    int getActiveSessionCount() const { return sessionCount; }
    void terminateAllSessions();
    
    // Statistics
    unsigned long getTotalRequests() const { return totalRequests; }
    unsigned long getSuccessfulRequests() const { return successfulRequests; }
    float getSuccessRate() const;
    String getServerStats();
    WebServerStats getServerStatistics() const;
    
    // Configuration
    void setUploadDirectory(const String& path);
    void setMaxUploadSize(unsigned long bytes);
    void setSessionTimeout(unsigned long ms);
};

// ========================================
// WEB INTERFACE CONTENT
// ========================================
class WebInterface {
  private:
    // HTML Templates
    String htmlHeader;
    String htmlFooter;
    String cssStyles;
    String jsScripts;
    
    // Page Content
    String generateMainPage();
    String generateFileListPage();
    String generateUploadPage();
    String generateStatusPage();
    
    // Mobile-Optimized HTML
    String generateMobileCSS();
    String generateMobileJS();
    
  public:
    WebInterface();
    ~WebInterface();
    
    // Page Generation
    String getMainPage();
    String getFileListJSON();
    String getStatusJSON();
    String getUploadForm();
    
    // Template Management
    void loadTemplates();
    String processTemplate(const String& templateStr, const String& data);
    
    // Mobile Optimization
    String getMobileOptimizedPage();
    bool isMobileClient(const String& userAgent);
};

// ========================================
// GPX FILE VALIDATOR
// ========================================
class GPXValidator {
  private:
    bool validateXMLStructure(const String& content);
    bool validateGPXElements(const String& content);
    bool validateCoordinates(const String& content);
    
  public:
    GPXValidator();
    ~GPXValidator();
    
    // Validation Methods
    bool isValidGPX(const String& filename);
    bool validateGPXContent(const String& content);
    bool validateGPXFile(const String& filepath);
    
    // File Analysis
    int getTrackPointCount(const String& content);
    int getWaypointCount(const String& content);
    String getGPXSummary(const String& content);
    
    // Repair & Cleanup
    String repairGPXFile(const String& content);
    String cleanupGPXFile(const String& content);
    bool canRepairFile(const String& content);
};

// ========================================
// GLOBAL INSTANCES
// ========================================
extern WebFileServer webServer;
extern WebInterface webInterface;
extern GPXValidator gpxValidator;

// ========================================
// GLOBAL ACCESS FUNCTIONS
// ========================================
bool startWebServer();
bool stopWebServer();
void updateWebServer();
bool isWebServerRunning();
String getWebServerURL();
FileTransferStatus getCurrentFileTransfer();
WebServerStats getWebServerStats();

// ========================================
// EMBEDDED WEB PAGE - PROPERLY ESCAPED
// ========================================

// FIXED: All HTML/CSS/JavaScript properly contained in raw string literal
const char MAIN_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>E_NAVI File Transfer</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: #1a1a1a; 
            color: #ffffff;
        }
        .container { 
            max-width: 600px; 
            margin: 0 auto; 
            background: #2d2d2d; 
            padding: 20px; 
            border-radius: 10px;
        }
        .header { 
            text-align: center; 
            margin-bottom: 30px; 
            border-bottom: 2px solid #00ff00;
            padding-bottom: 20px;
        }
        .logo { 
            font-size: 32px; 
            font-weight: bold; 
            color: #00ff00;
            margin-bottom: 10px;
        }
        .tagline { 
            font-size: 16px; 
            color: #cccccc;
        }
        .section { 
            margin: 20px 0; 
            padding: 15px; 
            background: #3d3d3d; 
            border-radius: 8px;
        }
        .button { 
            display: block; 
            width: 100%; 
            padding: 15px; 
            margin: 10px 0; 
            background: #00ff00; 
            color: #000000; 
            text-decoration: none; 
            text-align: center; 
            border-radius: 5px; 
            font-weight: bold;
            font-size: 18px;
            border: none;
            cursor: pointer;
        }
        .button:hover { 
            background: #00cc00; 
        }
        .button.danger {
            background: #ff4444;
            color: #ffffff;
        }
        .button.danger:hover {
            background: #cc3333;
        }
        .info { 
            background: #444444; 
            padding: 10px; 
            border-radius: 5px; 
            margin: 10px 0;
            font-size: 14px;
        }
        .file-list {
            background: #333333;
            border-radius: 5px;
            padding: 10px;
            margin: 10px 0;
            max-height: 300px;
            overflow-y: auto;
        }
        .file-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 8px;
            margin: 5px 0;
            background: #444444;
            border-radius: 3px;
        }
        .file-name {
            flex-grow: 1;
            margin-right: 10px;
        }
        .file-actions {
            display: flex;
            gap: 5px;
        }
        .file-btn {
            padding: 5px 10px;
            background: #00ff00;
            color: #000000;
            border: none;
            border-radius: 3px;
            cursor: pointer;
            font-size: 12px;
        }
        .file-btn:hover {
            background: #00cc00;
        }
        .file-btn.delete {
            background: #ff4444;
            color: #ffffff;
        }
        .file-btn.delete:hover {
            background: #cc3333;
        }
        .upload-area {
            border: 2px dashed #666666;
            padding: 20px;
            text-align: center;
            border-radius: 5px;
            cursor: pointer;
            transition: border-color 0.3s;
        }
        .upload-area:hover {
            border-color: #00ff00;
        }
        .upload-area.dragover {
            border-color: #00ff00;
            background: #333333;
        }
        .status {
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            text-align: center;
        }
        .status.connected {
            background: #004400;
            color: #00ff00;
        }
        .status.error {
            background: #440000;
            color: #ff4444;
        }
        .progress {
            width: 100%;
            height: 20px;
            background: #333333;
            border-radius: 10px;
            overflow: hidden;
            margin: 10px 0;
        }
        .progress-bar {
            height: 100%;
            background: linear-gradient(90deg, #00ff00, #00cc00);
            width: 0%;
            transition: width 0.3s;
        }
        #fileInput {
            display: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">E_NAVI GPS</div>
            <div class="tagline">Lost, Never Again</div>
        </div>
        
        <div class="section">
            <h3>System Status</h3>
            <div id="statusDisplay" class="status connected">Connected - Ready for file transfer</div>
            <div class="info">
                <strong>IP Address:</strong> <span id="ipAddress">192.168.4.1</span><br>
                <strong>Files:</strong> <span id="fileCount">Loading...</span><br>
                <strong>Storage:</strong> <span id="storageInfo">Loading...</span>
            </div>
        </div>
        
        <div class="section">
            <h3>Upload Files</h3>
            <div id="uploadArea" class="upload-area">
                <p>📁 Click here or drag GPX files to upload</p>
                <p style="font-size: 14px; color: #cccccc;">Supported: .gpx, .kml, .json, .txt</p>
            </div>
            <input type="file" id="fileInput" multiple accept=".gpx,.kml,.json,.txt">
            <div id="uploadProgress" style="display: none;">
                <div class="progress">
                    <div id="progressBar" class="progress-bar"></div>
                </div>
                <div id="uploadStatus">Uploading...</div>
            </div>
        </div>
        
        <div class="section">
            <h3>Available Files</h3>
            <button class="button" onclick="refreshFileList()">🔄 Refresh List</button>
            <div id="fileList" class="file-list">
                <div style="text-align: center; color: #cccccc; padding: 20px;">
                    Loading files...
                </div>
            </div>
            <button class="button danger" onclick="deleteAllFiles()" id="deleteAllBtn" style="display: none;">
                🗑️ Delete All Files
            </button>
        </div>
        
        <div class="section">
            <h3>Batch Operations</h3>
            <button class="button" onclick="downloadAllFiles()">📥 Download All Files</button>
            <button class="button" onclick="window.location.reload()">🔄 Refresh Page</button>
        </div>
    </div>

    <script>
        // Fixed JavaScript - properly contained in raw string literal
        let fileListData = [];
        
        window.onload = function() {
            updateStatus();
            refreshFileList();
            setupEventListeners();
        };
        
        function setupEventListeners() {
            document.getElementById('uploadArea').onclick = function() {
                document.getElementById('fileInput').click();
            };
            
            document.getElementById('fileInput').onchange = function(e) {
                uploadFiles(e.target.files);
            };
            
            document.getElementById('uploadArea').ondragover = function(e) {
                e.preventDefault();
                this.classList.add('dragover');
            };
            
            document.getElementById('uploadArea').ondragleave = function(e) {
                e.preventDefault();
                this.classList.remove('dragover');
            };
            
            document.getElementById('uploadArea').ondrop = function(e) {
                e.preventDefault();
                this.classList.remove('dragover');
                uploadFiles(e.dataTransfer.files);
            };
        }
        
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('ipAddress').textContent = data.ip || '192.168.4.1';
                    document.getElementById('fileCount').textContent = data.fileCount || '0';
                    document.getElementById('storageInfo').textContent = data.storage || 'Unknown';
                })
                .catch(error => {
                    document.getElementById('statusDisplay').className = 'status error';
                    document.getElementById('statusDisplay').textContent = 'Connection Error';
                });
        }
        
        function refreshFileList() {
            fetch('/files')
                .then(response => response.json())
                .then(data => {
                    fileListData = data.files || [];
                    updateFileListDisplay();
                })
                .catch(error => {
                    document.getElementById('fileList').innerHTML = 
                        '<div style="text-align: center; color: #ff4444; padding: 20px;">Error loading files</div>';
                });
        }
        
        function updateFileListDisplay() {
            const fileList = document.getElementById('fileList');
            const deleteAllBtn = document.getElementById('deleteAllBtn');
            
            if (fileListData.length === 0) {
                fileList.innerHTML = '<div style="text-align: center; color: #cccccc; padding: 20px;">No files found</div>';
                deleteAllBtn.style.display = 'none';
                return;
            }
            
            let html = '';
            fileListData.forEach(file => {
                html += `
                    <div class="file-item">
                        <div class="file-name">${file.name}</div>
                        <div class="file-actions">
                            <button class="file-btn" onclick="downloadFile('${file.name}')">📥</button>
                            <button class="file-btn delete" onclick="deleteFile('${file.name}')">🗑️</button>
                        </div>
                    </div>
                `;
            });
            
            fileList.innerHTML = html;
            deleteAllBtn.style.display = 'block';
        }
        
        function uploadFiles(files) {
            if (files.length === 0) return;
            
            const uploadProgress = document.getElementById('uploadProgress');
            const progressBar = document.getElementById('progressBar');
            const uploadStatus = document.getElementById('uploadStatus');
            
            uploadProgress.style.display = 'block';
            
            Array.from(files).forEach((file, index) => {
                uploadFile(file, index, files.length);
            });
        }
        
        function uploadFile(file) {
            const formData = new FormData();
            formData.append('file', file);
            
            const progressBar = document.getElementById('progressBar');
            const uploadStatus = document.getElementById('uploadStatus');
            
            uploadStatus.textContent = `Uploading ${file.name}...`;
            
            fetch('/upload', {
                method: 'POST',
                body: formData
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    uploadStatus.textContent = `✅ Uploaded ${file.name}`;
                    setTimeout(() => {
                        document.getElementById('uploadProgress').style.display = 'none';
                        refreshFileList();
                    }, 2000);
                } else {
                    uploadStatus.textContent = `❌ Failed to upload ${file.name}`;
                }
            })
            .catch(error => {
                uploadStatus.textContent = `❌ Upload error: ${error.message}`;
            });
        }
        
        function downloadFile(filename) {
            window.open(`/download/${encodeURIComponent(filename)}`, '_blank');
        }
        
        function deleteFile(filename) {
            if (!confirm(`Delete ${filename}?`)) return;
            
            fetch(`/delete/${encodeURIComponent(filename)}`, {
                method: 'DELETE'
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    refreshFileList();
                } else {
                    alert(`Failed to delete ${filename}`);
                }
            })
            .catch(error => {
                alert(`Error deleting file: ${error.message}`);
            });
        }
        
        function downloadAllFiles() {
            window.open('/download-all', '_blank');
        }
        
        function deleteAllFiles() {
            if (!confirm('Delete ALL files? This cannot be undone!')) return;
            
            const deletePromises = fileListData.map(file => 
                fetch(`/delete/${encodeURIComponent(file.name)}`, { method: 'DELETE' })
            );
            
            Promise.all(deletePromises)
                .then(() => {
                    refreshFileList();
                    alert('All files deleted');
                })
                .catch(error => {
                    alert(`Error deleting files: ${error.message}`);
                });
        }
        
        // Auto-refresh every 30 seconds
        setInterval(updateStatus, 30000);
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_SERVER_H