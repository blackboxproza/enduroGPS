/*
 * E_NAVI EnduroGPS V2 - SD Card Manager
 * Professional SD Card Management for GPS Data Storage
 * 
 * FINAL FIXED VERSION - All conflicts resolved
 * FIXED: Function overload conflict, renamed conflicting methods
 * Features: File operations, GPX handling, directory management,
 *          backup system, filesystem monitoring, error recovery
 */

#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPI.h>
#include "config.h"

// Forward declarations to avoid circular dependency
struct TrackPoint;
struct Waypoint;

#include "shared_data.h"

// ========================================
// SD CARD OPERATION RESULTS
// ========================================

enum FileResult {
  FILE_SUCCESS,
  FILE_ERROR_NOT_FOUND,
  FILE_ERROR_ACCESS_DENIED,
  FILE_ERROR_DISK_FULL,
  FILE_ERROR_WRITE_PROTECTED,
  FILE_ERROR_CORRUPTED,
  FILE_ERROR_TIMEOUT,
  FILE_ERROR_INVALID_PARAMETER,
  FILE_ERROR_HARDWARE_FAILURE,
  FILE_ERROR_FILESYSTEM_ERROR
};

// ========================================
// SD CARD INFORMATION STRUCTURES
// ========================================

struct SDCardInfo {
  bool cardPresent = false;
  bool cardInitialized = false;
  uint64_t cardSize = 0;                // Total card size (bytes)
  uint64_t usedSpace = 0;               // Used space (bytes)
  uint64_t freeSpace = 0;               // Free space (bytes)
  String cardType = "Unknown";          // SD, SDHC, SDXC
  uint32_t cardSpeed = 0;               // Maximum speed class
  String filesystemType = "FAT32";      // Filesystem type
  bool isHighSpeed = false;             // High speed card
  bool hasErrors = false;               // Card has errors
  unsigned long lastCheck = 0;          // Last status check time
};

struct DirectoryInfo {
  String path = "";
  bool exists = false;
  bool isDirectory = false;
  bool isWritable = false;
  int fileCount = 0;
  unsigned long totalSize = 0;          // Total size of all files (bytes)
  unsigned long lastModified = 0;       // Last modification time
  bool hasSubdirectories = false;
  bool hasHiddenFiles = false;
};

// ========================================
// PERFORMANCE MONITORING
// ========================================

struct SDPerformance {
  // Operation Counters
  unsigned long totalReads = 0;
  unsigned long totalWrites = 0;
  unsigned long totalErrors = 0;
  unsigned long totalRetries = 0;
  
  // Timing Statistics
  unsigned long averageReadTime = 0;    // Microseconds
  unsigned long averageWriteTime = 0;   // Microseconds
  unsigned long maxReadTime = 0;        // Microseconds
  unsigned long maxWriteTime = 0;       // Microseconds
  
  // Performance Metrics
  float readSpeedKBps = 0.0;            // Read speed (KB/s)
  float writeSpeedKBps = 0.0;           // Write speed (KB/s)
  float errorRate = 0.0;                // Error rate (%)
  bool isHealthy = true;                // Overall health status
  
  // System Health
  unsigned long uptime = 0;             // SD manager uptime (ms)
  unsigned long lastErrorTime = 0;      // Last error timestamp
  String lastErrorDescription = "";     // Last error message
};

// ========================================
// MAIN SD MANAGER CLASS
// ========================================

class SDManager {
  private:
    // Hardware Configuration
    int chipSelectPin = SD_CS;
    uint32_t spiSpeed = SD_CARD_SPEED;
    bool initialized = false;
    bool cardPresent = false;
    
    // Card Information
    SDCardInfo cardInfo;
    SDPerformance performance;
    
    // Error Handling
    int maxRetries = 3;
    unsigned long retryDelay = 100;      // ms
    bool autoRecovery = true;
    String lastError = "";
    unsigned long lastErrorTime = 0;
    
    // Performance Tracking
    unsigned long startTime = 0;
    bool performanceMonitoring = true;
    
