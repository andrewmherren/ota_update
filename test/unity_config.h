#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Unity Configuration for OTA Update Module Tests
// Increase Unity's buffer sizes for complex test messages
#ifndef UNITY_OUTPUT_CHAR_BUFFER_SIZE
#define UNITY_OUTPUT_CHAR_BUFFER_SIZE 256
#endif

// Enable Unity's 64-bit integer support
#define UNITY_SUPPORT_64

// Enable Unity's floating point support
#define UNITY_INCLUDE_DOUBLE

#endif // UNITY_CONFIG_H
