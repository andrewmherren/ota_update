#include <Arduino.h>
#include <unity.h>
#include <ota_update_module.h>

// ESP32 on-device tests for OTA Update Module
// These tests exercise hardware-dependent paths safely, tolerant of hardware presence

// Basic begin() smoke test - should not crash
void test_esp32_ota_module_begins_without_crash() {
  OTAUpdateModule module;
  
  // Begin with default config
  module.begin();
  
  // Should have a valid version
  String version = module.getCurrentVersion().c_str();
  TEST_ASSERT_TRUE(version.length() > 0);
}

// Handle() should not crash
void test_esp32_ota_module_handle_no_crash() {
  OTAUpdateModule module;
  module.begin();
  
  // Call handle multiple times
  module.handle();
  delay(100);
  module.handle();
  
  // No assumptions about update state, just ensure we didn't crash
  TEST_ASSERT_TRUE(true);
}

// Old native-only tests below (kept for reference but not used in ESP32 build)
#ifdef NATIVE_PLATFORM
#include <ArduinoFake.h>
#include <ArduinoJson.h>
#include <interface/core/web_request_core.h>
#include <interface/core/web_response_core.h>
#include <testing/mock_web_platform.h>
#include <ota_update_module.h>

using namespace fakeit;

// ===========================================================================
// Test Helpers
// ===========================================================================

static MockWebPlatformProvider* testProvider = nullptr;

void setUp() {
  // Reset ArduinoFake for each test
  ArduinoFakeReset();
  
  // Create test provider if not exists
  if (!testProvider) {
    testProvider = new MockWebPlatformProvider();
  }
}

// ===========================================================================
// Module Metadata Tests
// ===========================================================================

static void test_module_metadata() {
  OTAUpdateModule module(testProvider);
  
  TEST_ASSERT_EQUAL_STRING("OTA Update Manager", module.getModuleName().c_str());
  TEST_ASSERT_EQUAL_STRING("0.1.0", module.getModuleVersion().c_str());
  TEST_ASSERT_TRUE_MESSAGE(module.getModuleDescription().length() > 0,
                           "Description should be non-empty");
}

// ===========================================================================
// Lifecycle Tests
// ===========================================================================

static void test_begin_initialization() {
  OTAUpdateModule module(testProvider);
  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char *)))
      .AlwaysReturn(1);
  
  module.begin();
  
  std::string version = module.getCurrentVersion();
  TEST_ASSERT_TRUE(version.length() > 0);
  TEST_ASSERT_EQUAL_STRING("1.0.0-test", version.c_str()); // From platformio.ini
}

static void test_begin_with_config() {
  OTAUpdateModule module(testProvider);
  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char *)))
      .AlwaysReturn(1);
  
  DynamicJsonDocument config(256);
  config["auto_check"] = true;
  config["check_interval"] = 7200;
  
  module.begin(config.as<JsonVariant>());
  
  std::string version = module.getCurrentVersion();
  TEST_ASSERT_EQUAL_STRING("1.0.0-test", version.c_str());
}

static void test_handle_completes() {
  OTAUpdateModule module(testProvider);
  module.handle();
  TEST_ASSERT_TRUE(true);
}

// ===========================================================================
// Route Registration Tests
// ===========================================================================

static void test_routes_built_and_sizes() {
  OTAUpdateModule module(testProvider);
  auto http = module.getHttpRoutes();
  auto https = module.getHttpsRoutes();
  
  // Expected routes:
  // - 1 web page (/)
  // - 1 asset (ota-utils.js)
  // - 6 API routes (status, manifest, history, check, install, progress)
  // + 2 dev mode routes if OTA_DEVELOPMENT_MODE defined
#ifdef OTA_DEVELOPMENT_MODE
  TEST_ASSERT_EQUAL(10, http.size());
#else
  TEST_ASSERT_EQUAL(8, http.size());
#endif
  
  TEST_ASSERT_EQUAL(http.size(), https.size());
}

static void test_http_routes_structure() {
  OTAUpdateModule module(testProvider);
  auto routes = module.getHttpRoutes();
  
  for (const auto &route : routes) {
    TEST_ASSERT_TRUE(route.isWebRoute() || route.isApiRoute());
  }
}

// ===========================================================================
// Status and API Handler Tests
// ===========================================================================

static void test_status_api_returns_json() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  WebRequestCore req;
  WebResponseCore res;
  
  module.statusApiHandler(req, res);
  
  TEST_ASSERT_EQUAL_STRING("application/json", res.getMimeType().c_str());
  
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");
  
  TEST_ASSERT_TRUE(doc.containsKey("current_version"));
  TEST_ASSERT_TRUE(doc.containsKey("state"));
  TEST_ASSERT_TRUE(doc.containsKey("message"));
  TEST_ASSERT_TRUE(doc.containsKey("progress"));
}

