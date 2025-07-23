/*
 * E_NAVI File Transfer Interface - Client-Side JavaScript
 * Professional GPS file transfer system for mobile devices
 * 
 * Features: Drag & drop, progress tracking, file validation, 
 *          real-time status updates, mobile optimization
 */

// ===================================================================
// GLOBAL CONFIGURATION
// ===================================================================

const CONFIG = {
    // File upload settings
    MAX_FILE_SIZE: 10 * 1024 * 1024, // 10MB
    ALLOWED_EXTENSIONS: ['.gpx', '.kml', '.json', '.txt'],
    UPLOAD_TIMEOUT: 30000, // 30 seconds
    
    // UI update intervals
    STATUS_UPDATE_INTERVAL: 5000,    // 5 seconds
    FILE_LIST_REFRESH: 30000,        // 30 seconds
    SESSION_TIMER_UPDATE: 1000,      // 1 second
    
    // API endpoints
    ENDPOINTS: {
        UPLOAD: '/upload',
        FILES: '/files',
        DOWNLOAD: '/download',
        DELETE: '/delete',
        STATUS: '/status'
    },
    
    // UI settings
    TOAST_DURATION: 4000,
    MODAL_FADE_DURATION: 300,
    PROGRESS_ANIMATION_SPEED: 200
};

// ===================================================================
// APPLICATION STATE
// ===================================================================

const AppState = {
    // Connection status
    isConnected: false,
    lastStatusUpdate: 0,
    
    // File operations
    activeUploads: new Map(),
    totalFilesUploaded: 0,
    totalBytesTransferred: 0,
    
    // UI state
    currentFiles: [],
    sessionStartTime: Date.now(),
    
    // Settings
    settings: {
        recordingMode: 'auto',
        trackQuality: 'high',
        autoSave: 300
    }
};

// ===================================================================
// UTILITY FUNCTIONS
// ===================================================================

class Utils {
    static formatFileSize(bytes) {
        if (typeof bytes === 'string') bytes = parseInt(bytes);
        if (bytes === 0) return '0 B';
        
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        
        return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
    }
    
    static formatDate(dateStr) {
        try {
            const date = new Date(dateStr);
            const now = new Date();
            const diffTime = Math.abs(now - date);
            const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));
            
            if (diffDays === 1) return 'Today';
            if (diffDays === 2) return 'Yesterday';
            if (diffDays <= 7) return `${diffDays} days ago`;
            
            return date.toLocaleDateString() + ' ' + 
                   date.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'});
        } catch (e) {
            return dateStr;
        }
    }
    
    static formatDuration(milliseconds) {
        const seconds = Math.floor(milliseconds / 1000);
        const minutes = Math.floor(seconds / 60);
        const hours = Math.floor(minutes / 60);
        
        if (hours > 0) {
            return `${hours}:${(minutes % 60).toString().padStart(2, '0')}:${(seconds % 60).toString().padStart(2, '0')}`;
        }
        return `${minutes}:${(seconds % 60).toString().padStart(2, '0')}`;
    }
    
    static getFileExtension(filename) {
        return '.' + filename.split('.').pop().toLowerCase();
    }
    
    static getFileIcon(filename) {
        const ext = this.getFileExtension(filename);
        const icons = {
            '.gpx': '🗺️',
            '.kml': '📍',
            '.json': '📄',
            '.txt': '📝',
            default: '📁'
        };
        return icons[ext] || icons.default;
    }
    
    static sanitizeFilename(filename) {
        return filename.replace(/[^a-zA-Z0-9._-]/g, '_');
    }
    
    static generateId() {
        return 'id_' + Math.random().toString(36).substr(2, 9);
    }
    
    static debounce(func, wait, immediate) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                timeout = null;
                if (!immediate) func(...args);
            };
            const callNow = immediate && !timeout;
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
            if (callNow) func(...args);
        };
    }
    
    static throttle(func, limit) {
        let inThrottle;
        return function(...args) {
            if (!inThrottle) {
                func.apply(this, args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, limit);
            }
        };
    }
    
    static isValidFileType(filename) {
        const ext = this.getFileExtension(filename);
        return CONFIG.ALLOWED_EXTENSIONS.includes(ext);
    }
    
    static isValidFileSize(size) {
        return size <= CONFIG.MAX_FILE_SIZE;
    }
}

