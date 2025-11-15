#ifndef OTA_STATUS_HTML_H
#define OTA_STATUS_HTML_H

#include <Arduino.h>

const char OTA_STATUS_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="/assets/style.css" type="text/css">
    <!-- Optional app-specific theme CSS is loaded after default styles -->
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
    <script src="/assets/web-platform-utils.js"></script>
    <script src="/assets/ota-utils.js"></script>
    <title>OTA Update Manager - {{DEVICE_NAME}}</title>
</head>
<body>
    {{NAV_MENU}}
    
    <div class="container">
        <h1>Firmware Update Manager</h1>
        
        <!-- Current Status Card -->
        <div class="status-card">
            <h2>Current Status</h2>
            <div class="status-grid">
                <div class="status-item">
                    <label>Current Version:</label>
                    <span id="current-version">Loading...</span>
                </div>
                <div class="status-item">
                    <label>Status:</label>
                    <span id="update-status" class="status-idle">Idle</span>
                </div>
                <div class="status-item">
                    <label>Last Check:</label>
                    <span id="last-check">Never</span>
                </div>
                <div class="status-item">
                    <label>Available Versions:</label>
                    <span id="updates-available">0</span>
                </div>
            </div>
        </div>

        <!-- Progress Card (hidden by default) -->
        <div id="progress-card" class="status-card" style="display: none;">
            <h2>Installation Progress</h2>
            <div class="progress-container">
                <div class="progress-bar">
                    <div id="progress-fill" class="progress-fill"></div>
                </div>
                <div id="progress-text">0%</div>
            </div>
            <div id="progress-message" class="progress-message">Preparing...</div>
        </div>

        <!-- Available Versions -->
        <div class="status-card">
            <h2>Available Versions</h2>
            <div class="update-actions">
                <button id="check-updates" class="btn btn-primary">Check for Updates</button>
                <button id="install-latest" class="btn btn-secondary" disabled>Install Latest</button>
            </div>
            <div id="available-versions" class="version-list">
                <p>Click "Check for Updates" to see available versions.</p>
            </div>
        </div>

        <!-- Update History -->
        <div class="status-card">
            <h2>Update History</h2>
            <div id="update-history" class="history-list">
                <p>Loading history...</p>
            </div>
        </div>

        <!-- Configuration -->
        <div class="status-card">
            <h2>Configuration</h2>
            <div class="status-grid">
                <div class="status-item">
                    <label>Manifest URL:</label>
                    <span class="manifest-url-display">Configured at compile-time</span>
                </div>
            </div>
            <div class="form-group">
                <label>
                    <input type="checkbox" id="auto-check"> Enable automatic update checks
                </label>
            </div>
            <div class="form-group">
                <label for="check-interval">Check Interval (seconds):</label>
                <input type="number" id="check-interval" class="form-control" min="300" max="86400" value="3600">
            </div>
            <button id="save-config" class="btn btn-primary">Save Configuration</button>
            <p class="text-muted"><small>Note: Auto-check settings are temporary and reset on reboot. Manifest URL is configured at build time.</small></p>
        </div>
    </div>

    <style>
        .progress-container {
            margin: 20px 0;
        }
        
        .progress-bar {
            width: 100%;
            height: 20px;
            background-color: #e0e0e0;
            border-radius: 10px;
            overflow: hidden;
            margin-bottom: 10px;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #4CAF50, #45a049);
            width: 0%;
            transition: width 0.3s ease;
        }
        
        .progress-message {
            font-style: italic;
            color: #666;
            margin-top: 10px;
        }
        
        .version-list {
            margin-top: 20px;
        }
        
        .version-item {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 15px;
            margin-bottom: 10px;
            background: rgba(255, 255, 255, 0.1);
        }
        
        .version-header {
            display: flex;
            justify-content: between;
            align-items: center;
            margin-bottom: 10px;
        }
        
        .version-number {
            font-weight: bold;
            color: #007bff;
        }
        
        .version-date {
            color: #666;
            font-size: 0.9em;
        }
        
        .version-notes {
            margin: 10px 0;
            font-style: italic;
        }
        
        .version-actions {
            margin-top: 10px;
        }
        
        .breaking-change {
            background-color: rgba(255, 193, 7, 0.1);
            border-color: #ffc107;
        }
        
        .breaking-badge {
            background-color: #ffc107;
            color: #212529;
            padding: 2px 8px;
            border-radius: 12px;
            font-size: 0.8em;
            margin-left: 10px;
        }
        
        .status-idle { color: #28a745; }
        .status-checking { color: #007bff; }
        .status-downloading { color: #ffc107; }
        .status-installing { color: #fd7e14; }
        .status-error { color: #dc3545; }
        .status-complete { color: #28a745; }
        
        .history-list {
            margin-top: 15px;
        }
        
        .history-item {
            padding: 10px;
            border-bottom: 1px solid #ddd;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .history-item:last-child {
            border-bottom: none;
        }
        
        .history-success {
            color: #28a745;
        }
        
        .history-error {
            color: #dc3545;
        }
        
        .update-actions {
            margin-bottom: 20px;
        }
        
        .update-actions button {
            margin-right: 10px;
        }
    </style>
    
    <script src="/ota/assets/ota-utils.js"></script>
    <script>
        // Initialize OTA manager when page loads
        document.addEventListener('DOMContentLoaded', function() {
            window.OTAManager = new OTAUpdateManager();
            window.OTAManager.init();
        });
    </script>
</body>
</html>)rawliteral";

#endif // OTA_STATUS_HTML_H