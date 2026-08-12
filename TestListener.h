/**
 * @file TestListener.h
 * @brief Custom delegate to handle all DCCEXProtocolDelegate events.
 *
 * @details Implements every callback from DCCEXProtocolDelegate so that all broadcasts/responses from the
 * EX-CommandStation are printed to the serial console of the device under test for visual verification.
 */

#ifndef TEST_LISTENER_H
#define TEST_LISTENER_H

#include "PrintHelpers.h"

/**
 * @brief Custom delegate to handle events
 */
class CSListener : public DCCEXProtocolDelegate {
public:
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
    Serial.println();
  }

  /**
   * @brief Print a received broadcast message
   * @param message Message that has been broadcast
   */
  void receivedMessage(const char *message) override {
    Serial.print("Received broadcast message: ");
    Serial.println(message);
    Serial.println();
  }

  /**
   * @brief Print the received roster
   */
  void receivedRosterList() override {
    Serial.println("Received roster list");
    printRoster();
  }

  /**
   * @brief Print the received turnout list
   */
  void receivedTurnoutList() override {
    Serial.println("Received turnout list");
    printTurnouts();
  }

  /**
   * @brief Print the received route list
   */
  void receivedRouteList() override {
    Serial.println("Received route list");
    printRoutes();
  }

  /**
   * @brief Print the received turntable list
   */
  void receivedTurntableList() override {
    Serial.println("Received turntable list");
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
    Serial.println();
  }

  /**
   * @brief Print the received global track power state
   * @param state Power state received (PowerOff|PowerOn|PowerUnknown)
   */
  void receivedTrackPower(TrackPower state) override {
    Serial.print("Received Track Power: ");
    Serial.println(trackPowerToString(state));
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
  }

  /**
   * @brief Print a loco address read from the programming track
   * @param address DCC address read from the programming track, or -1 for a failure to read
   */
  void receivedReadLoco(int address) override {
    Serial.print("Received Read Loco: address=");
    Serial.println(address);
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
    Serial.println();
  }

  /**
   * @brief Print a loco address written on the programming track
   * @param address DCC address written to the loco, or -1 for a failure to write
   */
  void receivedWriteLoco(int address) override {
    Serial.print("Received Write Loco: address=");
    Serial.println(address);
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
    Serial.println();
  }
};

#endif // TEST_LISTENER_H
