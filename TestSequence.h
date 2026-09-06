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
 * @brief Start a compact `<< <n> ...` phase marker on the DUT serial console
 *
 * @details The operator narration lives in README.md (see AGENTS.md), so the DUT prints short `<< <phase> <op>`
 * markers instead of full sentences. Markers reuse a small pool of shared literals at each call site - the same
 * fragment-reuse idea as DCCEXProtocol's _cmdAppend(), applied to streamed Serial output rather than a buffer.
 * @param n Phase number within the test
 */
static void phase(int n) {
  Serial.print("<< ");
  Serial.print(n);
  Serial.print(' ');
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
  Serial.println("Verify the version matches - all lists print below:");
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
    Serial.println("connecting to the CS over WiFi...");
    if (!wifiClient.connect(serverAddress, serverPort)) {
      Serial.println("ERROR: connection to the CS failed");
      Serial.println("  is myAutomation.h running? myWiFi.h correct? then retry <C>");
      return false;
    }
    csClient.connect(&wifiClient);
  }
#elif defined(ARDUINO_ARCH_STM32)
  csClient.connect(&Serial1);
#endif
  Serial.println("requesting version + lists...");
  csClient.refreshAllLists();
  csClient.requestServerVersion();
  Serial.print("Waiting up to ");
  Serial.print(CONNECT_TIMEOUT / 1000);
  Serial.println(" s for the CS to respond...");
  unsigned long start = millis();
  while (!(csClient.receivedVersion() && csClient.receivedLists())) {
    if (millis() - start > CONNECT_TIMEOUT) {
      Serial.println("ERROR: no response from the CS");
      Serial.println("  is the CS powered with myAutomation.h? retry <C>");
      return false;
    }
    csClient.getLists(); // gated: requests the next list once the previous one is in
    csClient.check();
  }
  csConnected = true;
  printConnectionSummary();
  Serial.println("verify myAutomation.h objects:");
  Serial.println("  - roster 2010/2014/2016/2030");
  Serial.println("  - turnouts 100/101/102/110 and 121 (linear acc 500)");
  Serial.println("  - turntables 2/4 and 21 JMRI sensors (6000-6300)");
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
      Serial.println("ERROR: timed out waiting for the lists");
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
  phase(1);
  Serial.println("reset lists, request version");
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
  phase(2);
  Serial.println("refresh all lists");
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
  phase(3);
  Serial.println("received flags");
  Serial.print("  roster=");
  Serial.println(csClient.receivedRoster() ? "true" : "false");
  Serial.print("  turnouts=");
  Serial.println(csClient.receivedTurnoutList() ? "true" : "false");
  Serial.print("  routes=");
  Serial.println(csClient.receivedRouteList() ? "true" : "false");
  Serial.print("  turntables=");
  Serial.println(csClient.receivedTurntableList() ? "true" : "false");
  csListener.printExpectationResult();
}

// T 2 - Roster loco control
static void testRosterLocoControl() {
  testBanner("Roster Loco Control");
  csListener.clearExpectations();
  Loco *loco2010 = csClient.findLocoInRoster(2010);
  if (loco2010) {
    phase(1);
    Serial.println("roster loco 2010");
    printLoco(loco2010);
    phase(2);
    Serial.print("throttle ");
    Serial.print(2010);
    Serial.println(" <t 2010 30 1>");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(loco2010, 30, Forward);
    waitForExpectations();
    phase(3);
    Serial.println("fn 0 on");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.functionOn(loco2010, 0);
    waitForExpectations();
    phase(4);
    Serial.println("fn 0 off");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.functionOff(loco2010, 0);
    waitForExpectations();
    phase(5);
    Serial.print("throttle ");
    Serial.print(2010);
    Serial.println(" <t 2010 60 0>");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(loco2010, 60, Reverse);
    waitForExpectations();
    phase(6);
    Serial.print("stop ");
    Serial.println(2010);
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(loco2010, 0, Forward);
    waitForExpectations();
    phase(7);
    Serial.print("update ");
    Serial.print(2010);
    Serial.println(" <t 2010>");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.requestLocoUpdate(2010);
    waitForExpectations();
    Serial.print("isFunctionOn(loco2010, 0)=");
    Serial.println(csClient.isFunctionOn(loco2010, 0) ? "true" : "false");
    Serial.println(">> cross-check on the CS console");
  } else {
    Serial.println("ERROR: Loco 2010 not found in roster");
  }
  csListener.printExpectationResult();
}

