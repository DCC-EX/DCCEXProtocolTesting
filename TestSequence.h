/**
 * @file TestSequence.h
 * @brief Menu driven operator assisted test suite for the DCCEXProtocol library.
 *
 * @details This sketch is designed for operator assisted testing on physical hardware. The device under test (DUT)
 * connects to an EX-CommandStation and displays a menu of available tests.
 *
 * Each test is commenced by entering a command on the DUT serial console in DCC-EX protocol style:
 *  - <R id> starts a ROUTE on the command station via startRoute() - the command station drives the activity and
 *    broadcasts, and the DUT monitors and prints the received delegate callbacks
 *  - <T id> runs a LOCAL test on the DUT - the DUT drives activity via the DCCEXProtocol library API and monitors
 *    the resulting responses/broadcasts
 *
 * Test ids are sequential and independent of the underlying command station route ids. The localTests[] and
 * routeTests[] arrays below are the single source of truth for the menus, validation and dispatch - to add a test
 * add one row to the relevant array and implement the referenced function. Do not hardcode menu text or dispatch
 * logic elsewhere.
 *
 * After each test completes, the list of available tests is displayed again.
 *
 * The operator should monitor BOTH the serial console of the device under test (to verify the delegate callbacks
 * print the expected output) AND the serial console of the EX-CommandStation (to verify route PRINT markers and
 * command responses).
 *
 * @note Test bodies are currently placeholders (menu preview phase). See AGENTS.md for the project conventions and
 * the validation engine that will replace the placeholder bodies.
 */

#ifndef TEST_SEQUENCE_H
#define TEST_SEQUENCE_H

#include "Console.h"
#include "TestListener.h"

#ifndef CONNECT_TIMEOUT
#define CONNECT_TIMEOUT 10000 // Default maximum time in ms to wait for the command station to respond to <C>
#endif

/**
 * @brief Wait the specified time while processing inbound traffic and printing the optional alive heartbeat
 * @param ms Delay in milliseconds
 */
static void testDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    csClient.check();
#ifdef ALIVE_DELAY
    if (millis() - lastAlive > ALIVE_DELAY) {
      lastAlive = millis();
      Serial.println("Testing still live");
    }
#endif
  }
}

/**
 * @brief Print a test banner to the DUT serial console
 * @param name Name of the test
 */
static void testBanner(const char *name) {
  Serial.println();
  Serial.println("============================================================");
  Serial.print("TEST: ");
  Serial.println(name);
  Serial.println("============================================================");
}

/**
 * @brief Placeholder body used while the menu structure is being reviewed
 * @param testName Name of the test that has not been migrated yet
 */
static void notMigrated(const char *testName) {
  testBanner(testName);
  Serial.println();
  Serial.println("NOTE: The body of this test has not been migrated yet.");
  Serial.println("This is a placeholder so the menu structure can be reviewed.");
  Serial.println();
}

/**
 * @brief Print the connection summary, server version and retrieved lists
 */
static void printConnectionSummary() {
  Serial.println();
  Serial.println("============================================================");
  Serial.println("Connected to EX-CommandStation");
  Serial.println("============================================================");
  Serial.print("DCCEXProtocol library version: ");
  Serial.println(DCCEXProtocol::getLibraryVersion());
  Serial.print("Connected to EX-CommandStation version: ");
  Serial.print(csClient.getMajorVersion());
  Serial.print(".");
  Serial.print(csClient.getMinorVersion());
  Serial.print(".");
  Serial.println(csClient.getPatchVersion());
  Serial.print("receivedVersion()=");
  Serial.println(csClient.receivedVersion() ? "true" : "false");
  Serial.print("receivedLists()=");
  Serial.println(csClient.receivedLists() ? "true" : "false");
  Serial.print("Roster count: ");
  Serial.println(csClient.getRosterCount());
  Serial.print("Turnout count: ");
  Serial.println(csClient.getTurnoutCount());
  Serial.print("Route count: ");
  Serial.println(csClient.getRouteCount());
  Serial.print("Turntable count: ");
  Serial.println(csClient.getTurntableCount());
  Serial.println("Verify the version matches the CS console and all lists are printed below:");
  printRoster();
  printTurnouts();
  printRoutes();
  printTurntables();
  testDelay(2000);
}

