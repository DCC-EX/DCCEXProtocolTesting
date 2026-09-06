/**
 * @file TestListener.h
 * @brief Custom delegate to handle all DCCEXProtocolDelegate events.
 *
 * @details Implements every callback from DCCEXProtocolDelegate so that all broadcasts/responses from the
 * EX-CommandStation are printed to the serial console of the device under test for visual verification.
 *
 * The validation engine lets tests set typed expectations before driving activity, then wait for them in an
 * observation window. See the expect*() helpers below and waitForExpectations() in TestSequence.h. Rules:
 *  - no expectation for a callback        -> print normally, ignore
 *  - expectation but different object    -> print normally, ignore (never fails)
 *  - expectation, same object, bad value -> immediate FAIL (UNEXPECTED VALUE), test marked failed
 *  - event matches                        -> expectation satisfied
 *  - window expires with expectations left -> FAIL listing every missing expectation
 * EXPECT_ANY (-2) matches any value (note -1 is used by the library for failed read/write responses).
 */

#ifndef TEST_LISTENER_H
#define TEST_LISTENER_H

#include <string.h>

#include "PrintHelpers.h"

static const int EXPECT_ANY = -2;             //<! Match any value for an expectation argument
static const int maxExpectations = 40;        //<! Maximum number of simultaneous expectations
static const int expectationWindowMs = 10000; //<! Default observation window for an expectation phase

/**
 * @brief The delegate callback each expectation is checked against
 */
enum ExpectType {
  TRACK_POWER,
  INDIVIDUAL_TRACK_POWER,
  TRACK_CURRENT_GAUGE,
  TRACK_CURRENT,
  TRACK_TYPE,
  TURNOUT_ACTION,
  TURNTABLE_ACTION,
  LOCO_UPDATE,
  LOCO_BROADCAST,
  READ_LOCO,
  VALIDATE_CV,
  VALIDATE_CV_BIT,
  WRITE_LOCO,
  WRITE_CV,
  MESSAGE,
  SCREEN_UPDATE,
  CS_CONSIST,
  SET_FAST_CLOCK,
  FAST_CLOCK_TIME,
  JMRI_SENSOR_BROADCAST,
  SERVER_VERSION,
  ROSTER_LIST,
  TURNOUT_LIST,
  ROUTE_LIST,
  TURNTABLE_LIST
};

/**
 * @brief A typed expectation set on the listener before driving activity
 */
struct Expect {
  ExpectType type;
  int a1 = EXPECT_ANY;       //<! Primary (object) argument, EXPECT_ANY to match anything
  int a2 = EXPECT_ANY;       //<! Secondary argument
  int a3 = EXPECT_ANY;       //<! Tertiary argument
  const char *str = nullptr; //<! Optional string to match (nullptr = any)
  int remaining = 1;         //<! Number of matching events still required (0 = satisfied)
};

/**
 * @brief Result of checking an inbound event against the active expectations
 */
enum ExpectResult : uint8_t { EXPECT_IGNORED, EXPECT_SATISFIED, EXPECT_FAILED };

static Expect expectationList[maxExpectations];
static int expectationCount = 0;
static bool expectationFailed = false;

/**
 * @brief Return a short description of the provided expectation
 * @param expect Expect to describe
 */
static String describeExpect(const Expect &expect) {
  String result;
  switch (expect.type) {
  case TRACK_POWER:
    result = F("track power");
    break;
  case INDIVIDUAL_TRACK_POWER:
    result = F("individual track power");
    break;
  case TRACK_CURRENT_GAUGE:
    result = F("track current gauge");
    break;
  case TRACK_CURRENT:
    result = F("track current");
    break;
  case TRACK_TYPE:
    result = F("track type");
    break;
  case TURNOUT_ACTION:
    result = F("turnout action");
    break;
  case TURNTABLE_ACTION:
    result = F("turntable action");
    break;
  case LOCO_UPDATE:
    result = F("loco update");
    break;
  case LOCO_BROADCAST:
    result = F("loco broadcast");
    break;
  case READ_LOCO:
    result = F("read loco");
    break;
  case VALIDATE_CV:
    result = F("validate CV");
    break;
  case VALIDATE_CV_BIT:
    result = F("validate CV bit");
    break;
  case WRITE_LOCO:
    result = F("write loco");
    break;
  case WRITE_CV:
    result = F("write CV");
    break;
  case MESSAGE:
    result = F("message");
    break;
  case SCREEN_UPDATE:
    result = F("screen update");
    break;
  case CS_CONSIST:
    result = F("CS consist");
    break;
  case SET_FAST_CLOCK:
    result = F("set fast clock");
    break;
  case FAST_CLOCK_TIME:
    result = F("fast clock time");
    break;
  case JMRI_SENSOR_BROADCAST:
    result = F("JMRI sensor broadcast");
    break;
  case SERVER_VERSION:
    result = F("server version");
    break;
  case ROSTER_LIST:
    result = F("roster list");
    break;
  case TURNOUT_LIST:
    result = F("turnout list");
    break;
  case ROUTE_LIST:
    result = F("route list");
    break;
  case TURNTABLE_LIST:
    result = F("turntable list");
    break;
  }
  result += F("(");
  result += String(expect.a1) + F(",");
  result += String(expect.a2) + F(",");
  result += String(expect.a3);
  if (expect.str) {
    result += F(",\"");
    result += expect.str;
    result += F("\"");
  }
  result += F(")");
  return result;
}

