#include <unity.h>

#ifdef NATIVE_PLATFORM
#include <ArduinoFake.h>
#include <ArduinoJson.h>
#include <ota_core.h>
#include <ota_config.h>

using namespace fakeit;

// ===========================================================================
// Test Manifest JSON
// ===========================================================================

const char* VALID_MANIFEST = R"({
  "product": "test-device",
  "latest": "2.0.0",
  "versions": {
    "1.5.0": {
      "url": "https://cdn.example.com/firmware/v1.5.0.bin",
      "sha256": "abcd1234567890abcd1234567890abcd1234567890abcd1234567890abcd1234",
      "released": "2024-01-01T10:00:00Z",
      "size_bytes": 1048576,
      "notes": "Bug fixes"
    },
    "2.0.0": {
      "url": "https://cdn.example.com/firmware/v2.0.0.bin",
      "sha256": "1234abcd567890abcd1234567890abcd1234567890abcd1234567890abcd1234",
      "released": "2024-01-15T10:00:00Z",
      "size_bytes": 1153433,
      "notes": "Major update",
      "breaking": true
    }
  }
})";

const char* INVALID_JSON = "{ not valid json }";

const char* MISSING_LATEST = R"({
  "versions": {
    "1.0.0": {
      "url": "https://cdn.example.com/firmware/v1.0.0.bin",
      "sha256": "abcd1234567890abcd1234567890abcd1234567890abcd1234567890abcd1234"
    }
  }
})";

const char* HTTP_URL_MANIFEST = R"({
  "latest": "1.0.0",
  "versions": {
    "1.0.0": {
      "url": "http://insecure.example.com/firmware.bin",
      "sha256": "abcd1234567890abcd1234567890abcd1234567890abcd1234567890abcd1234",
      "size_bytes": 1048576
    }
  }
})";

const char* INVALID_HASH_MANIFEST = R"({
  "latest": "1.0.0",
  "versions": {
    "1.0.0": {
      "url": "https://cdn.example.com/firmware.bin",
      "sha256": "not-a-valid-hash",
      "size_bytes": 1048576
    }
  }
})";

// ===========================================================================
// Manifest Parsing Tests
// ===========================================================================

static void test_parse_valid_manifest() {
  ManifestData manifest = OTACore::parseManifest(VALID_MANIFEST, "1.0.0", false);
  
  TEST_ASSERT_TRUE(manifest.isValid());
  TEST_ASSERT_EQUAL_STRING("2.0.0", manifest.latestVersion.c_str());
  TEST_ASSERT_EQUAL(2, manifest.versions.size());
  
  // Check first version
  const FirmwareVersion& v1 = manifest.versions[0];
  TEST_ASSERT_EQUAL_STRING("1.5.0", v1.version.c_str());
  TEST_ASSERT_TRUE(v1.url.find("https://") == 0);
  TEST_ASSERT_EQUAL(64, v1.sha256.length());
  TEST_ASSERT_EQUAL(1048576, v1.size_bytes);
  TEST_ASSERT_FALSE(v1.breaking);
}

static void test_parse_invalid_json() {
  ManifestData manifest = OTACore::parseManifest(INVALID_JSON, "1.0.0", false);
  
  TEST_ASSERT_FALSE(manifest.isValid());
  TEST_ASSERT_EQUAL(0, manifest.versions.size());
}

static void test_parse_missing_latest() {
  ManifestData manifest = OTACore::parseManifest(MISSING_LATEST, "1.0.0", false);
  
  TEST_ASSERT_FALSE(manifest.isValid());
}

static void test_parse_filters_older_versions() {
  // Current version is 1.5.0, should only get 2.0.0
  ManifestData manifest = OTACore::parseManifest(VALID_MANIFEST, "1.5.0", false);
  
  TEST_ASSERT_TRUE(manifest.isValid());
  TEST_ASSERT_EQUAL(1, manifest.versions.size()); // Only 2.0.0
  TEST_ASSERT_EQUAL_STRING("2.0.0", manifest.versions[0].version.c_str());
}

static void test_parse_allows_older_in_dev_mode() {
  // Development mode allows downgrades
  ManifestData manifest = OTACore::parseManifest(VALID_MANIFEST, "3.0.0", true);
  
  TEST_ASSERT_TRUE(manifest.isValid());
  TEST_ASSERT_EQUAL(2, manifest.versions.size()); // Both versions
}

static void test_parse_rejects_http_urls() {
  ManifestData manifest = OTACore::parseManifest(HTTP_URL_MANIFEST, "0.9.0", false);
  
  // Manifest parses but version is invalid due to HTTP URL
  TEST_ASSERT_TRUE(manifest.isValid()); // Manifest structure is valid
  TEST_ASSERT_EQUAL(0, manifest.versions.size()); // But version filtered out
}