// ===================================================================
// API CLIENT
// ===================================================================

class APIClient {
    static async request(url, options = {}) {
        const defaultOptions = {
            headers: {
                'Content-Type': 'application/json',
            },
            timeout: 10000
        };
        
        const mergedOptions = { ...defaultOptions, ...options };
        
        try {
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), mergedOptions.timeout);
            
            const response = await fetch(url, {
                ...mergedOptions,
                signal: controller.signal
            });
            
            clearTimeout(timeoutId);
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            return response;
        } catch (error) {
            console.error('API request failed:', error);
            throw error;
        }
    }
    
    static async getJSON(url, options = {}) {
        const response = await this.request(url, options);
        return response.json();
    }
    
    static async postJSON(url, data, options = {}) {
        return this.request(url, {
            method: 'POST',
            body: JSON.stringify(data),
            ...options
        });
    }
    
    static async uploadFile(file, onProgress) {
        return new Promise((resolve, reject) => {
            const formData = new FormData();
            formData.append('file', file);
            
            const xhr = new XMLHttpRequest();
            
            // Upload progress
            xhr.upload.addEventListener('progress', (e) => {
                if (e.lengthComputable && onProgress) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    onProgress(percentComplete, e.loaded, e.total);
                }
            });
            
            // Handle completion
            xhr.addEventListener('load', () => {
                if (xhr.status >= 200 && xhr.status < 300) {
                    try {
                        const response = JSON.parse(xhr.responseText);
                        resolve(response);
                    } catch (e) {
                        resolve({ success: true });
                    }
                } else {
                    reject(new Error(`Upload failed: ${xhr.status} ${xhr.statusText}`));
                }
            });
            
            // Handle errors
            xhr.addEventListener('error', () => {
                reject(new Error('Upload failed: Network error'));
            });
            
            xhr.addEventListener('timeout', () => {
                reject(new Error('Upload failed: Timeout'));
            });
            
            // Start upload
            xhr.timeout = CONFIG.UPLOAD_TIMEOUT;
            xhr.open('POST', CONFIG.ENDPOINTS.UPLOAD);
            xhr.send(formData);
        });
    }
    
    static async getStatus() {
        try {
            return await this.getJSON(CONFIG.ENDPOINTS.STATUS);
        } catch (error) {
            return { status: 'disconnected', error: error.message };
        }
    }
    
    static async getFiles() {
        try {
            const response = await this.getJSON(CONFIG.ENDPOINTS.FILES);
            return response.files || [];
        } catch (error) {
            console.error('Failed to get file list:', error);
            return [];
        }
    }
    
    static downloadFile(filename) {
        const url = `${CONFIG.ENDPOINTS.DOWNLOAD}?file=${encodeURIComponent(filename)}`;
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        link.style.display = 'none';
        
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    }
    
    static async deleteFile(filename) {
        const url = `${CONFIG.ENDPOINTS.DELETE}?file=${encodeURIComponent(filename)}`;
        const response = await this.request(url, { method: 'DELETE' });
        return response.json();
    }
}

// ===================================================================
// UI MANAGER
// ===================================================================

class UIManager {
    static showToast(message, type = 'info', duration = CONFIG.TOAST_DURATION) {
        const toastIcons = {
            'success': '✅',
            'error': '❌',
            'warning': '⚠️',
            'info': 'ℹ️'
        };
        
        const toast = document.getElementById('toast');
        const toastIcon = document.getElementById('toastIcon');
        const toastMessage = document.getElementById('toastMessage');
        
        toastIcon.textContent = toastIcons[type] || 'ℹ️';
        toastMessage.textContent = message;
        
        toast.className = `toast toast-${type}`;
        toast.style.display = 'block';
        
        // Auto-hide
        setTimeout(() => {
            toast.style.display = 'none';
        }, duration);
        
        console.log(`[${type.toUpperCase()}] ${message}`);
    }
    
