#include "ota_update_module.h"
#include "ota_config.h"

#ifndef STANDALONE_TESTS
#include "platform_provider.h"
#endif

// Platform-specific includes
#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#endif

// Global instance for production builds
#if defined(ARDUINO) || defined(ESP_PLATFORM)
// NOSONAR - This module instance must be mutable as it maintains state and
// implements lifecycle methods
OTAUpdateModule otaUpdateModule; // NOSONAR
#endif

// ===========================================================================
// Constructor & Lifecycle
// ===========================================================================

OTAUpdateModule::OTAUpdateModule()
    : platformProvider(nullptr), autoCheckInterval(3600), lastCheckTime(0),
      autoCheckEnabled(OTA_DEFAULT_AUTO_CHECK), pendingInstall(false)
#if defined(ARDUINO) || defined(ESP_PLATFORM)
      ,
      httpsCapable(false)
#endif
{
#ifndef STANDALONE_TESTS
  // In production, use the global platform provider
  platformProvider = IWebPlatformProvider::instance;
#endif

#if OTA_AUTH_MODE == SHARED_KEY
#ifdef OTA_SHARED_KEY
  sharedKey = OTA_SHARED_KEY;
#endif
#endif
}

OTAUpdateModule::OTAUpdateModule(IWebPlatformProvider *provider)
    : platformProvider(provider), autoCheckInterval(3600), lastCheckTime(0),
      autoCheckEnabled(OTA_DEFAULT_AUTO_CHECK), pendingInstall(false)
#if defined(ARDUINO) || defined(ESP_PLATFORM)
      ,
      httpsCapable(false)
#endif
{
#if OTA_AUTH_MODE == SHARED_KEY
#ifdef OTA_SHARED_KEY
  sharedKey = OTA_SHARED_KEY;
#endif
#endif
}

void OTAUpdateModule::begin() {
  DEBUG_PRINTLN("OTA Update Module initializing...");

  // Load stored configuration
  loadStoredConfig();

#if defined(ARDUINO) || defined(ESP_PLATFORM)
  // Check HTTPS capability
  httpsCapable = true; // ESP32 supports HTTPS
#endif

  // Always use build-time version as the primary source
  // (esp_ota_get_app_description() returns ESP-IDF version, not firmware
  // version)
  currentVersion = String(OTA_FIRMWARE_VERSION);

  currentStatus.currentVersion = currentVersion;

  DEBUG_PRINTF("OTA initialized. Current version: %s\n",
               currentVersion.c_str());
  DEBUG_PRINTF("Manifest URL: %s\n", OTA_MANIFEST_URL);
  DEBUG_PRINTF("Auto-check: %s (interval: %d seconds)\n",
               autoCheckEnabled ? "enabled" : "disabled", autoCheckInterval);
}

void OTAUpdateModule::begin(const JsonVariant &config) {
  parseConfig(config);
  begin();
}

void OTAUpdateModule::handle() {
  // Process pending installation (async start)
  if (pendingInstall && currentStatus.state == UpdateStatus::IDLE) {
    DEBUG_PRINTF("OTA: Processing pending install for version: %s\n",
                 pendingInstallVersion.c_str());
    pendingInstall = false;
    installUpdate(pendingInstallVersion);
    return; // Exit after starting install to avoid timeout check
  }

  // Check for stuck operations (timeout after 5 minutes)
  if (currentStatus.state != UpdateStatus::IDLE &&
      currentStatus.state != UpdateStatus::COMPLETE &&
      currentStatus.startTime > 0 &&
      (millis() - currentStatus.startTime) > 300000) { // 5 minutes
    DEBUG_PRINTLN("OTA: Operation timeout - resetting to IDLE");
    updateStatus(UpdateStatus::ERROR, "Operation timed out");
    delay(100); // Brief delay to allow error state to be read
    updateStatus(UpdateStatus::IDLE, "Ready");
  }

  // Handle automatic update checks
  if (autoCheckEnabled &&
      (millis() - lastCheckTime) > (autoCheckInterval * 1000UL)) {

    if (currentStatus.state == UpdateStatus::IDLE) {
      DEBUG_PRINTLN("Performing automatic update check...");
      checkForUpdates();
    }
    lastCheckTime = millis();
  }
}