// T 3 - Local (non-roster) loco control
static void testLocalLocoControl() {
  testBanner("Local Loco Control");
  csListener.clearExpectations();
  phase(1);
  Serial.print("new loco ");
  Serial.println(9999);
  Loco *localLoco = new Loco(9999, LocoSourceEntry);
  localLoco->setName("Local 9999");
  printLocalLocos();
  printLoco(localLoco);
  phase(2);
  Serial.print("throttle ");
  Serial.print(9999);
  Serial.println(" <t 9999 25 1>");
  csListener.expectLocoBroadcast(9999);
  csListener.expectLocoUpdate(9999);
  csClient.setThrottle(localLoco, 25, Forward);
  waitForExpectations();
  phase(3);
  Serial.println("fn 1 on");
  csListener.expectLocoBroadcast(9999);
  csListener.expectLocoUpdate(9999);
  csClient.functionOn(localLoco, 1);
  waitForExpectations();
  phase(4);
  Serial.print("stop ");
  Serial.println(9999);
  csListener.expectLocoBroadcast(9999);
  csListener.expectLocoUpdate(9999);
  csClient.setThrottle(localLoco, 0, Forward);
  waitForExpectations();
  phase(5);
  Serial.println("delete local loco");
  delete localLoco;
  testDelay(1000);
  printLocalLocos();
  csListener.printExpectationResult();
}

// T 4 - Turnout control
static void testTurnoutControl() {
  testBanner("Turnout Control");
  csListener.clearExpectations();
  phase(1);
  Serial.print("turnout ");
  Serial.print(100);
  Serial.println(" throw");
  csListener.expectTurnoutAction(100, true);
  csClient.throwTurnout(100);
  waitForExpectations();
  phase(2);
  Serial.print("turnout ");
  Serial.print(100);
  Serial.println(" close <T 100 0>");
  csListener.expectTurnoutAction(100, false);
  csClient.closeTurnout(100);
  waitForExpectations();
  phase(3);
  Serial.print("toggle ");
  Serial.println(101);
  csListener.expectTurnoutAction(101, EXPECT_ANY);
  csClient.toggleTurnout(101);
  waitForExpectations();
  phase(4);
  Serial.print("toggle ");
  Serial.println(101);
  csListener.expectTurnoutAction(101, EXPECT_ANY);
  csClient.toggleTurnout(101);
  waitForExpectations();
  phase(5);
  Serial.print("turnout ");
  Serial.print(102);
  Serial.println(" throw");
  csListener.expectTurnoutAction(102, true);
  csClient.throwTurnout(102);
  waitForExpectations();
  Turnout *turnout100 = csClient.getTurnoutById(100);
  if (turnout100) {
    Serial.print("getTurnoutById(100) state=");
    Serial.println(turnout100->getThrown() ? "Thrown" : "Closed");
  }
  Serial.println(">> cross-check on the CS console");
  csListener.printExpectationResult();
}

