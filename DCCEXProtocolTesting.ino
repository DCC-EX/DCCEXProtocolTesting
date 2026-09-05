/**
 * @file DCCEXProtocolTesting.ino
 * @brief Arduino sketch for DCCEXProtocol library testing with hardware.
 *
 * @details Use this sketch with either a serially connected STM32 Bluepill or WiFi connected ESP32 device to connect to
 * an EX-CommandStation with various objects created (see EX-CommandStation_Automation/myAutomation.h) to ensure the
 * protocol can be used to connect, retrieve objects, control locos/turnouts/routes/turntables, and respond to
 * broadcasts/responses.
 *
 * Connect the device being tested to an EX-CommandStation and monitor the serial console of the device under test to
 * observe it receiving the various object lists and then broadcasts/responses. Testing is menu driven and operator
 * assisted - the EX-CommandStation console should also be monitored. Tests are commenced by entering a command in
 * DCC-EX protocol style on the serial console of the device under test:
 *  - <C> connects to the EX-CommandStation manually (requests the server version and object lists)
 *  - <R id> starts a ROUTE on the command station via startRoute() (the command station drives the activity)
 *  - <T id> runs a LOCAL test on the device under test (the DUT drives activity via the library API)
 *
 * After each test completes, the list of available tests is displayed again.
 *
 * The test suite is split across the following files:
 *  - Globals.h      - extern declarations for the global objects
 *  - PrintHelpers.h - enum renderers and list printers
 *  - TestListener.h - the delegate handling all DCCEXProtocolDelegate callbacks
 *  - Console.h      - interactive serial console used to enter <C> / <T id> / <R id> commands
 *  - TestSequence.h - the menu driven operator assisted test suite
 *
 * @author peteGSX
 */

#include <DCCEXProtocol.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>

// If we haven't got a custom myWiFi.h, use the example
#if __has_include("myWiFi.h")
#include "myWiFi.h"
#else
#warning myWiFi.h not found. Using defaults from myWiFi.example.h
#include "myWiFi.example.h"
#endif

#endif

// If we haven't got a custom myConfig.h, use the example
#if __has_include("myConfig.h")
#include "myConfig.h"
#else
#warning myConfig.h not found. Using defaults from myConfig.example.h
#include "myConfig.example.h"
#endif

// Test suite helper headers
#include "Console.h"
#include "Globals.h"
#include "PrintHelpers.h"
#include "TestListener.h"
#include "TestSequence.h"

/*
Global objects
*/
DCCEXProtocol csClient;      // Client connection to the EX-CommandStation using DCCEXProtocol
CSListener csListener;       // Listener for EX-CommandStation broadcasts/responses using DCCEXProtocolDelegate
unsigned long lastAlive = 0; // Last time alive was displayed
#if defined(ARDUINO_ARCH_ESP32)
WiFiClient wifiClient;
#endif

bool consoleStarted = false; // Flag to ensure the menu driven test console only starts once

void setup() {
#if defined(ARDUINO_ARCH_STM32)
  // Disable JTAG and enable SWD by clearing the SWJ_CFG bits
  AFIO->MAPR &= ~(AFIO_MAPR_SWJ_CFG);
#endif

  Serial.begin(115200);

#ifdef STARTUP_DELAY
  delay(STARTUP_DELAY);
#endif

  Serial.println("DCCEXProtocol Testing");

  csClient.setLogStream(&Serial);    // Set up serial connection for the console logs
  csClient.setDelegate(&csListener); // Set up the listener for broadcasts/events
  csClient.enableHeartbeat(30000);   // Enable heartbeats to help WiFi connections stay alive

#if defined(ARDUINO_ARCH_STM32) // If using Bluepill, use Serial1 for the CS connection
  Serial1.begin(115200);
  csClient.connect(&Serial1);
#endif
  // No connection is made at boot - the operator connects to the EX-CommandStation manually with <C>
  // (see connectToCommandStation() in TestSequence.h). For ESP32 this also brings up the WiFi link.
}

void loop() {
  csClient.check();

  // Start the menu driven test console immediately. The EX-CommandStation connection is established later with <C>.
  if (!consoleStarted) {
    consoleStarted = true;
    runTestConsole();
  }

#ifdef ALIVE_DELAY
  if (millis() - lastAlive > ALIVE_DELAY) {
    lastAlive = millis();
    Serial.println("Testing still live");
  }
#endif
}
