/**
 * @file TestSequence.h
 * @brief Menu driven test sequence for the DCCEXProtocol library.
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
 * After each test completes, the list of available tests is displayed again.
 *
 * The operator should monitor BOTH the serial console of the device under test (to verify the delegate callbacks print
 * the expected output) AND the serial console of the EX-CommandStation (to verify route PRINT markers and command
 * responses).
 */

#ifndef TEST_SEQUENCE_H
#define TEST_SEQUENCE_H

#include "TestListener.h"
#include "Console.h"

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

// --------------------------------------------------------------------------
// Local (DUT driven) tests, commenced with <T id>
// --------------------------------------------------------------------------

// T 704 - Track power
static void testTrackPower() {
  testBanner("Track Power (local)");
  Serial.println("Turning all track power ON (<1>)");
  csClient.powerOn();
  testDelay(3000);
  Serial.println("Turning all track power OFF (<0>)");
  csClient.powerOff();
  testDelay(3000);
  Serial.println("Turning all track power ON again (<1>)");
  csClient.powerOn();
  testDelay(3000);
  Serial.println("Turning MAIN track OFF then ON (<0 MAIN>/<1 MAIN>)");
  csClient.powerMainOff();
  testDelay(3000);
  csClient.powerMainOn();
  testDelay(3000);
  Serial.println("Turning PROG track OFF then ON (<0 PROG>/<1 PROG>)");
  csClient.powerProgOff();
  testDelay(3000);
  csClient.powerProgOn();
  testDelay(3000);
}

// T 710 - Track types
static void testTrackTypes() {
  testBanner("Track Types (local)");
  Serial.println("NOTE: These commands change the actual track modes on the command station");
  Serial.println("Setting Track A to MAIN");
  csClient.setTrackType('A', MAIN, 0);
  testDelay(3000);
  Serial.println("Setting Track A to PROG");
  csClient.setTrackType('A', PROG, 0);
  testDelay(3000);
  Serial.println("Setting Track A to DC address 5");
  csClient.setTrackType('A', DC, 5);
  testDelay(3000);
  Serial.println("Setting Track A to DCX address 6");
  csClient.setTrackType('A', DCX, 6);
  testDelay(3000);
  Serial.println("Restoring Track A to MAIN");
  csClient.setTrackType('A', MAIN, 0);
  testDelay(3000);
}

// T 711 - Track currents
static void testTrackCurrents() {
  testBanner("Track Currents (local)");
  Serial.println("Requesting track current gauges (<jG>)");
  csClient.requestTrackCurrentGauges();
  testDelay(4000);
  Serial.println("Requesting track currents (<jI>)");
  csClient.requestTrackCurrents();
  testDelay(4000);
}

// T 700 - Roster loco control
static void testRosterLocoControl() {
  testBanner("Roster Loco Control (local)");
  Loco *loco2010 = csClient.findLocoInRoster(2010);
  if (loco2010) {
    Serial.println("Testing with roster loco 2010");
    printLoco(loco2010);
    Serial.println("Set throttle to speed 30 Forward (<t 2010 30 1>)");
    csClient.setThrottle(loco2010, 30, Forward);
    testDelay(3000);
    Serial.println("Function 0 ON");
    csClient.functionOn(loco2010, 0);
    testDelay(2000);
    Serial.println("Function 0 OFF");
    csClient.functionOff(loco2010, 0);
    testDelay(2000);
    Serial.println("Set throttle to speed 60 Reverse (<t 2010 60 0>)");
    csClient.setThrottle(loco2010, 60, Reverse);
    testDelay(3000);
    Serial.println("Stopping loco 2010 (<t 2010 0 1>)");
    csClient.setThrottle(loco2010, 0, Forward);
    testDelay(3000);
    Serial.println("Requesting a loco update (<t 2010>)");
    csClient.requestLocoUpdate(2010);
    testDelay(3000);
    Serial.print("isFunctionOn(loco2010, 0)=");
    Serial.println(csClient.isFunctionOn(loco2010, 0) ? "true" : "false");
  } else {
    Serial.println("ERROR: Loco 2010 not found in roster");
  }
  testDelay(1000);
}

// T 701 - Local (non-roster) loco control
static void testLocalLocoControl() {
  testBanner("Local (non-roster) Loco Control (local)");
  Serial.println("Creating a local loco with address 9999");
  Loco *localLoco = new Loco(9999, LocoSourceEntry);
  localLoco->setName("Local 9999");
  printLocalLocos();
  printLoco(localLoco);
  Serial.println("Set throttle to speed 25 Forward (<t 9999 25 1>)");
  csClient.setThrottle(localLoco, 25, Forward);
  testDelay(3000);
  Serial.println("Function 1 ON");
  csClient.functionOn(localLoco, 1);
  testDelay(2000);
  Serial.println("Stopping loco 9999 (<t 9999 0 1>)");
  csClient.setThrottle(localLoco, 0, Forward);
  testDelay(3000);
  Serial.println("Deleting the local loco to test list cleanup");
  delete localLoco;
  testDelay(1000);
  printLocalLocos();
}