static void test_parse_rejects_invalid_hashes() {
  ManifestData manifest = OTACore::parseManifest(INVALID_HASH_MANIFEST, "0.9.0", false);
  
  TEST_ASSERT_TRUE(manifest.isValid()); // Manifest structure valid
  TEST_ASSERT_EQUAL(0, manifest.versions.size()); // Version filtered out
}

// ===========================================================================
// Version Selection Tests
// ===========================================================================

static void test_select_specific_version() {
  ManifestData manifest = OTACore::parseManifest(VALID_MANIFEST, "1.0.0", false);
  
  std::string selected = OTACore::selectVersion(manifest, "1.5.0", "1.0.0", true);
  
  TEST_ASSERT_FALSE(selected.empty());
  TEST_ASSERT_EQUAL_STRING("1.5.0", selected.c_str());
}

static void test_select_latest_when_no_version_specified() {
  ManifestData manifest = OTACore::parseManifest(VALID_MANIFEST, "1.0.0", false);
  
  std::string selected = OTACore::selectVersion(manifest, "", "1.0.0", true);
  
  TEST_ASSERT_FALSE(selected.empty());
  TEST_ASSERT_EQUAL_STRING("2.0.0", selected.c_str()); // Latest
}

static void test_select_fails_for_nonexistent_version() {
  ManifestData manifest = OTACore::parseManifest(VALID_MANIFEST, "1.0.0", false);
  
  std::string selected = OTACore::selectVersion(manifest, "3.0.0", "1.0.0", true);
  
  TEST_ASSERT_TRUE(selected.empty());
}

static void test_select_skips_major_update_when_not_allowed() {
  ManifestData manifest = OTACore::parseManifest(VALID_MANIFEST, "1.0.0", false);
  
  std::string selected = OTACore::selectVersion(manifest, "", "1.0.0", false);
  
  TEST_ASSERT_FALSE(selected.empty());
  TEST_ASSERT_EQUAL_STRING("1.5.0", selected.c_str()); // Skip 2.0.0
}

static void test_select_from_invalid_manifest() {
  ManifestData manifest; // Empty/invalid
  
  std::string selected = OTACore::selectVersion(manifest, "", "1.0.0", true);
  
  TEST_ASSERT_TRUE(selected.empty());
}

// ===========================================================================
// canInstallVersion Tests
// ===========================================================================

static void test_can_install_newer_version() {
  bool can = OTACore::canInstallVersion("1.0.0", "1.1.0", false);
  TEST_ASSERT_TRUE(can);
}

static void test_cannot_install_older_version_in_production() {
  bool can = OTACore::canInstallVersion("2.0.0", "1.5.0", false);
  TEST_ASSERT_FALSE(can);
}

static void test_cannot_install_same_version_in_production() {
  bool can = OTACore::canInstallVersion("1.5.0", "1.5.0", false);
  TEST_ASSERT_FALSE(can);
}

#ifdef OTA_DEVELOPMENT_MODE
static void test_can_install_any_version_in_dev_mode() {
  bool can = OTACore::canInstallVersion("2.0.0", "1.0.0", true);
  TEST_ASSERT_TRUE(can);
  
  can = OTACore::canInstallVersion("1.0.0", "1.0.0", true);
  TEST_ASSERT_TRUE(can);
}
#endif

// ===========================================================================
// Validation Helper Tests
// ===========================================================================

static void test_valid_sha256() {
  TEST_ASSERT_TRUE(OTACore::isValidSHA256(
      "abcd1234567890abcd1234567890abcd1234567890abcd1234567890abcd1234"));
  TEST_ASSERT_TRUE(OTACore::isValidSHA256(
      "ABCD1234567890ABCD1234567890ABCD1234567890ABCD1234567890ABCD1234"));
}

static void test_invalid_sha256_length() {
  TEST_ASSERT_FALSE(OTACore::isValidSHA256("abcd1234")); // Too short
  TEST_ASSERT_FALSE(OTACore::isValidSHA256(
      "abcd1234567890abcd1234567890abcd1234567890abcd1234567890abcd1234extra")); // Too long
  TEST_ASSERT_FALSE(OTACore::isValidSHA256("")); // Empty
}

static void test_invalid_sha256_characters() {
  TEST_ASSERT_FALSE(OTACore::isValidSHA256(
      "zzzz1234567890abcd1234567890abcd1234567890abcd1234567890abcd1234")); // Invalid char
  TEST_ASSERT_FALSE(OTACore::isValidSHA256(
      "abcd1234567890abcd1234567890abcd1234567890abcd1234567890abcd123!")); // Special char
}