    static showModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) {
            modal.style.display = 'flex';
            document.body.style.overflow = 'hidden';
        }
    }
    
    static hideModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) {
            modal.style.display = 'none';
            document.body.style.overflow = '';
        }
    }
    
    static showSuccessModal(message) {
        document.getElementById('successMessage').textContent = message;
        this.showModal('successModal');
    }
    
    static showErrorModal(message) {
        document.getElementById('errorMessage').textContent = message;
        this.showModal('errorModal');
    }
    
    static showConfirmModal(title, message, callback) {
        document.getElementById('confirmTitle').textContent = title;
        document.getElementById('confirmMessage').textContent = message;
        
        const confirmButton = document.getElementById('confirmButton');
        confirmButton.onclick = () => {
            callback();
            this.hideModal('confirmModal');
        };
        
        this.showModal('confirmModal');
    }
    
    static showLoadingOverlay(title, message) {
        document.getElementById('loadingTitle').textContent = title;
        document.getElementById('loadingMessage').textContent = message;
        this.showModal('loadingOverlay');
    }
    
    static hideLoadingOverlay() {
        this.hideModal('loadingOverlay');
    }
    
    static updateLoadingMessage(message) {
        document.getElementById('loadingMessage').textContent = message;
    }
    
    static updateConnectionStatus(status) {
        const indicator = document.getElementById('statusIndicator');
        const statusText = document.getElementById('statusText');
        
        if (status.status === 'connected' || status.status === 'ok') {
            indicator.className = 'status-indicator connected';
            statusText.textContent = 'Connected';
            AppState.isConnected = true;
        } else {
            indicator.className = 'status-indicator disconnected';
            statusText.textContent = status.error || 'Disconnected';
            AppState.isConnected = false;
        }
        
        // Update storage info
        if (status.freeSpace) {
            document.getElementById('storageStatus').textContent = status.freeSpace;
        }
    }
    
    static updateSessionTimer() {
        const elapsed = Date.now() - AppState.sessionStartTime;
        const formattedTime = Utils.formatDuration(elapsed);
        document.getElementById('sessionTime').textContent = formattedTime;
    }
    
    static updateFileStats(files) {
        const totalFiles = files.length;
        const totalSizeBytes = files.reduce((sum, file) => {
            const size = typeof file.size === 'string' ? parseInt(file.size) : file.size;
            return sum + (size || 0);
        }, 0);
        
        document.getElementById('totalFiles').textContent = totalFiles;
        document.getElementById('totalSize').textContent = Utils.formatFileSize(totalSizeBytes);
        document.getElementById('fileCount').textContent = `${totalFiles} track${totalFiles !== 1 ? 's' : ''}`;
    }
}

// ===================================================================
// FILE UPLOAD MANAGER
// ===================================================================

class FileUploadManager {
    constructor() {
        this.activeUploads = new Map();
        this.setupEventListeners();
    }
    
    setupEventListeners() {
        // File input change
        const fileInput = document.getElementById('fileInput');
        fileInput.addEventListener('change', (e) => {
            this.handleFiles(Array.from(e.target.files));
        });
        
        // Upload area click
        const uploadArea = document.getElementById('uploadArea');
        uploadArea.addEventListener('click', () => {
            fileInput.click();
        });
        
        // Drag and drop
        this.setupDragAndDrop();
    }
    