// T 712 - Momentum
static void testMomentum() {
  testBanner("Momentum (local)");
  Serial.println("NOTE: No delegate callbacks are expected for these commands, check the CS console for responses");
  Serial.println("Setting momentum algorithm to Linear");
  csClient.setMomentumAlgorithm(Linear);
  testDelay(2000);
  Serial.println("Setting default momentum to 10");
  csClient.setDefaultMomentum(10);
  testDelay(2000);
  Serial.println("Setting momentum for address 2010 to 20");
  csClient.setMomentum(2010, 20);
  testDelay(2000);
  Serial.println("Setting accelerating/braking momentum for address 2010 to 10/5");
  csClient.setMomentum(2010, 10, 5);
  testDelay(2000);
  Loco *loco2014 = csClient.findLocoInRoster(2014);
  if (loco2014) {
    Serial.println("Setting momentum for Loco 2014 to 15");
    csClient.setMomentum(loco2014, 15);
    testDelay(2000);
  }
}

// T 702 - Turnout control
static void testTurnoutControl() {
  testBanner("Turnout Control (local)");
  Serial.println("Throwing turnout 100 (<T 100 1>)");
  csClient.throwTurnout(100);
  testDelay(3000);
  Serial.println("Closing turnout 100 (<T 100 0>)");
  csClient.closeTurnout(100);
  testDelay(3000);
  Serial.println("Toggling turnout 101 (<T 101 -1>)");
  csClient.toggleTurnout(101);
  testDelay(3000);
  Serial.println("Toggling turnout 101 again");
  csClient.toggleTurnout(101);
  testDelay(3000);
  Serial.println("Throwing turnout 102");
  csClient.throwTurnout(102);
  testDelay(3000);
  Turnout *turnout100 = csClient.getTurnoutById(100);
  if (turnout100) {
    Serial.print("getTurnoutById(100) state=");
    Serial.println(turnout100->getThrown() ? "Thrown" : "Closed");
  }
}

// T 703 - DCC turntable control
static void testTurntableControl() {
  testBanner("DCC Turntable Control (local)");
  Serial.println("Rotating turntable 2 to position 2 (<I 2 2>)");
  csClient.rotateTurntable(2, 2);
  testDelay(5000);
  Serial.println("Rotating turntable 2 to position 4 (<I 2 4>)");
  csClient.rotateTurntable(2, 4);
  testDelay(5000);
  Serial.println("Rotating turntable 2 to home position 0 (<I 2 0>)");
  csClient.rotateTurntable(2, 0);
  testDelay(5000);
  Serial.println("Rotating turntable 4 to position 1 (<I 4 1>)");
  csClient.rotateTurntable(4, 1);
  testDelay(5000);
  Serial.println("Rotating turntable 4 to position 3 (<I 4 3>)");
  csClient.rotateTurntable(4, 3);
  testDelay(5000);
  Turntable *turntable2 = csClient.getTurntableById(2);
  if (turntable2) {
    Serial.print("getTurntableById(2) index=");
    Serial.println(turntable2->getIndex());
  }
}

// T 713 - DCC accessories
static void testAccessories() {
  testBanner("DCC Accessories (local)");
  Serial.println("NOTE: No delegate callbacks are expected for these commands, check the CS console for responses");
  Serial.println("Activating accessory 10 subaddress 1 (<A 10 1 1>)");
  csClient.activateAccessory(10, 1);
  testDelay(2000);
  Serial.println("Deactivating accessory 10 subaddress 1 (<A 10 1 0>)");
  csClient.deactivateAccessory(10, 1);
  testDelay(2000);
  Serial.println("Activating linear accessory 500 (<a 500 1>)");
  csClient.activateLinearAccessory(500);
  testDelay(2000);
  Serial.println("Deactivating linear accessory 500 (<a 500 0>)");
  csClient.deactivateLinearAccessory(500);
  testDelay(2000);
}

// T 714 - CV programming
static void testCVProgramming() {
  testBanner("CV Programming (local)");
  Serial.println("WARNING: This command operates on the programming track and may affect a decoder if one is connected");
  Serial.println(">>> Ensure a loco is on the PROG track (or accept failures)");
  testDelay(15000);
  Serial.println("Reading loco address from the programming track (<R>)");
  csClient.readLoco();
  testDelay(10000);
}

