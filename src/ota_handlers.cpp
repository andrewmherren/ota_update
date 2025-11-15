#include "../assets/ota_status_html.h"
#include "ota_update_module.h"

// ===========================================================================
// Route Handlers Implementation
// ===========================================================================

void OTAUpdateModule::statusPageHandler(RequestT &req, ResponseT &res) {
  res.setProgmemContent(OTA_STATUS_HTML, "text/html");
}

void OTAUpdateModule::statusApiHandler(RequestT &req, ResponseT &res) {
  respondJson(res, [this](JsonObject &json) {
    json["current_version"] = currentVersion;
    json["state"] = (int)currentStatus.state;
    json["message"] = currentStatus.message;
    json["progress"] = currentStatus.progress;
    json["total"] = currentStatus.total;

    // Include last check timestamp (seconds since device boot)
    if (lastCheckTime > 0) {
      json["last_check"] = (uint32_t)(lastCheckTime / 1000);
    } else {
      json["last_check"] = 0; // Never checked
    }

    if (!manifestData.latestVersion.empty()) {
      json["latest_version"] = manifestData.latestVersion.c_str();
      json["updates_available"] = manifestData.versions.size();
    }

    if (currentStatus.state != UpdateStatus::IDLE &&
        currentStatus.targetVersion.length() > 0) {
      json["target_version"] = currentStatus.targetVersion.c_str();
    }

    JsonArray versions = json.createNestedArray("available_versions");
    for (const auto &version : manifestData.versions) {
      JsonObject v = versions.createNestedObject();
      v["version"] = version.version.c_str();
      v["released"] = version.released.c_str();
      v["notes"] = version.notes.c_str();
      v["size_bytes"] = version.size_bytes;
      v["breaking"] = version.breaking;
    }
  });
}

void OTAUpdateModule::manifestApiHandler(RequestT &req, ResponseT &res) {
  if (!manifestData.isValid() || manifestData.versions.empty()) {
    res.setStatus(404);
    respondJson(res, [](JsonObject &json) {
      json["error"] =
          "No manifest data available. Run check for updates first.";
    });
    return;
  }

  respondJson(res, [this](JsonObject &json) {
    json["latest"] = manifestData.latestVersion;

    JsonObject versions = json.createNestedObject("versions");
    for (const auto &version : manifestData.versions) {
      JsonObject v = versions.createNestedObject(version.version.c_str());
      v["url"] = version.url.c_str();
      v["sha256"] = version.sha256.c_str();
      v["released"] = version.released.c_str();
      v["size_bytes"] = version.size_bytes;
      v["notes"] = version.notes.c_str();
      if (version.breaking) {
        v["breaking"] = true;
      }
    }
  });
}

void OTAUpdateModule::historyApiHandler(RequestT &req, ResponseT &res) {
  JsonArray history = getUpdateHistory();

  respondJson(res, [&history](JsonObject &json) { json["history"] = history; });
}

void OTAUpdateModule::checkUpdatesHandler(RequestT &req, ResponseT &res) {
  bool success = checkForUpdates();

  respondJson(res, [this, success](JsonObject &json) {
    json["success"] = success;
    if (success) {
      json["message"] = manifestData.versions.size() > 0
                            ? "Updates found"
                            : "No updates available";
      json["updates_available"] = manifestData.versions.size();
    } else {
      json["message"] = currentStatus.message;
    }
  });
}

void OTAUpdateModule::installUpdateHandler(RequestT &req, ResponseT &res) {
  // Parse version from request body if provided
  String targetVersion = "";
  String body = req.getBody();
  if (!body.isEmpty()) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, body.c_str());
    if (!error && doc.containsKey("version")) {
      targetVersion = doc["version"].as<String>();
    }
  }

  // Check if we're already busy
  if (currentStatus.state != UpdateStatus::IDLE) {
    respondJson(res, [this](JsonObject &json) {
      json["success"] = false;
      json["message"] = "Update operation already in progress";
    });
    return;
  }

  // Check if manifest is available
  if (!manifestData.isValid()) {
    respondJson(res, [](JsonObject &json) {
      json["success"] = false;
      json["message"] = "No manifest available. Run check for updates first.";
    });
    return;
  }

  // Queue the installation for async processing
  pendingInstallVersion = targetVersion;
  pendingInstall = true;

  DEBUG_PRINTF("OTA: Installation queued for version: %s\n",
               targetVersion.isEmpty() ? "latest" : targetVersion.c_str());

  // Return immediately - installation will happen in handle() loop
  respondJson(res, [targetVersion](JsonObject &json) {
    json["success"] = true;
    json["message"] = "Update installation queued";
    if (!targetVersion.isEmpty()) {
      json["target_version"] = targetVersion.c_str();
    }
  });
}

void OTAUpdateModule::progressApiHandler(RequestT &req, ResponseT &res) {
  respondJson(res, [this](JsonObject &json) {
    json["state"] = (int)currentStatus.state;
    json["progress"] = currentStatus.progress;
    json["total"] = currentStatus.total;
    json["message"] = currentStatus.message;

    if (currentStatus.startTime > 0) {
      json["elapsed_time"] = (millis() - currentStatus.startTime) / 1000;
    }

    DEBUG_PRINTF("OTA Progress API: state=%d, progress=%d, total=%d, msg=%s\n",
                 (int)currentStatus.state, currentStatus.progress,
                 currentStatus.total, currentStatus.message.c_str());
  });
}

#ifdef OTA_DEVELOPMENT_MODE
#if defined(ARDUINO) || defined(ESP_PLATFORM)
void OTAUpdateModule::rollbackHandler(RequestT &req, ResponseT &res) {
  // SECURITY FIX: Use esp_ota_get_next_update_partition() instead of
  // esp_ota_get_last_invalid_partition() to get the previous partition
  const esp_partition_t *current_partition = esp_ota_get_running_partition();
  const esp_partition_t *update_partition =
      esp_ota_get_next_update_partition(NULL);

  if (update_partition != nullptr && update_partition != current_partition) {
    esp_err_t err = esp_ota_set_boot_partition(update_partition);

    if (err == ESP_OK) {
      respondJson(res, [](JsonObject &json) {
        json["success"] = true;
        json["message"] = "Rollback initiated. Rebooting...";
      });

      delay(2000);
      ESP.restart();
    } else {
      res.setStatus(500);
      respondJson(res, [err](JsonObject &json) {
        json["success"] = false;
        json["error"] =
            "Failed to set boot partition: " + String(esp_err_to_name(err));
      });
    }
  } else {
    res.setStatus(400);
    respondJson(res, [](JsonObject &json) {
      json["success"] = false;
      json["error"] =
          "No valid previous firmware partition available for rollback";
    });
  }
}
#else
void OTAUpdateModule::rollbackHandler(RequestT &req, ResponseT &res) {
  res.setStatus(501);
  respondJson(res, [](JsonObject &json) {
    json["success"] = false;
    json["error"] = "Rollback not available in test environment";
  });
}
#endif // ARDUINO || ESP_PLATFORM
#endif // OTA_DEVELOPMENT_MODE