    setupDragAndDrop() {
        const uploadArea = document.getElementById('uploadArea');
        
        // Prevent default drag behaviors
        ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
            uploadArea.addEventListener(eventName, this.preventDefaults, false);
            document.body.addEventListener(eventName, this.preventDefaults, false);
        });
        
        // Highlight drop area
        ['dragenter', 'dragover'].forEach(eventName => {
            uploadArea.addEventListener(eventName, () => {
                uploadArea.classList.add('drag-over');
            }, false);
        });
        
        ['dragleave', 'drop'].forEach(eventName => {
            uploadArea.addEventListener(eventName, () => {
                uploadArea.classList.remove('drag-over');
            }, false);
        });
        
        // Handle dropped files
        uploadArea.addEventListener('drop', (e) => {
            const files = Array.from(e.dataTransfer.files);
            this.handleFiles(files);
        }, false);
    }
    
    preventDefaults(e) {
        e.preventDefault();
        e.stopPropagation();
    }
    
    async handleFiles(files) {
        console.log('Processing', files.length, 'files');
        
        // Validate files
        const validFiles = this.validateFiles(files);
        
        if (validFiles.length === 0) {
            UIManager.showToast('❌ No valid files selected', 'error');
            return;
        }
        
        if (validFiles.length !== files.length) {
            const rejected = files.length - validFiles.length;
            UIManager.showToast(`⚠️ ${rejected} file(s) rejected`, 'warning');
        }
        
        // Start uploads
        await this.uploadFiles(validFiles);
    }
    
    validateFiles(files) {
        return files.filter(file => {
            // Check file type
            if (!Utils.isValidFileType(file.name)) {
                UIManager.showToast(`❌ ${file.name} - Invalid file type`, 'error');
                return false;
            }
            
            // Check file size
            if (!Utils.isValidFileSize(file.size)) {
                const maxSize = Utils.formatFileSize(CONFIG.MAX_FILE_SIZE);
                UIManager.showToast(`❌ ${file.name} - File too large (max ${maxSize})`, 'error');
                return false;
            }
            
            return true;
        });
    }
    
    async uploadFiles(files) {
        console.log('Uploading', files.length, 'files');
        
        this.showUploadProgress();
        
        let uploadedCount = 0;
        let failedCount = 0;
        
        for (let i = 0; i < files.length; i++) {
            const file = files[i];
            const uploadId = Utils.generateId();
            
            try {
                this.updateProgressFile(file.name, i + 1, files.length);
                
                const result = await this.uploadSingleFile(file, uploadId);
                
                if (result.success) {
                    uploadedCount++;
                    UIManager.showToast(`✅ ${file.name} uploaded`, 'success');
                    AppState.totalFilesUploaded++;
                    AppState.totalBytesTransferred += file.size;
                } else {
                    failedCount++;
                    UIManager.showToast(`❌ Failed: ${file.name}`, 'error');
                }
                
            } catch (error) {
                console.error('Upload error:', error);
                failedCount++;
                UIManager.showToast(`❌ Error: ${file.name}`, 'error');
            }
            
            // Update overall progress
            const progress = ((i + 1) / files.length) * 100;
            this.updateProgressBar(progress);
        }
        
        // Hide progress and show results
        this.hideUploadProgress();
        
        if (uploadedCount > 0) {
            UIManager.showSuccessModal(`Successfully uploaded ${uploadedCount} file(s)!`);
            // Refresh file list
            setTimeout(() => fileListManager.refreshFileList(), 1000);
        }
        
        if (failedCount > 0) {
            UIManager.showErrorModal(`Failed to upload ${failedCount} file(s). Please try again.`);
        }
    }
    
    async uploadSingleFile(file, uploadId) {
        return new Promise((resolve) => {
            this.activeUploads.set(uploadId, {
                file: file,
                startTime: Date.now(),
                progress: 0
            });
            
            APIClient.uploadFile(file, (progress, loaded, total) => {
                // Update individual file progress
                this.updateProgressBar(progress);
                this.updateProgressStatus(`Uploading ${file.name}... ${Math.round(progress)}%`);
                
                // Update active upload tracking
                const upload = this.activeUploads.get(uploadId);
                if (upload) {
                    upload.progress = progress;
                    upload.loaded = loaded;
                    upload.total = total;
                }
            })
            .then(result => {
                this.activeUploads.delete(uploadId);
                resolve(result);
            })
            .catch(error => {
                this.activeUploads.delete(uploadId);
                console.error('Upload failed:', error);
                resolve({ success: false, error: error.message });
            });
        });
    }
    
    showUploadProgress() {
        document.getElementById('uploadContent').style.display = 'none';
        document.getElementById('uploadProgress').style.display = 'block';
    }
    
    hideUploadProgress() {
        document.getElementById('uploadContent').style.display = 'block';
        document.getElementById('uploadProgress').style.display = 'none';
        this.resetProgress();
    }
    
    updateProgressFile(filename, current, total) {
        document.getElementById('progressFile').textContent = `${filename} (${current}/${total})`;
    }
    
    updateProgressBar(percent) {
        const fill = document.getElementById('progressFill');
        const percentText = document.getElementById('progressPercent');
        
        fill.style.width = Math.min(100, Math.max(0, percent)) + '%';
        percentText.textContent = Math.round(percent) + '%';
    }
    
    updateProgressStatus(status) {
        document.getElementById('progressStatus').textContent = status;
    }
    
    resetProgress() {
        this.updateProgressBar(0);
        this.updateProgressStatus('Ready');
        document.getElementById('progressFile').textContent = '';
    }
}

