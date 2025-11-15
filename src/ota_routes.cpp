#include "../assets/ota_utils_js.h"
#include "ota_update_module.h"

// ===========================================================================
// Route Registration
// ===========================================================================

std::vector<RouteVariant> OTAUpdateModule::getHttpRoutes() {
  return {
      // Main status page
      WebRoute("/", WebModule::WM_GET,
               [this](RequestT &req, ResponseT &res) {
                 statusPageHandler(req, res);
               },
               {AuthType::SESSION}),

      // JavaScript assets
      WebRoute("/assets/ota-utils.js", WebModule::WM_GET,
               [](RequestT &req, ResponseT &res) {
                 res.setProgmemContent(OTA_UTILS_JS, "application/javascript");
                 res.setHeader("Cache-Control", "public, max-age=3600");
               },
               {AuthType::NONE}),

      // API endpoints
      ApiRoute(
          "/api/status", WebModule::WM_GET,
          [this](RequestT &req, ResponseT &res) {
            statusApiHandler(req, res);
          },
          {AuthType::SESSION, AuthType::TOKEN},
          API_DOC("Get OTA update status",
                  "Returns current OTA status, available versions, and update "
                  "progress",
                  "getOTAStatus", {"ota", "firmware"})),

      ApiRoute(
          "/api/manifest", WebModule::WM_GET,
          [this](RequestT &req, ResponseT &res) {
            manifestApiHandler(req, res);
          },
          {AuthType::SESSION, AuthType::TOKEN},
          API_DOC("Get firmware manifest",
                  "Returns the firmware manifest with available versions",
                  "getFirmwareManifest", {"ota", "firmware"})),

      ApiRoute(
          "/api/history", WebModule::WM_GET,
          [this](RequestT &req, ResponseT &res) {
            historyApiHandler(req, res);
          },
          {AuthType::SESSION, AuthType::TOKEN},
          API_DOC("Get update history", "Returns history of firmware updates",
                  "getUpdateHistory", {"ota", "firmware"})),

      ApiRoute(
          "/api/check", WebModule::WM_POST,
          [this](RequestT &req, ResponseT &res) {
            checkUpdatesHandler(req, res);
          },
          {AuthType::SESSION, AuthType::TOKEN},
          API_DOC("Check for updates",
                  "Manually trigger a check for firmware updates",
                  "checkForUpdates", {"ota", "firmware"})),

      ApiRoute(
          "/api/install", WebModule::WM_POST,
          [this](RequestT &req, ResponseT &res) {
            installUpdateHandler(req, res);
          },
          {AuthType::SESSION, AuthType::TOKEN},
          API_DOC("Install firmware update",
                  "Start installation of available firmware update",
                  "installFirmwareUpdate", {"ota", "firmware"})
              .withRequestBody(
                  R"raw({
                        "content": {
                            "application/json": {
                                "schema": {
                                    "type": "object",
                                    "properties": {
                                        "version": {
                                            "type": "string",
                                            "description": "Specific version to install (optional, defaults to latest)"
                                        }
                                    }
                                }
                            }
                        }
                    })raw")
              .withRequestExample(R"({"version": "2.1.0"})")
              .withResponseExample(
                  R"({"success": true, "message": "Update installation started"})")),

      ApiRoute(
          "/api/progress", WebModule::WM_GET,
          [this](RequestT &req, ResponseT &res) {
            progressApiHandler(req, res);
          },
          {AuthType::SESSION, AuthType::TOKEN},
          API_DOC("Get installation progress",
                  "Returns real-time progress of firmware installation",
                  "getInstallationProgress", {"ota", "firmware"}))

#ifdef OTA_DEVELOPMENT_MODE
          ,
      ApiRoute(
          "/api/rollback", WebModule::WM_POST,
          [this](RequestT &req, ResponseT &res) {
            rollbackHandler(req, res);
          },
          {AuthType::SESSION},
          API_DOC("Rollback firmware",
                  "Rollback to previous firmware version (development only)",
                  "rollbackFirmware", {"ota", "firmware", "development"})),

      ApiRoute(
          "/api/install/{version}", WebModule::WM_POST,
          [this](RequestT &req, ResponseT &res) {
            installSpecificHandler(req, res);
          },
          {AuthType::SESSION},
          API_DOC("Install specific version",
                  "Install any available version (development only)",
                  "installSpecificVersion", {"ota", "firmware", "development"}))
#endif
  };
}

std::vector<RouteVariant> OTAUpdateModule::getHttpsRoutes() {
  return getHttpRoutes();
}