void OTAUpdateModule::parseConfig(const JsonVariant &config) {
  if (config.isNull()) {
    DEBUG_PRINTLN("OTA: Using default configuration");
    return;
  }

  // Manifest URL is now compile-time only - no runtime configuration needed

  if (config.containsKey("auto_check")) {
    autoCheckEnabled = config["auto_check"].as<bool>();
    DEBUG_PRINTF("OTA: Auto-check %s\n",
                 autoCheckEnabled ? "enabled" : "disabled");
  }

  if (config.containsKey("check_interval")) {
    autoCheckInterval = config["check_interval"].as<uint32_t>();
    DEBUG_PRINTF("OTA: Check interval: %d seconds\n", autoCheckInterval);
  }

#if OTA_AUTH_MODE == SHARED_KEY
  if (config.containsKey("shared_key")) {
    sharedKey = config["shared_key"].as<String>();
    DEBUG_PRINTLN("OTA: Shared key configured");
  }
#elif OTA_AUTH_MODE == DEVICE_SPECIFIC
  if (config.containsKey("device_id")) {
    deviceId = config["device_id"].as<String>();
    DEBUG_PRINTF("OTA: Device ID: %s\n", deviceId.c_str());
  }
  if (config.containsKey("device_key")) {
    deviceKey = config["device_key"].as<String>();
    DEBUG_PRINTLN("OTA: Device key configured");
  }
#endif
}

void OTAUpdateModule::loadStoredConfig() {
  // Load configuration from platform storage using JSON driver for simple
  // config
  try {
    IDatabaseDriver &driver = StorageManager::driver("json");
    QueryBuilder configQuery(&driver, OTA_CONFIG_TABLE);

    QueryBuilder autoCheckQuery(&driver, OTA_CONFIG_TABLE);
    String autoCheckData =
        autoCheckQuery.where("key", OTA_KEY_AUTO_CHECK_ENABLED).get();
    if (autoCheckData.length() > 0) {
      DynamicJsonDocument doc(256);
      if (deserializeJson(doc, autoCheckData) == DeserializationError::Ok) {
        autoCheckEnabled = doc["value"].as<bool>();
      }
    }

    // Check interval is now compile-time/module config only - no storage needed

    DEBUG_PRINTLN("OTA: Configuration loaded from storage");
  } catch (const std::exception &e) {
    DEBUG_PRINTF("OTA: Failed to load stored config: %s\n", e.what());
  }
}

void OTAUpdateModule::saveConfig() {
  // Save configuration to platform storage using JSON driver for simple config
  try {
    IDatabaseDriver &driver = StorageManager::driver("json");
    QueryBuilder configQuery(&driver, OTA_CONFIG_TABLE);

    DynamicJsonDocument autoCheckDoc(256);
    autoCheckDoc["key"] = OTA_KEY_AUTO_CHECK_ENABLED;
    autoCheckDoc["value"] = autoCheckEnabled;
    String autoCheckJson;
    serializeJson(autoCheckDoc, autoCheckJson);
    configQuery.store(OTA_KEY_AUTO_CHECK_ENABLED, autoCheckJson);

    DynamicJsonDocument lastCheckDoc(256);
    lastCheckDoc["key"] = OTA_KEY_LAST_CHECK;
    lastCheckDoc["value"] = (uint32_t)(millis() / 1000);
    String lastCheckJson;
    serializeJson(lastCheckDoc, lastCheckJson);
    configQuery.store(OTA_KEY_LAST_CHECK, lastCheckJson);

    DEBUG_PRINTLN("OTA: Configuration saved to storage");
  } catch (const std::exception &e) {
    DEBUG_PRINTF("OTA: Failed to save config: %s\n", e.what());
  }
}

void OTAUpdateModule::updateStatus(UpdateStatus::State state,
                                   const String &message, int progress) {
  currentStatus.state = state;
  currentStatus.message = message;

  if (progress >= 0) {
    currentStatus.progress = progress;
  }

  if (state == UpdateStatus::CHECKING || state == UpdateStatus::DOWNLOADING ||
      state == UpdateStatus::INSTALLING) {
    if (currentStatus.startTime == 0) {
      currentStatus.startTime = millis();
    }
  } else if (state == UpdateStatus::COMPLETE || state == UpdateStatus::ERROR ||
             state == UpdateStatus::IDLE) {
    currentStatus.startTime = 0;
  }

  // Call progress callback if set
  if (progressCallback) {
    progressCallback(currentStatus.progress, currentStatus.total, message);
  }

  DEBUG_PRINTF("OTA Status: %d - %s (%d%%)\n", state, message.c_str(),
               currentStatus.progress);
}

