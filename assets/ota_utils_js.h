#ifndef OTA_UTILS_JS_H
#define OTA_UTILS_JS_H

#include <Arduino.h>

const char OTA_UTILS_JS[] PROGMEM = R"rawliteral(
/**
 * OTA Update Manager JavaScript Utilities
 * Provides a complete interface for managing firmware updates
 */
class OTAUpdateManager {
    constructor() {
        this.baseUrl = '/ota';
        this.pollingInterval = null;
        this.isPolling = false;
        this.currentStatus = { state: 0 }; // IDLE
    }

    async init() {
        console.log('OTA Update Manager initialized');
        
        // Bind event handlers
        this.bindEventHandlers();
        
        // Load initial status
        await this.refreshStatus();
        await this.loadHistory();
        
        // Start status polling if update is in progress
        if (this.currentStatus.state > 0 && this.currentStatus.state < 4) {
            this.startProgressPolling();
        }
    }

    bindEventHandlers() {
        // Check for updates button
        document.getElementById('check-updates')?.addEventListener('click', () => {
            this.checkForUpdates();
        });

        // Install latest button
        document.getElementById('install-latest')?.addEventListener('click', () => {
            this.installLatest();
        });

        // Save configuration button (for auto-check settings only)
        document.getElementById('save-config')?.addEventListener('click', () => {
            this.saveConfiguration();
        });
    }

    async refreshStatus() {
        try {
            const response = await AuthUtils.fetch(this.baseUrl + '/api/status');
            const data = await response.json();
            
            this.currentStatus = data;
            this.updateStatusDisplay(data);
            this.updateAvailableVersions(data.available_versions || []);
            
        } catch (error) {
            console.error('Failed to refresh OTA status:', error);
            UIUtils.showAlert('Failed to load OTA status', 'error');
        }
    }

