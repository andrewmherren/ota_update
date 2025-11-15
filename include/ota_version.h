#ifndef OTA_VERSION_H
#define OTA_VERSION_H

#include <string>
#include <vector>

struct SemanticVersion {
  int major;
  int minor;
  int patch;
  std::string prerelease;
  std::string build;

  SemanticVersion() : major(0), minor(0), patch(0) {}
  SemanticVersion(int maj, int min, int pat)
      : major(maj), minor(min), patch(pat) {}

  std::string toString() const {
    std::string result = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    if (prerelease.length() > 0) {
      result += "-" + prerelease;
    }
    if (build.length() > 0) {
      result += "+" + build;
    }
    return result;
  }
};

class VersionManager {
public:
  // Parse version string into components
  static SemanticVersion parseVersion(const std::string &versionStr);

  // Compare two versions (-1: v1 < v2, 0: v1 == v2, 1: v1 > v2)
  static int compareVersions(const std::string &v1, const std::string &v2);
  static int compareVersions(const SemanticVersion &v1,
                             const SemanticVersion &v2);

  // Check if version is newer than current
  static bool isNewer(const std::string &currentVersion,
                      const std::string &targetVersion);

  // Check if major version update (breaking change)
  static bool isMajorUpdate(const std::string &currentVersion,
                            const std::string &targetVersion);

  // Get next allowed version based on current version and update policy
  static std::string
  getNextAllowedVersion(const std::string &currentVersion,
                        const std::vector<std::string> &availableVersions,
                        bool allowMajorUpdates = true);

  // Validate version string format
  static bool isValidVersion(const std::string &version);

private:
  static int comparePrerelease(const std::string &pre1, const std::string &pre2);
};

#endif // OTA_VERSION_H