/**
 * @brief Add an expectation to the active list
 * @param type Callback type to expect
 * @param a1 Primary (object) argument
 * @param a2 Secondary argument
 * @param a3 Tertiary argument
 * @param str Optional string argument (nullptr = any)
 * @param count Number of matching events required (default 1)
 */
static void addExpectation(ExpectType type, int a1, int a2 = EXPECT_ANY, int a3 = EXPECT_ANY, const char *str = nullptr,
                           int count = 1) {
  if (expectationCount >= maxExpectations) {
    Serial.println("WARNING: Expectation list full - ignoring it");
    expectationFailed = true;
    return;
  }
  Expect &expect = expectationList[expectationCount++];
  expect.type = type;
  expect.a1 = a1;
  expect.a2 = a2;
  expect.a3 = a3;
  expect.str = str;
  expect.remaining = count;
}

/**
 * @brief Check an inbound event against the active expectations
 * @param type Event type
 * @param a1 Primary (object) argument
 * @param a2 Secondary argument
 * @param a3 Tertiary argument
 * @param str Optional string argument
 * @return EXPECT_IGNORED, EXPECT_SATISFIED or EXPECT_FAILED
 */
static ExpectResult checkExpectation(ExpectType type, int a1, int a2 = EXPECT_ANY, int a3 = EXPECT_ANY,
                                     const char *str = nullptr) {
  for (int i = 0; i < expectationCount; i++) {
    Expect &expect = expectationList[i];
    if (expect.remaining <= 0 || expect.type != type)
      continue;
    // Different object - ignore (never fails). EXPECT_ANY object matches anything.
    if (expect.a1 != EXPECT_ANY && expect.a1 != a1)
      continue;
    // Same object - compare the remaining arguments
    bool valuesMatch = (expect.a2 == EXPECT_ANY || expect.a2 == a2) && (expect.a3 == EXPECT_ANY || expect.a3 == a3) &&
                       (expect.str == nullptr || (str != nullptr && strcmp(expect.str, str) == 0));
    if (valuesMatch) {
      expect.remaining--;
      return EXPECT_SATISFIED;
    }
    expectationFailed = true;
    Serial.print("FAIL: UNEXPECTED VALUE while waiting for ");
    Serial.print(describeExpect(expect));
    return EXPECT_FAILED;
  }
  return EXPECT_IGNORED;
}

/**
 * @brief Print the outcome of an expectation check for the event just processed
 * @param result Result returned by checkExpectation()
 */
static void reportExpectation(ExpectResult result) {
  if (result == EXPECT_SATISFIED) {
    Serial.println("  -> expectation satisfied");
  }
}

/**
 * @brief Custom delegate to handle events
 */
class CSListener : public DCCEXProtocolDelegate {
public:
  // Captured values from the CV programming test (T14) so it can self-restore writes
  int lastReadLocoAddress = -1; //<! Last address read from the PROG track
  int lastWriteCVValue = -1;    //<! Last CV value read/written (from the <r cv value> response)

  /**
   * @brief Clear all active expectations before a test starts
   */
  void clearExpectations() {
    expectationCount = 0;
    expectationFailed = false;
    for (int i = 0; i < maxExpectations; i++) {
      expectationList[i].remaining = 0;
    }
  }

  /**
   * @brief Check whether any event failed its expectation (UNEXPECTED VALUE)
   */
  bool hasFailed() { return expectationFailed; }