// ===================================================================
// FILE LIST MANAGER
// ===================================================================

class FileListManager {
    constructor() {
        this.lastRefresh = 0;
        this.refreshInterval = null;
    }
    
    async refreshFileList() {
        try {
            console.log('Refreshing file list...');
            
            const files = await APIClient.getFiles();
            
            AppState.currentFiles = files;
            this.displayFileList(files);
            UIManager.updateFileStats(files);
            
            this.lastRefresh = Date.now();
            
        } catch (error) {
            console.error('Error refreshing file list:', error);
            UIManager.showToast('❌ Error loading file list', 'error');
            this.showErrorState();
        }
    }
    
    displayFileList(files) {
        const fileListElement = document.getElementById('fileList');
        
        if (files.length === 0) {
            this.showEmptyState();
            return;
        }
        
        // Sort files by date (newest first)
        files.sort((a, b) => new Date(b.date) - new Date(a.date));
        
        let html = '';
        files.forEach(file => {
            html += this.createFileItemHTML(file);
        });
        
        fileListElement.innerHTML = html;
    }
    
    createFileItemHTML(file) {
        const fileSize = Utils.formatFileSize(file.size);
        const fileDate = Utils.formatDate(file.date);
        const fileIcon = Utils.getFileIcon(file.name);
        const safeFilename = file.name.replace(/'/g, "\\'");
        
        return `
            <div class="file-item">
                <div class="file-info">
                    <div class="file-icon">${fileIcon}</div>
                    <div class="file-details">
                        <div class="file-name" title="${file.name}">${file.name}</div>
                        <div class="file-meta">
                            <span class="file-size">${fileSize}</span>
                            <span class="file-date">${fileDate}</span>
                        </div>
                    </div>
                </div>
                <div class="file-actions">
                    <button class="btn-download" onclick="fileListManager.downloadFile('${safeFilename}')">
                        <span class="btn-icon">📥</span>
                        Download
                    </button>
                    <button class="btn-delete" onclick="fileListManager.confirmDeleteFile('${safeFilename}')">
                        <span class="btn-icon">🗑️</span>
                    </button>
                </div>
            </div>
        `;
    }
    
    showEmptyState() {
        const fileListElement = document.getElementById('fileList');
        fileListElement.innerHTML = `
            <div class="file-list-empty">
                <div class="empty-icon">📂</div>
                <h3>No tracks recorded yet</h3>
                <p>Your recorded tracks will appear here</p>
                <p class="empty-tip">Start riding and they'll show up automatically! 🏍️</p>
            </div>
        `;
    }
    
    showErrorState() {
        const fileListElement = document.getElementById('fileList');
        fileListElement.innerHTML = `
            <div class="file-list-error">
                <div class="error-icon">⚠️</div>
                <h3>Unable to load file list</h3>
                <p>Check your connection and try again</p>
                <button class="btn-secondary" onclick="fileListManager.refreshFileList()">
                    🔄 Try Again
                </button>
            </div>
        `;
    }
    
    downloadFile(filename) {
        console.log('Downloading file:', filename);
        
        try {
            APIClient.downloadFile(filename);
            UIManager.showToast(`📥 Downloading ${filename}`, 'info');
        } catch (error) {
            console.error('Download error:', error);
            UIManager.showToast(`❌ Download failed: ${filename}`, 'error');
        }
    }
    
    confirmDeleteFile(filename) {
        UIManager.showConfirmModal(
            'Delete Track File',
            `Are you sure you want to delete "${filename}"? This action cannot be undone.`,
            () => this.deleteFile(filename)
        );
    }
    
    async deleteFile(filename) {
        try {
            UIManager.showLoadingOverlay('Deleting File', `Removing ${filename}...`);
            
            const result = await APIClient.deleteFile(filename);
            
            UIManager.hideLoadingOverlay();
            
            if (result.success) {
                UIManager.showToast(`✅ Deleted ${filename}`, 'success');
                this.refreshFileList();
            } else {
                UIManager.showToast(`❌ Failed to delete ${filename}`, 'error');
            }
            
        } catch (error) {
            console.error('Delete error:', error);
            UIManager.hideLoadingOverlay();
            UIManager.showToast(`❌ Error deleting ${filename}`, 'error');
        }
    }
    
    startAutoRefresh() {
        this.refreshInterval = setInterval(() => {
            this.refreshFileList();
        }, CONFIG.FILE_LIST_REFRESH);
    }
    
    stopAutoRefresh() {
        if (this.refreshInterval) {
            clearInterval(this.refreshInterval);
            this.refreshInterval = null;
        }
    }
}

// ===================================================================
// STATUS MANAGER
// ===================================================================

class StatusManager {
    constructor() {
        this.statusInterval = null;
        this.timerInterval = null;
    }
    
