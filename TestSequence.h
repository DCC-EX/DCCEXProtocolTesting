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
 * @note Tests are driven by typed expectations set on the listener (see TestListener.h) and observed in per-phase
 * observation windows replaced by waitForExpectations(). See AGENTS.md for the project conventions.
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
 * myAutomation.h are printed. Sets csConnected so the <T>/<R> tests can run (they are refused before a
 * successful connect). Can be repeated at any time to reconnect.
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
    csClient.getLists(); // gated: requests the next list once the previous one is in
    csClient.check();
  }
  csConnected = true;
  printConnectionSummary();
  Serial.println("Verify the command station is running EX-CommandStation_Automation/myAutomation.h:");
  Serial.println("  - 15 roster entries (2004-2066, so the roster list shows all entries)");
  Serial.println("  - turnouts 100-105, 110, 120, 121 (105 is HIDDEN so it is not in the turnout list)");
  Serial.println("  - turntables 2/3/4 and 21 JMRI sensors (6000-6300)");
  Serial.println("  - routes 500, 700-706, 708 and AUTOMATION 301");
  return true;
}

/**
 * @brief Pump csClient.check() until all active expectations are satisfied, a value fails, or the window expires
 * @param windowMs Observation window in milliseconds (default expectationWindowMs)
 * @details This replaces raw testDelay() waits for phases with expectations set on the listener. Events with no
 * matching expectation print normally and are ignored; observation ends early once every expectation is satisfied.
 */