bool OTAUpdateModule::checkForUpdates() {
  if (currentStatus.state != UpdateStatus::IDLE) {
    DEBUG_PRINTLN("OTA: Update operation already in progress");
    return false;
  }

  updateStatus(UpdateStatus::CHECKING, "Fetching update manifest...");

#if defined(ARDUINO) || defined(ESP_PLATFORM)
  bool success = fetchManifest();
  if (success) {
    updateStatus(UpdateStatus::IDLE, manifestData.versions.size() > 0
                                         ? "Updates available"
                                         : "No updates available");
  } else {
    updateStatus(UpdateStatus::ERROR, "Failed to fetch manifest");
  }

  return success;
#else
  // In native tests, cannot fetch manifest without HTTP client
  DEBUG_PRINTLN("OTA: checkForUpdates not available in native tests");
  updateStatus(UpdateStatus::ERROR, "HTTP not available in test environment");
  return false;
#endif
}

#if defined(ARDUINO) || defined(ESP_PLATFORM)
bool OTAUpdateModule::fetchManifest() {
  // Build manifest URL with authentication (returns std::string)
  std::string urlStdStr =
      OTACore::buildManifestUrl(std::string(OTA_MANIFEST_URL), OTA_AUTH_MODE
#if OTA_AUTH_MODE == SHARED_KEY
                                ,
                                std::string(sharedKey.c_str())
#elif OTA_AUTH_MODE == DEVICE_SPECIFIC
                                ,
                                "", std::string(deviceId.c_str()),
                                std::string(deviceKey.c_str())
#else
                                ,
                                "", "", ""
#endif
      );

  String url = String(urlStdStr.c_str());
  DEBUG_PRINTF("OTA: Fetching manifest from %s\n", url.c_str());

  httpClient.begin(url);
  httpClient.setTimeout(10000); // 10 second timeout

  int httpCode = httpClient.GET();

  if (httpCode != HTTP_CODE_OK) {
    DEBUG_PRINTF("OTA: HTTP error %d fetching manifest\n", httpCode);
    httpClient.end();
    return false;
  }

  String payload = httpClient.getString();
  httpClient.end();

  // Parse manifest using core logic (convert Arduino String to std::string)
  manifestData = OTACore::parseManifest(
      std::string(payload.c_str()), std::string(currentVersion.c_str()),
#ifdef OTA_DEVELOPMENT_MODE
      true // Allow downgrades in development mode
#else
      false
#endif
  );

  if (!manifestData.isValid()) {
    DEBUG_PRINTLN("OTA: Failed to parse manifest");
    return false;
  }

  DEBUG_PRINTF("OTA: Parsed manifest with %d available versions\n",
               manifestData.versions.size());
  return true;
}
#endif // ARDUINO || ESP_PLATFORM

bool OTAUpdateModule::installUpdate(const String &version) {
  if (currentStatus.state != UpdateStatus::IDLE) {
    DEBUG_PRINTLN("OTA: Update already in progress");
    return false;
  }

  if (!manifestData.isValid()) {
    DEBUG_PRINTLN(
        "OTA: Manifest not loaded - attempting automatic fetch before install");
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    bool fetched = fetchManifest();
    if (!fetched || !manifestData.isValid()) {
      updateStatus(UpdateStatus::ERROR,
                   "No manifest available. Run check for updates first.");
      return false;
    }
#else
    updateStatus(UpdateStatus::ERROR,
                 "No manifest available. Run check for updates first.");
    return false;
#endif
  }

  // Use core logic to select the best version (returns version string)
  std::string selectedVersionStr = OTACore::selectVersion(
      manifestData, std::string(version.c_str()),
      std::string(currentVersion.c_str()),
      true // Allow major updates (can be made configurable)
  );

  if (selectedVersionStr.empty()) {
    updateStatus(UpdateStatus::ERROR, "Version not found or not installable");
    return false;
  }

  // Look up the full FirmwareVersion object from manifest
  bool found = false;
  for (const auto &fwVersion : manifestData.versions) {
    if (fwVersion.version == selectedVersionStr) {
      selectedVersion = fwVersion;
      found = true;
      break;
    }
  }

  if (!found) {
    updateStatus(UpdateStatus::ERROR, "Selected version not found in manifest");
    return false;
  }

  currentStatus.targetVersion = selectedVersion.version.c_str();

#if defined(ARDUINO) || defined(ESP_PLATFORM)
  return downloadAndInstall(selectedVersion);
#else
  // In native tests, cannot actually install
  DEBUG_PRINTLN("OTA: installUpdate not available in native tests");
  updateStatus(UpdateStatus::ERROR,
               "Installation not available in test environment");
  return false;
#endif
}

