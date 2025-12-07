#include "ota_version.h"
#include <string>
#include <algorithm>

SemanticVersion VersionManager::parseVersion(const std::string& versionStr) {
    SemanticVersion version;
    std::string workingStr = versionStr;
    
    // Handle empty or whitespace-only strings
    if (workingStr.empty()) {
        return version; // Return default (0.0.0)
    }
    
    size_t buildIndex = workingStr.find('+');
    if (buildIndex != std::string::npos) {
        version.build = workingStr.substr(buildIndex + 1);
        workingStr = workingStr.substr(0, buildIndex);
    }
    
    size_t prereleaseIndex = workingStr.find('-');
    if (prereleaseIndex != std::string::npos) {
        version.prerelease = workingStr.substr(prereleaseIndex + 1);
        workingStr = workingStr.substr(0, prereleaseIndex);
    }
    
    size_t firstDot = workingStr.find('.');
    if (firstDot == std::string::npos) {
        // Major version only
        if (!workingStr.empty()) {
            try {
                version.major = std::stoi(workingStr);
            } catch (...) {
                // Invalid number, return default
            }
        }
        return version;
    }
    
    // Parse major
    std::string majorStr = workingStr.substr(0, firstDot);
    if (!majorStr.empty()) {
        try {
            version.major = std::stoi(majorStr);
        } catch (...) {
            // Invalid number, leave as 0
        }
    }
    
    size_t secondDot = workingStr.find('.', firstDot + 1);
    if (secondDot == std::string::npos) {
        // Major.minor only
        std::string minorStr = workingStr.substr(firstDot + 1);
        if (!minorStr.empty()) {
            try {
                version.minor = std::stoi(minorStr);
            } catch (...) {
                // Invalid number, leave as 0
            }
        }
        return version;
    }
    
    // Parse minor
    std::string minorStr = workingStr.substr(firstDot + 1, secondDot - firstDot - 1);
    if (!minorStr.empty()) {
        try {
            version.minor = std::stoi(minorStr);
        } catch (...) {
            // Invalid number, leave as 0
        }
    }
    
    // Parse patch
    std::string patchStr = workingStr.substr(secondDot + 1);
    if (!patchStr.empty()) {
        try {
            version.patch = std::stoi(patchStr);
        } catch (...) {
            // Invalid number, leave as 0
        }
    }
    
    return version;
}

int VersionManager::compareVersions(const std::string& v1, const std::string& v2) {
    return compareVersions(parseVersion(v1), parseVersion(v2));
}

int VersionManager::compareVersions(const SemanticVersion& v1, const SemanticVersion& v2) {
    if (v1.major != v2.major) {
        return v1.major > v2.major ? 1 : -1;
    }
    if (v1.minor != v2.minor) {
        return v1.minor > v2.minor ? 1 : -1;
    }
    if (v1.patch != v2.patch) {
        return v1.patch > v2.patch ? 1 : -1;
    }
    if (v1.prerelease.empty() && v2.prerelease.empty()) {
        return 0;
    }
    if (v1.prerelease.empty() && !v2.prerelease.empty()) {
        return 1;
    }
    if (!v1.prerelease.empty() && v2.prerelease.empty()) {
        return -1;
    }
    return comparePrerelease(v1.prerelease, v2.prerelease);
}

int VersionManager::comparePrerelease(const std::string& pre1, const std::string& pre2) {
    if (pre1 == pre2) return 0;
    return pre1 < pre2 ? -1 : 1;
}

bool VersionManager::isNewer(const std::string& currentVersion, const std::string& targetVersion) {
    return compareVersions(targetVersion, currentVersion) > 0;
}

bool VersionManager::isMajorUpdate(const std::string& currentVersion, const std::string& targetVersion) {
    SemanticVersion current = parseVersion(currentVersion);
    SemanticVersion target = parseVersion(targetVersion);
    return target.major > current.major;
}

std::string VersionManager::getNextAllowedVersion(const std::string& currentVersion, const std::vector<std::string>& availableVersions, bool allowMajorUpdates) {
    if (availableVersions.empty()) {
        return "";
    }
    SemanticVersion current = parseVersion(currentVersion);
    std::string bestVersion = "";
    int bestComparison = -1;
    for (const auto& version : availableVersions) {
        if (version.empty()) {
            continue;
        }
        int comparison = compareVersions(version, currentVersion);
        if (comparison <= 0) {
            continue;
        }
        if (!allowMajorUpdates && isMajorUpdate(currentVersion, version)) {
            continue;
        }
        if (bestVersion.empty() || comparison > bestComparison) {
            bestVersion = version;
            bestComparison = comparison;
        } else if (compareVersions(version, bestVersion) > 0) {
            bestVersion = version;
        }
    }
    return bestVersion;
}

bool VersionManager::isValidVersion(const std::string& version) {
    if (version.empty()) {
        return false;
    }
    SemanticVersion parsed = parseVersion(version);
    return parsed.major >= 0 && parsed.minor >= 0 && parsed.patch >= 0;
}