/**
 * @brief Connect to the EX-CommandStation manually with <C>
 *
 * @details Brings up the physical link (ESP32 WiFi only - Serial1 is opened in setup() on the STM32), requests the
 * server version and all object lists, then waits up to CONNECT_TIMEOUT milliseconds for them to be received. On
 * success the connection summary and a checklist of the objects expected from EX-CommandStation_Automation/
 * myAutomation.h are printed. Can be repeated at any time to reconnect.
 * @return true if the link came up and all lists were received
 */
static bool connectToCommandStation() {
  testBanner("Connect to EX-CommandStation");
#if defined(ARDUINO_ARCH_ESP32)
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      csClient.check();
      delay(500);
    }
    Serial.print("WiFi connected with IP ");
    Serial.println(WiFi.localIP());
  }
  if (!wifiClient.connected()) {
    Serial.println("Connecting to the command station over WiFi...");
    if (!wifiClient.connect(serverAddress, serverPort)) {
      Serial.println("ERROR: Connection to the command station failed");
      Serial.println(
          ">>> Check the command station is powered and is running EX-CommandStation_Automation/myAutomation.h,");
      Serial.println(">>> and that serverAddress/serverPort in myWiFi.h are correct, then retry with <C>.");
      return false;
    }
    csClient.connect(&wifiClient);
  }
#elif defined(ARDUINO_ARCH_STM32)
  csClient.connect(&Serial1);
#endif
  Serial.println("Requesting the server version and object lists...");
  csClient.refreshAllLists();
  csClient.requestServerVersion();
  csClient.getLists();
  Serial.print("Waiting up to ");
  Serial.print(CONNECT_TIMEOUT / 1000);
  Serial.println(" seconds for the command station to respond...");
  unsigned long start = millis();
  while (!(csClient.receivedVersion() && csClient.receivedLists())) {
    if (millis() - start > CONNECT_TIMEOUT) {
      Serial.println("ERROR: No response from the command station");
      Serial.println(">>> Check the command station is powered and connected, is running");
      Serial.println(">>> EX-CommandStation_Automation/myAutomation.h, then retry with <C>.");
      return false;
    }
    csClient.check();
  }
  printConnectionSummary();
  Serial.println("Verify the command station is running EX-CommandStation_Automation/myAutomation.h:");
  Serial.println("  - 15 roster entries (2004-2066, so the roster list shows all entries)");
  Serial.println("  - turnouts 100-105, 110, 120, 121 (105 is HIDDEN so it is not in the turnout list)");
  Serial.println("  - turntables 2/3/4 and 21 JMRI sensors (6000-6300)");
  Serial.println("  - routes 500, 700-706, 708 and AUTOMATION 301");
  return true;
}

// --------------------------------------------------------------------------
// Local (DUT driven) test bodies, commenced with <T id>
// --------------------------------------------------------------------------

// T 1 - Version, Lists & Refresh
static void testVersionLists() { notMigrated("Version, Lists & Refresh"); }

// T 2 - Roster loco control
static void testRosterLocoControl() { notMigrated("Roster Loco Control"); }

// T 3 - Local (non-roster) loco control
static void testLocalLocoControl() { notMigrated("Local Loco Control"); }

// T 4 - Turnout control
static void testTurnoutControl() { notMigrated("Turnout Control"); }

// T 5 - Turntable control
static void testTurntableControl() { notMigrated("Turntable Control"); }

// T 6 - Track power
static void testTrackPower() { notMigrated("Track Power"); }

// T 7 - Track types
static void testTrackTypes() { notMigrated("Track Types"); }

// T 8 - Track currents
static void testTrackCurrents() { notMigrated("Track Currents"); }