// T 5 - Turntable control
static void testTurntableControl() {
  testBanner("Turntable Control");
  csListener.clearExpectations();
  phase(1);
  Serial.print("rot tt ");
  Serial.print(2);
  Serial.println(" -> 2");
  csListener.expectTurntableAction(2, 2);
  csClient.rotateTurntable(2, 2);
  waitForExpectations(8000);
  phase(2);
  Serial.print("rot tt ");
  Serial.print(2);
  Serial.println(" -> 4");
  csListener.expectTurntableAction(2, 4);
  csClient.rotateTurntable(2, 4);
  waitForExpectations(8000);
  phase(3);
  Serial.print("rot tt ");
  Serial.print(2);
  Serial.println(" -> home");
  csListener.expectTurntableAction(2, 0);
  csClient.rotateTurntable(2, 0);
  waitForExpectations(8000);
  phase(4);
  Serial.print("rot tt ");
  Serial.print(4);
  Serial.println(" -> 1");
  csListener.expectTurntableAction(4, 1);
  csClient.rotateTurntable(4, 1);
  waitForExpectations(8000);
  phase(5);
  Serial.print("rot tt ");
  Serial.print(4);
  Serial.println(" -> 3");
  csListener.expectTurntableAction(4, 3);
  csClient.rotateTurntable(4, 3);
  waitForExpectations(8000);
  Turntable *turntable2 = csClient.getTurntableById(2);
  if (turntable2) {
    Serial.print("getTurntableById(2) index=");
    Serial.println(turntable2->getIndex());
  }
  Serial.println(">> cross-check on the CS console");
  csListener.printExpectationResult();
}

// T 6 - Track power
static void testTrackPower() {
  testBanner("Track Power");
  csListener.clearExpectations();
  phase(1);
  Serial.println("power all on <1>");
  csListener.expectTrackPower(PowerOn);
  csClient.powerOn();
  waitForExpectations();
  phase(2);
  Serial.println("power all off <0>");
  csListener.expectTrackPower(PowerOff);
  csClient.powerOff();
  waitForExpectations();
  phase(3);
  Serial.println("power all on <1>");
  csListener.expectTrackPower(PowerOn);
  csClient.powerOn();
  waitForExpectations();
  phase(4);
  Serial.println("MAIN off then on <0/1 MAIN>");
  Serial.println(">> MAIN may broadcast global or per track");
  csListener.expectTrackPower(EXPECT_ANY);
  csClient.powerMainOff();
  waitForExpectations();
  csListener.expectTrackPower(EXPECT_ANY);
  csClient.powerMainOn();
  waitForExpectations();
  phase(5);
  Serial.println("PROG off then on <0/1 PROG>");
  Serial.println(">> PROG power often not broadcast");
  csClient.powerProgOff();
  testDelay(3000);
  csClient.powerProgOn();
  testDelay(3000);
  phase(6);
  Serial.println("track B on then off <1/0 B>");
  csListener.expectIndividualTrackPower('B', PowerOn);
  csListener.expectTrackPower(PowerOn);
  csClient.powerTrackOn('B');
  waitForExpectations();
  csListener.expectIndividualTrackPower('B', PowerOff);
  csListener.expectTrackPower(PowerOff);
  csClient.powerTrackOff('B');
  waitForExpectations();
  phase(7);
  Serial.println("join PROG <1 JOIN>");
  Serial.println(">> joinProg() may not broadcast");
  csClient.joinProg();
  testDelay(3000);
  csListener.printExpectationResult();
}

// T 7 - Track types
static void testTrackTypes() {
  testBanner("Track Types");
  csListener.clearExpectations();
  Serial.println("NOTE: changes track modes on the CS");
  phase(1);
  Serial.println("A -> MAIN");
  csListener.expectTrackType('A', MAIN, EXPECT_ANY);
  csClient.setTrackType('A', MAIN, 0);
  waitForExpectations(5000);
  phase(2);
  Serial.println("A -> PROG");
  csListener.expectTrackType('A', PROG, EXPECT_ANY);
  csClient.setTrackType('A', PROG, 0);
  waitForExpectations(5000);
  phase(3);
  Serial.println("A -> DC 5");
  csListener.expectTrackType('A', DC, 5);
  csClient.setTrackType('A', DC, 5);
  waitForExpectations(5000);
  phase(4);
  Serial.println("A -> DCX 6");
  csListener.expectTrackType('A', DCX, 6);
  csClient.setTrackType('A', DCX, 6);
  waitForExpectations(5000);
  phase(5);
  Serial.println("A -> MAIN");
  csListener.expectTrackType('A', MAIN, EXPECT_ANY);
  csClient.setTrackType('A', MAIN, 0);
  waitForExpectations(5000);
  Serial.println(">> cross-check on the CS console");
  csListener.printExpectationResult();
}