    // Internal Methods
    bool detectCard();
    bool initializeCard();
    void updateCardInfo();
    void updatePerformanceStats(unsigned long operationTime, bool isWrite, bool success);
    bool retryOperation(std::function<bool()> operation);
    void reportError(const String& error);
    void logOperation(const String& operation, bool success, unsigned long duration);
    
  public:
    SDManager();
    ~SDManager();
    
    // ========================================
    // INITIALIZATION & SETUP
    // ========================================
    bool initialize();
    bool shutdown();
    bool reset();
    bool isInitialized() const { return initialized; }
    bool isReady() const { return initialized && cardPresent; }  // ADDED: For compatibility
    
    // Hardware Configuration
    void setChipSelectPin(int pin);
    void setSPISpeed(uint32_t speed);
    bool testHardwareConnection();
    bool formatCard(const String& label = "E_NAVI_GPS");
    
    // ========================================
    // CARD STATUS & INFORMATION
    // ========================================
    
    // Basic Status
    bool isCardPresent();
    bool isCardMounted();
    bool isCardWritable();
    bool isCardHealthy();
    
    // Card Information - FIXED: No more function overload conflict
    SDCardInfo getCardInfo();                    // Returns structured data
    String getCardInfoString();                  // RENAMED: Returns formatted string
    String getCardSummary();                     // ADDED: Alternative string method
    
    // Space Information
    uint64_t getTotalSpace();                    // Total space (bytes)
    uint64_t getFreeSpace();                     // Free space (bytes) 
    uint64_t getUsedSpace();                     // Used space (bytes)
    uint64_t getCardSize() const { return cardInfo.cardSize; }  // ADDED: For compatibility
    float getUsagePercent();                     // Usage percentage
    String getFilesystemType() const { return cardInfo.filesystemType; }
    
    // ========================================
    // DIRECTORY OPERATIONS
    // ========================================
    bool createDirectory(const String& path);
    bool removeDirectory(const String& path, bool recursive = false);
    bool directoryExists(const String& path);
    DirectoryInfo getDirectoryInfo(const String& path);
    int getFileCount(const String& path);
    unsigned long getDirectorySize(const String& path);
    bool listDirectory(const String& path, String* files, int maxFiles, int& count);
    
    // ========================================
    // FILE OPERATIONS
    // ========================================
    
    // Basic File Operations
    FileResult createFile(const String& filename);
    FileResult deleteFile(const String& filename);
    FileResult copyFile(const String& source, const String& destination);
    FileResult moveFile(const String& source, const String& destination);
    FileResult renameFile(const String& oldName, const String& newName);
    bool fileExists(const String& filename);
    unsigned long getFileSize(const String& filename);
    String getFileDate(const String& filename);
    
    // File Reading
    File openFileRead(const String& filename);
    String readFile(const String& filename);
    bool readFile(const String& filename, uint8_t* buffer, size_t maxSize, size_t& bytesRead);
    String readLine(File& file);
    bool readBinaryFile(const String& filename, uint8_t* buffer, size_t size);
    
    // File Writing
    File openFileWrite(const String& filename, bool append = false);
    FileResult writeFile(const String& filename, const String& content, bool append = false);
    FileResult writeFile(const String& filename, const uint8_t* data, size_t size, bool append = false);
    FileResult appendLine(const String& filename, const String& line);
    bool writeBinaryFile(const String& filename, const uint8_t* data, size_t size);
    bool syncFile(File& file);                   // Force write to disk
    
    // ========================================
    // GPX FILE OPERATIONS
    // ========================================
    
    // GPX File Reading
    bool isValidGPXFile(const String& filename);
    int getGPXTrackCount(const String& filename);
    int getGPXWaypointCount(const String& filename);
    String getGPXMetadata(const String& filename, const String& key);
    bool extractGPXTrackPoints(const String& filename, TrackPoint* points, int maxPoints, int& count);
    bool extractGPXWaypoints(const String& filename, Waypoint* waypoints, int maxWaypoints, int& count);
    