static void waitForExpectations(unsigned long windowMs = expectationWindowMs) {
  unsigned long start = millis();
  while ((!csListener.hasFailed()) && (!csListener.allExpectationsMatched()) && (millis() - start < windowMs)) {
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
 * @brief Pump csClient.check() and cascade csClient.getLists() until all lists are received
 *
 * @details getLists() is gated in the library: each call requests only the next list after the previous one has
 * arrived, so it must be called repeatedly in the wait loop. Use this instead of a single getLists() call.
 * @param windowMs Observation window in milliseconds (defaults to CONNECT_TIMEOUT)
 */
static void waitForAllLists(unsigned long windowMs = CONNECT_TIMEOUT) {
  unsigned long start = millis();
  while (!csClient.receivedLists()) {
    if (millis() - start > windowMs) {
      Serial.println("ERROR: Timed out waiting for the object lists");
      return;
    }
    csClient.getLists(); // gated: requests the next list once the previous one is in
    csClient.check();
  }
}

// --------------------------------------------------------------------------
// Local (DUT driven) test bodies, commenced with <T id>
// --------------------------------------------------------------------------

// T 1 - Version, Lists & Refresh
static void testVersionLists() {
  testBanner("Version, Lists & Refresh");
  csListener.clearExpectations();
  // First clear the client lists (refreshAllLists()) so the later getLists() cascade genuinely re-requests them
  // over the wire. A getLists() call is a no-op while receivedLists() is already true (see waitForAllLists()).
  Serial.println("Reset the client lists, then request the server version (<s>) and all object lists");
  csClient.refreshAllLists();
  csClient.requestServerVersion();
  csListener.expectServerVersion();
  csListener.expectRosterList();
  csListener.expectTurnoutList();
  csListener.expectRouteList();
  csListener.expectTurntableList();
  waitForAllLists();
  Serial.print("Roster count: ");
  Serial.println(csClient.getRosterCount());
  Serial.print("Turnout count: ");
  Serial.println(csClient.getTurnoutCount());
  Serial.print("Route count: ");
  Serial.println(csClient.getRouteCount());
  Serial.print("Turntable count: ");
  Serial.println(csClient.getTurntableCount());
  Serial.println("Refreshing all lists (refreshAllLists() then getLists())");
  csClient.refreshAllLists();
  csListener.expectRosterList();
  csListener.expectTurnoutList();
  csListener.expectRouteList();
  csListener.expectTurntableList();
  waitForAllLists();
  Serial.println("Lists after refresh:");
  printRoster();
  printTurnouts();
  printRoutes();
  printTurntables();
  csListener.printExpectationResult();
}

// T 2 - Roster loco control
static void testRosterLocoControl() {
  testBanner("Roster Loco Control");
  csListener.clearExpectations();
  Loco *loco2010 = csClient.findLocoInRoster(2010);
  if (loco2010) {
    Serial.println("Testing with roster loco 2010");
    printLoco(loco2010);
    Serial.println("Set throttle to speed 30 Forward (<t 2010 30 1>)");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(loco2010, 30, Forward);
    waitForExpectations();
    Serial.println("Function 0 ON");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.functionOn(loco2010, 0);
    waitForExpectations();
    Serial.println("Function 0 OFF");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.functionOff(loco2010, 0);
    waitForExpectations();
    Serial.println("Set throttle to speed 60 Reverse (<t 2010 60 0>)");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(loco2010, 60, Reverse);
    waitForExpectations();
    Serial.println("Stopping loco 2010 (<t 2010 0 1>)");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(loco2010, 0, Forward);
    waitForExpectations();
    Serial.println("Requesting a loco update (<t 2010>)");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.requestLocoUpdate(2010);
    waitForExpectations();
    Serial.print("isFunctionOn(loco2010, 0)=");
    Serial.println(csClient.isFunctionOn(loco2010, 0) ? "true" : "false");
    Serial.println(">>> Verify the speed/direction/function changes on the CS console");
  } else {
    Serial.println("ERROR: Loco 2010 not found in roster");
  }
  csListener.printExpectationResult();
}

// T 3 - Local (non-roster) loco control
static void testLocalLocoControl() {
  testBanner("Local (non-roster) Loco Control");
  csListener.clearExpectations();
  Serial.println("Creating a local loco with address 9999");
  Loco *localLoco = new Loco(9999, LocoSourceEntry);
  localLoco->setName("Local 9999");
  printLocalLocos();
  printLoco(localLoco);
  Serial.println("Set throttle to speed 25 Forward (<t 9999 25 1>)");
  csListener.expectLocoBroadcast(9999);
  csListener.expectLocoUpdate(9999);
  csClient.setThrottle(localLoco, 25, Forward);
  waitForExpectations();
  Serial.println("Function 1 ON");
  csListener.expectLocoBroadcast(9999);
  csListener.expectLocoUpdate(9999);
  csClient.functionOn(localLoco, 1);
  waitForExpectations();
  Serial.println("Stopping loco 9999 (<t 9999 0 1>)");
  csListener.expectLocoBroadcast(9999);
  csListener.expectLocoUpdate(9999);
  csClient.setThrottle(localLoco, 0, Forward);
  waitForExpectations();
  Serial.println("Deleting the local loco to test list cleanup");
  delete localLoco;
  testDelay(1000);
  printLocalLocos();
  csListener.printExpectationResult();
}

// T 4 - Turnout control
static void testTurnoutControl() {
  testBanner("Turnout Control");
  csListener.clearExpectations();
  Serial.println("Throwing turnout 100 (<T 100 1>)");
  csListener.expectTurnoutAction(100, true);
  csClient.throwTurnout(100);
  waitForExpectations();
  Serial.println("Closing turnout 100 (<T 100 0>)");
  csListener.expectTurnoutAction(100, false);
  csClient.closeTurnout(100);
  waitForExpectations();
  Serial.println("Toggling turnout 101 (<T 101 -1>)");
  csListener.expectTurnoutAction(101, EXPECT_ANY);
  csClient.toggleTurnout(101);
  waitForExpectations();
  Serial.println("Toggling turnout 101 again");
  csListener.expectTurnoutAction(101, EXPECT_ANY);
  csClient.toggleTurnout(101);
  waitForExpectations();
  Serial.println("Throwing turnout 102");
  csListener.expectTurnoutAction(102, true);
  csClient.throwTurnout(102);
  waitForExpectations();
  Turnout *turnout100 = csClient.getTurnoutById(100);
  if (turnout100) {
    Serial.print("getTurnoutById(100) state=");
    Serial.println(turnout100->getThrown() ? "Thrown" : "Closed");
  }
  Serial.println(">>> Verify the turnout states on the CS console");
  csListener.printExpectationResult();
}

// T 5 - Turntable control
static void testTurntableControl() {
  testBanner("Turntable Control");
  csListener.clearExpectations();
  Serial.println("Rotating turntable 2 to position 2 (<I 2 2>)");
  csListener.expectTurntableAction(2, 2);
  csClient.rotateTurntable(2, 2);
  waitForExpectations(8000);
  Serial.println("Rotating turntable 2 to position 4 (<I 2 4>)");
  csListener.expectTurntableAction(2, 4);
  csClient.rotateTurntable(2, 4);
  waitForExpectations(8000);
  Serial.println("Rotating turntable 2 to home position 0 (<I 2 0>)");
  csListener.expectTurntableAction(2, 0);
  csClient.rotateTurntable(2, 0);
  waitForExpectations(8000);
  Serial.println("Rotating turntable 4 to position 1 (<I 4 1>)");
  csListener.expectTurntableAction(4, 1);
  csClient.rotateTurntable(4, 1);
  waitForExpectations(8000);
  Serial.println("Rotating turntable 4 to position 3 (<I 4 3>)");
  csListener.expectTurntableAction(4, 3);
  csClient.rotateTurntable(4, 3);
  waitForExpectations(8000);
  Turntable *turntable2 = csClient.getTurntableById(2);
  if (turntable2) {
    Serial.print("getTurntableById(2) index=");
    Serial.println(turntable2->getIndex());
  }
  Serial.println(">>> Verify the turntable activity on the CS console");
  csListener.printExpectationResult();
}

// T 6 - Track power
static void testTrackPower() {
  testBanner("Track Power");
  csListener.clearExpectations();
  Serial.println("Turning all track power ON (<1>)");
  csListener.expectTrackPower(PowerOn);
  csClient.powerOn();
  waitForExpectations();
  Serial.println("Turning all track power OFF (<0>)");
  csListener.expectTrackPower(PowerOff);
  csClient.powerOff();
  waitForExpectations();
  Serial.println("Turning all track power ON again (<1>)");
  csListener.expectTrackPower(PowerOn);
  csClient.powerOn();
  waitForExpectations();
  Serial.println("Turning MAIN track OFF then ON (<0 MAIN>/<1 MAIN>)");
  Serial.println(">>> MAIN power may broadcast globally or per track - verify the track state on the CS console");
  csListener.expectTrackPower(EXPECT_ANY);
  csClient.powerMainOff();
  waitForExpectations();
  csListener.expectTrackPower(EXPECT_ANY);
  csClient.powerMainOn();
  waitForExpectations();
  Serial.println("Turning PROG track OFF then ON (<0 PROG>/<1 PROG>)");
  Serial.println(">>> PROG power changes are often not broadcast - verify the PROG track on the CS console");
  csClient.powerProgOff();
  testDelay(3000);
  csClient.powerProgOn();
  testDelay(3000);
  Serial.println("Turning track B ON then OFF (<1 B>/<0 B>)");
  csListener.expectIndividualTrackPower('B', PowerOn);
  csListener.expectTrackPower(PowerOn);
  csClient.powerTrackOn('B');
  waitForExpectations();
  csListener.expectIndividualTrackPower('B', PowerOff);
  csListener.expectTrackPower(PowerOff);
  csClient.powerTrackOff('B');
  waitForExpectations();
  Serial.println("Powering the PROG track as a joined track (joinProg() <1 JOIN>)");
  Serial.println(">>> joinProg() may not be broadcast - verify the PROG track joins on the CS console");
  csClient.joinProg();
  testDelay(3000);
  csListener.printExpectationResult();
}

// T 7 - Track types
static void testTrackTypes() {
  testBanner("Track Types");
  csListener.clearExpectations();
  Serial.println("NOTE: These commands change the actual track modes on the command station");
  Serial.println("Setting Track A to MAIN");
  csListener.expectTrackType('A', MAIN, EXPECT_ANY);
  csClient.setTrackType('A', MAIN, 0);
  waitForExpectations(5000);
  Serial.println("Setting Track A to PROG");
  csListener.expectTrackType('A', PROG, EXPECT_ANY);
  csClient.setTrackType('A', PROG, 0);
  waitForExpectations(5000);
  Serial.println("Setting Track A to DC address 5");
  csListener.expectTrackType('A', DC, 5);
  csClient.setTrackType('A', DC, 5);
  waitForExpectations(5000);
  Serial.println("Setting Track A to DCX address 6");
  csListener.expectTrackType('A', DCX, 6);
  csClient.setTrackType('A', DCX, 6);
  waitForExpectations(5000);
  Serial.println("Restoring Track A to MAIN");
  csListener.expectTrackType('A', MAIN, EXPECT_ANY);
  csClient.setTrackType('A', MAIN, 0);
  waitForExpectations(5000);
  Serial.println(">>> Verify the track mode changes on the CS console");
  csListener.printExpectationResult();
}

// T 8 - Track currents
static void testTrackCurrents() {
  testBanner("Track Currents");
  csListener.clearExpectations();
  Serial.println("Requesting track current gauges (<jG>)");
  csListener.expectTrackCurrentGauge(EXPECT_ANY, EXPECT_ANY, 8);
  csClient.requestTrackCurrentGauges();
  waitForExpectations(15000);
  Serial.println("Requesting track currents (<jI>)");
  csListener.expectTrackCurrent(EXPECT_ANY, EXPECT_ANY, 8);
  csClient.requestTrackCurrents();
  waitForExpectations(15000);
  Serial.println(">>> Verify the current values match the CS console");
  csListener.printExpectationResult();
}

// T 9 - Momentum
static void testMomentum() {
  testBanner("Momentum");
  csListener.clearExpectations();
  Serial.println("NOTE: No delegate callbacks are expected for these commands, check the CS console for responses");
  Serial.println("Setting momentum algorithm to Linear");
  csClient.setMomentumAlgorithm(Linear);
  testDelay(2000);
  Serial.println("Setting default momentum to 10");
  csClient.setDefaultMomentum(10);
  testDelay(2000);
  Serial.println("Setting default momentum to 10 accelerating / 5 braking");
  csClient.setDefaultMomentum(10, 5);
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
    Serial.println("Setting accelerating/braking momentum for Loco 2014 to 15/10");
    csClient.setMomentum(loco2014, 15, 10);
    testDelay(2000);
  }
  csListener.printExpectationResult();
}

// T 10 - DCC accessories
static void testAccessories() {
  testBanner("DCC Accessories");
  csListener.clearExpectations();
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
  csListener.printExpectationResult();
}

// T 11 - Command station consists
static void testConsistOps() {
  testBanner("Command Station Consists");
  csListener.clearExpectations();
  Serial.println("Creating a CSConsist with lead loco 2010 (createCSConsist(2010))");
  CSConsist *csConsist = csClient.createCSConsist(2010);
  if (csConsist) {
    Serial.println("Adding member 2014 (reversed)");
    csListener.expectCSConsist(2010);
    csClient.addCSConsistMember(csConsist, 2014, true);
    waitForExpectations();
    Serial.println("Adding member 2016");
    csListener.expectCSConsist(2010);
    csClient.addCSConsistMember(csConsist, 2016);
    waitForExpectations();
    printCSConsists();
    Serial.println("Setting consist throttle to speed 20 Forward");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(csConsist, 20, Forward);
    waitForExpectations();
    Serial.println("Function 0 ON for the consist");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.functionOn(csConsist, 0);
    waitForExpectations();
    Serial.println("Function 0 OFF for the consist");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.functionOff(csConsist, 0);
    waitForExpectations();
    Serial.print("isFunctionOn(csConsist, 0)=");
    Serial.println(csClient.isFunctionOn(csConsist, 0) ? "true" : "false");
    Serial.println("Looking up the consist by lead/member loco");
    Serial.print("getCSConsistByLeadLoco(2010)=");
    Serial.println(csClient.getCSConsistByLeadLoco(2010) ? "found" : "not found");
    Loco *loco2010 = csClient.findLocoInRoster(2010);
    if (loco2010) {
      Serial.print("getCSConsistByLeadLoco(loco2010)=");
      Serial.println(csClient.getCSConsistByLeadLoco(loco2010) ? "found" : "not found");
    }
    Loco *loco2014 = csClient.findLocoInRoster(2014);
    if (loco2014) {
      Serial.print("getCSConsistByMemberLoco(2014)=");
      Serial.println(csClient.getCSConsistByMemberLoco(2014) ? "found" : "not found");
      Serial.print("getCSConsistByMemberLoco(loco2014)=");
      Serial.println(csClient.getCSConsistByMemberLoco(loco2014) ? "found" : "not found");
    }
    Serial.println("Stopping the consist");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(csConsist, 0, Forward);
    waitForExpectations();
    Serial.println("Removing member 2016 (removeCSConsistMember())");
    csListener.expectCSConsist(2010);
    csClient.removeCSConsistMember(csConsist, 2016);
    waitForExpectations();
    printCSConsists();
    Serial.println("Deleting the CSConsist via the pointer overload (deleteCSConsist(csConsist))");
    Serial.println(">>> A consist deletion may not be broadcast - verify no consist 2010 remains on the CS console");
    csClient.deleteCSConsist(csConsist);
    testDelay(2000);
    Serial.println(
        "Building a reversed replicated-functions consist with lead loco 2030 (createCSConsist(2030, true, true))");
    CSConsist *reversedConsist = csClient.createCSConsist(2030, true, true);
    if (reversedConsist) {
      csListener.expectCSConsist(2030);
      csClient.addCSConsistMember(reversedConsist, 2014, true);
      waitForExpectations();
      printCSConsists();
      Serial.println("Deleting the reversed consist via the int overload (deleteCSConsist(2030))");
      Serial.println(">>> A consist deletion may not be broadcast - verify no consist 2030 remains on the CS console");
      csClient.deleteCSConsist(2030);
      testDelay(2000);
    }
    Serial.println("Clearing the client-side consist list (clearCSConsists() - local only)");
    csClient.clearCSConsists();
    testDelay(1000);
    printCSConsists();
  } else {
    Serial.println("ERROR: Could not create CSConsist");
  }
  Serial.println("Requesting the list of CSConsists from the CS (<^>)");
  Serial.println(">>> Verify no consists remain configured on the CS console");
  csClient.requestCSConsists();
  testDelay(2000);
  printCSConsists();
  csListener.printExpectationResult();
}

// T 12 - Automation handoff
static void testAutomationHandoff() {
  testBanner("Automation Handoff");
  csListener.clearExpectations();
  Serial.println("Handing off loco 3001 to AUTOMATION 301 (< / START 3001 301>)");
  Serial.println(">>> The automation drives loco 3001, so expect loco broadcasts from it");
  csListener.expectLocoBroadcast(3001, EXPECT_ANY, EXPECT_ANY, 4);
  csClient.handOffLoco(3001, 301);
  waitForExpectations(20000);
  csListener.printExpectationResult();
}

// T 13 - JMRI sensor list request
static void testJMRISensorList() {
  testBanner("JMRI Sensor List");
  csListener.clearExpectations();
  Serial.println("Requesting the JMRI sensor list (<Q>)");
  csListener.expectJMRISensorBroadcast(EXPECT_ANY, 21);
  csClient.requestJMRISensorList();
  waitForExpectations(15000);
  Serial.println(">>> Verify all 21 sensors are reported on the CS console");
  csListener.printExpectationResult();
}

// T 14 - CV programming
// Self-restoring: every value is read first and then written back unchanged, so no decoder state is altered.
// Requires a loco on the PROG track for the write phases (a failed read skips them).
static void testCVProgramming() {
  testBanner("CV Programming (read/write)");
  csListener.clearExpectations();
  csListener.lastReadLocoAddress = -1;
  csListener.lastWriteCVValue = -1;
  Serial.println("WARNING: This test drives the programming track and may affect a decoder if one is connected");
  Serial.println("WARNING: Later phases write a loco's CV while on the main - ensure a loco is on the layout");
  Serial.println(">>> Take the locos off the track or ensure a decoder is connected as the operator prefers");
  testDelay(10000);

  Serial.println("Reading the loco address from the programming track (<R>)");
  csListener.expectReadLoco(EXPECT_ANY);
  csClient.readLoco();
  waitForExpectations(15000);
  if (csListener.lastReadLocoAddress > 0) {
    Serial.println("Writing the same address back (writeLocoAddress() - restores, no net change)");
    csListener.expectWriteLoco(csListener.lastReadLocoAddress);
    csClient.writeLocoAddress(csListener.lastReadLocoAddress);
    waitForExpectations(15000);
  } else {
    Serial.println("No loco detected on the PROG track - skipping the address/CV write phases");
  }
  csListener.printExpectationResult();
  Serial.println();

  Serial.println("Reading CV29 from the programming track (readCV() <R 29>)");
  csListener.expectWriteCV(29, EXPECT_ANY);
  csClient.readCV(29);
  waitForExpectations(15000);
  if (csListener.lastWriteCVValue >= 0) {
    Serial.println("Validating CV29 with the same value (validateCV() - writes and verifies, no net change)");
    csListener.expectValidateCV(29, csListener.lastWriteCVValue);
    csClient.validateCV(29, csListener.lastWriteCVValue);
    waitForExpectations(15000);
    Serial.println("Writing CV29 bit 5 to 1 on the programming track (writeCVBit())");
    csListener.expectWriteCV(29, EXPECT_ANY);
    csClient.writeCVBit(29, 5, 1);
    waitForExpectations(15000);
    Serial.println("Validating CV29 bit 5 (validateCVBit())");
    csListener.expectValidateCVBit(29, 5, EXPECT_ANY);
    csClient.validateCVBit(29, 5, 1);
    waitForExpectations(15000);
    Serial.println("Writing CV29 on the main for loco 2010 (writeCVOnMain() <w 2010 29 value>)");
    Serial.println(
        ">>> Writes the CV29 value just read - no net change if the layout loco was the one on the PROG track");
    csListener.expectWriteCV(29, csListener.lastWriteCVValue);
    csClient.writeCVOnMain(2010, 29, csListener.lastWriteCVValue);
    waitForExpectations(15000);
    Serial.println("Writing CV29 bit 5 to 1 on the main for loco 2010 (writeCVBitOnMain())");
    csListener.expectWriteCV(29, EXPECT_ANY);
    csClient.writeCVBitOnMain(2010, 29, 5, 1);
    waitForExpectations(15000);
    Serial.println("Restoring CV29 on the main to its original value (writeCVOnMain() - self restore)");
    csListener.expectWriteCV(29, csListener.lastWriteCVValue);
    csClient.writeCVOnMain(2010, 29, csListener.lastWriteCVValue);
    waitForExpectations(15000);
    Serial.println("Restoring CV29 to its original value (writeCV() - self restore)");
    csListener.expectWriteCV(29, csListener.lastWriteCVValue);
    csClient.writeCV(29, csListener.lastWriteCVValue);
    waitForExpectations(15000);
  } else {
    Serial.println("No decoder detected on the PROG track (readCV failed) - skipping the write phases");
  }
  csListener.printExpectationResult();
}

// T 15 - Fast clock
static void testFastClock() {
  testBanner("Fast Clock");
  csListener.clearExpectations();
  Serial.println("Setting fast clock to 7:00am with speed factor 4");
  csListener.expectSetFastClock(420, 4);
  csClient.setFastClock(420, 4);
  waitForExpectations();
  Serial.println("Requesting the fast clock time (<jC>)");
  csListener.expectFastClockTime(EXPECT_ANY);
  csClient.requestFastClockTime();
  waitForExpectations();
  Serial.println(">>> Verify the fast clock display on the CS console");
  csListener.printExpectationResult();
}

// T 16 - Delayed activity with pause/resume
static void testDelayedActivity() {
  testBanner("Delayed Activity + Pause/Resume");
  csListener.clearExpectations();
  Serial.println("Starting ROUTE 708 (Delayed Activity) via startRoute() (< / START 708>)");
  csListener.expectTurnoutAction(100, true);
  csClient.startRoute(708);
  waitForExpectations(7000);
  Serial.println("Pausing all routes (< / PAUSE>) - verify the route stops on the CS console");
  csClient.pauseRoutes();
  testDelay(5000);
  Serial.println("Resuming all routes (< / RESUME>) - verify the route continues on the CS console");
  csClient.resumeRoutes();
  csListener.expectTurnoutAction(101, true);
  csListener.expectTurnoutAction(100, false);
  csListener.expectTurnoutAction(101, false);
  waitForExpectations(25000);
  Serial.println(">>> Route 708 should be complete on the CS console now");
  csListener.printExpectationResult();
}

// T 17 - Miscellaneous
static void testMiscellaneous() {
  testBanner("Miscellaneous");
  csListener.clearExpectations();
  Serial.print("Library version: ");
  Serial.println(csClient.getLibraryVersion());
  Serial.println("Clearing local locos (clearLocalLocos())");
  csClient.clearLocalLocos();
  testDelay(1000);
  Serial.println("Requesting the number of supported locos (<#>)");
  csClient.getNumberSupportedLocos();
  testDelay(2000);
  Serial.println("Emergency stop (emergencyStop() <!>)");
  Serial.println("WARNING: This powers off all tracks - the locos will come to an abrupt stop");
  csListener.expectTrackPower(PowerOff);
  csClient.emergencyStop();
  waitForExpectations(5000);
  Serial.println("Restoring track power (powerOn() <1>)");
  csListener.expectTrackPower(PowerOn);
  csClient.powerOn();
  waitForExpectations(5000);
  Serial.println("Enabling debug output and requesting the server version");
  csClient.setDebug(true);
  csClient.requestServerVersion();
  csListener.expectServerVersion();
  waitForExpectations();
  Serial.println("Disabling debug output");
  csClient.setDebug(false);
  testDelay(1000);
  Serial.println("Refreshing all lists (roster, turnouts, routes, turntables)");
  csClient.refreshAllLists();
  csListener.expectRosterList();
  csListener.expectTurnoutList();
  csListener.expectRouteList();
  csListener.expectTurntableList();
  waitForAllLists();
  Serial.print("receivedLists() after refresh=");
  Serial.println(csClient.receivedLists() ? "true" : "false");
  Serial.println("Lists after refresh:");
  printRoster();
  printTurnouts();
  printRoutes();
  printTurntables();
  Serial.print("Last server response time: ");
  Serial.println(csClient.getLastServerResponseTime());
  csListener.printExpectationResult();
}

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
  void (*fn)();            //<! Body that sets the expectations before the route is started
};