// T 9 - Momentum
static void testMomentum() { notMigrated("Momentum"); }

// T 10 - DCC accessories
static void testAccessories() { notMigrated("DCC Accessories"); }

// T 11 - Command station consists
static void testConsistOps() { notMigrated("Consist Operations"); }

// T 12 - Automation handoff
static void testAutomationHandoff() { notMigrated("Automation Handoff"); }

// T 13 - JMRI sensor list request
static void testJMRISensorList() { notMigrated("JMRI Sensor List"); }

// T 14 - CV programming
static void testCVProgramming() { notMigrated("CV Programming (read)"); }

// T 15 - Fast clock
static void testFastClock() { notMigrated("Fast Clock"); }

// T 16 - Delayed activity with pause/resume
static void testDelayedActivity() { notMigrated("Delayed Activity + Pause/Resume"); }

// T 17 - Miscellaneous
static void testMiscellaneous() { notMigrated("Miscellaneous"); }

// --------------------------------------------------------------------------
// Test registries - single source of truth for the menus and dispatch
// --------------------------------------------------------------------------

/**
 * @brief Definition of a local (DUT driven) test
 */
struct LocalTest {
  int id;                  //<! Sequential test id entered as <T id>
  const char *name;        //<! Short menu name
  const char *description; //<! One line description of what the test does
  void (*fn)();            //<! Test body
};

/**
 * @brief Definition of a route (command station driven) test
 */
struct RouteTest {
  int id;                  //<! Sequential test id entered as <R id>
  const char *name;        //<! Short menu name
  const char *description; //<! One line description, empty if none
  int csRouteId;           //<! Underlying command station route id used for startRoute()
  unsigned long observeMs; //<! Observation window in milliseconds
};

static const LocalTest localTests[] = {
    {1, "Version, Lists & Refresh", "getLists(), refresh, counts", testVersionLists},
    {2, "Roster Loco Control", "loco 2010 throttle/functions/update", testRosterLocoControl},
    {3, "Local Loco Control", "non-roster loco 9999", testLocalLocoControl},
    {4, "Turnout Control", "throw/close/toggle 100/101/102", testTurnoutControl},
    {5, "Turntable Control", "rotate DCC turntables 2/4", testTurntableControl},
    {6, "Track Power", "all/MAIN/PROG on-off", testTrackPower},
    {7, "Track Types", "A: MAIN/PROG/DC/DCX", testTrackTypes},
    {8, "Track Currents", "gauges + currents", testTrackCurrents},
    {9, "Momentum", "algorithm/default/loco", testMomentum},
    {10, "DCC Accessories", "address 10, linear 500", testAccessories},
    {11, "Consist Operations", "CS consist build/drive/break", testConsistOps},
    {12, "Automation Handoff", "handOffLoco 3001 -> automation 301", testAutomationHandoff},
    {13, "JMRI Sensor List", "request <Q> list", testJMRISensorList},
    {14, "CV Programming (read)", "readLoco() + readCV", testCVProgramming},
    {15, "Fast Clock", "set + request time", testFastClock},
    {16, "Delayed Activity + Pause/Resume", "startRoute 708", testDelayedActivity},
    {17, "Miscellaneous", "locos/accessories/debug/lists", testMiscellaneous},
};

static const RouteTest routeTests[] = {
    {1, "Loco Drive", "loco 2010", 700, 25000},
    {2, "Local Loco Drive", "loco 9999", 701, 25000},
    {3, "Turnout Ops", "", 702, 25000},
    {4, "Turntable Ops", "", 703, 30000},
    {5, "Power Changes", "", 704, 25000},
    {6, "Messages", "", 705, 25000},
    {7, "Consist Ops", "", 706, 25000},
    {8, "JMRI Sensor Test", "", 500, 30000},
    {9, "Delayed Activity", "long running", 708, 30000},
};

static const int localTestCount = sizeof(localTests) / sizeof(localTests[0]);
static const int routeTestCount = sizeof(routeTests) / sizeof(routeTests[0]);