// T 8 - Track currents
static void testTrackCurrents() {
  testBanner("Track Currents");
  csListener.clearExpectations();
  phase(1);
  Serial.println("gauges <jG>");
  csListener.expectTrackCurrentGauge(EXPECT_ANY, EXPECT_ANY, 8);
  csClient.requestTrackCurrentGauges();
  waitForExpectations(15000);
  phase(2);
  Serial.println("currents <jI>");
  csListener.expectTrackCurrent(EXPECT_ANY, EXPECT_ANY, 8);
  csClient.requestTrackCurrents();
  waitForExpectations(15000);
  Serial.println(">> cross-check on the CS console");
  csListener.printExpectationResult();
}

// T 9 - Momentum
static void testMomentum() {
  testBanner("Momentum");
  csListener.clearExpectations();
  Serial.println("NOTE: no callbacks - check the CS console");
  phase(1);
  Serial.println("alg -> Linear");
  csClient.setMomentumAlgorithm(Linear);
  testDelay(2000);
  phase(2);
  Serial.println("default mom 10");
  csClient.setDefaultMomentum(10);
  testDelay(2000);
  phase(3);
  Serial.println("default mom 10/5");
  csClient.setDefaultMomentum(10, 5);
  testDelay(2000);
  phase(4);
  Serial.print("mom ");
  Serial.print(2010);
  Serial.println(" 20");
  csClient.setMomentum(2010, 20);
  testDelay(2000);
  phase(5);
  Serial.print("mom ");
  Serial.print(2010);
  Serial.println(" 10/5");
  csClient.setMomentum(2010, 10, 5);
  testDelay(2000);
  Loco *loco2014 = csClient.findLocoInRoster(2014);
  if (loco2014) {
    phase(6);
    Serial.print("mom ");
    Serial.print(2014);
    Serial.println(" 15");
    csClient.setMomentum(loco2014, 15);
    testDelay(2000);
    phase(7);
    Serial.print("mom ");
    Serial.print(2014);
    Serial.println(" 15/10");
    csClient.setMomentum(loco2014, 15, 10);
    testDelay(2000);
  }
  csListener.printExpectationResult();
}

// T 10 - DCC accessories
static void testAccessories() {
  testBanner("DCC Accessories");
  csListener.clearExpectations();
  Serial.println("NOTE: no callbacks - check the CS console");
  phase(1);
  Serial.println("acc 10 sub 1 on");
  csClient.activateAccessory(10, 1);
  testDelay(2000);
  phase(2);
  Serial.println("acc 10 sub 1 off");
  csClient.deactivateAccessory(10, 1);
  testDelay(2000);
  phase(3);
  Serial.println("lacc 500 on");
  csClient.activateLinearAccessory(500);
  testDelay(2000);
  phase(4);
  Serial.println("lacc 500 off");
  csClient.deactivateLinearAccessory(500);
  testDelay(2000);
  csListener.printExpectationResult();
}