// Route test expectation bodies - set the expectations the route broadcasts must satisfy

// R 1 - CS route 700 (Loco Drive): drive roster loco 2010, function 0 on/off
static void routeLocoDrive() {
  csListener.expectLocoBroadcast(2010, EXPECT_ANY, EXPECT_ANY, 4);
  csListener.expectLocoUpdate(2010, 4);
}

// R 2 - CS route 701 (Local Loco Drive): drive non-roster loco 9999, function 1 on/off
static void routeLocalLocoDrive() { csListener.expectLocoBroadcast(9999, EXPECT_ANY, EXPECT_ANY, 4); }

// R 3 - CS route 702 (Turnout Ops): throw/close/toggle 100/101/110/102
static void routeTurnoutOps() {
  csListener.expectTurnoutAction(100, true);
  csListener.expectTurnoutAction(100, false);
  csListener.expectTurnoutAction(101, EXPECT_ANY, 2);
  csListener.expectTurnoutAction(110, true);
  csListener.expectTurnoutAction(110, false);
  csListener.expectTurnoutAction(102, true);
  csListener.expectTurnoutAction(102, false);
}

// R 4 - CS route 703 (Turntable Ops): rotate DCC turntables 2 and 4
static void routeTurntableOps() {
  csListener.expectTurntableAction(2, 2);
  csListener.expectTurntableAction(2, 4);
  csListener.expectTurntableAction(2, 0);
  csListener.expectTurntableAction(4, 1);
  csListener.expectTurntableAction(4, 3);
  csListener.expectTurntableAction(4, 0);
}