#if defined(ARDUINO) || defined(ESP_PLATFORM)
bool OTAUpdateModule::downloadAndInstall(const FirmwareVersion &version) {
  // Phase 1: Downloading (state = DOWNLOADING). We stream and write directly to
  // flash, but we still present this to the UI as a download phase so users see
  // progress before we switch to an installation/finalization phase.
  updateStatus(
      UpdateStatus::DOWNLOADING,
      "Downloading firmware " + String(version.version.c_str()) + "...", 0);

  DEBUG_PRINTF("OTA: Downloading %s from %s\n", version.version.c_str(),
               version.url.c_str());

  httpClient.begin(String(version.url.c_str()));
  httpClient.setTimeout(30000); // 30 second timeout

  // TODO: Add certificate validation for HTTPS

  int httpCode = httpClient.GET();
  if (httpCode != HTTP_CODE_OK) {
    DEBUG_PRINTF("OTA: HTTP error %d downloading firmware\n", httpCode);
    String errorMsg = "Download failed: HTTP " + String(httpCode);
    updateStatus(UpdateStatus::ERROR, errorMsg);
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    httpClient.end();
    return false;
  }

  int contentLength = httpClient.getSize();
  if (contentLength <= 0 || contentLength > 10 * 1024 * 1024) { // Max 10MB
    String errorMsg =
        contentLength <= 0 ? "Invalid content length" : "Firmware too large";
    updateStatus(UpdateStatus::ERROR, errorMsg);
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    httpClient.end();
    return false;
  }

  // Validate content length matches manifest (if provided)
  if (version.size_bytes > 0 && (uint32_t)contentLength != version.size_bytes) {
    DEBUG_PRINTF("OTA: Size mismatch - expected %u, got %d\n",
                 version.size_bytes, contentLength);
    String errorMsg = "Size mismatch with manifest";
    updateStatus(UpdateStatus::ERROR, errorMsg);
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    httpClient.end();
    return false;
  }

  currentStatus.total = contentLength;

  // Check available OTA partition space
  const esp_partition_t *update_partition =
      esp_ota_get_next_update_partition(NULL);
  if (update_partition == nullptr) {
    String errorMsg = "No OTA partition available";
    updateStatus(UpdateStatus::ERROR, errorMsg);
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    httpClient.end();
    return false;
  }

  if (update_partition->size < (size_t)contentLength) {
    String errorMsg = "Not enough space in OTA partition";
    updateStatus(UpdateStatus::ERROR, errorMsg);
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    httpClient.end();
    return false;
  }

  // Begin OTA update
  if (!Update.begin(contentLength)) {
    String errorMsg = "OTA begin failed: " + String(Update.errorString());
    updateStatus(UpdateStatus::ERROR, errorMsg);
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    httpClient.end();
    return false;
  }

  // Keep reporting as DOWNLOADING while streaming bytes. Switch to INSTALLING
  // only after the full image has been received and verified, right before
  // finalizing/activating the new partition.

  // Initialize SHA256 context for verification
  mbedtls_sha256_context sha256_ctx;
  mbedtls_sha256_init(&sha256_ctx);
  mbedtls_sha256_starts(&sha256_ctx, 0); // 0 = SHA256 (not SHA224)

  // Download and write firmware with hash calculation
  WiFiClient *stream = httpClient.getStreamPtr();
  size_t written = 0;
  uint8_t buffer[1024];
  bool writeError = false;
  int lastReportedProgress = -1;

  DEBUG_PRINTF("OTA: Starting download loop, contentLength=%d\n",
               contentLength);

  while (httpClient.connected() && written < (size_t)contentLength &&
         !writeError) {
    size_t available = stream->available();
    if (available) {
      size_t bytesToRead = min(available, sizeof(buffer));
      size_t bytesRead = stream->readBytes(buffer, bytesToRead);

      if (bytesRead == 0) {
        delay(10);
        continue;
      }

      // Update SHA256 hash
      mbedtls_sha256_update(&sha256_ctx, buffer, bytesRead);

      // Write to flash
      if (Update.write(buffer, bytesRead) != bytesRead) {
        writeError = true;
        break;
      }

      written += bytesRead;
      int progress = (written * 100) / contentLength;

      // Update status when progress percentage changes
      if (progress != lastReportedProgress) {
        DEBUG_PRINTF("OTA: Download progress: %d%% (%zu/%d bytes)\n", progress,
                     written, contentLength);
        updateStatus(UpdateStatus::DOWNLOADING,
                     "Downloading firmware... " + String(progress) + "%",
                     progress);
        lastReportedProgress = progress;
      }
    } else {
      delay(10); // Small delay if no data available
    }
  }

  DEBUG_PRINTF("OTA: Download loop complete, written=%zu, contentLength=%d\n",
               written, contentLength);

  httpClient.end();

  // Check for write errors
  if (writeError) {
    mbedtls_sha256_free(&sha256_ctx);
    String errorMsg = "Flash write failed";
    updateStatus(UpdateStatus::ERROR, errorMsg);
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    Update.abort();
    return false;
  }

  // Check for incomplete download
  if (written != (size_t)contentLength) {
    mbedtls_sha256_free(&sha256_ctx);
    String errorMsg =
        "Incomplete download: " + String(written) + "/" + String(contentLength);
    updateStatus(UpdateStatus::ERROR, errorMsg);
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    Update.abort();
    return false;
  }

  // Phase 2: Completed data transfer; finalize SHA256 hash
  uint8_t hash[32];
  mbedtls_sha256_finish(&sha256_ctx, hash);
  mbedtls_sha256_free(&sha256_ctx);

  // Convert hash to hex string
  char hashStr[65];
  for (int i = 0; i < 32; i++) {
    sprintf(&hashStr[i * 2], "%02x", hash[i]);
  }
  hashStr[64] = '\0';

  // Verify SHA256 hash matches manifest (convert std::string to Arduino String
  // for comparison)
  String expectedHash = String(version.sha256.c_str());
  if (!expectedHash.equalsIgnoreCase(hashStr)) {
    String errorMsg = "SHA256 verification failed - expected: " + expectedHash +
                      ", got: " + String(hashStr);
    DEBUG_PRINTLN(errorMsg);
    updateStatus(UpdateStatus::ERROR, "Hash verification failed");
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    Update.abort();
    return false;
  }

  DEBUG_PRINTLN("OTA: SHA256 verification passed");

  // Phase 3: Installation / finalization
  updateStatus(UpdateStatus::INSTALLING, "Installing firmware...", 95);
  if (!Update.end(true)) { // true = set new firmware as boot partition
    String errorMsg =
        "Update finalization failed: " + String(Update.errorString());
    updateStatus(UpdateStatus::ERROR, "Finalization failed");
    storeUpdateHistory(String(version.version.c_str()), false, errorMsg);
    return false;
  }

  // Store successful update history
  storeUpdateHistory(String(version.version.c_str()), true);

  updateStatus(UpdateStatus::COMPLETE, "Update completed! Rebooting...", 100);

  DEBUG_PRINTF("OTA: Successfully installed version %s\n",
               version.version.c_str());

  // Schedule reboot
  delay(2000);
  ESP.restart();

  return true;
}