    async updateStatus() {
        try {
            const status = await APIClient.getStatus();
            UIManager.updateConnectionStatus(status);
            AppState.lastStatusUpdate = Date.now();
        } catch (error) {
            console.error('Status update failed:', error);
            UIManager.updateConnectionStatus({ 
                status: 'disconnected', 
                error: 'Connection failed' 
            });
        }
    }
    
    startStatusUpdates() {
        // Initial update
        this.updateStatus();
        
        // Periodic updates
        this.statusInterval = setInterval(() => {
            this.updateStatus();
        }, CONFIG.STATUS_UPDATE_INTERVAL);
        
        // Session timer
        this.timerInterval = setInterval(() => {
            UIManager.updateSessionTimer();
        }, CONFIG.SESSION_TIMER_UPDATE);
    }
    
    stopStatusUpdates() {
        if (this.statusInterval) {
            clearInterval(this.statusInterval);
            this.statusInterval = null;
        }
        
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
            this.timerInterval = null;
        }
    }
}

// ===================================================================
// QUICK ACTIONS MANAGER
// ===================================================================

class QuickActionsManager {
    async downloadAllTracks() {
        UIManager.showLoadingOverlay('Preparing Download', 'Gathering all your track files...');
        
        try {
            const files = AppState.currentFiles;
            
            if (files.length === 0) {
                UIManager.hideLoadingOverlay();
                UIManager.showToast('📂 No tracks to download', 'info');
                return;
            }
            
            // Download each file with delay
            for (let i = 0; i < files.length; i++) {
                const file = files[i];
                UIManager.updateLoadingMessage(`Downloading ${file.name} (${i + 1}/${files.length})`);
                
                APIClient.downloadFile(file.name);
                
                // Small delay between downloads
                await new Promise(resolve => setTimeout(resolve, 500));
            }
            
            UIManager.hideLoadingOverlay();
            UIManager.showToast(`📦 Downloaded ${files.length} track files!`, 'success');
            
        } catch (error) {
            console.error('Download all error:', error);
            UIManager.hideLoadingOverlay();
            UIManager.showToast('❌ Error downloading tracks', 'error');
        }
    }
    
    clearOldTracks() {
        UIManager.showConfirmModal(
            'Clear Old Tracks',
            'This will delete tracks older than 30 days. Are you sure?',
            () => this.performClearOldTracks()
        );
    }
    
