#ifndef OTA_UPDATE_MODULE_H
#define OTA_UPDATE_MODULE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <interface/auth_types.h>
#include <interface/request_response_types.h>
#include <interface/openapi_factory.h>
#include <interface/openapi_types.h>
#include <interface/utils/route_variant.h>
#include <interface/web_module_interface.h>
#include <web_platform_interface.h>
#include <utility>
#include "ota_core.h"
#include "ota_version.h"
#include "version_autogen.h"

// Compile-time check: ensure version was injected
#ifndef WEB_MODULE_VERSION_STR
#error "WEB_MODULE_VERSION_STR not defined (version_autogen.h missing)."
#endif

// Platform-specific includes (only in Arduino/ESP32 builds)
#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>

// Storage APIs from web_platform (available in ESP32 builds with full platform)
// These are only used in loadStoredConfig(), saveConfig(), and storeUpdateHistory()
#include <storage/storage_manager.h>
#include <storage/query_builder.h>
#include <storage/database_driver_interface.h>
#endif

// OTA Authentication modes (build-time selection)
#ifndef OTA_AUTH_MODE
#define OTA_AUTH_MODE PUBLIC_ACCESS // Default to public access
#endif

enum class OTAAuthMode {
  PUBLIC_ACCESS,
  SHARED_KEY,
  DEVICE_SPECIFIC,
  SIGNED_URLS
};

// Build configuration
// OTA_FIRMWARE_VERSION must be defined in platformio.ini build_flags
// Example: -DOTA_FIRMWARE_VERSION='"1.2.3"'
#ifndef OTA_FIRMWARE_VERSION
#error                                                                         \
    "OTA_FIRMWARE_VERSION must be defined in build_flags. Add -DOTA_FIRMWARE_VERSION='\"x.y.z\"' to platformio.ini"
#endif

// Auto-check interval is defined in ota_config.h

// Progress callback function type
typedef std::function<void(int progress, int total, String status)>
    ProgressCallback;

// UpdateStatus - tracks state of ongoing update operations
struct UpdateStatus {
  enum State {
    IDLE,
    CHECKING,
    DOWNLOADING,
    INSTALLING,
    COMPLETE,
    ERROR,
    ROLLBACK
  };

  State state;
  int progress;
  int total;
  String message;
  String error;
  String currentVersion;
  String targetVersion;
  unsigned long startTime;

  UpdateStatus() : state(IDLE), progress(0), total(0), startTime(0) {}
};

/**
 * @brief OTA Update Module - Over-the-air firmware update system
 * 
 * Provides secure OTA updates with semantic versioning, multiple authentication
 * modes, and comprehensive update management via web interface and REST API.
 */
class OTAUpdateModule : public IWebModule {
public:
  // Default constructor - uses global provider instance
  OTAUpdateModule();
  
  // Optional constructor for dependency injection (tests)
  explicit OTAUpdateModule(IWebPlatformProvider *provider);
  
  ~OTAUpdateModule() override = default;

  // Module lifecycle
  using IWebModule::begin; // Bring base class overloads into scope
  void begin() override;
  void begin(const JsonVariant &config) override;
  void handle() override;

  // IWebModule interface implementation
  std::vector<RouteVariant> getHttpRoutes() override;
  std::vector<RouteVariant> getHttpsRoutes() override;
  String getModuleName() const override { return "OTA Update Manager"; }
  String getModuleVersion() const override { return WEB_MODULE_VERSION_STR; }
  String getModuleDescription() const override {
    return "Over-the-air firmware update management with semantic versioning";
  }

  // Public API methods
  bool checkForUpdates();
  bool installUpdate(const String &version = "");
  UpdateStatus getUpdateStatus() const { return currentStatus; }
  String getCurrentVersion() const { return currentVersion; }

  // Configuration methods (manifest URL removed - now compile-time)
  void setAutoCheckInterval(uint32_t seconds); // Temporary change only (not persisted)
  void setProgressCallback(ProgressCallback callback);

private:
  // Platform provider (injected or global)
  IWebPlatformProvider *platformProvider;

  // Helper to access the platform
  IWebPlatform &getPlatform() const { return platformProvider->getPlatform(); }

  // Helper to reduce platform lookup duplication when creating JSON responses
  template <typename Fn>
  inline void respondJson(ResponseT &res, Fn &&fn) const {
    IWebPlatformProvider::getPlatformInstance().createJsonResponse(
        res, std::forward<Fn>(fn));
  }

  // Configuration
  String currentVersion;
  uint32_t autoCheckInterval; // Set from JSON config, defaults to 3600 (1 hour)
  unsigned long lastCheckTime;
  bool autoCheckEnabled;
  ProgressCallback progressCallback;

#if OTA_AUTH_MODE == SHARED_KEY
  String sharedKey;
#elif OTA_AUTH_MODE == DEVICE_SPECIFIC
  String deviceId;
  String deviceKey;
#endif

  // Update status
  UpdateStatus currentStatus;
  ManifestData manifestData; // Parsed manifest with available versions
  FirmwareVersion selectedVersion;

#if defined(ARDUINO) || defined(ESP_PLATFORM)
  // HTTP client - only available on Arduino/ESP32
  HTTPClient httpClient;
  bool httpsCapable;
#endif

  // Internal methods
  void parseConfig(const JsonVariant &config);
  void loadStoredConfig();
  void saveConfig();
  void updateStatus(UpdateStatus::State state, const String &message,
                    int progress = -1);

  // Update process methods (platform-specific)
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  bool fetchManifest();
  bool downloadAndInstall(const FirmwareVersion &version);
  bool verifyFirmware(const uint8_t *data, size_t length,
                      const String &expectedHash);
#endif

  // Storage methods
  void storeUpdateHistory(const String &version, bool success,
                          const String &error = "");
  JsonArray getUpdateHistory();

  // Route handlers (unified signatures for ESP32 and native)
  void statusPageHandler(RequestT &req, ResponseT &res);
  void statusApiHandler(RequestT &req, ResponseT &res);
  void manifestApiHandler(RequestT &req, ResponseT &res);
  void historyApiHandler(RequestT &req, ResponseT &res);
  void checkUpdatesHandler(RequestT &req, ResponseT &res);
  void installUpdateHandler(RequestT &req, ResponseT &res);
  void progressApiHandler(RequestT &req, ResponseT &res);

#ifdef OTA_DEVELOPMENT_MODE
  void rollbackHandler(RequestT &req, ResponseT &res);
  void installSpecificHandler(RequestT &req, ResponseT &res);
#endif
};

// Global instance for production builds
// NOSONAR - This module instance must be mutable as it maintains state and
// implements lifecycle methods
#if defined(ARDUINO) || defined(ESP_PLATFORM)
extern OTAUpdateModule otaUpdateModule; // NOSONAR
#endif

#endif // OTA_UPDATE_MODULE_H