// R 5 - CS route 704 (Power Changes): global power x3 and individual track B x2
static void routePowerChanges() {
  csListener.expectTrackPower(PowerOff);
  csListener.expectTrackPower(PowerOn, 2);
  csListener.expectIndividualTrackPower('B', PowerOff);
  csListener.expectIndividualTrackPower('B', PowerOn);
}

// R 6 - CS route 705 (Messages): two broadcast messages and one screen update
static void routeMessages() {
  csListener.expectMessage("Route 705 message to all throttles");
  csListener.expectScreenUpdate(0, 1, "Route 705 screen update");
  csListener.expectMessage("Route 705 second message");
}

// R 7 - CS route 706 (Consist Ops): build, drive and break a consist led by 2010
// R 7 - CS route 706 (Consist Ops): build 2014, build 2016, then break
// (the build broadcasts receivedCSConsist(); the break/delete may not be broadcast)
static void routeConsistOps() { csListener.expectCSConsist(2010, 2); }

// R 8 - CS route 500 (Sensor Test): activate/deactivate the JMRI sensors
static void routeJMRISensorTest() {
  csListener.expectJMRISensorBroadcast(6000, 2);
  csListener.expectJMRISensorBroadcast(6001, 2);
  csListener.expectJMRISensorBroadcast(6100, 2);
  csListener.expectJMRISensorBroadcast(6101, 2);
  csListener.expectJMRISensorBroadcast(6200, 2);
  csListener.expectJMRISensorBroadcast(6201, 2);
  csListener.expectJMRISensorBroadcast(6300, 2);
}

