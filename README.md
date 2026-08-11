# DCC-EX Protocol Testing

This repository contains a simple Arduino sketch with PlatformIO support to perform basic hardware testing of the DCCEXProtocol Arduino library.

This supports both STM32 Bluepill devices via a serial connection and ESP32 based devices via a WiFi connection.

The goal is to ensure changes to the library don't introduce breaking changes to user's throttle projects and new features/changes don't introduce bugs.

Use this sketch as an augmentation to the existing native Google tests run with PlatformIO for the DCCEXProtocol library.

## Sample myAutomation.h

Included in this repository in the `EX-CommandStation_Automation` directory is a sample `myAutomation.h` that can be used to create dummy objects and routes that can be used to exercise the various library functions for testing on physical devices.