  /**
   * @brief Check whether all active expectations have been satisfied
   */
  bool allExpectationsMatched() {
    if (expectationFailed)
      return false;
    for (int i = 0; i < expectationCount; i++) {
      if (expectationList[i].remaining > 0)
        return false;
    }
    return true;
  }

  /**
   * @brief Print the result of the last observation window and return whether the test passed
   */
  bool printExpectationResult() {
    if (expectationFailed) {
      Serial.println("RESULT: FAIL (unexpected value, see above)");
      return false;
    }
    int missingCount = 0;
    for (int i = 0; i < expectationCount; i++) {
      if (expectationList[i].remaining > 0) {
        missingCount++;
        Serial.print("MISSING EXPECTATION: ");
        Serial.println(describeExpect(expectationList[i]));
      }
    }
    if (expectationCount > 0) {
      Serial.print("RESULT: ");
      Serial.println(missingCount == 0 ? F("PASS") : F("FAIL"));
    } else {
      Serial.println("RESULT: no expectations set (observe the CS console)");
    }
    return missingCount == 0 && !expectationFailed;
  }

  // Typed expectation helpers (see AGENTS.md for the matching rules)

  void expectServerVersion() { addExpectation(SERVER_VERSION, EXPECT_ANY, EXPECT_ANY, EXPECT_ANY); }
  void expectRosterList() { addExpectation(ROSTER_LIST, 0, 0, 0); }
  void expectTurnoutList() { addExpectation(TURNOUT_LIST, 0, 0, 0); }
  void expectRouteList() { addExpectation(ROUTE_LIST, 0, 0, 0); }
  void expectTurntableList() { addExpectation(TURNTABLE_LIST, 0, 0, 0); }
  void expectTrackPower(int state, int count = 1) { addExpectation(TRACK_POWER, state, 0, 0, nullptr, count); }
  void expectIndividualTrackPower(int track, int state, int count = 1) {
    addExpectation(INDIVIDUAL_TRACK_POWER, track, state, 0, nullptr, count);
  }
  void expectTrackCurrentGauge(int track, int limit, int count = 1) {
    addExpectation(TRACK_CURRENT_GAUGE, track, limit, 0, nullptr, count);
  }
  void expectTrackCurrent(int track, int current, int count = 1) {
    addExpectation(TRACK_CURRENT, track, current, 0, nullptr, count);
  }
  void expectTrackType(int track, TrackManagerMode type, int address = EXPECT_ANY) {
    addExpectation(TRACK_TYPE, track, (int)type, address);
  }
  void expectTurnoutAction(int turnoutId, int thrown, int count = 1) {
    addExpectation(TURNOUT_ACTION, turnoutId, thrown, 0, nullptr, count);
  }
  void expectTurntableAction(int turntableId, int position, int count = 1) {
    addExpectation(TURNTABLE_ACTION, turntableId, position, EXPECT_ANY, nullptr, count);
  }
  void expectLocoUpdate(int address, int count = 1) {
    addExpectation(LOCO_UPDATE, address, EXPECT_ANY, EXPECT_ANY, nullptr, count);
  }
  void expectLocoBroadcast(int address, int speed = EXPECT_ANY, int functionMap = EXPECT_ANY, int count = 1) {
    addExpectation(LOCO_BROADCAST, address, speed, functionMap, nullptr, count);
  }
  void expectReadLoco(int address = EXPECT_ANY) { addExpectation(READ_LOCO, address, 0, 0); }
  void expectValidateCV(int cv, int value) { addExpectation(VALIDATE_CV, cv, value, 0); }
  void expectValidateCVBit(int cv, int bit, int value) { addExpectation(VALIDATE_CV_BIT, cv, bit, value); }
  void expectWriteLoco(int address = EXPECT_ANY) { addExpectation(WRITE_LOCO, address, 0, 0); }
  void expectWriteCV(int cv, int value) { addExpectation(WRITE_CV, cv, value, 0); }
  void expectMessage(const char *message) { addExpectation(MESSAGE, EXPECT_ANY, EXPECT_ANY, EXPECT_ANY, message); }
  void expectScreenUpdate(int screen, int row, const char *message = nullptr) {
    addExpectation(SCREEN_UPDATE, screen, row, 0, message);
  }
  void expectCSConsist(int leadLoco, int count = 1) { addExpectation(CS_CONSIST, leadLoco, 0, 0, nullptr, count); }
  void expectSetFastClock(int minutes, int speedFactor) { addExpectation(SET_FAST_CLOCK, minutes, speedFactor, 0); }
  void expectFastClockTime(int minutes = EXPECT_ANY) { addExpectation(FAST_CLOCK_TIME, minutes, 0, 0); }
  void expectJMRISensorBroadcast(int id = EXPECT_ANY, int count = 1) {
    addExpectation(JMRI_SENSOR_BROADCAST, id, EXPECT_ANY, 0, nullptr, count);
  }