static void test_manifest_api_before_check() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  WebRequestCore req;
  WebResponseCore res;
  
  module.manifestApiHandler(req, res);
  
  // Should return 404 when no manifest loaded
  TEST_ASSERT_EQUAL(404, res.getStatusCode());
}

static void test_history_api_returns_json() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  WebRequestCore req;
  WebResponseCore res;
  
  module.historyApiHandler(req, res);
  
  TEST_ASSERT_EQUAL_STRING("application/json", res.getMimeType().c_str());
  
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");
  
  TEST_ASSERT_TRUE(doc.containsKey("history"));
}

static void test_progress_api_returns_json() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  WebRequestCore req;
  WebResponseCore res;
  
  module.progressApiHandler(req, res);
  
  TEST_ASSERT_EQUAL_STRING("application/json", res.getMimeType().c_str());
  
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");
  
  TEST_ASSERT_TRUE(doc.containsKey("state"));
  TEST_ASSERT_TRUE(doc.containsKey("progress"));
}

// ===========================================================================
// Update API Handler Tests
// ===========================================================================

static void test_check_updates_handler_in_native() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  WebRequestCore req;
  WebResponseCore res;
  
  module.checkUpdatesHandler(req, res);
  
  // In native tests, checkForUpdates should fail (no HTTP client)
  DynamicJsonDocument doc(512);
  deserializeJson(doc, res.getContent());
  
  TEST_ASSERT_TRUE(doc.containsKey("success"));
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
}

static void test_install_update_handler_without_manifest() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  WebRequestCore req;
  WebResponseCore res;
  
  module.installUpdateHandler(req, res);
  
  // Should fail without manifest data
  DynamicJsonDocument doc(512);
  deserializeJson(doc, res.getContent());
  
  TEST_ASSERT_TRUE(doc.containsKey("success"));
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
}

// ===========================================================================
// Configuration Tests
// ===========================================================================

static void test_get_current_version() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  std::string version = module.getCurrentVersion();
  TEST_ASSERT_EQUAL_STRING("1.0.0-test", version.c_str());
}

static void test_get_update_status() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  UpdateStatus status = module.getUpdateStatus();
  TEST_ASSERT_EQUAL(UpdateStatus::IDLE, status.state);
  TEST_ASSERT_EQUAL(0, status.progress);
}

static void test_set_auto_check_interval() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  module.setAutoCheckInterval(3600);
  // No direct way to verify, but should not crash
  TEST_ASSERT_TRUE(true);
}

// ===========================================================================
// Development Mode Tests
// ===========================================================================

#ifdef OTA_DEVELOPMENT_MODE
static void test_rollback_handler_in_native() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  WebRequestCore req;
  WebResponseCore res;
  
  module.rollbackHandler(req, res);
  
  // In native tests, should return 501 (not implemented)
  TEST_ASSERT_EQUAL(501, res.getStatusCode());
  
  DynamicJsonDocument doc(512);
  deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
}

static void test_install_specific_handler_without_manifest() {
  OTAUpdateModule module(testProvider);
  module.begin();
  
  WebRequestCore req;
  WebResponseCore res;
  
  // Test with version parameter in URL
  // Note: WebRequestCore doesn't support path params easily in tests
  // This is a basic test to ensure handler doesn't crash
  module.installSpecificHandler(req, res);
  
  DynamicJsonDocument doc(512);
  deserializeJson(doc, res.getContent());
  
  // Should have success field
  TEST_ASSERT_TRUE(doc.containsKey("success"));
}
#endif

// ===========================================================================
// Test Registration
// ===========================================================================

void register_ota_module_tests() {
  // Metadata tests
  RUN_TEST(test_module_metadata);
  
  // Lifecycle tests
  RUN_TEST(test_begin_initialization);
  RUN_TEST(test_begin_with_config);
  RUN_TEST(test_handle_completes);
  
  // Route registration tests
  RUN_TEST(test_routes_built_and_sizes);
  RUN_TEST(test_http_routes_structure);
  
  // Handler tests
  RUN_TEST(test_status_api_returns_json);
  RUN_TEST(test_manifest_api_before_check);
  RUN_TEST(test_history_api_returns_json);
  RUN_TEST(test_progress_api_returns_json);
  
  // Update API tests
  RUN_TEST(test_check_updates_handler_in_native);
  RUN_TEST(test_install_update_handler_without_manifest);
  
  // Configuration tests
  RUN_TEST(test_get_current_version);
  RUN_TEST(test_get_update_status);
  RUN_TEST(test_set_auto_check_interval);
  
#ifdef OTA_DEVELOPMENT_MODE
  // Development mode tests
  RUN_TEST(test_rollback_handler_in_native);
  RUN_TEST(test_install_specific_handler_without_manifest);
#endif
}

#endif // NATIVE_PLATFORM
