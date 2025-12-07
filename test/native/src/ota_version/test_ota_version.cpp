#include <unity.h>

#ifdef NATIVE_PLATFORM
#include <ArduinoFake.h>
#include <ota_version.h>

using namespace fakeit;

// ===========================================================================
// Version Parsing Tests
// ===========================================================================

static void test_parse_simple_version() {
  SemanticVersion v = VersionManager::parseVersion("1.2.3");
  TEST_ASSERT_EQUAL(1, v.major);
  TEST_ASSERT_EQUAL(2, v.minor);
  TEST_ASSERT_EQUAL(3, v.patch);
  TEST_ASSERT_TRUE(v.prerelease.empty());
  TEST_ASSERT_TRUE(v.build.empty());
}

static void test_parse_version_with_prerelease() {
  SemanticVersion v = VersionManager::parseVersion("2.0.0-alpha.1");
  TEST_ASSERT_EQUAL(2, v.major);
  TEST_ASSERT_EQUAL(0, v.minor);
  TEST_ASSERT_EQUAL(0, v.patch);
  TEST_ASSERT_EQUAL_STRING("alpha.1", v.prerelease.c_str());
  TEST_ASSERT_TRUE(v.build.empty());
}

static void test_parse_version_with_build() {
  SemanticVersion v = VersionManager::parseVersion("1.0.0+build.123");
  TEST_ASSERT_EQUAL(1, v.major);
  TEST_ASSERT_EQUAL(0, v.minor);
  TEST_ASSERT_EQUAL(0, v.patch);
  TEST_ASSERT_TRUE(v.prerelease.empty());
  TEST_ASSERT_EQUAL_STRING("build.123", v.build.c_str());
}

static void test_parse_version_with_prerelease_and_build() {
  SemanticVersion v = VersionManager::parseVersion("3.1.0-rc.2+20240115");
  TEST_ASSERT_EQUAL(3, v.major);
  TEST_ASSERT_EQUAL(1, v.minor);
  TEST_ASSERT_EQUAL(0, v.patch);
  TEST_ASSERT_EQUAL_STRING("rc.2", v.prerelease.c_str());
  TEST_ASSERT_EQUAL_STRING("20240115", v.build.c_str());
}

static void test_parse_major_only() {
  SemanticVersion v = VersionManager::parseVersion("5");
  TEST_ASSERT_EQUAL(5, v.major);
  TEST_ASSERT_EQUAL(0, v.minor);
  TEST_ASSERT_EQUAL(0, v.patch);
}

static void test_parse_major_minor_only() {
  SemanticVersion v = VersionManager::parseVersion("2.4");
  TEST_ASSERT_EQUAL(2, v.major);
  TEST_ASSERT_EQUAL(4, v.minor);
  TEST_ASSERT_EQUAL(0, v.patch);
}

// ===========================================================================
// Version Comparison Tests
// ===========================================================================

static void test_compare_equal_versions() {
  int result = VersionManager::compareVersions("1.2.3", "1.2.3");
  TEST_ASSERT_EQUAL(0, result);
}

static void test_compare_patch_difference() {
  TEST_ASSERT_EQUAL(-1, VersionManager::compareVersions("1.2.3", "1.2.4"));
  TEST_ASSERT_EQUAL(1, VersionManager::compareVersions("1.2.5", "1.2.3"));
}

static void test_compare_minor_difference() {
  TEST_ASSERT_EQUAL(-1, VersionManager::compareVersions("1.2.3", "1.3.0"));
  TEST_ASSERT_EQUAL(1, VersionManager::compareVersions("1.5.0", "1.2.9"));
}

static void test_compare_major_difference() {
  TEST_ASSERT_EQUAL(-1, VersionManager::compareVersions("1.9.9", "2.0.0"));
  TEST_ASSERT_EQUAL(1, VersionManager::compareVersions("3.0.0", "2.5.8"));
}

static void test_compare_prerelease_vs_release() {
  // Prerelease versions are lower than release versions
  TEST_ASSERT_EQUAL(-1, VersionManager::compareVersions("1.0.0-alpha", "1.0.0"));
  TEST_ASSERT_EQUAL(1, VersionManager::compareVersions("1.0.0", "1.0.0-beta"));
}

static void test_compare_prerelease_versions() {
  // Two prerelease versions should be comparable
  int result = VersionManager::compareVersions("1.0.0-alpha", "1.0.0-beta");
  TEST_ASSERT_NOT_EQUAL(0, result); // Should not be equal
}

static void test_build_metadata_ignored() {
  // Build metadata should not affect comparison (per semver spec)
  // Note: Current implementation might not handle this correctly
  int result = VersionManager::compareVersions("1.0.0+build1", "1.0.0+build2");
  TEST_ASSERT_EQUAL(0, result);
}

// ===========================================================================
// Helper Method Tests
// ===========================================================================

static void test_is_newer_true() {
  TEST_ASSERT_TRUE(VersionManager::isNewer("1.0.0", "1.0.1"));
  TEST_ASSERT_TRUE(VersionManager::isNewer("1.0.0", "1.1.0"));
  TEST_ASSERT_TRUE(VersionManager::isNewer("1.0.0", "2.0.0"));
}

