#include "ota_core.h"
#include "ota_config.h"
#include <algorithm>

// Debug macros (can be disabled by not defining DEBUG_PRINTF)
#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(...)
#endif
#ifndef DEBUG_PRINTLN
#define DEBUG_PRINTLN(...)
#endif

// ===========================================================================
// Manifest Parsing
// ===========================================================================

ManifestData OTACore::parseManifest(const std::string &json,
                                    const std::string &currentVersion,
                                    bool allowOlder) {
  ManifestData result;

  DynamicJsonDocument doc(4096); // Increased size for larger manifests
  DeserializationError error = deserializeJson(doc, json.c_str());

  if (error) {
    DEBUG_PRINTF("OTA Core: Failed to parse manifest JSON: %s\n",
                 error.c_str());
    return result; // Return invalid manifest
  }

  // Validate manifest structure
  if (!doc.containsKey("versions") || !doc["versions"].is<JsonObject>()) {
    DEBUG_PRINTLN("OTA Core: Manifest missing 'versions' object");
    return result;
  }

  if (!doc.containsKey("latest")) {
    DEBUG_PRINTLN("OTA Core: Manifest missing 'latest' field");
    return result;
  }

  // Get latest version
  const char *latest = doc["latest"];
  if (latest == nullptr || latest[0] == '\0') {
    DEBUG_PRINTLN("OTA Core: Manifest 'latest' field is empty");
    return result;
  }

  result.latestVersion = latest;
  JsonObject versions = doc["versions"];

  // Parse each version in the manifest
  for (JsonPair versionPair : versions) {
    std::string versionStr = versionPair.key().c_str();

    // Validate version string format
    if (!VersionManager::isValidVersion(versionStr)) {
      DEBUG_PRINTF("OTA Core: Skipping invalid version: %s\n",
                   versionStr.c_str());
      continue;
    }

    JsonObject versionData = versionPair.value();
    FirmwareVersion version = parseVersionObject(versionStr, versionData);

    if (!version.isValid()) {
      DEBUG_PRINTF("OTA Core: Skipping incomplete version: %s\n",
                   versionStr.c_str());
      continue;
    }

    // Filter versions based on policy
    if (canInstallVersion(currentVersion, versionStr, allowOlder)) {
      result.versions.push_back(version);
    } else {
      DEBUG_PRINTF(
          "OTA Core: Version %s filtered (current: %s, allowOlder: %d)\n",
          versionStr.c_str(), currentVersion.c_str(), allowOlder);
    }
  }

  DEBUG_PRINTF(
      "OTA Core: Parsed manifest with %d installable versions (latest: %s)\n",
      (int)result.versions.size(), result.latestVersion.c_str());

  return result;
}

FirmwareVersion OTACore::parseVersionObject(const std::string &versionStr,
                                            const JsonObject &versionData) {
  FirmwareVersion version;
  version.version = versionStr;

  // Required fields
  if (versionData.containsKey("url")) {
    const char *url = versionData["url"];
    version.url = url ? url : "";
  }

  if (versionData.containsKey("sha256")) {
    const char *sha256 = versionData["sha256"];
    version.sha256 = sha256 ? sha256 : "";
  }

  // Optional fields with defaults
  const char *released = versionData["released"];
  version.released = released ? released : "";

  version.size_bytes = versionData["size_bytes"] | 0;

  const char *notes = versionData["notes"];
  version.notes = notes ? notes : "";

  version.breaking = versionData["breaking"] | false;

  // Validate required fields
  if (!isValidFirmwareURL(version.url, true)) {
    DEBUG_PRINTF("OTA Core: Invalid firmware URL for %s: %s\n",
                 versionStr.c_str(), version.url.c_str());
    version.url = ""; // Mark as invalid
  }

  if (!isValidSHA256(version.sha256)) {
    DEBUG_PRINTF("OTA Core: Invalid SHA256 for %s: %s\n", versionStr.c_str(),
                 version.sha256.c_str());
    version.sha256 = ""; // Mark as invalid
  }

  return version;
}

