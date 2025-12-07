#ifndef OTA_CORE_H
#define OTA_CORE_H

#include <string>
#include <vector>
#include <ArduinoJson.h>
#include "ota_version.h"

// Forward declare Arduino String for conversion helpers
#if !defined(STANDALONE_TESTS)
class String;
#endif

/**
 * @brief Platform-agnostic OTA core logic
 * 
 * This class contains all business logic that doesn't require Arduino/ESP32 APIs.
 * Uses std::string for testability in native environments.
 * Arduino String conversion happens at module boundaries.
 */

struct FirmwareVersion {
  std::string version;
  std::string url;
  std::string sha256;
  std::string released;
  uint32_t size_bytes;
  std::string notes;
  bool breaking;

  FirmwareVersion() : size_bytes(0), breaking(false) {}
  
  bool isValid() const {
    return !version.empty() && !url.empty() && !sha256.empty();
  }
};

struct ManifestData {
  std::string latestVersion;
  std::vector<FirmwareVersion> versions;
  
  bool isValid() const {
    // Manifest is valid if it has latestVersion (structure is valid)
    // versions can be empty if all versions are filtered out
    return !latestVersion.empty();
  }
  
  void clear() {
    latestVersion = "";
    versions.clear();
  }
};

/**
 * @brief Core OTA logic without platform dependencies
 * 
 * All methods are static to emphasize stateless design and testability.
 */
class OTACore {
public:
  /**
   * @brief Parse manifest JSON into structured data
   * @param json Raw JSON string from manifest file
   * @param currentVersion Current firmware version (for filtering)
   * @param allowOlder True in development mode to allow downgrades
   * @return Parsed manifest data, or invalid manifest on error
   */
  static ManifestData parseManifest(const std::string &json, 
                                    const std::string &currentVersion,
                                    bool allowOlder = false);
  
  /**
   * @brief Select best version to install based on policy
   * @param manifest Available versions from manifest
   * @param targetVersion Specific version requested (empty = latest)
   * @param currentVersion Current firmware version
   * @param allowMajorUpdates Whether to allow major version jumps
   * @return Version string to install, or empty string if none suitable
   */
  static std::string selectVersion(const ManifestData &manifest,
                                   const std::string &targetVersion,
                                   const std::string &currentVersion,
                                   bool allowMajorUpdates = true);
  
  /**
   * @brief Check if a version can be installed given current version and policy
   * @param currentVersion Current firmware version
   * @param targetVersion Version to check
   * @param developmentMode Whether dev mode is enabled (allows downgrades)
   * @return True if version can be installed
   */
  static bool canInstallVersion(const std::string &currentVersion,
                                const std::string &targetVersion,
                                bool developmentMode);
  
  /**
   * @brief Validate SHA256 hash format
   * @param hash Hash string to validate
   * @return True if hash is valid hex string of correct length (64 chars)
   */
  static bool isValidSHA256(const std::string &hash);
  
  /**
   * @brief Validate firmware URL format
   * @param url URL to validate
   * @param requireHttps Whether to require HTTPS (default true for security)
   * @return True if URL is valid and meets security requirements
   */
  static bool isValidFirmwareURL(const std::string &url, bool requireHttps = true);
  
  /**
   * @brief Build manifest URL with authentication parameters
   * @param baseUrl Base manifest URL
   * @param authMode Authentication mode
   * @param sharedKey Shared key (for SHARED_KEY mode)
   * @param deviceId Device ID (for DEVICE_SPECIFIC mode)
   * @param deviceKey Device key (for DEVICE_SPECIFIC mode)
   * @return Complete URL with auth parameters
   */
  static std::string buildManifestUrl(const std::string &baseUrl,
                                      int authMode,
                                      const std::string &sharedKey = "",
                                      const std::string &deviceId = "",
                                      const std::string &deviceKey = "");

private:
  // Helper to parse a single version object from JSON
  static FirmwareVersion parseVersionObject(const std::string &versionStr, 
                                            const JsonObject &versionData);
};

#endif // OTA_CORE_H
