/**
 * @file Globals.h
 * @brief Global object declarations for the DCCEXProtocol testing sketch.
 *
 * @details The actual objects are defined in DCCEXProtocolTesting.ino, this header simply declares them as extern so
 * the helper headers (PrintHelpers.h, TestListener.h, TestSequence.h) can reference them.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <DCCEXProtocol.h>

class CSListener; // Defined in TestListener.h

extern DCCEXProtocol csClient;  // Client connection to the EX-CommandStation using DCCEXProtocol
extern CSListener csListener;   // Listener for EX-CommandStation broadcasts/responses using DCCEXProtocolDelegate
extern unsigned long lastAlive; // Last time alive was displayed
extern bool csConnected;        // Set true when <C> has successfully connected to the command station

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
extern WiFiClient wifiClient;
#endif

#endif // GLOBALS_H