// T 11 - Command station consists
static void testConsistOps() {
  testBanner("Command Station Consists");
  csListener.clearExpectations();
  phase(1);
  Serial.println("consist lead 2010");
  CSConsist *csConsist = csClient.createCSConsist(2010);
  if (csConsist) {
    phase(2);
    Serial.println("member 2014 rev");
    csListener.expectCSConsist(2010);
    csClient.addCSConsistMember(csConsist, 2014, true);
    waitForExpectations();
    phase(3);
    Serial.println("member 2016");
    csListener.expectCSConsist(2010);
    csClient.addCSConsistMember(csConsist, 2016);
    waitForExpectations();
    printCSConsists();
    phase(4);
    Serial.println("consist thr 20 F");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(csConsist, 20, Forward);
    waitForExpectations();
    phase(5);
    Serial.println("consist fn 0 on");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.functionOn(csConsist, 0);
    waitForExpectations();
    phase(6);
    Serial.println("consist fn 0 off");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.functionOff(csConsist, 0);
    waitForExpectations();
    Serial.print("isFunctionOn(csConsist, 0)=");
    Serial.println(csClient.isFunctionOn(csConsist, 0) ? "true" : "false");
    phase(7);
    Serial.println("lookup by lead/member");
    Serial.print("byLead(2010)=");
    Serial.println(csClient.getCSConsistByLeadLoco(2010) ? "found" : "not found");
    Loco *loco2010 = csClient.findLocoInRoster(2010);
    if (loco2010) {
      Serial.print("byLead(loco2010)=");
      Serial.println(csClient.getCSConsistByLeadLoco(loco2010) ? "found" : "not found");
    }
    Loco *loco2014 = csClient.findLocoInRoster(2014);
    if (loco2014) {
      Serial.print("byMember(2014)=");
      Serial.println(csClient.getCSConsistByMemberLoco(2014) ? "found" : "not found");
      Serial.print("byMember(loco2014)=");
      Serial.println(csClient.getCSConsistByMemberLoco(loco2014) ? "found" : "not found");
    }
    phase(8);
    Serial.println("consist stop");
    csListener.expectLocoBroadcast(2010);
    csListener.expectLocoUpdate(2010);
    csClient.setThrottle(csConsist, 0, Forward);
    waitForExpectations();
    phase(9);
    Serial.println("remove member 2016");
    csListener.expectCSConsist(2010);
    csClient.removeCSConsistMember(csConsist, 2016);
    waitForExpectations();
    printCSConsists();
    phase(10);
    Serial.println("delete consist 2010");
    Serial.println(">> deletion may not broadcast - check CS console");
    csClient.deleteCSConsist(csConsist);
    testDelay(2000);
    phase(11);
    Serial.println("replicated-functions consist lead 2030");
    CSConsist *reversedConsist = csClient.createCSConsist(2030, true, true);
    if (reversedConsist) {
      csListener.expectCSConsist(2030);
      csClient.addCSConsistMember(reversedConsist, 2014, true);
      waitForExpectations();
      printCSConsists();
      phase(12);
      Serial.println("delete consist 2030");
      Serial.println(">> deletion may not broadcast - check CS console");
      csClient.deleteCSConsist(2030);
      testDelay(2000);
    }
    phase(13);
    Serial.println("clear consist list");
    csClient.clearCSConsists();
    testDelay(1000);
    printCSConsists();
  } else {
    Serial.println("ERROR: Could not create CSConsist");
  }
  phase(14);
  Serial.println("req consist list <^>");
  Serial.println(">> cross-check on the CS console");
  csClient.requestCSConsists();
  testDelay(2000);
  printCSConsists();
  csListener.printExpectationResult();
}

// T 12 - Automation handoff
static void testAutomationHandoff() {
  testBanner("Automation Handoff");
  csListener.clearExpectations();
  phase(1);
  Serial.println("handoff 3001 -> automation 301");
  Serial.println(">> automation drives loco 3001 - expect broadcasts");
  csListener.expectLocoBroadcast(3001, EXPECT_ANY, EXPECT_ANY, 4);
  csClient.handOffLoco(3001, 301);
  waitForExpectations(20000);
  csListener.printExpectationResult();
}