    async performClearOldTracks() {
        UIManager.showLoadingOverlay('Cleaning Storage', 'Removing old track files...');
        
        try {
            // Filter files older than 30 days
            const thirtyDaysAgo = Date.now() - (30 * 24 * 60 * 60 * 1000);
            const oldFiles = AppState.currentFiles.filter(file => {
                const fileDate = new Date(file.date);
                return fileDate.getTime() < thirtyDaysAgo;
            });
            
            if (oldFiles.length === 0) {
                UIManager.hideLoadingOverlay();
                UIManager.showToast('📂 No old files to remove', 'info');
                return;
            }
            
            let deletedCount = 0;
            
            for (const file of oldFiles) {
                try {
                    await APIClient.deleteFile(file.name);
                    deletedCount++;
                } catch (error) {
                    console.error('Failed to delete:', file.name, error);
                }
            }
            
            UIManager.hideLoadingOverlay();
            
            if (deletedCount > 0) {
                UIManager.showToast(`🗑️ Removed ${deletedCount} old files!`, 'success');
                fileListManager.refreshFileList();
            } else {
                UIManager.showToast('❌ No files were removed', 'warning');
            }
            
        } catch (error) {
            console.error('Clear tracks error:', error);
            UIManager.hideLoadingOverlay();
            UIManager.showToast('❌ Error cleaning storage', 'error');
        }
    }
    
    showSettings() {
        UIManager.showModal('settingsModal');
    }
    
    saveSettings() {
        const recordingMode = document.getElementById('recordingMode').value;
        const trackQuality = document.getElementById('trackQuality').value;
        const autoSave = document.getElementById('autoSave').value;
        
        AppState.settings = {
            recordingMode,
            trackQuality,
            autoSave: parseInt(autoSave)
        };
        
        console.log('Settings saved:', AppState.settings);
        
        // In a real implementation, this would save to the GPS device
        UIManager.showToast('⚙️ Settings saved to GPS device', 'success');
        UIManager.hideModal('settingsModal');
    }
}

// ===================================================================
// GLOBAL INSTANCES
// ===================================================================

let fileUploadManager;
let fileListManager;
let statusManager;
let quickActionsManager;

// ===================================================================
// APPLICATION INITIALIZATION
// ===================================================================

function initializeApp() {
    console.log('🏍️ E_NAVI File Transfer Interface Loading...');
    
    // Initialize managers
    fileUploadManager = new FileUploadManager();
    fileListManager = new FileListManager();
    statusManager = new StatusManager();
    quickActionsManager = new QuickActionsManager();
    
    // Start services
    statusManager.startStatusUpdates();
    fileListManager.startAutoRefresh();
    
    // Load initial data
    fileListManager.refreshFileList();
    
    console.log('✅ E_NAVI Interface Ready');
}

// ===================================================================
// GLOBAL FUNCTIONS (called from HTML)
// ===================================================================

function refreshFileList() {
    fileListManager.refreshFileList();
}

function downloadAllTracks() {
    quickActionsManager.downloadAllTracks();
}

function clearOldTracks() {
    quickActionsManager.clearOldTracks();
}

function showSettings() {
    quickActionsManager.showSettings();
}

function saveSettings() {
    quickActionsManager.saveSettings();
}

function closeModal(modalId) {
    UIManager.hideModal(modalId);
}

// ===================================================================
// DOCUMENT READY
// ===================================================================

document.addEventListener('DOMContentLoaded', function() {
    console.log('🚀 E_NAVI File Transfer System Starting...');
    
    // Initialize the application
    initializeApp();
    
    // Show welcome message
    setTimeout(() => {
        UIManager.showToast('🏍️ Welcome to E_NAVI File Transfer!', 'success');
    }, 1000);
    
    // Setup global error handling
    window.addEventListener('error', (e) => {
        console.error('Global error:', e.error);
        UIManager.showToast('❌ An unexpected error occurred', 'error');
    });
    
    // Setup unload cleanup
    window.addEventListener('beforeunload', () => {
        statusManager.stopStatusUpdates();
        fileListManager.stopAutoRefresh();
    });
    
    console.log('🎉 E_NAVI Ready for Adventure!');
});