bool OTAUpdateModule::verifyFirmware(const uint8_t *data, size_t length,
                                     const String &expectedHash) {
  // Calculate SHA256 hash
  uint8_t hash[32];
  mbedtls_sha256(data, length, hash, 0); // 0 = SHA256

  // Convert to hex string
  char hashStr[65];
  for (size_t i = 0; i < 32; i++) {
    sprintf(&hashStr[i * 2], "%02x", hash[i]);
  }
  hashStr[64] = '\0';

  // Compare with expected hash (case-insensitive)
  bool matches = expectedHash.equalsIgnoreCase(hashStr);

  if (!matches) {
    DEBUG_PRINTF("OTA: Hash mismatch - expected: %s, got: %s\n",
                 expectedHash.c_str(), hashStr);
  }

  return matches;
}
#endif // ARDUINO || ESP_PLATFORM

void OTAUpdateModule::storeUpdateHistory(const String &version, bool success,
                                         const String &error) {
  try {
    // Use LittleFS driver for larger JSON documents like update history
    IDatabaseDriver &driver = StorageManager::driver("littlefs");
    QueryBuilder historyQuery(&driver, OTA_HISTORY_TABLE);

    String updateId = String(millis());
    DynamicJsonDocument doc(512);
    doc["version"] = version;

    // Get Unix timestamp (seconds since epoch) from platform
    // Use platform's time synchronization (NTP) for reliable timestamps
    unsigned long timestamp = 0;
    if (platformProvider) {
      timestamp = platformProvider->getPlatform().getCurrentTime();
      if (timestamp == 0 ||
          !platformProvider->getPlatform().isTimeSynchronized()) {
        // Time not synchronized yet, use millis as fallback
        timestamp = millis() / 1000;
        DEBUG_PRINTF("OTA: Time not synchronized, using millis fallback\n");
      }
    } else {
      // No platform provider (shouldn't happen), use millis
      timestamp = millis() / 1000;
    }
    doc["installed_at"] = (uint32_t)timestamp;

    doc["success"] = success;
    doc["previous_version"] = currentVersion;
    if (!error.isEmpty()) {
      doc["error"] = error;
    }

    String historyJson;
    serializeJson(doc, historyJson);
    historyQuery.store(updateId, historyJson);

    DEBUG_PRINTF("OTA: Update history stored: %s -> %s (%s)\n",
                 currentVersion.c_str(), version.c_str(),
                 success ? "success" : "failed");

    // TODO: Implement history cleanup (keep only last 10)
  } catch (const std::exception &e) {
    DEBUG_PRINTF("OTA: Failed to store history: %s\n", e.what());
  }
}