// T 13 - JMRI sensor list request
static void testJMRISensorList() {
  testBanner("JMRI Sensor List");
  csListener.clearExpectations();
  phase(1);
  Serial.println("sensor list <Q>");
  csListener.expectJMRISensorBroadcast(EXPECT_ANY, 21);
  csClient.requestJMRISensorList();
  waitForExpectations(15000);
  phase(2);
  Serial.println("expect 21 broadcasts");
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
  Serial.println("WARNING: drives the PROG track");
  Serial.println("WARNING: later writes CVs on the main");
  Serial.println(">> remove locos or use a decoder as you prefer");
  testDelay(10000);

  phase(1);
  Serial.println("read addr <R>");
  csListener.expectReadLoco(EXPECT_ANY);
  csClient.readLoco();
  waitForExpectations(15000);
  if (csListener.lastReadLocoAddress > 0) {
    phase(2);
    Serial.println("write addr back");
    csListener.expectWriteLoco(csListener.lastReadLocoAddress);
    csClient.writeLocoAddress(csListener.lastReadLocoAddress);
    waitForExpectations(15000);
  } else {
    Serial.println(">> no loco on PROG - skipped addr/CV writes");
  }
  csListener.printExpectationResult();
  Serial.println();

  phase(3);
  Serial.println("read cv 29");
  csListener.expectWriteCV(29, EXPECT_ANY);
  csClient.readCV(29);
  waitForExpectations(15000);
  if (csListener.lastWriteCVValue >= 0) {
    phase(4);
    Serial.println("validate cv 29");
    csListener.expectValidateCV(29, csListener.lastWriteCVValue);
    csClient.validateCV(29, csListener.lastWriteCVValue);
    waitForExpectations(15000);
    phase(5);
    Serial.println("cv bit 29:5 -> 1");
    csListener.expectWriteCV(29, EXPECT_ANY);
    csClient.writeCVBit(29, 5, 1);
    waitForExpectations(15000);
    phase(6);
    Serial.println("validate bit 29:5");
    csListener.expectValidateCVBit(29, 5, EXPECT_ANY);
    csClient.validateCVBit(29, 5, 1);
    waitForExpectations(15000);
    phase(7);
    Serial.println("cv 29 on main 2010");
    Serial.println(">> writes the value just read - no net change if same loco");
    csListener.expectWriteCV(29, csListener.lastWriteCVValue);
    csClient.writeCVOnMain(2010, 29, csListener.lastWriteCVValue);
    waitForExpectations(15000);
    phase(8);
    Serial.println("cv bit 29:5 on main 2010");
    csListener.expectWriteCV(29, EXPECT_ANY);
    csClient.writeCVBitOnMain(2010, 29, 5, 1);
    waitForExpectations(15000);
    phase(9);
    Serial.println("restore main cv 29");
    csListener.expectWriteCV(29, csListener.lastWriteCVValue);
    csClient.writeCVOnMain(2010, 29, csListener.lastWriteCVValue);
    waitForExpectations(15000);
    phase(10);
    Serial.println("restore cv 29");
    csListener.expectWriteCV(29, csListener.lastWriteCVValue);
    csClient.writeCV(29, csListener.lastWriteCVValue);
    waitForExpectations(15000);
  } else {
    Serial.println(">> no decoder on PROG - skipped CV writes");
  }
  csListener.printExpectationResult();
}

// T 15 - Fast clock
static void testFastClock() {
  testBanner("Fast Clock");
  csListener.clearExpectations();
  phase(1);
  Serial.println("clock 07:00 x4");
  csListener.expectSetFastClock(420, 4);
  csClient.setFastClock(420, 4);
  waitForExpectations();
  phase(2);
  Serial.println("clock time <jC>");
  csListener.expectFastClockTime(EXPECT_ANY);
  csClient.requestFastClockTime();
  waitForExpectations();
  Serial.println(">> cross-check on the CS console");
  csListener.printExpectationResult();
}

