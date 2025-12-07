/**
 * Basic OTA Update Module Usage Example
 *
 * This example demonstrates how to integrate the OTA Update Module
 * with the TickerTape web platform for ESP32 devices.
 */

#include <Arduino.h>
#include <ota_update_module.h>
#include <web_platform.h>

// Global instances (defined in libraries)
extern WebPlatform webPlatform;
extern OTAUpdateModule otaUpdateModule;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting OTA Update Example...");

  // Configure OTA module (manifest URL is now compile-time)
  JsonDocument otaConfig;

  // Basic configuration
  otaConfig["auto_check"] = false;    // Manual checks only for this example
  otaConfig["check_interval"] = 3600; // 1 hour if auto_check enabled

#if OTA_AUTH_MODE == SHARED_KEY
  // Shared key mode configuration
  otaConfig["shared_key"] = OTA_SHARED_KEY;
#elif OTA_AUTH_MODE == DEVICE_SPECIFIC
  // Device-specific mode configuration
  // In production, these would be provisioned during manufacturing
  otaConfig["device_id"] = "device-12345";
  otaConfig["device_key"] = "unique-device-secret";
#endif

  // Register OTA module with web platform
  webPlatform.registerModule("/ota", &otaUpdateModule,
                             otaConfig.as<JsonVariant>());

  // Initialize web platform
  webPlatform.begin("OTA-Example-Device");

  // Set up progress callback for debugging
  otaUpdateModule.setProgressCallback(
      [](int progress, int total, String status) {
        Serial.printf("OTA Progress: %d/%d - %s\n", progress, total,
                      status.c_str());
      });

  Serial.println("OTA Update Module initialized!");
  Serial.printf("Access the OTA interface at: %s/ota/\n",
                webPlatform.getBaseUrl().c_str());

  // Optional: Perform initial update check
  Serial.println("Performing initial update check...");
  if (otaUpdateModule.checkForUpdates()) {
    Serial.println("Update check completed successfully");
  } else {
    Serial.println("Update check failed - check configuration");
  }
}

void loop() {
  // Handle web platform requests and OTA operations
  webPlatform.handle();

  // Your application code here
  delay(100);
}

/**
 * Advanced Configuration Example
 *
 * This shows more advanced configuration options and integration patterns.
 */
void advancedSetup() {
  // Example with more detailed configuration
  JsonDocument advancedConfig;

  // Manifest URL is now compile-time only
  advancedConfig["auto_check"] = true;     // Enable automatic checks
  advancedConfig["check_interval"] = 7200; // Check every 2 hours

  // Set up more detailed progress callback
  otaUpdateModule.setProgressCallback(
      [](int progress, int total, String status) {
        // Calculate percentage
        float percentage = total > 0 ? (float)progress / total * 100.0 : 0.0;

        Serial.printf("[OTA] %s - %.1f%% (%d/%d bytes)\n", status.c_str(),
                      percentage, progress, total);

        // You could also update an LED, display, or send status via MQTT
        // updateStatusLED(percentage);
        // sendMQTTStatus(status, percentage);
      });

  // Register with custom prefix
  webPlatform.registerModule("/firmware", &otaUpdateModule,
                             advancedConfig.as<JsonVariant>());
}

/**
 * Programmatic Usage Example
 *
 * Shows how to trigger updates programmatically instead of using web interface.
 */
void programmaticExample() {
  // Check for updates
  if (otaUpdateModule.checkForUpdates()) {
    Serial.println("Updates available!");

    UpdateStatus status = otaUpdateModule.getUpdateStatus();
    Serial.printf("Current version: %s\n", status.currentVersion.c_str());

    // Install latest version
    if (otaUpdateModule.installUpdate()) {
      Serial.println("Update installation started...");

      // Monitor progress
      while (true) {
        status = otaUpdateModule.getUpdateStatus();

        if (status.state == UpdateStatus::COMPLETE) {
          Serial.println("Update completed! Rebooting...");
          break;
        } else if (status.state == UpdateStatus::ERROR) {
          Serial.printf("Update failed: %s\n", status.error.c_str());
          break;
        }

        delay(1000); // Check every second
      }
    }
  }
}

/**
 * Custom Authentication Handler Example
 *
 * For DEVICE_SPECIFIC mode, you might want to customize device authentication.
 */
#if OTA_AUTH_MODE == DEVICE_SPECIFIC
String getDeviceCredentials() {
  // In production, read from secure storage, EEPROM, or provisioning system
  String deviceId = "device-" + WiFi.macAddress();
  String deviceKey = "provisioned-key-from-manufacturing";

  // You could also read from a configuration file or web service
  return deviceId + ":" + deviceKey;
}
#endif

/**
 * Build Configuration Examples
 *
 * Add these to your platformio.ini file:
 *
 * # Public access mode (no authentication)
 * build_flags =
 *   -DOTA_AUTH_MODE=PUBLIC_ACCESS
 *   -DOTA_FIRMWARE_VERSION='"1.0.0"'
 *   -DOTA_MANIFEST_URL='"https://your-cdn.cloudfront.net/manifest.json"'
 *
 * # Shared key mode
 * build_flags =
 *   -DOTA_AUTH_MODE=SHARED_KEY
 *   -DOTA_SHARED_KEY='"your-project-key-2024"'
 *   -DOTA_FIRMWARE_VERSION='"1.0.0"'
 *   -DOTA_MANIFEST_URL='"https://your-cdn.cloudfront.net/manifest.json"'
 *
 * # Device-specific mode
 * build_flags =
 *   -DOTA_AUTH_MODE=DEVICE_SPECIFIC
 *   -DOTA_FIRMWARE_VERSION='"1.0.0"'
 *   -DOTA_MANIFEST_URL='"https://your-cdn.cloudfront.net/manifest.json"'
 *
 * # Development mode (allows downgrades and rollback)
 * build_flags =
 *   -DOTA_AUTH_MODE=PUBLIC_ACCESS
 *   -DOTA_FIRMWARE_VERSION='"1.0.0"'
 *   -DOTA_MANIFEST_URL='"https://your-cdn.cloudfront.net/manifest.json"'
 *   -DOTA_DEVELOPMENT_MODE
 *   -DOTA_SEMANTIC_STRICT
 *
 * # Custom auto-check interval
 * build_flags =
 *   -DOTA_AUTH_MODE=PUBLIC_ACCESS
 *   -DOTA_FIRMWARE_VERSION='"1.0.0"'
 *   -DOTA_MANIFEST_URL='"https://your-cdn.cloudfront.net/manifest.json"'
 */