#ifndef OTA_CONFIG_H
#define OTA_CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

// OTA Configuration constants
#define OTA_CONFIG_TABLE "ota_config"
#define OTA_HISTORY_TABLE "ota_history"

// Default configuration values
#define OTA_DEFAULT_AUTO_CHECK false
#define OTA_MAX_HISTORY_ENTRIES 10

// Authentication mode constants
#define OTA_AUTH_MODE_PUBLIC_ACCESS 0
#define OTA_AUTH_MODE_SHARED_KEY 1
#define OTA_AUTH_MODE_DEVICE_SPECIFIC 2
#define OTA_AUTH_MODE_SIGNED_URLS 3

// Legacy aliases (for backward compatibility)
#define PUBLIC_ACCESS OTA_AUTH_MODE_PUBLIC_ACCESS
#define SHARED_KEY OTA_AUTH_MODE_SHARED_KEY
#define DEVICE_SPECIFIC OTA_AUTH_MODE_DEVICE_SPECIFIC
#define SIGNED_URLS OTA_AUTH_MODE_SIGNED_URLS

// Required build-time configuration - manifest URL must be defined
#ifndef OTA_MANIFEST_URL
#error "OTA_MANIFEST_URL must be defined as a compile-time build flag"
#endif

// Configuration keys (manifest_url and check_interval removed - now
// compile-time/module config)
#define OTA_KEY_AUTO_CHECK_ENABLED "auto_check_enabled"
#define OTA_KEY_LAST_CHECK "last_check"
#define OTA_KEY_CURRENT_VERSION "current_version"

#if OTA_AUTH_MODE == SHARED_KEY
#define OTA_KEY_SHARED_KEY "shared_key"
#elif OTA_AUTH_MODE == DEVICE_SPECIFIC
#define OTA_KEY_DEVICE_ID "device_id"
#define OTA_KEY_DEVICE_KEY "device_key"
#endif

// Utility macros for development mode
#ifdef OTA_DEVELOPMENT_MODE
#define OTA_DEV_ONLY(x) x
#define OTA_DEV_LOG(msg) DEBUG_PRINTLN("[OTA-DEV] " + String(msg))
#else
#define OTA_DEV_ONLY(x)
#define OTA_DEV_LOG(msg)
#endif

// Semantic versioning enforcement
#ifdef OTA_SEMANTIC_STRICT
#define OTA_STRICT_VERSIONING true
#else
#define OTA_STRICT_VERSIONING false
#endif

#endif // OTA_CONFIG_H