  /**
   * @brief Print the EX-CommandStation version
   * @param major Major version
   * @param minor Minor version
   * @param patch Patch version
   */
  void receivedServerVersion(int major, int minor, int patch) override {
    Serial.print("Received EX-CommandStation version: ");
    Serial.print(major);
    Serial.print(".");
    Serial.print(minor);
    Serial.print(".");
    Serial.println(patch);
    reportExpectation(checkExpectation(SERVER_VERSION, EXPECT_ANY, EXPECT_ANY, EXPECT_ANY));
    Serial.println();
  }

  /**
   * @brief Print a received broadcast message
   * @param message Message that has been broadcast
   */
  void receivedMessage(const char *message) override {
    Serial.print("Received broadcast message: ");
    Serial.println(message);
    reportExpectation(checkExpectation(MESSAGE, EXPECT_ANY, EXPECT_ANY, EXPECT_ANY, message));
    Serial.println();
  }

  /**
   * @brief Print the received roster
   */
  void receivedRosterList() override {
    Serial.println("Received roster list");
    reportExpectation(checkExpectation(ROSTER_LIST, 0, 0, 0));
    printRoster();
  }

  /**
   * @brief Print the received turnout list
   */
  void receivedTurnoutList() override {
    Serial.println("Received turnout list");
    reportExpectation(checkExpectation(TURNOUT_LIST, 0, 0, 0));
    printTurnouts();
  }

  /**
   * @brief Print the received route list
   */
  void receivedRouteList() override {
    Serial.println("Received route list");
    reportExpectation(checkExpectation(ROUTE_LIST, 0, 0, 0));
    printRoutes();
  }

  /**
   * @brief Print the received turntable list
   */
  void receivedTurntableList() override {
    Serial.println("Received turntable list");
    reportExpectation(checkExpectation(TURNTABLE_LIST, 0, 0, 0));
    printTurntables();
  }

  /**
   * @brief Print an update to a Loco object
   * @param loco Pointer to the loco object
   */
  void receivedLocoUpdate(Loco *loco) override {
    Serial.print("Received Loco update: ");
    printLoco(loco);
    Serial.println();
    reportExpectation(checkExpectation(LOCO_UPDATE, loco ? loco->getAddress() : EXPECT_ANY, EXPECT_ANY, EXPECT_ANY));
  }

  /**
   * @brief Print a Loco broadcast (suitable for non-roster locos)
   * @param address DCC address of the loco
   * @param speed Speed as derived from the speed byte
   * @param direction Direction as derived from the speed byte
   * @param functionMap Function map
   */
  void receivedLocoBroadcast(int address, int speed, Direction direction, int functionMap) override {
    Serial.print("Received Loco broadcast: address=");
    Serial.print(address);
    Serial.print(" speed=");
    Serial.print(speed);
    Serial.print(" direction=");
    Serial.print(directionToString(direction));
    Serial.print(" functions=");
    printFunctionStates(functionMap);
    Serial.println();
    reportExpectation(checkExpectation(LOCO_BROADCAST, address, speed, functionMap));
    Serial.println();
  }

  /**
   * @brief Print the received global track power state
   * @param state Power state received (PowerOff|PowerOn|PowerUnknown)
   */
  void receivedTrackPower(TrackPower state) override {
    Serial.print("Received Track Power: ");
    Serial.println(trackPowerToString(state));
    reportExpectation(checkExpectation(TRACK_POWER, (int)state, 0, 0));
    Serial.println();
  }

  /**
   * @brief Print a received track current limit value
   * @param track Track (A - H)
   * @param limit Current limit in mA
   */
  void receivedTrackCurrentGauge(char track, int limit) override {
    Serial.print("Received Track ");
    Serial.print(track);
    Serial.print(" current gauge: ");
    Serial.print(limit);
    Serial.println(" mA");
    reportExpectation(checkExpectation(TRACK_CURRENT_GAUGE, track, limit, 0));
    Serial.println();
  }