    // GPX File Writing
    bool createGPXFile(const String& filename, const String& trackName);
    bool writeGPXHeader(File& file, const String& trackName);
    bool writeGPXTrackPoint(File& file, const TrackPoint& point);
    bool writeGPXWaypoint(File& file, const Waypoint& waypoint);
    bool writeGPXFooter(File& file);
    bool finalizeGPXFile(const String& filename);
    
    // GPX Utilities
    bool mergeGPXFiles(const String* sourceFiles, int count, const String& outputFile);
    bool splitGPXFile(const String& sourceFile, unsigned long splitTime);
    bool validateGPXStructure(const String& filename);
    String getGPXSummary(const String& filename);
    bool optimizeGPXFile(const String& filename);    // Remove redundant points
    
    // ========================================
    // BATCH OPERATIONS
    // ========================================
    bool copyDirectory(const String& source, const String& destination);
    int deleteOldFiles(const String& directory, unsigned long maxAge); // Delete files older than maxAge (ms)
    bool backupDirectoryContents(const String& source, const String& backup); // RENAMED: Was backupDirectoryTo
    FileResult compressFile(const String& filename);        // Simple compression
    FileResult decompressFile(const String& filename);
    
    // ========================================
    // FILE VALIDATION & INTEGRITY
    // ========================================
    bool validateFile(const String& filename);              // Basic file integrity
    bool validateGPXFile(const String& filename);           // GPX format validation
    bool validateImageFile(const String& filename);         // Image format validation
    String calculateChecksum(const String& filename);       // MD5 checksum
    bool verifyChecksum(const String& filename, const String& expectedChecksum);
    
    // ========================================
    // PERFORMANCE & DIAGNOSTICS
    // ========================================
    SDPerformance getPerformance() const { return performance; }
    bool testSpeed();                            // Benchmark read/write speed
    bool testReliability();                      // Test filesystem reliability
    void resetPerformanceStats();
    String getPerformanceReport();
    bool isPerformanceGood();
    String getDiagnosticInfo();
    void printDiagnostics();
    
    // ========================================
    // ERROR HANDLING & RECOVERY
    // ========================================
    void setRetryParameters(int maxRetries, unsigned long delayMs);
    void enableAutoRecovery(bool enable);
    bool hasRecentErrors();
    String getLastErrorDescription();
    bool repairFilesystem();                     // Attempt filesystem repair
    void clearErrors();
    FileResult getLastFileResult();
    
    // ========================================
    // MAINTENANCE OPERATIONS
    // ========================================
    bool defragment();                           // Defragment filesystem (if supported)
    bool cleanup();                              // Clean temporary files
    bool optimizeForPerformance();               // Optimize settings for speed
    bool optimizeForReliability();               // Optimize settings for reliability
    bool removeEmptyDirectories();               // Cleanup empty directories
    
    // ========================================
    // SYSTEM INTEGRATION
    // ========================================
    void notifyLowSpace(unsigned long thresholdBytes = 1048576);   // 1MB threshold
    bool hasSpaceFor(unsigned long bytes);       // Check if space available
    unsigned long getRecommendedCleanupSize();   // Bytes to free up
    void scheduleCleanup();                      // Schedule automatic cleanup
    bool enableWriteCache(bool enable);          // Enable/disable write caching
};

// ========================================
// DIRECTORY STRUCTURE MANAGEMENT
// ========================================

class DirectoryManager {
  private:
    String baseDirectory = "/";
    bool autoCreate = true;
    
  public:
    // Standard Directory Creation
    bool createStandardDirectories();
    bool createTrackDirectory();
    bool createWaypointDirectory();
    bool createMapDirectory();
    bool createSettingsDirectory();
    bool createLogDirectory();
    bool createTempDirectory();
    bool createBackupDirectory();
    
    // Directory Validation
    bool validateDirectoryStructure();
    bool repairDirectoryStructure();
    void setAutoCreate(bool enable);
    