static void test_is_newer_false() {
  TEST_ASSERT_FALSE(VersionManager::isNewer("1.0.1", "1.0.0"));
  TEST_ASSERT_FALSE(VersionManager::isNewer("1.0.0", "1.0.0"));
  TEST_ASSERT_FALSE(VersionManager::isNewer("2.0.0", "1.9.9"));
}

static void test_is_major_update_true() {
  TEST_ASSERT_TRUE(VersionManager::isMajorUpdate("1.5.3", "2.0.0"));
  TEST_ASSERT_TRUE(VersionManager::isMajorUpdate("1.0.0", "3.0.0"));
}

static void test_is_major_update_false() {
  TEST_ASSERT_FALSE(VersionManager::isMajorUpdate("1.0.0", "1.5.0"));
  TEST_ASSERT_FALSE(VersionManager::isMajorUpdate("2.1.0", "2.2.0"));
  TEST_ASSERT_FALSE(VersionManager::isMajorUpdate("2.0.0", "1.9.9")); // Downgrade
}

static void test_is_valid_version() {
  TEST_ASSERT_TRUE(VersionManager::isValidVersion("1.2.3"));
  TEST_ASSERT_TRUE(VersionManager::isValidVersion("0.0.1"));
  TEST_ASSERT_TRUE(VersionManager::isValidVersion("10.20.30"));
  TEST_ASSERT_TRUE(VersionManager::isValidVersion("1.0.0-alpha"));
  TEST_ASSERT_FALSE(VersionManager::isValidVersion(""));
}

// ===========================================================================
// Next Allowed Version Tests
// ===========================================================================

static void test_get_next_allowed_version_basic() {
  std::vector<std::string> versions = {"1.0.1", "1.1.0", "2.0.0"};
  
  std::string next = VersionManager::getNextAllowedVersion("1.0.0", versions, true);
  TEST_ASSERT_EQUAL_STRING("2.0.0", next.c_str()); // Should return newest
}

static void test_get_next_allowed_version_no_major() {
  std::vector<std::string> versions = {"1.0.1", "1.1.0", "2.0.0", "2.1.0"};
  
  std::string next = VersionManager::getNextAllowedVersion("1.0.0", versions, false);
  TEST_ASSERT_EQUAL_STRING("1.1.0", next.c_str()); // Should skip major updates
}

static void test_get_next_allowed_version_empty() {
  std::vector<std::string> versions;
  
  std::string next = VersionManager::getNextAllowedVersion("1.0.0", versions, true);
  TEST_ASSERT_TRUE(next.empty());
}

static void test_get_next_allowed_version_no_newer() {
  std::vector<std::string> versions = {"0.9.0", "1.0.0"};
  
  std::string next = VersionManager::getNextAllowedVersion("2.0.0", versions, true);
  TEST_ASSERT_TRUE(next.empty()); // All versions are older
}

// ===========================================================================
// Edge Cases and Error Handling
// ===========================================================================

static void test_parse_malformed_version() {
  SemanticVersion v = VersionManager::parseVersion("not-a-version");
  TEST_ASSERT_EQUAL(0, v.major);
  TEST_ASSERT_EQUAL(0, v.minor);
  TEST_ASSERT_EQUAL(0, v.patch);
}

static void test_parse_empty_string() {
  SemanticVersion v = VersionManager::parseVersion("");
  TEST_ASSERT_EQUAL(0, v.major);
  TEST_ASSERT_EQUAL(0, v.minor);
  TEST_ASSERT_EQUAL(0, v.patch);
}

static void test_toString_round_trip() {
  std::string original = "1.2.3-alpha+build";
  SemanticVersion v = VersionManager::parseVersion(original);
  std::string result = v.toString();
  TEST_ASSERT_EQUAL_STRING(original.c_str(), result.c_str());
}

// ===========================================================================
// Test Registration
// ===========================================================================

void register_ota_version_tests() {
  // Parsing tests
  RUN_TEST(test_parse_simple_version);
  RUN_TEST(test_parse_version_with_prerelease);
  RUN_TEST(test_parse_version_with_build);
  RUN_TEST(test_parse_version_with_prerelease_and_build);
  RUN_TEST(test_parse_major_only);
  RUN_TEST(test_parse_major_minor_only);
  
  // Comparison tests
  RUN_TEST(test_compare_equal_versions);
  RUN_TEST(test_compare_patch_difference);
  RUN_TEST(test_compare_minor_difference);
  RUN_TEST(test_compare_major_difference);
  RUN_TEST(test_compare_prerelease_vs_release);
  RUN_TEST(test_compare_prerelease_versions);
  RUN_TEST(test_build_metadata_ignored);
  
  // Helper method tests
  RUN_TEST(test_is_newer_true);
  RUN_TEST(test_is_newer_false);
  RUN_TEST(test_is_major_update_true);
  RUN_TEST(test_is_major_update_false);
  RUN_TEST(test_is_valid_version);
  
  // Next allowed version tests
  RUN_TEST(test_get_next_allowed_version_basic);
  RUN_TEST(test_get_next_allowed_version_no_major);
  RUN_TEST(test_get_next_allowed_version_empty);
  RUN_TEST(test_get_next_allowed_version_no_newer);
  
  // Edge cases
  RUN_TEST(test_parse_malformed_version);
  RUN_TEST(test_parse_empty_string);
  RUN_TEST(test_toString_round_trip);
}

#endif // NATIVE_PLATFORM