  /**
   * @brief Print a received track current value
   * @param track Track (A - H)
   * @param current Current in mA
   */
  void receivedTrackCurrent(char track, int current) override {
    Serial.print("Received Track ");
    Serial.print(track);
    Serial.print(" current: ");
    Serial.print(current);
    Serial.println(" mA");
    reportExpectation(checkExpectation(TRACK_CURRENT, track, current, 0));
    Serial.println();
  }

  /**
   * @brief Print a received individual track power state change
   * @param state Power state received (PowerOff|PowerOn|PowerUnknown)
   * @param track Track identifier - 65=A..72=H | 2698315=MAIN | 2788330=PROG | 2183=DC | 71999=DCX
   */
  void receivedIndividualTrackPower(TrackPower state, int track) override {
    Serial.print("Received Individual Track Power: ");
    Serial.print(trackPowerToString(state));
    Serial.print(" for track ");
    if (track >= 'A' && track <= 'H') {
      Serial.print((char)track);
    } else {
      switch (track) {
      case 2698315:
        Serial.print("MAIN");
        break;
      case 2788330:
        Serial.print("PROG");
        break;
      case 2183:
        Serial.print("DC");
        break;
      case 71999:
        Serial.print("DCX");
        break;
      default:
        Serial.print(track);
        break;
      }
    }
    Serial.println();
    reportExpectation(checkExpectation(INDIVIDUAL_TRACK_POWER, track, (int)state, 0));
    Serial.println();
  }

  /**
   * @brief Print a received track type change
   * @param track Track that changed
   * @param type Type received (MAIN|PROG|DC|DCX|NONE)
   * @param address Address received for DC and DCX (zero if other types)
   */
  void receivedTrackType(char track, TrackManagerMode type, int address) override {
    Serial.print("Received Track Type: track=");
    Serial.print(track);
    Serial.print(" type=");
    Serial.print(trackModeToString(type));
    Serial.print(" address=");
    Serial.println(address);
    reportExpectation(checkExpectation(TRACK_TYPE, track, (int)type, address));
    Serial.println();
  }

  /**
   * @brief Print a received turnout state change
   * @param turnoutId ID of the turnout
   * @param thrown Whether it is thrown or not (true|false)
   */
  void receivedTurnoutAction(int turnoutId, bool thrown) override {
    Serial.print("Received Turnout Action: turnout=");
    Serial.print(turnoutId);
    Serial.print(" state=");
    Serial.println(thrown ? "Thrown" : "Closed");
    reportExpectation(checkExpectation(TURNOUT_ACTION, turnoutId, thrown ? 1 : 0, 0));
    Serial.println();
  }

  /**
   * @brief Print a received turntable index change
   * @param turntableId ID of the turntable
   * @param position Index of the position it is moving (or has moved) to
   * @param moving Whether it is moving or not (true|false)
   */
  void receivedTurntableAction(int turntableId, int position, bool moving) override {
    Serial.print("Received Turntable Action: turntable=");
    Serial.print(turntableId);
    Serial.print(" position=");
    Serial.print(position);
    Serial.print(" moving=");
    Serial.println(moving ? "true" : "false");
    Serial.println();
    reportExpectation(checkExpectation(TURNTABLE_ACTION, turntableId, position, EXPECT_ANY));
  }

  /**
   * @brief Print a loco address read from the programming track
   * @param address DCC address read from the programming track, or -1 for a failure to read
   */
  void receivedReadLoco(int address) override {
    Serial.print("Received Read Loco: address=");
    Serial.println(address);
    lastReadLocoAddress = address;
    reportExpectation(checkExpectation(READ_LOCO, address, 0, 0));
    Serial.println();
  }

  /**
   * @brief Print a CV read or validated from the programming track
   * @param cv CV the value has been read from
   * @param value Value read from the CV, or -1 for a failure to read
   */
  void receivedValidateCV(int cv, int value) override {
    Serial.print("Received Validate CV: cv=");
    Serial.print(cv);
    Serial.print(" value=");
    Serial.println(value);
    reportExpectation(checkExpectation(VALIDATE_CV, cv, value, 0));
    Serial.println();
  }