JsonArray OTAUpdateModule::getUpdateHistory() {
  DynamicJsonDocument doc(1024);
  JsonArray history = doc.to<JsonArray>();

  try {
    // Load history from LittleFS storage for larger JSON documents
    IDatabaseDriver &driver = StorageManager::driver("littlefs");
    QueryBuilder historyQuery(&driver, OTA_HISTORY_TABLE);
    std::vector<String> records = historyQuery.getAll();

    for (const String &record : records) {
      if (record.length() > 0) {
        DynamicJsonDocument recordDoc(512);
        if (deserializeJson(recordDoc, record) == DeserializationError::Ok) {
          JsonObject entry = history.createNestedObject();
          entry.set(recordDoc.as<JsonObject>());
        }
      }
    }

    DEBUG_PRINTF("OTA: Loaded %d history entries\n", history.size());
  } catch (const std::exception &e) {
    DEBUG_PRINTF("OTA: Failed to load history: %s\n", e.what());
  }

  return history;
}

#ifdef OTA_DEVELOPMENT_MODE
void OTAUpdateModule::installSpecificHandler(WebRequest &req,
                                             WebResponse &res) {
  String version = req.getParam("version");

  if (version.isEmpty()) {
    res.setStatus(400);
    respondJson(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "Version parameter required";
    });
    return;
  }

  bool success = installUpdate(version);

  respondJson(res, [&](JsonObject &json) {
    json["success"] = success;
    if (success) {
      json["message"] = "Download started for version " + version;
      json["state"] = static_cast<int>(currentStatus.state); // DOWNLOADING
    } else {
      json["error"] = currentStatus.message;
      json["state"] = static_cast<int>(currentStatus.state);
    }
  });
}
#endif

// Configuration methods (manifest URL removed - now compile-time only)

void OTAUpdateModule::setAutoCheckInterval(uint32_t seconds) {
  autoCheckInterval = seconds;
  // Note: Check interval changes are not persisted - they reset on reboot
  DEBUG_PRINTF("OTA: Check interval updated to %d seconds (temporary)\n",
               seconds);
}

void OTAUpdateModule::setProgressCallback(ProgressCallback callback) {
  progressCallback = callback;
}