// T 16 - Delayed activity with pause/resume
static void testDelayedActivity() {
  testBanner("Delayed Activity + Pause/Resume");
  csListener.clearExpectations();
  phase(1);
  Serial.println("start route 708");
  csListener.expectTurnoutAction(100, true);
  csClient.startRoute(708);
  waitForExpectations(7000);
  phase(2);
  Serial.println("pause routes < / PAUSE>");
  csClient.pauseRoutes();
  testDelay(5000);
  phase(3);
  Serial.println("resume routes < / RESUME>");
  csClient.resumeRoutes();
  csListener.expectTurnoutAction(101, true);
  csListener.expectTurnoutAction(100, false);
  csListener.expectTurnoutAction(101, false);
  waitForExpectations(25000);
  Serial.println(">> route 708 should be complete now");
  csListener.printExpectationResult();
}

// T 17 - Miscellaneous
static void testMiscellaneous() {
  testBanner("Miscellaneous");
  csListener.clearExpectations();
  Serial.print("Library version: ");
  Serial.println(csClient.getLibraryVersion());
  phase(1);
  Serial.println("clear local locos");
  csClient.clearLocalLocos();
  testDelay(1000);
  phase(2);
  Serial.println("supported locos <#>");
  csClient.getNumberSupportedLocos();
  testDelay(2000);
  phase(3);
  Serial.println("emergency stop");
  Serial.println("WARNING: powers off all tracks");
  csListener.expectTrackPower(PowerOff);
  csClient.emergencyStop();
  waitForExpectations(5000);
  phase(4);
  Serial.println("power all on <1>");
  csListener.expectTrackPower(PowerOn);
  csClient.powerOn();
  waitForExpectations(5000);
  phase(5);
  Serial.println("legacy Consist loco 2015");
  Consist legacyConsist;
  legacyConsist.addLoco(2015, FacingForward);
  csClient.setThrottle(&legacyConsist, 25, Forward); // local user-speed mutation, no CS traffic
  Serial.print("legacy speed = ");
  Serial.println(legacyConsist.getSpeed());
  csClient.functionOn(&legacyConsist, 1); // sends <F 2015 1 1>
  testDelay(2000);
  Serial.print("legacy fn1 after on = ");
  Serial.println(csClient.isFunctionOn(&legacyConsist, 1) ? "true" : "false");
  csClient.functionOff(&legacyConsist, 1); // sends <F 2015 1 0>
  testDelay(2000);
  Serial.print("legacy fn1 after off = ");
  Serial.println(csClient.isFunctionOn(&legacyConsist, 1) ? "true" : "false");
  Serial.println(">> cross-check on the CS console");
  phase(6);
  Serial.println("debug on");
  csClient.setDebug(true);
  csClient.requestServerVersion();
  csListener.expectServerVersion();
  waitForExpectations();
  phase(7);
  Serial.println("debug off");
  csClient.setDebug(false);
  testDelay(1000);
  phase(8);
  Serial.println("clear all lists");
  csClient.clearAllLists();
  testDelay(1000);
  phase(9);
  Serial.println("refresh all lists");
  csClient.refreshAllLists();
  csListener.expectRosterList();
  csListener.expectTurnoutList();
  csListener.expectRouteList();
  csListener.expectTurntableList();
  waitForAllLists();
  Serial.print("lists received=");
  Serial.println(csClient.receivedLists() ? "true" : "false");
  Serial.println("Lists after refresh:");
  printRoster();
  printTurnouts();
  printRoutes();
  printTurntables();
  Serial.print("last response time: ");
  Serial.println(csClient.getLastServerResponseTime());
  phase(10);
  Serial.println("sendCommand <J C>");
  csListener.expectFastClockTime(EXPECT_ANY);
  csClient.sendCommand("J C");
  waitForExpectations();
  csListener.printExpectationResult();
}