    async checkForUpdates() {
        const button = document.getElementById('check-updates');
        if (button) {
            button.disabled = true;
            button.textContent = 'Checking...';
        }
        
        try {
            const response = await AuthUtils.fetch(this.baseUrl + '/api/check', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' }
            });
            
            const result = await response.json();
            
            if (result.success) {
                UIUtils.showAlert('Update check completed', 'success');
                await this.refreshStatus();
            } else {
                UIUtils.showAlert('Update check failed: ' + result.message, 'error');
            }
            
        } catch (error) {
            console.error('Failed to check for updates:', error);
            UIUtils.showAlert('Failed to check for updates', 'error');
        } finally {
            if (button) {
                button.disabled = false;
                button.textContent = 'Check for Updates';
            }
        }
    }

    async installLatest() {
        if (!confirm('Are you sure you want to install the latest firmware update? The device will reboot.')) {
            return;
        }

        const button = document.getElementById('install-latest');
        if (button) {
            button.disabled = true;
            button.textContent = 'Installing...';
        }

        // Show progress card immediately
        this.showProgressCard();
        this.updateProgressDisplay({
            state: 2, // DOWNLOADING
            progress: 0,
            total: 100,
            message: 'Starting firmware download...'
        });

        try {
            const response = await AuthUtils.fetch(this.baseUrl + '/api/install', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            });

            const result = await response.json();

            if (result.success) {
                UIUtils.showAlert('Firmware installation started', 'success');
                this.startProgressPolling();
            } else {
                UIUtils.showAlert('Installation failed: ' + result.message, 'error');
                this.hideProgressCard();
                if (button) {
                    button.disabled = false;
                    button.textContent = 'Install Latest';
                }
            }

        } catch (error) {
            console.error('Failed to start installation:', error);
            UIUtils.showAlert('Failed to start installation', 'error');
            this.hideProgressCard();
            if (button) {
                button.disabled = false;
                button.textContent = 'Install Latest';
            }
        }
    }

    async installSpecificVersion(version) {
        if (!confirm(`Are you sure you want to install firmware version ${version}? The device will reboot.`)) {
            return;
        }

        // Show progress card immediately
        this.showProgressCard();
        this.updateProgressDisplay({
            state: 2, // DOWNLOADING
            progress: 0,
            total: 100,
            message: `Starting firmware ${version} download...`
        });

        try {
            const response = await AuthUtils.fetch(this.baseUrl + '/api/install', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ version: version })
            });

            const result = await response.json();

            if (result.success) {
                UIUtils.showAlert(`Installing firmware ${version}...`, 'success');
                this.startProgressPolling();
            } else {
                UIUtils.showAlert('Installation failed: ' + result.message, 'error');
                this.hideProgressCard();
            }

        } catch (error) {
            console.error('Failed to install version:', error);
            UIUtils.showAlert('Failed to start installation', 'error');
            this.hideProgressCard();
        }
    }

    startProgressPolling() {
        if (this.isPolling) return;
        
        this.isPolling = true;
        this.showProgressCard();
        
        this.pollingInterval = setInterval(async () => {
            await this.updateProgress();
        }, 1000); // Update every second
    }

    stopProgressPolling() {
        if (this.pollingInterval) {
            clearInterval(this.pollingInterval);
            this.pollingInterval = null;
        }
        this.isPolling = false;
    }

    async updateProgress() {
        try {
            const response = await AuthUtils.fetch(this.baseUrl + '/api/progress');
            const data = await response.json();
            
            this.updateProgressDisplay(data);
            
            // Stop polling if installation is complete or failed
            if (data.state === 3 || data.state === 4 || data.state === 0) { // COMPLETE, ERROR, or IDLE
                this.stopProgressPolling();
                
                if (data.state === 3) { // COMPLETE
                    UIUtils.showAlert('Firmware update completed! Device is rebooting...', 'success');
                    setTimeout(() => {
                        window.location.reload();
                    }, 5000);
                } else if (data.state === 4) { // ERROR
                    this.hideProgressCard();
                    UIUtils.showAlert('Firmware update failed: ' + data.message, 'error');
                    await this.refreshStatus();
                }
            }
            
        } catch (error) {
            console.error('Failed to get progress:', error);
            this.stopProgressPolling();
            this.hideProgressCard();
        }
    }

    async loadHistory() {
        try {
            const response = await AuthUtils.fetch(this.baseUrl + '/api/history');
            const data = await response.json();
            
            this.updateHistoryDisplay(data.history || []);
            
        } catch (error) {
            console.error('Failed to load history:', error);
            document.getElementById('update-history').innerHTML = '<p class="error">Failed to load update history</p>';
        }
    }

    async saveConfiguration() {
        const autoCheck = document.getElementById('auto-check')?.checked;
        const checkInterval = document.getElementById('check-interval')?.value;

        // Manifest URL is compile-time only, no need to save it
        console.log('Save configuration:', { autoCheck, checkInterval });
        
        // Note: Currently only updates in-memory settings
        // Future enhancement: Add API endpoint to persist auto-check settings
        UIUtils.showAlert('Configuration updated (in-memory only)', 'warning');
    }

    // Display update methods
    updateStatusDisplay(data) {
        // Update current version
        const currentVersionEl = document.getElementById('current-version');
        if (currentVersionEl) {
            currentVersionEl.textContent = data.current_version || 'Unknown';
        }

        // Update status
        const statusEl = document.getElementById('update-status');
        if (statusEl) {
            const stateNames = ['Idle', 'Checking', 'Downloading', 'Installing', 'Complete', 'Error'];
            const stateClasses = ['idle', 'checking', 'downloading', 'installing', 'complete', 'error'];
            
            statusEl.className = 'status-' + stateClasses[data.state] || 'idle';
            statusEl.textContent = stateNames[data.state] || 'Unknown';
        }

        // Update available updates count
        const availableEl = document.getElementById('updates-available');
        if (availableEl) {
            availableEl.textContent = data.updates_available || 0;
        }

        // Enable/disable install button
        const installBtn = document.getElementById('install-latest');
        if (installBtn) {
            installBtn.disabled = !data.updates_available || data.state !== 0; // Only enable if idle and updates available
        }
    }

    updateAvailableVersions(versions) {
        const container = document.getElementById('available-versions');
        if (!container) return;

        if (!versions || versions.length === 0) {
            container.innerHTML = '<p>No updates available.</p>';
            return;
        }

        let html = '';
        versions.forEach(version => {
            const isBreaking = version.breaking ? ' breaking-change' : '';
            const breakingBadge = version.breaking ? '<span class="breaking-badge">BREAKING</span>' : '';
            
            html += `
                <div class="version-item${isBreaking}">
                    <div class="version-header">
                        <span class="version-number">v${version.version}</span>
                        ${breakingBadge}
                        <span class="version-date">${new Date(version.released).toLocaleDateString()}</span>
                    </div>
                    <div class="version-notes">${version.notes || 'No release notes available'}</div>
                    <div class="version-actions">
                        <button class="btn btn-secondary btn-sm" onclick="window.OTAManager.installSpecificVersion('${version.version}')">
                            Install v${version.version}
                        </button>
                        <small class="text-muted">${this.formatBytes(version.size_bytes)}</small>
                    </div>
                </div>
            `;
        });

        container.innerHTML = html;
    }

    updateProgressDisplay(data) {
        const progressFill = document.getElementById('progress-fill');
        const progressText = document.getElementById('progress-text');
        const progressMessage = document.getElementById('progress-message');

        if (progressFill && data.total > 0) {
            const percentage = Math.round((data.progress / data.total) * 100);
            progressFill.style.width = percentage + '%';
        }

        if (progressText && data.total > 0) {
            const percentage = Math.round((data.progress / data.total) * 100);
            progressText.textContent = percentage + '%';
        } else if (progressText) {
            progressText.textContent = 'Working...';
        }

        if (progressMessage) {
            progressMessage.textContent = data.message || 'Processing...';
        }
    }

    updateHistoryDisplay(history) {
        const container = document.getElementById('update-history');
        if (!container) return;

        if (!history || history.length === 0) {
            container.innerHTML = '<p>No update history available.</p>';
            return;
        }

        let html = '';
        history.reverse().forEach(entry => { // Show newest first
            const statusClass = entry.success ? 'history-success' : 'history-error';
            const statusText = entry.success ? '✓ Success' : '✗ Failed';
            const date = new Date(entry.installed_at * 1000).toLocaleString();
            const errorText = entry.error ? ` - ${entry.error}` : '';
            
            html += `
                <div class="history-item">
                    <div>
                        <strong>v${entry.version}</strong>
                        <small class="text-muted">${date}</small>
                        <br>
                        <small>From v${entry.previous_version || 'Unknown'}</small>
                    </div>
                    <div class="${statusClass}">
                        ${statusText}${errorText}
                    </div>
                </div>
            `;
        });

        container.innerHTML = html;
    }

    showProgressCard() {
        const card = document.getElementById('progress-card');
        if (card) {
            card.style.display = 'block';
        }
    }

    hideProgressCard() {
        const card = document.getElementById('progress-card');
        if (card) {
            card.style.display = 'none';
        }
    }

    // Utility methods
    formatBytes(bytes) {
        if (bytes === 0) return '0 Bytes';
        
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }
}

// Make it globally available
window.OTAUpdateManager = OTAUpdateManager;
)rawliteral";

#endif // OTA_UTILS_JS_H