// R 9 - CS route 708 (Delayed Activity): throw/close 100 and 101 spread over 20 seconds
static void routeDelayedActivity() {
  csListener.expectTurnoutAction(100, true);
  csListener.expectTurnoutAction(101, true);
  csListener.expectTurnoutAction(100, false);
  csListener.expectTurnoutAction(101, false);
}

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
    {14, "CV Programming (read/write)", "read/write/validate CVs, self restoring", testCVProgramming},
    {15, "Fast Clock", "set + request time", testFastClock},
    {16, "Delayed Activity + Pause/Resume", "startRoute 708", testDelayedActivity},
    {17, "Miscellaneous", "locos/accessories/debug/lists", testMiscellaneous},
};

static const RouteTest routeTests[] = {
    {1, "Loco Drive", "loco 2010", 700, 25000, routeLocoDrive},
    {2, "Local Loco Drive", "loco 9999", 701, 25000, routeLocalLocoDrive},
    {3, "Turnout Ops", "", 702, 25000, routeTurnoutOps},
    {4, "Turntable Ops", "", 703, 30000, routeTurntableOps},
    {5, "Power Changes", "", 704, 25000, routePowerChanges},
    {6, "Messages", "", 705, 25000, routeMessages},
    {7, "Consist Ops", "", 706, 25000, routeConsistOps},
    {8, "JMRI Sensor Test", "", 500, 30000, routeJMRISensorTest},
    {9, "Delayed Activity", "long running", 708, 30000, routeDelayedActivity},
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
  if (!csConnected) {
    Serial.println("ERROR: Not connected to the command station");
    Serial.println(">>> Enter <C> first to connect, then try again with <T id>");
    return;
  }
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
  if (!csConnected) {
    Serial.println("ERROR: Not connected to the command station");
    Serial.println(">>> Enter <C> first to connect, then try again with <R id>");
    return;
  }
  const RouteTest *test = findRouteTest(testId);
  if (!test) {
    Serial.print("ERROR: Unknown route test id: ");
    Serial.println(testId);
    return;
  }
  testBanner(test->name);
  csListener.clearExpectations();
  if (test->fn) {
    test->fn();
  }
  Serial.print("Starting CS route ");
  Serial.print(test->csRouteId);
  Serial.println(" via startRoute() (< / START id>)");
  csClient.startRoute(test->csRouteId);
  Serial.print(">>> Monitoring ");
  Serial.print(test->observeMs / 1000);
  Serial.println(" seconds for route activity...");
  waitForExpectations(test->observeMs);
  Serial.println(">>> Route test complete - verify the PRINT markers on the CS console");
  csListener.printExpectationResult();
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