/**
 * @brief Find a local test by its id
 * @param testId Test id to find
 * @return Pointer to the matching LocalTest entry, or nullptr if not found
 */
static const LocalTest *findLocalTest(int testId) {
  for (int i = 0; i < localTestCount; i++) {
    if (localTests[i].id == testId)
      return &localTests[i];
  }
  return nullptr;
}

/**
 * @brief Find a route test by its id
 * @param testId Test id to find
 * @return Pointer to the matching RouteTest entry, or nullptr if not found
 */
static const RouteTest *findRouteTest(int testId) {
  for (int i = 0; i < routeTestCount; i++) {
    if (routeTests[i].id == testId)
      return &routeTests[i];
  }
  return nullptr;
}

/**
 * @brief Print the provided text padded with spaces to the specified width
 * @param text Text to print
 * @param width Minimum width to pad to
 */
static void printPadded(const char *text, int width) {
  int length = 0;
  while (text[length])
    length++;
  Serial.print(text);
  for (int i = length; i < width; i++)
    Serial.print(F(" "));
}

/**
 * @brief Print the menu of available tests, rendered from the registries
 */
static void printTestMenu() {
  char line[96];
  Serial.println();
  Serial.println("============================================================");
  Serial.println("DCCEXProtocol testing menu");
  Serial.println("============================================================");
  Serial.println("Local tests on the DUT (<T id>):");
  for (int i = 0; i < localTestCount; i++) {
    snprintf(line, sizeof(line), "  <T %2d>  ", localTests[i].id);
    Serial.print(line);
    printPadded(localTests[i].name, 34);
    Serial.println(localTests[i].description);
  }
  Serial.println();
  Serial.println("Routes to start on the command station (<R id>):");
  for (int i = 0; i < routeTestCount; i++) {
    snprintf(line, sizeof(line), "  <R %2d>  ", routeTests[i].id);
    Serial.print(line);
    printPadded(routeTests[i].name, 24);
    snprintf(line, sizeof(line), "CS route %d", routeTests[i].csRouteId);
    Serial.print(line);
    if (routeTests[i].description[0]) {
      snprintf(line, sizeof(line), "  (%s)", routeTests[i].description);
      Serial.print(line);
    }
    Serial.println();
  }
  Serial.println();
}

/**
 * @brief Run a local (DUT driven) test for the provided test id
 * @param testId Test id to run
 */
static void runLocalTest(int testId) {
  const LocalTest *test = findLocalTest(testId);
  if (test) {
    test->fn();
  } else {
    Serial.print("ERROR: Unknown local test id: ");
    Serial.println(testId);
  }
}

/**
 * @brief Start a route test (command station driven) for the provided test id
 *
 * @note Phase 1a placeholder - the route is not actually started until the menu structure has been approved.
 * @param testId Test id to run
 */
static void runRouteTest(int testId) {
  const RouteTest *test = findRouteTest(testId);
  if (!test) {
    Serial.print("ERROR: Unknown route test id: ");
    Serial.println(testId);
    return;
  }
  testBanner(test->name);
  Serial.println();
  Serial.println("NOTE: The body of this test has not been migrated yet.");
  Serial.print("It will start CS route ");
  Serial.print(test->csRouteId);
  Serial.print(" via startRoute() and monitor for ");
  Serial.print(test->observeMs / 1000);
  Serial.println(" seconds.");
  Serial.println();
}

/**
 * @brief Run the menu driven test console, printing the menu after every test
 */
static void runTestConsole() {
  while (true) {
    printTestMenu();
    char opcode;
    int id;
    if (consoleReadCommand(&opcode, &id)) {
      if (opcode == 'C') {
        connectToCommandStation();
        continue;
      }
      if (opcode == 'R') {
        runRouteTest(id);
      } else {
        runLocalTest(id);
      }
      Serial.println();
      Serial.println("============================================================");
      Serial.println("TEST COMPLETE");
      Serial.println("============================================================");
    }
  }
}

#endif // TEST_SEQUENCE_H