  /**
   * @brief Print a CV bit validated from the programming track
   * @param cv CV the bit is being validated in
   * @param bit Bit of the CV being validated
   * @param value Value validated from the bit, or -1 if not valid
   */
  void receivedValidateCVBit(int cv, int bit, int value) override {
    Serial.print("Received Validate CV Bit: cv=");
    Serial.print(cv);
    Serial.print(" bit=");
    Serial.print(bit);
    Serial.print(" value=");
    Serial.println(value);
    reportExpectation(checkExpectation(VALIDATE_CV_BIT, cv, bit, value));
    Serial.println();
  }

  /**
   * @brief Print a loco address written on the programming track
   * @param address DCC address written to the loco, or -1 for a failure to write
   */
  void receivedWriteLoco(int address) override {
    Serial.print("Received Write Loco: address=");
    Serial.println(address);
    reportExpectation(checkExpectation(WRITE_LOCO, address, 0, 0));
    Serial.println();
  }

  /**
   * @brief Print a CV written on the programming track
   * @param cv CV being written to
   * @param value Value written, or -1 for failure
   */
  void receivedWriteCV(int cv, int value) override {
    Serial.print("Received Write CV: cv=");
    Serial.print(cv);
    Serial.print(" value=");
    Serial.println(value);
    lastWriteCVValue = value;
    reportExpectation(checkExpectation(WRITE_CV, cv, value, 0));
    Serial.println();
  }

  /**
   * @brief Print a received screen update
   * @param screen Screen number
   * @param row Row number
   * @param message Message to display on the screen/row
   */
  void receivedScreenUpdate(int screen, int row, const char *message) override {
    Serial.print("Received Screen Update: screen=");
    Serial.print(screen);
    Serial.print(" row=");
    Serial.print(row);
    Serial.print(" message=");
    Serial.println(message);
    reportExpectation(checkExpectation(SCREEN_UPDATE, screen, row, 0, message));
    Serial.println();
  }

  /**
   * @brief Print a received command station consist
   * @param leadLoco DCC address of the lead loco for the consist
   * @param csConsist Pointer to the CSConsist object controlled by the lead loco
   */
  void receivedCSConsist(int leadLoco, CSConsist *csConsist) override {
    Serial.print("Received CSConsist: lead=");
    Serial.print(leadLoco);
    Serial.print(" members:");
    if (csConsist) {
      for (CSConsistMember *member = csConsist->getFirstMember(); member; member = member->next) {
        Serial.print(" ");
        Serial.print(member->address);
        if (member->reversed)
          Serial.print("(rev)");
      }
    }
    Serial.println();
    reportExpectation(checkExpectation(CS_CONSIST, leadLoco, 0, 0));
    Serial.println();
  }

  /**
   * @brief Print a fast clock time that has been set
   * @param minutes Time since midnight in minutes
   * @param speedFactor Speed factor multiplier
   */
  void receivedSetFastClock(int minutes, int speedFactor) override {
    Serial.print("Received Set Fast Clock: time=");
    Serial.print(minutes);
    Serial.print(" min (");
    Serial.print(minutes / 60);
    Serial.print(":");
    if ((minutes % 60) < 10)
      Serial.print("0");
    Serial.print(minutes % 60);
    Serial.print(") speedFactor=");
    Serial.println(speedFactor);
    reportExpectation(checkExpectation(SET_FAST_CLOCK, minutes, speedFactor, 0));
    Serial.println();
  }

  /**
   * @brief Print a fast clock time that has been received
   * @param minutes Time in minutes
   */
  void receivedFastClockTime(int minutes) override {
    Serial.print("Received Fast Clock Time: ");
    Serial.print(minutes);
    Serial.print(" min (");
    Serial.print(minutes / 60);
    Serial.print(":");
    if ((minutes % 60) < 10)
      Serial.print("0");
    Serial.print(minutes % 60);
    Serial.println(")");
    reportExpectation(checkExpectation(FAST_CLOCK_TIME, minutes, 0, 0));
    Serial.println();
  }

  /**
   * @brief Print a JMRI sensor broadcast
   * @param id ID of the JMRI sensor
   * @param state JMRISensorState enum value
   */
  void receivedJMRISensorBroadcast(int id, JMRISensorState state) override {
    Serial.print("Received JMRI Sensor Broadcast: sensor=");
    Serial.print(id);
    Serial.print(" state=");
    Serial.println(jmriSensorStateToString(state));
    reportExpectation(checkExpectation(JMRI_SENSOR_BROADCAST, id, (int)state, 0));
    Serial.println();
  }
};

#endif // TEST_LISTENER_H