    // Directory Utilities
    String getTrackDirectory() const { return DIR_TRACKS; }
    String getWaypointDirectory() const { return DIR_WAYPOINTS; }
    String getMapDirectory() const { return DIR_MAPS; }
    String getSettingsDirectory() const { return DIR_SETTINGS; }
    String getLogDirectory() const { return DIR_LOGS; }
    String getTempDirectory() const { return "/temp"; }
    String getBackupDirectory() const { return "/backup"; }
    
    // Directory Status
    DirectoryInfo getDirectoryStatus(const String& path);
    bool isDirectoryHealthy(const String& path);
    unsigned long getTotalDirectorySize();
    void printDirectoryTree();
};

// ========================================
// BACKUP MANAGEMENT SYSTEM
// ========================================

class BackupManager {
  private:
    String backupBasePath = "/backup";   // RENAMED: Was backupDirectory
    int maxBackups = 10;
    bool compressBackups = true;
    bool verifyBackups = true;
    
  public:
    // Backup Operations
    bool backupFile(const String& filename);
    bool backupDirectoryContents(const String& directory);  // RENAMED: Consistent naming
    bool createFullBackup();
    bool createIncrementalBackup();
    
    // Restore Operations
    bool restoreFile(const String& filename);
    bool restoreDirectoryContents(const String& directory); // RENAMED: Consistent naming
    bool restoreFromBackup(const String& backupName);
    
    // Backup Management
    int getBackupCount();
    String* getBackupList(int& count);
    bool deleteOldBackups(int keepCount = 5);
    unsigned long getBackupSize(const String& backupName);
    String getBackupInfo(const String& backupName);
    
    // Backup Verification
    bool verifyBackup(const String& backupName);
    bool verifyAllBackups();
    String getBackupChecksum(const String& backupName);
    
    // Configuration
    void setMaxBackups(int count);
    void setCompressBackups(bool compress);
    void setVerifyBackups(bool verify);
    void setBackupBasePath(const String& path);     // RENAMED: From setBackupPath
    String getBackupBasePath() const { return backupBasePath; } // RENAMED: From getBackupPath
};

// ========================================
// FILE SYSTEM MONITORING
// ========================================

class FileSystemMonitor {
  private:
    unsigned long lastCheck = 0;
    unsigned long checkInterval = 60000;         // 1 minute
    bool monitoringEnabled = true;
    
    // Thresholds
    float lowSpaceThreshold = 10.0;              // 10% free space warning
    float criticalSpaceThreshold = 5.0;          // 5% free space critical
    unsigned long maxFileAge = 2592000000;       // 30 days
    int maxFileCount = 1000;                     // Maximum files per directory
    
    // Alerts
    bool lowSpaceAlerted = false;
    bool criticalSpaceAlerted = false;
    unsigned long lastAlert = 0;
    
  public:
    // Monitoring Control
    void enableMonitoring(bool enable);
    void setCheckInterval(unsigned long ms);
    void update();                               // Call periodically
    
    // Threshold Configuration
    void setLowSpaceThreshold(float percent);
    void setCriticalSpaceThreshold(float percent);
    void setMaxFileAge(unsigned long ms);
    void setMaxFileCount(int count);
    
    // Status Checks
    bool isLowSpace();
    bool isCriticalSpace();
    bool hasOldFiles();
    bool hasTooManyFiles();
    bool needsCleanup();
    
    // Automated Actions
    bool performAutomaticCleanup();
    bool archiveOldFiles();
    bool optimizeDirectoryStructure();
    void sendLowSpaceAlert();
    void sendCriticalSpaceAlert();
};

// ========================================
// FILE SYSTEM ERROR HANDLING
// ========================================

enum FSError {
  FS_ERROR_NONE,                                // No error
  FS_ERROR_NOT_INITIALIZED,                     // SD card not initialized
  FS_ERROR_CARD_NOT_PRESENT,                    // SD card not inserted
  FS_ERROR_MOUNT_FAILED,                        // Failed to mount filesystem
  FS_ERROR_WRITE_PROTECTED,                     // SD card is write protected
  FS_ERROR_DISK_FULL,                           // Insufficient space
  FS_ERROR_FILE_NOT_FOUND,                      // File does not exist
  FS_ERROR_ACCESS_DENIED,                       // Permission denied
  FS_ERROR_IO_ERROR,                            // Hardware I/O error
  FS_ERROR_CORRUPTION,                          // Filesystem corruption
  FS_ERROR_TIMEOUT,                             // Operation timeout
  FS_ERROR_INVALID_PARAMETER                    // Invalid function parameter
};