// T 715 - Fast clock
static void testFastClock() {
  testBanner("Fast Clock (local)");
  Serial.println("Setting fast clock to 7:00am with speed factor 4");
  csClient.setFastClock(420, 4);
  testDelay(3000);
  Serial.println("Requesting the fast clock time (<jC>)");
  csClient.requestFastClockTime();
  testDelay(3000);
}

// T 500 - JMRI sensor list request
static void testJMRISensorList() {
  testBanner("JMRI Sensors (local)");
  Serial.println("Requesting the JMRI sensor list (<Q>)");
  csClient.requestJMRISensorList();
  testDelay(5000);
}

// T 706 - Command station consists
static void testConsistOps() {
  testBanner("Command Station Consists (local)");
  Serial.println("Creating a CSConsist with lead loco 2010");
  CSConsist *csConsist = csClient.createCSConsist(2010);
  if (csConsist) {
    Serial.println("Adding member 2014 (reversed)");
    csClient.addCSConsistMember(csConsist, 2014, true);
    testDelay(2000);
    Serial.println("Adding member 2016");
    csClient.addCSConsistMember(csConsist, 2016);
    testDelay(3000);
    printCSConsists();
    Serial.println("Setting consist throttle to speed 20 Forward");
    csClient.setThrottle(csConsist, 20, Forward);
    testDelay(3000);
    Serial.println("Function 0 ON for the consist");
    csClient.functionOn(csConsist, 0);
    testDelay(2000);
    Serial.println("Function 0 OFF for the consist");
    csClient.functionOff(csConsist, 0);
    testDelay(2000);
    Serial.println("Stopping the consist");
    csClient.setThrottle(csConsist, 0, Forward);
    testDelay(3000);
    Serial.println("Removing member 2016");
    csClient.removeCSConsistMember(csConsist, 2016);
    testDelay(3000);
    printCSConsists();
    Serial.println("Deleting the CSConsist");
    csClient.deleteCSConsist(csConsist);
    testDelay(3000);
    Serial.println("Clearing all CSConsists");
    csClient.clearCSConsists();
    testDelay(1000);
  } else {
    Serial.println("ERROR: Could not create CSConsist");
  }
  Serial.println("Requesting the list of CSConsists from the CS (<^>)");
  csClient.requestCSConsists();
  testDelay(3000);
}

// T 301 - Automation handoff
static void testAutomationHandoff() {
  testBanner("Automation Handoff (local)");
  Serial.println("Handing off loco 3001 to AUTOMATION 301 (< / START 3001 301>)");
  csClient.handOffLoco(3001, 301);
  testDelay(15000);
}

// T 708 - Delayed activity with pause/resume
static void testDelayedActivity() {
  testBanner("Delayed Activity with Pause/Resume (local)");
  Serial.println("Starting ROUTE 708 (Delayed Activity) via startRoute() (< / START 708>)");
  csClient.startRoute(708);
  testDelay(5000);
  Serial.println("Pausing all routes (< / PAUSE>) - verify the route stops on the CS console");
  csClient.pauseRoutes();
  testDelay(5000);
  Serial.println("Resuming all routes (< / RESUME>) - verify the route continues on the CS console");
  csClient.resumeRoutes();
  testDelay(20000);
  Serial.println(">>> Route 708 should be complete on the CS console now");
}

// T 716 - List refresh and miscellaneous
static void testListRefreshMisc() {
  testBanner("List Refresh and Miscellaneous (local)");
  Serial.println("Clearing local locos (clearLocalLocos())");
  csClient.clearLocalLocos();
  testDelay(1000);
  Serial.println("Requesting the number of supported locos (<#>)");
  csClient.getNumberSupportedLocos();
  testDelay(2000);
  Serial.println("Enabling debug output and requesting the server version");
  csClient.setDebug(true);
  csClient.requestServerVersion();
  testDelay(3000);
  Serial.println("Disabling debug output");
  csClient.setDebug(false);
  testDelay(1000);
  Serial.println("Refreshing all lists (roster, turnouts, routes, turntables)");
  csClient.refreshAllLists();
  testDelay(10000);
  Serial.print("receivedLists() after refresh=");
  Serial.println(csClient.receivedLists() ? "true" : "false");
  Serial.println("Lists after refresh:");
  printRoster();
  printTurnouts();
  printRoutes();
  printTurntables();
  Serial.print("Last server response time: ");
  Serial.println(csClient.getLastServerResponseTime());
}

// --------------------------------------------------------------------------
// Route (command station driven) tests, commenced with <R id>
// --------------------------------------------------------------------------

/**
 * @brief Check whether the provided route id exists in myAutomation.h
 * @param routeId Route id to check
 * @return true if the route is defined in myAutomation.h
 */
static bool isKnownRoute(int routeId) {
  switch (routeId) {
  case 200:
  case 300:
  case 400:
  case 500:
  case 700:
  case 701:
  case 702:
  case 703:
  case 704:
  case 705:
  case 706:
  case 708:
    return true;
  default:
    return false;
  }
}