static void test_valid_firmware_url() {
  TEST_ASSERT_TRUE(OTACore::isValidFirmwareURL("https://cdn.example.com/firmware.bin", true));
  TEST_ASSERT_TRUE(OTACore::isValidFirmwareURL("https://s3.amazonaws.com/bucket/firmware.bin", true));
}

static void test_http_url_rejected_when_https_required() {
  TEST_ASSERT_FALSE(OTACore::isValidFirmwareURL("http://insecure.com/firmware.bin", true));
}

static void test_http_url_allowed_when_not_required() {
  TEST_ASSERT_TRUE(OTACore::isValidFirmwareURL("http://insecure.com/firmware.bin", false));
}

static void test_invalid_url_schemes() {
  TEST_ASSERT_FALSE(OTACore::isValidFirmwareURL("ftp://server.com/firmware.bin", true));
  TEST_ASSERT_FALSE(OTACore::isValidFirmwareURL("firmware.bin", true));
  TEST_ASSERT_FALSE(OTACore::isValidFirmwareURL("", true));
}

// ===========================================================================
// buildManifestUrl Tests
// ===========================================================================

static void test_build_public_access_url() {
  std::string url = OTACore::buildManifestUrl(
      "https://cdn.example.com/manifest.json",
      PUBLIC_ACCESS,
      "", "", ""
  );
  
  TEST_ASSERT_EQUAL_STRING("https://cdn.example.com/manifest.json", url.c_str());
}

static void test_build_shared_key_url() {
  std::string url = OTACore::buildManifestUrl(
      "https://cdn.example.com/manifest.json",
      SHARED_KEY,
      "my-secret-key",
      "", ""
  );
  
  TEST_ASSERT_TRUE(url.find("?key=my-secret-key") != std::string::npos);
}

static void test_build_device_specific_url() {
  std::string url = OTACore::buildManifestUrl(
      "https://api.example.com/manifest.json",
      DEVICE_SPECIFIC,
      "",
      "device-123",
      "device-secret"
  );
  
  TEST_ASSERT_TRUE(url.find("device_id=device-123") != std::string::npos);
  TEST_ASSERT_TRUE(url.find("device_key=device-secret") != std::string::npos);
}

static void test_build_url_with_existing_query_params() {
  std::string url = OTACore::buildManifestUrl(
      "https://cdn.example.com/manifest.json?product=test",
      SHARED_KEY,
      "my-key",
      "", ""
  );
  
  TEST_ASSERT_TRUE(url.find("&key=my-key") != std::string::npos);
  TEST_ASSERT_TRUE(url.find("?key=") == std::string::npos);
}

// ===========================================================================
// Test Registration
// ===========================================================================

void register_ota_core_tests() {
  // Manifest parsing tests
  RUN_TEST(test_parse_valid_manifest);
  RUN_TEST(test_parse_invalid_json);
  RUN_TEST(test_parse_missing_latest);
  RUN_TEST(test_parse_filters_older_versions);
  RUN_TEST(test_parse_allows_older_in_dev_mode);
  RUN_TEST(test_parse_rejects_http_urls);
  RUN_TEST(test_parse_rejects_invalid_hashes);
  
  // Version selection tests
  RUN_TEST(test_select_specific_version);
  RUN_TEST(test_select_latest_when_no_version_specified);
  RUN_TEST(test_select_fails_for_nonexistent_version);
  RUN_TEST(test_select_skips_major_update_when_not_allowed);
  RUN_TEST(test_select_from_invalid_manifest);
  
  // canInstallVersion tests
  RUN_TEST(test_can_install_newer_version);
  RUN_TEST(test_cannot_install_older_version_in_production);
  RUN_TEST(test_cannot_install_same_version_in_production);
#ifdef OTA_DEVELOPMENT_MODE
  RUN_TEST(test_can_install_any_version_in_dev_mode);
#endif
  
  // Validation helpers
  RUN_TEST(test_valid_sha256);
  RUN_TEST(test_invalid_sha256_length);
  RUN_TEST(test_invalid_sha256_characters);
  RUN_TEST(test_valid_firmware_url);
  RUN_TEST(test_http_url_rejected_when_https_required);
  RUN_TEST(test_http_url_allowed_when_not_required);
  RUN_TEST(test_invalid_url_schemes);
  
  // buildManifestUrl tests
  RUN_TEST(test_build_public_access_url);
  RUN_TEST(test_build_shared_key_url);
  RUN_TEST(test_build_device_specific_url);
  RUN_TEST(test_build_url_with_existing_query_params);
}

#endif // NATIVE_PLATFORM
