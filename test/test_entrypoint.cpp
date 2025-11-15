#include <unity.h>

// Native tests with ArduinoFake mocks
#ifdef NATIVE_PLATFORM
#include <ArduinoFake.h>
using namespace fakeit;

// Forward declarations for native test registration
extern void register_ota_version_tests();
extern void register_ota_core_tests();

extern "C" void setUp(void) {
  // Reset ArduinoFake state before each test
  ArduinoFakeReset();
}

extern "C" void tearDown(void) {
  // Clean up after each test (if needed)
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  
  // Register and run native tests (core logic only)
  register_ota_version_tests();
  register_ota_core_tests();
  
  UNITY_END();
  return 0;
}

// ESP32 on-device tests
#else
#include <Arduino.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

// Forward declarations for ESP32 hardware tests
// Define these in test/esp32/src/*.cpp files
void test_esp32_ota_module_begins_without_crash();
void test_esp32_ota_module_handle_no_crash();

void setup() {
  // Allow USB CDC/Serial to enumerate
  delay(2000);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  UNITY_BEGIN();
  // Give the serial monitor a moment to attach
  delay(500);
  
  // Run ESP32 hardware tests
  RUN_TEST(test_esp32_ota_module_begins_without_crash);
  RUN_TEST(test_esp32_ota_module_handle_no_crash);
  
  UNITY_END();
}

void loop() {
  // Tests run once in setup()
}
#endif