/**
 * @brief Return the observation time in milliseconds for the provided route id
 */
static unsigned long routeObserveMs(int routeId) {
  switch (routeId) {
  case 500:
    return 30000;
  case 703:
  case 708:
    return 30000;
  default:
    return 25000;
  }
}

/**
 * @brief Start a route on the command station and monitor the broadcasts
 * @param routeId Route id to start via startRoute()
 */
static void runRouteTest(int routeId) {
  if (!isKnownRoute(routeId)) {
    Serial.print("ERROR: Route ");
    Serial.print(routeId);
    Serial.println(" is not defined in myAutomation.h");
    return;
  }
  testBanner("Route");
  Serial.print("Starting route ");
  Serial.print(routeId);
  Serial.println(" via startRoute() (< / START id>)");
  csClient.startRoute(routeId);
  unsigned long observeMs = routeObserveMs(routeId);
  Serial.print(">>> Monitoring ");
  Serial.print(observeMs / 1000);
  Serial.println(" seconds for route activity...");
  unsigned long start = millis();
  while (millis() - start < observeMs) {
    csClient.check();
  }
  Serial.println(">>> Route test complete");
}

/**
 * @brief Run a local (DUT driven) test for the provided test id
 * @param testId Test id to run
 */
static void runLocalTest(int testId) {
  switch (testId) {
  case 500:
    testJMRISensorList();
    break;
  case 700:
    testRosterLocoControl();
    break;
  case 701:
    testLocalLocoControl();
    break;
  case 702:
    testTurnoutControl();
    break;
  case 703:
    testTurntableControl();
    break;
  case 704:
    testTrackPower();
    break;
  case 706:
    testConsistOps();
    break;
  case 301:
    testAutomationHandoff();
    break;
  case 708:
    testDelayedActivity();
    break;
  case 710:
    testTrackTypes();
    break;
  case 711:
    testTrackCurrents();
    break;
  case 712:
    testMomentum();
    break;
  case 713:
    testAccessories();
    break;
  case 714:
    testCVProgramming();
    break;
  case 715:
    testFastClock();
    break;
  case 716:
    testListRefreshMisc();
    break;
  default:
    Serial.print("ERROR: Unknown local test id: ");
    Serial.println(testId);
    break;
  }
}

/**
 * @brief Print the menu of available tests
 */
static void printTestMenu() {
  Serial.println();
  Serial.println("============================================================");
  Serial.println("DCCEXProtocol testing menu");
  Serial.println("============================================================");
  Serial.println("Routes to start on the command station (<R id>):");
  Serial.println("  <R 200>  Route 200 (basic)");
  Serial.println("  <R 300>  Automation 300 (basic)");
  Serial.println("  <R 400>  Route 400 (basic)");
  Serial.println("  <R 500>  JMRI Sensor Test (sensor broadcasts)");
  Serial.println("  <R 700>  Loco Drive (roster loco 2010)");
  Serial.println("  <R 701>  Local Loco Drive (non-roster loco 9999)");
  Serial.println("  <R 702>  Turnout Ops");
  Serial.println("  <R 703>  Turntable Ops");
  Serial.println("  <R 704>  Power Changes");
  Serial.println("  <R 705>  Messages");
  Serial.println("  <R 706>  Consist Ops");
  Serial.println("  <R 708>  Delayed Activity (long running)");
  Serial.println();
  Serial.println("Local tests on the DUT (<T id>):");
  Serial.println("  <T 301>  Automation Handoff (handOffLoco to automation 301)");
  Serial.println("  <T 500>  Request JMRI sensor list");
  Serial.println("  <T 700>  Roster Loco Control (loco 2010)");
  Serial.println("  <T 701>  Local Loco Control (loco 9999)");
  Serial.println("  <T 702>  Turnout Control");
  Serial.println("  <T 703>  Turntable Control");
  Serial.println("  <T 704>  Track Power");
  Serial.println("  <T 706>  Command Station Consist Operations");
  Serial.println("  <T 708>  Delayed Activity with Pause/Resume");
  Serial.println("  <T 710>  Track Types");
  Serial.println("  <T 711>  Track Currents");
  Serial.println("  <T 712>  Momentum");
  Serial.println("  <T 713>  DCC Accessories");
  Serial.println("  <T 714>  CV Programming (read loco address only)");
  Serial.println("  <T 715>  Fast Clock");
  Serial.println("  <T 716>  List Refresh and Miscellaneous");
  Serial.println();
}

/**
 * @brief Run the menu driven test console, printing the menu after every test
 */
static void runTestConsole() {
  printConnectionSummary();
  while (true) {
    printTestMenu();
    char opcode;
    int id;
    if (consoleReadCommand(&opcode, &id)) {
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