class FileSystemErrorHandler {
  private:
    FSError lastError = FS_ERROR_NONE;
    String lastErrorMessage = "";
    unsigned long errorTime = 0;
    int errorCount = 0;
    bool autoRecovery = true;
    
  public:
    // Error Reporting
    void reportError(FSError error, const String& message = "");
    FSError getLastError() const { return lastError; }
    String getLastErrorMessage() const { return lastErrorMessage; }
    bool hasError() const { return lastError != FS_ERROR_NONE; }
    int getErrorCount() const { return errorCount; }
    
    // Error Recovery
    bool attemptRecovery();
    void clearError();
    void enableAutoRecovery(bool enable);
    
    // Error Analysis
    String getErrorDescription(FSError error);
    bool isCriticalError(FSError error);
    bool isRecoverableError(FSError error);
    String getRecoveryRecommendation(FSError error);
};

// ========================================
// TRACK MANAGEMENT INTEGRATION
// ========================================

// TrackManager class removed - using full implementation from track_management.h

// ========================================
// GLOBAL INSTANCES
// ========================================
extern SDManager sdManager;
extern DirectoryManager directoryManager;
extern BackupManager backupManager;
extern FileSystemMonitor fsMonitor;
extern FileSystemErrorHandler fsErrorHandler;
extern TrackManager trackManager;               // ADDED: For compatibility

// ========================================
// GLOBAL ACCESS FUNCTIONS
// ========================================

// Initialization
bool initializeSDCard();
void updateSDCard();
bool shutdownSDCard();

// Quick Status Functions
bool isSDCardReady();
bool hasSDCardSpace(unsigned long bytes);
String getSDCardStatus();
float getSDCardUsage();

// Quick File Operations
bool writeTextFile(const String& filename, const String& content);
String readTextFile(const String& filename);
bool appendToFile(const String& filename, const String& content);
bool fileExistsSD(const String& filename);
unsigned long getFileSizeSD(const String& filename);

// Quick Directory Operations
bool createDirectorySD(const String& path);
bool listFilesSD(const String& path, String* files, int maxFiles, int& count);
int getFileCountSD(const String& path);

// GPX Helper Functions
bool saveGPXTrack(const String& filename, const TrackPoint* points, int count, const String& trackName);
bool loadGPXTrack(const String& filename, TrackPoint* points, int maxPoints, int& count);
bool saveGPXWaypoints(const String& filename, const Waypoint* waypoints, int count);
bool loadGPXWaypoints(const String& filename, Waypoint* waypoints, int maxWaypoints, int& count);

// ========================================
// CONVENIENCE FUNCTIONS
// ========================================

// Quick file operations (with error handling)
bool quickWriteFile(const String& filename, const String& content);
String quickReadFile(const String& filename);
bool quickCopyFile(const String& source, const String& destination);
bool quickDeleteFile(const String& filename);
bool quickFileExists(const String& filename);
unsigned long quickFileSize(const String& filename);

// Configuration file helpers
bool saveConfig(const String& filename, const String& key, const String& value);
String loadConfig(const String& filename, const String& key, const String& defaultValue = "");
bool configExists(const String& filename, const String& key);

// Log file helpers
bool appendLog(const String& filename, const String& message);
bool appendLogWithTimestamp(const String& filename, const String& message);
bool rotateLogFile(const String& filename, int maxSize = 1048576); // 1MB default

// File maintenance helpers
bool cleanupTempFiles();
bool archiveOldLogs();
bool validateAllGPXFiles();
void optimizeSDCardPerformance();

#endif // SD_MANAGER_H