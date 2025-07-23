/*
 * E_NAVI EnduroGPS - WiFi Manager
 * On-Demand WiFi Hotspot for GPX File Transfer
 * 
 * Features: Hotspot creation, QR code display, connection management
 * Security: WPA2 password protection, session timeouts
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "shared_data.h"
#include <DNSServer.h>
#include "config.h"

// ========================================
// WIFI CONFIGURATION
// ========================================

// WiFi Hotspot Settings
#define WIFI_SSID_PREFIX    "E_NAVI_"
#define WIFI_PASSWORD       "enavi2025"
#define WIFI_CHANNEL        6
#define WIFI_MAX_CLIENTS    4
#define WIFI_HIDDEN         false

// Connection Management
#define WIFI_TIMEOUT_MS     300000      // 5 minutes max connection time
#define WIFI_STATE_OFF_TIMEOUT   120000      // 2 minutes idle timeout
#define WIFI_CHECK_INTERVAL 5000        // Check clients every 5 seconds

// Network Configuration
#define WIFI_LOCAL_IP       IPAddress(192, 168, 4, 1)
#define WIFI_GATEWAY        IPAddress(192, 168, 4, 1)
#define WIFI_SUBNET         IPAddress(255, 255, 255, 0)

// DNS Configuration
#define DNS_PORT            53

// ========================================
// WIFI STATE MANAGEMENT
// ========================================

enum WiFiConnectionEvent {
  WIFI_EVENT_CLIENT_CONNECT,
  WIFI_EVENT_CLIENT_DISCONNECT,
  WIFI_EVENT_FILE_UPLOAD,
  WIFI_EVENT_FILE_DOWNLOAD,
  WIFI_EVENT_TIMEOUT,
  WIFI_EVENT_ERROR
};

// ========================================
// WIFI STATISTICS
// ========================================
struct WiFiStats {
  unsigned long totalConnections = 0;     // Total client connections
  unsigned long filesUploaded = 0;        // Files uploaded
  unsigned long filesDownloaded = 0;      // Files downloaded
  unsigned long bytesUploaded = 0;        // Total bytes uploaded
  unsigned long bytesDownloaded = 0;      // Total bytes downloaded
  unsigned long totalSessionTime = 0;     // Total connection time
  unsigned long lastConnectionTime = 0;   // Last client connection
  String lastClientIP = "";               // Last connected client IP
  int maxClientsConnected = 0;            // Peak concurrent clients
};

// ========================================
// QR CODE GENERATOR
// ========================================
class QRCodeGenerator {
  private:
    static const int QR_SIZE = 120;       // QR code size in pixels
    bool qrGenerated = false;
    uint8_t qrData[QR_SIZE * QR_SIZE / 8]; // QR code bitmap data
    
  public:
    QRCodeGenerator();
    ~QRCodeGenerator();
    
    // QR Code Generation
    bool generateWiFiQR(const String& ssid, const String& password, 
                       const String& security = "WPA");
    bool generateURLQR(const String& url);
    
    // QR Code Display
    void drawQRCode(int x, int y, int size = QR_SIZE);
    void drawQRCodeWithInfo(int x, int y, const String& ssid, 
                           const String& password, const String& url);
    
    // QR Code Management
    bool isGenerated() const { return qrGenerated; }
    void clearQRCode();
    
    // Utility
    String formatWiFiQRString(const String& ssid, const String& password, 
                             const String& security = "WPA");
};

// ========================================
// WIFI MANAGER CLASS
// ========================================
class WiFiManager {
  private:
    // WiFi State
    WiFiState currentState = WIFI_STATE_OFF;
    bool hotspotActive = false;
    bool dnsServerActive = false;
    
    // Network Configuration
    String hotspotSSID = "";
    String hotspotPassword = WIFI_PASSWORD;
    IPAddress localIP = WIFI_LOCAL_IP;
    IPAddress gateway = WIFI_GATEWAY;
    IPAddress subnet = WIFI_SUBNET;
    
    // DNS Server for Captive Portal
    DNSServer dnsServer;
    
    // Connection Management
    unsigned long connectionStartTime = 0;
    unsigned long lastActivityTime = 0;
    unsigned long lastClientCheck = 0;
    int connectedClients = 0;
    int maxClients = WIFI_MAX_CLIENTS;
    
    // Statistics
    WiFiStats statistics;
    
    // QR Code System
    QRCodeGenerator qrGenerator;
    bool qrCodeVisible = false;
    
    // File Transfer State
    bool fileTransferActive = false;
    String currentTransferFile = "";
    unsigned long transferStartTime = 0;
    
    // Internal Methods
    bool initializeHotspot();
    bool startDNSServer();
    void stopDNSServer();
    void updateClientCount();
    void checkTimeouts();
    void updateStatistics();
    String generateSSID();
    
  public:
    WiFiManager();
    ~WiFiManager();

    bool initialize();                      // ADD THIS LINE ← NEW
    
    // WiFi Control
    bool startHotspot();
    bool stopHotspot();
    bool isActive() const { return hotspotActive; }
    WiFiState getState() const { return currentState; }
    
    // Connection Management
    void update();                          // Call regularly in main loop
    int getConnectedClients() const { return connectedClients; }
    bool hasClients() const { return connectedClients > 0; }
    String getClientIP(int index = 0);
    
    // Network Information
    String getSSID() const { return hotspotSSID; }
    String getPassword() const { return hotspotPassword; }
    String getLocalIP() const { return localIP.toString(); }
    String getURL() const { return "http://" + localIP.toString(); }
    
    // QR Code Management
    bool showQRCode();                      // Generate and display QR code
    void hideQRCode();                      // Hide QR code
    bool isQRCodeVisible() const { return qrCodeVisible; }
    void updateQRDisplay();                 // Update QR code display
    
    // File Transfer Integration
    void notifyFileUpload(const String& filename, unsigned long size);
    void notifyFileDownload(const String& filename, unsigned long size);
    void setFileTransferActive(bool active);
    bool isFileTransferActive() const { return fileTransferActive; }
    
    // Statistics
    WiFiStats getStatistics() const { return statistics; }
    void resetStatistics();
    String getConnectionSummary();
    
    // Configuration
    void setPassword(const String& password);
    void setMaxClients(int max);
    void setTimeout(unsigned long timeoutMs);
    
    // Status Display
    void drawConnectionScreen();            // Show connection info
    void drawTransferStatus();              // Show file transfer status
    void drawStatistics();                  // Show connection statistics
    
    // Error Handling
    bool hasError() const { return currentState == WIFI_STATE_ERROR; }
    String getLastError();
    void clearError();
};

// ========================================
// CAPTIVE PORTAL HANDLER
// ========================================
class CaptivePortal {
  private:
    bool portalActive = false;
    String redirectURL = "";
    unsigned long portalStartTime = 0;
    
  public:
    CaptivePortal();
    ~CaptivePortal();
    
    // Portal Control
    bool startPortal(const String& redirectTo);
    void stopPortal();
    bool isActive() const { return portalActive; }
    
    // DNS Handling
    bool handleDNSRequest();
    String getRedirectResponse();
    
    // Portal Management
    void update();
    unsigned long getPortalUptime();
};

// ========================================
// WIFI DISPLAY SYSTEM
// ========================================
class WiFiDisplayManager {
  private:
    WiFiManager* wifiManager;
    bool displayActive = false;
    int currentScreen = 0;               // 0=QR, 1=Status, 2=Stats
    unsigned long lastScreenUpdate = 0;
    unsigned long screenSwitchTime = 10000; // Switch screens every 10s
    
    // Display Elements
    void drawQRScreen();
    void drawStatusScreen();
    void drawStatsScreen();
    void drawHeader();
    void drawFooter();
    void drawClientList();
    void drawTransferProgress();
    
  public:
    WiFiDisplayManager(WiFiManager* manager);
    ~WiFiDisplayManager();
    
    // Display Control
    void startDisplay();
    void stopDisplay();
    void update();                       // Call in main loop
    bool isActive() const { return displayActive; }
    
    // Screen Management
    void nextScreen();
    void setScreen(int screen);
    int getCurrentScreen() const { return currentScreen; }
    
    // User Interaction
    void handleInput(ButtonID button, ButtonEvent event);
};

// ========================================
// GLOBAL INSTANCES
// ========================================
extern WiFiManager wifiManager;
extern CaptivePortal captivePortal;
extern WiFiDisplayManager wifiDisplay;

// ========================================
// UTILITY FUNCTIONS
// ========================================

// Network Utilities
String formatIPAddress(const IPAddress& ip);
String formatMACAddress(uint8_t* mac);
bool isValidIPAddress(const String& ip);
String getDeviceHostname();

// QR Code Utilities
String encodeWiFiQR(const String& ssid, const String& password, 
                   const String& security = "WPA", bool hidden = false);
String encodeURLQR(const String& url);

// Connection Helpers
String getConnectionQualityText(int rssi);
String formatDataSize(unsigned long bytes);
String formatDuration(unsigned long milliseconds);

// Display Helpers
void drawWiFiIcon(int x, int y, int size, bool connected = false);
void drawSignalStrength(int x, int y, int strength); // 0-4 bars
void drawTransferIcon(int x, int y, bool uploading = true);

// ========================================
// WIFI CONFIGURATION CONSTANTS
// ========================================

// Default Settings
#define DEFAULT_WIFI_TIMEOUT    300000   // 5 minutes
#define DEFAULT_IDLE_TIMEOUT    120000   // 2 minutes  
#define DEFAULT_MAX_CLIENTS     4        // Maximum concurrent clients
#define DEFAULT_CHANNEL         6        // WiFi channel

// Security Settings
#define MIN_PASSWORD_LENGTH     8        // Minimum password length
#define MAX_SSID_LENGTH         32       // Maximum SSID length
#define SESSION_TIMEOUT         1800000  // 30 minutes max session

// Performance Settings
#define WIFI_CHECK_INTERVAL     5000     // Check clients every 5s
#define DNS_RESPONSE_TIMEOUT    1000     // DNS response timeout
#define CAPTIVE_PORTAL_TIMEOUT  300000   // 5 minutes portal timeout

// Display Settings
#define QR_CODE_SIZE            120      // QR code display size
#define SCREEN_SWITCH_TIME      10000    // 10 seconds per screen
#define STATUS_UPDATE_INTERVAL  2000     // Update status every 2s

#endif // WIFI_MANAGER_H 