// ===========================================================================
// Version Selection
// ===========================================================================

std::string OTACore::selectVersion(const ManifestData &manifest,
                                   const std::string &targetVersion,
                                   const std::string &currentVersion,
                                   bool allowMajorUpdates) {
  if (!manifest.isValid()) {
    DEBUG_PRINTLN("OTA Core: Cannot select from invalid manifest");
    return "";
  }

  // If specific version requested, validate it exists
  if (!targetVersion.empty()) {
    for (const auto &version : manifest.versions) {
      if (version.version == targetVersion) {
        DEBUG_PRINTF("OTA Core: Selected requested version: %s\n",
                     targetVersion.c_str());
        return targetVersion;
      }
    }
    DEBUG_PRINTF("OTA Core: Requested version %s not found in manifest\n",
                 targetVersion.c_str());
    return ""; // Not found
  }

  // Otherwise, select best available version
  std::vector<std::string> versionStrs;
  for (const auto &v : manifest.versions) {
    versionStrs.push_back(v.version);
  }

  std::string bestVersionStr = VersionManager::getNextAllowedVersion(
      currentVersion, versionStrs, allowMajorUpdates);

  if (!bestVersionStr.empty()) {
    DEBUG_PRINTF("OTA Core: Selected best version: %s\n",
                 bestVersionStr.c_str());
  } else {
    DEBUG_PRINTLN("OTA Core: No suitable version found");
  }

  return bestVersionStr;
}

bool OTACore::canInstallVersion(const std::string &currentVersion,
                                const std::string &targetVersion,
                                bool developmentMode) {
  // Development mode allows any version (including downgrades)
  if (developmentMode) {
    return true;
  }

  // Production mode: only allow newer versions
  return VersionManager::isNewer(currentVersion, targetVersion);
}

// ===========================================================================
// Validation Helpers
// ===========================================================================

bool OTACore::isValidSHA256(const std::string &hash) {
  if (hash.length() != 64) {
    return false; // SHA256 is always 64 hex characters
  }

  // Check if all characters are valid hex
  for (char c : hash) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'))) {
      return false;
    }
  }

  return true;
}

bool OTACore::isValidFirmwareURL(const std::string &url, bool requireHttps) {
  if (url.empty()) {
    return false;
  }

  // Check for valid URL scheme
  if (url.find("https://") == 0) {
    return true;
  }

  if (url.find("http://") == 0) {
    if (requireHttps) {
      DEBUG_PRINTF("OTA Core: HTTP URL rejected (HTTPS required): %s\n",
                   url.c_str());
      return false; // Reject HTTP in production for security
    }
    return true; // Allow HTTP if explicitly permitted (testing only)
  }

  DEBUG_PRINTF("OTA Core: Invalid URL scheme: %s\n", url.c_str());
  return false; // No valid scheme
}

std::string OTACore::buildManifestUrl(const std::string &baseUrl, int authMode,
                                      const std::string &sharedKey,
                                      const std::string &deviceId,
                                      const std::string &deviceKey) {
  std::string url = baseUrl;

  // Add authentication parameters based on mode
  switch (authMode) {
  case OTA_AUTH_MODE_PUBLIC_ACCESS:
    // No authentication needed
    break;

  case OTA_AUTH_MODE_SHARED_KEY:
    if (!sharedKey.empty()) {
      url += (url.find('?') == std::string::npos) ? "?" : "&";
      url += "key=" + sharedKey;
    }
    break;

  case OTA_AUTH_MODE_DEVICE_SPECIFIC:
    if (!deviceId.empty() && !deviceKey.empty()) {
      url += (url.find('?') == std::string::npos) ? "?" : "&";
      url += "device_id=" + deviceId + "&device_key=" + deviceKey;
    }
    break;

  case OTA_AUTH_MODE_SIGNED_URLS:
    // Signed URLs are pre-authenticated, no modification needed
    break;

  default:
    DEBUG_PRINTF("OTA Core: Unknown auth mode: %d\n", authMode);
    break;
  }

  return url;
}
