#ifndef OTA_STYLES_CSS_H
#define OTA_STYLES_CSS_H

#include <Arduino.h>

// Additional CSS styles specific to OTA module
// These extend the web platform's base styles
const char OTA_STYLES_CSS[] PROGMEM = R"rawliteral(
/* OTA Update Module Specific Styles */

/* Progress indicators */
.ota-progress {
    margin: 20px 0;
}

.ota-progress-bar {
    width: 100%;
    height: 24px;
    background: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.2);
    border-radius: 12px;
    overflow: hidden;
    position: relative;
    backdrop-filter: blur(10px);
}

.ota-progress-fill {
    height: 100%;
    background: linear-gradient(90deg, 
        rgba(34, 197, 94, 0.8) 0%, 
        rgba(59, 130, 246, 0.8) 100%);
    border-radius: 11px;
    transition: width 0.3s ease;
    position: relative;
}

.ota-progress-fill::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: linear-gradient(45deg,
        transparent 40%,
        rgba(255, 255, 255, 0.3) 50%,
        transparent 60%);
    animation: progress-shine 2s infinite;
}

@keyframes progress-shine {
    0% { transform: translateX(-100%); }
    100% { transform: translateX(100%); }
}

.ota-progress-text {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    font-weight: 600;
    color: white;
    text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.5);
    font-size: 0.9em;
}

/* Version cards */
.version-card {
    background: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.2);
    border-radius: 12px;
    padding: 20px;
    margin-bottom: 15px;
    backdrop-filter: blur(10px);
    transition: all 0.3s ease;
}

.version-card:hover {
    background: rgba(255, 255, 255, 0.15);
    border-color: rgba(59, 130, 246, 0.5);
    transform: translateY(-2px);
    box-shadow: 0 8px 25px rgba(0, 0, 0, 0.2);
}

.version-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;
}

.version-number {
    font-size: 1.2em;
    font-weight: 600;
    color: #60a5fa;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
}

.version-date {
    color: rgba(255, 255, 255, 0.7);
    font-size: 0.9em;
}

.version-info {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 15px;
    margin-bottom: 15px;
}

.version-detail {
    display: flex;
    align-items: center;
    gap: 8px;
}

.version-detail-icon {
    width: 16px;
    height: 16px;
    opacity: 0.7;
}

.version-notes {
    background: rgba(0, 0, 0, 0.2);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    padding: 12px;
    margin: 12px 0;
    font-style: italic;
    color: rgba(255, 255, 255, 0.9);
}

.version-actions {
    display: flex;
    gap: 10px;
    align-items: center;
    margin-top: 15px;
}

/* Status badges */
.ota-status-badge {
    display: inline-block;
    padding: 4px 12px;
    border-radius: 20px;
    font-size: 0.8em;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.status-idle {
    background: rgba(34, 197, 94, 0.2);
    color: #22c55e;
    border: 1px solid rgba(34, 197, 94, 0.3);
}

.status-checking {
    background: rgba(59, 130, 246, 0.2);
    color: #3b82f6;
    border: 1px solid rgba(59, 130, 246, 0.3);
}

.status-downloading {
    background: rgba(251, 191, 36, 0.2);
    color: #fbbf24;
    border: 1px solid rgba(251, 191, 36, 0.3);
}

.status-installing {
    background: rgba(249, 115, 22, 0.2);
    color: #f97316;
    border: 1px solid rgba(249, 115, 22, 0.3);
}

.status-complete {
    background: rgba(34, 197, 94, 0.2);
    color: #22c55e;
    border: 1px solid rgba(34, 197, 94, 0.3);
}

.status-error {
    background: rgba(239, 68, 68, 0.2);
    color: #ef4444;
    border: 1px solid rgba(239, 68, 68, 0.3);
}

/* Breaking change indicator */
.breaking-change {
    background: rgba(251, 191, 36, 0.1);
    border-color: rgba(251, 191, 36, 0.3);
}

.breaking-badge {
    background: linear-gradient(135deg, #fbbf24, #f59e0b);
    color: #92400e;
    padding: 3px 8px;
    border-radius: 12px;
    font-size: 0.7em;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    box-shadow: 0 2px 4px rgba(251, 191, 36, 0.3);
}

/* History list */
.history-list {
    max-height: 300px;
    overflow-y: auto;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    background: rgba(0, 0, 0, 0.1);
}

.history-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.1);
    transition: background-color 0.2s ease;
}

.history-item:hover {
    background: rgba(255, 255, 255, 0.05);
}

.history-item:last-child {
    border-bottom: none;
}

.history-info {
    flex: 1;
}

.history-version {
    font-weight: 600;
    color: #60a5fa;
}

.history-date {
    font-size: 0.8em;
    color: rgba(255, 255, 255, 0.6);
    margin-top: 2px;
}

.history-status {
    padding: 4px 8px;
    border-radius: 12px;
    font-size: 0.75em;
    font-weight: 600;
    text-align: center;
    min-width: 70px;
}

.history-success {
    background: rgba(34, 197, 94, 0.2);
    color: #22c55e;
    border: 1px solid rgba(34, 197, 94, 0.3);
}

.history-error {
    background: rgba(239, 68, 68, 0.2);
    color: #ef4444;
    border: 1px solid rgba(239, 68, 68, 0.3);
}

/* Configuration form enhancements */
.ota-config-section {
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 12px;
    padding: 20px;
    margin-bottom: 20px;
}

.ota-config-title {
    color: #60a5fa;
    font-weight: 600;
    margin-bottom: 15px;
    display: flex;
    align-items: center;
    gap: 8px;
}

/* Action buttons styling */
.ota-action-buttons {
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    margin-bottom: 20px;
}

.ota-action-buttons .btn {
    min-width: 140px;
}

/* Responsive design */
@media (max-width: 768px) {
    .version-info {
        grid-template-columns: 1fr;
        gap: 10px;
    }
    
    .version-actions {
        flex-direction: column;
        align-items: stretch;
    }
    
    .ota-action-buttons {
        flex-direction: column;
    }
    
    .ota-action-buttons .btn {
        min-width: auto;
    }
    
    .history-item {
        flex-direction: column;
        align-items: flex-start;
        gap: 8px;
    }
    
    .history-status {
        align-self: flex-end;
    }
}

/* Animation for status updates */
.ota-status-updating {
    animation: pulse 2s infinite;
}

@keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.7; }
}

/* File size indicator */
.file-size {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    color: rgba(255, 255, 255, 0.6);
    font-size: 0.85em;
}

.file-size::before {
    content: '📦';
    font-size: 0.9em;
}

/* Network indicators */
.network-status {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    font-size: 0.85em;
    margin-left: 10px;
}

.network-status.online {
    color: #22c55e;
}

.network-status.offline {
    color: #ef4444;
}

.network-status::before {
    content: '🌐';
}
)rawliteral";

#endif // OTA_STYLES_CSS_H