// T 18 - Explicit per-list clear + refresh
static void testListMaintenance() {
  testBanner("List Maintenance");
  csListener.clearExpectations();
  phase(1);
  Serial.println("clear roster");
  csClient.clearRoster();
  testDelay(500);
  phase(2);
  Serial.println("fetch roster");
  csListener.expectRosterList();
  csClient.refreshRoster();
  waitForExpectations(15000);
  Serial.print("Roster count: ");
  Serial.println(csClient.getRosterCount());
  phase(3);
  Serial.println("clear turnouts");
  csClient.clearTurnoutList();
  testDelay(500);
  phase(4);
  Serial.println("fetch turnouts");
  csListener.expectTurnoutList();
  csClient.refreshTurnoutList();
  waitForExpectations(15000);
  Serial.print("Turnout count: ");
  Serial.println(csClient.getTurnoutCount());
  phase(5);
  Serial.println("clear routes");
  csClient.clearRouteList();
  testDelay(500);
  phase(6);
  Serial.println("fetch routes");
  csListener.expectRouteList();
  csClient.refreshRouteList();
  waitForExpectations(15000);
  Serial.print("Route count: ");
  Serial.println(csClient.getRouteCount());
  phase(7);
  Serial.println("clear turntables");
  csClient.clearTurntableList();
  testDelay(500);
  phase(8);
  Serial.println("fetch turntables");
  csListener.expectTurntableList();
  csClient.refreshTurntableList();
  waitForExpectations(15000);
  Serial.print("Turntable count: ");
  Serial.println(csClient.getTurntableCount());
  Serial.println(">> cross-check on the CS console");
  csListener.printExpectationResult();
}

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
    {18, "List Maintenance", "per-list clear + refresh", testListMaintenance},
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
  Serial.println();
  Serial.println("============================================================");
  Serial.println("DCCEXProtocol testing menu");
  Serial.println("============================================================");
  Serial.println("Local tests on the DUT (<T id>):");
  for (int i = 0; i < localTestCount; i++) {
    Serial.print("  <T ");
    if (localTests[i].id < 10)
      Serial.print(' ');
    Serial.print(localTests[i].id);
    Serial.print(">  ");
    printPadded(localTests[i].name, 34);
    Serial.println(localTests[i].description);
  }
  Serial.println();
  Serial.println("Routes to start on the CS (<R id>):");
  for (int i = 0; i < routeTestCount; i++) {
    Serial.print("  <R ");
    if (routeTests[i].id < 10)
      Serial.print(' ');
    Serial.print(routeTests[i].id);
    Serial.print(">  ");
    printPadded(routeTests[i].name, 24);
    Serial.print("CS route ");
    Serial.print(routeTests[i].csRouteId);
    if (routeTests[i].description[0]) {
      Serial.print("  (");
      Serial.print(routeTests[i].description);
      Serial.print(")");
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
    Serial.println("ERROR: not connected");
    Serial.println("  use <C> first, then <T id>");
    return;
  }
  const LocalTest *test = findLocalTest(testId);
  if (test) {
    test->fn();
  } else {
    Serial.print("ERROR: unknown local test id: ");
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
    Serial.println("ERROR: not connected");
    Serial.println("  use <C> first, then <R id>");
    return;
  }
  const RouteTest *test = findRouteTest(testId);
  if (!test) {
    Serial.print("ERROR: unknown route test id: ");
    Serial.println(testId);
    return;
  }
  testBanner(test->name);
  csListener.clearExpectations();
  if (test->fn) {
    test->fn();
  }
  Serial.print("<< startRoute ");
  Serial.print(test->csRouteId);
  Serial.println(" < / START id>");
  csClient.startRoute(test->csRouteId);
  Serial.print(">> monitoring ");
  Serial.print(test->observeMs / 1000);
  Serial.println("s for route activity");
  waitForExpectations(test->observeMs);
  Serial.println(">> route complete - check CS PRINT markers");
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