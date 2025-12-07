#ifndef PLATFORM_PROVIDER_H
#define PLATFORM_PROVIDER_H

#include <web_platform_interface.h>

// This file provides the global platform provider instance for production builds.
// In native tests, the platform provider is injected via constructor.

// The global platform provider instance is set by web_platform during initialization
// and accessed by all modules for dependency injection in production builds.

#endif // PLATFORM_PROVIDER_H
