/**
 * @file PrintHelpers.h
 * @brief Helper methods to render enums to strings and print the various object lists.
 *
 * @details All output is sent to the serial console of the device under test. The various enum renderers are used by
 * both the delegate callbacks (TestListener.h) and the test sequence (TestSequence.h).
 */

#ifndef PRINT_HELPERS_H
#define PRINT_HELPERS_H

#include "Globals.h"

/**
 * @brief Render a TrackPower enum value as a string
 * @param state TrackPower value to render
 * @return String representation
 */
inline const char *trackPowerToString(TrackPower state) {
  switch (state) {
  case PowerOff:
    return "Power Off";
  case PowerOn:
    return "Power On";
  default:
    return "Power Unknown";
  }
}

/**
 * @brief Render a TrackManagerMode enum value as a string
 * @param mode TrackManagerMode value to render
 * @return String representation
 */
inline const char *trackModeToString(TrackManagerMode mode) {
  switch (mode) {
  case MAIN:
    return "MAIN";
  case PROG:
    return "PROG";
  case DC:
    return "DC";
  case DCX:
    return "DCX";
  default:
    return "NONE";
  }
}

/**
 * @brief Render a Direction enum value as a string
 * @param direction Direction value to render
 * @return String representation
 */
inline const char *directionToString(Direction direction) {
  switch (direction) {
  case Forward:
    return "Forward";
  default:
    return "Reverse";
  }
}

/**
 * @brief Render a LocoSource enum value as a string
 * @param source LocoSource value to render
 * @return String representation
 */
inline const char *locoSourceToString(LocoSource source) {
  switch (source) {
  case LocoSourceRoster:
    return "Roster";
  default:
    return "User Entry";
  }
}

/**
 * @brief Render a JMRISensorState enum value as a string
 * @param state JMRISensorState value to render
 * @return String representation
 */
inline const char *jmriSensorStateToString(JMRISensorState state) {
  switch (state) {
  case Activated:
    return "Activated";
  default:
    return "Deactivated";
  }
}

/**
 * @brief Render a TurntableType enum value as a string
 * @param type TurntableType value to render
 * @return String representation
 */
inline const char *turntableTypeToString(TurntableType type) {
  switch (type) {
  case TurntableTypeDCC:
    return "DCC";
  case TurntableTypeEXTT:
    return "EX-Turntable";
  default:
    return "Unknown";
  }
}

/**
 * @brief Render a RouteType enum value as a string
 * @param type RouteType value to render
 * @return String representation
 */
inline const char *routeTypeToString(RouteType type) {
  switch (type) {
  case RouteTypeAutomation:
    return "Automation";
  default:
    return "Route";
  }
}

/**
 * @brief Print the list of function numbers currently set on for the provided function states
 * @param functionStates Integer representing the function states
 */
inline void printFunctionStates(int functionStates) {
  bool anyFunctions = false;
  for (int i = 0; i < MAX_FUNCTIONS; i++) {
    if (functionStates & (1L << i)) {
      if (anyFunctions)
        Serial.print(", ");
      Serial.print("F");
      Serial.print(i);
      anyFunctions = true;
    }
  }
  if (!anyFunctions)
    Serial.print("none");
}

/**
 * @brief Print the details of a Loco object
 * @param loco Pointer to the Loco object
 */
inline void printLoco(Loco *loco) {
  if (!loco)
    return;
  Serial.print("Loco: ");
  Serial.print(loco->getAddress());
  Serial.print(" ~");
  Serial.print(loco->getName());
  Serial.print("~ ");
  Serial.print(locoSourceToString(loco->getSource()));
  Serial.print(" speed=");
  Serial.print(loco->getSpeed());
  Serial.print(" direction=");
  Serial.print(directionToString(loco->getDirection()));
  Serial.print(" functions=");
  printFunctionStates(loco->getFunctionStates());
  Serial.println();
}

/**
 * @brief Print the roster list
 */
inline void printRoster() {
  Serial.println("Printing roster:");
  if (!csClient.roster->getFirst()) {
    Serial.println("(no roster entries)");
  }
  for (Loco *loco = csClient.roster->getFirst(); loco; loco = loco->getNext()) {
    printLoco(loco);
    for (int i = 0; i < MAX_FUNCTIONS; i++) {
      const char *fName = loco->getFunctionName(i);
      if (fName != nullptr) {
        Serial.print("  F");
        Serial.print(i);
        Serial.print(" ~");
        Serial.print(fName);
        Serial.print("~");
        if (loco->isFunctionMomentary(i)) {
          Serial.print(" - Momentary");
        }
        Serial.println();
      }
    }
  }
  Serial.println();
}

/**
 * @brief Print the list of local (non-roster) locos
 */
inline void printLocalLocos() {
  Serial.println("Printing local (non-roster) locos:");
  if (!Loco::getFirstLocalLoco()) {
    Serial.println("(no local locos)");
  }
  for (Loco *loco = Loco::getFirstLocalLoco(); loco; loco = loco->getNext()) {
    printLoco(loco);
  }
  Serial.println();
}

/**
 * @brief Print the turnout list
 */
inline void printTurnouts() {
  Serial.println("Printing turnout list:");
  if (!csClient.turnouts->getFirst()) {
    Serial.println("(no turnouts)");
  }
  for (Turnout *turnout = csClient.turnouts->getFirst(); turnout; turnout = turnout->getNext()) {
    Serial.print(turnout->getId());
    Serial.print(" ~");
    Serial.print(turnout->getName());
    Serial.print("~ ");
    Serial.print(turnout->getThrown() ? "Thrown" : "Closed");
    Serial.println();
  }
  Serial.println();
}

/**
 * @brief Print the route list (including automations)
 */
inline void printRoutes() {
  Serial.println("Printing route list:");
  if (!csClient.routes->getFirst()) {
    Serial.println("(no routes)");
  }
  for (Route *route = csClient.routes->getFirst(); route; route = route->getNext()) {
    Serial.print(route->getId());
    Serial.print(" ~");
    Serial.print(route->getName());
    Serial.print("~ (");
    Serial.print(routeTypeToString(route->getType()));
    Serial.println(")");
  }
  Serial.println();
}

/**
 * @brief Print the turntable list (including indexes)
 */
inline void printTurntables() {
  Serial.println("Printing turntable list:");
  if (!csClient.turntables->getFirst()) {
    Serial.println("(no turntables)");
  }
  for (Turntable *turntable = csClient.turntables->getFirst(); turntable; turntable = turntable->getNext()) {
    Serial.print(turntable->getId());
    Serial.print(" ~");
    Serial.print(turntable->getName());
    Serial.print("~ type=");
    Serial.print(turntableTypeToString(turntable->getType()));
    Serial.print(" index=");
    Serial.print(turntable->getIndex());
    Serial.print(" numberOfIndexes=");
    Serial.print(turntable->getNumberOfIndexes());
    Serial.print(" indexCount=");
    Serial.print(turntable->getIndexCount());
    Serial.print(" moving=");
    Serial.println(turntable->isMoving() ? "true" : "false");

    for (TurntableIndex *turntableIndex = turntable->getFirstIndex(); turntableIndex;
         turntableIndex = turntableIndex->getNextIndex()) {
      Serial.print("  index ");
      Serial.print(turntableIndex->getId());
      Serial.print(" ~");
      Serial.print(turntableIndex->getName());
      Serial.print("~ angle=");
      Serial.println(turntableIndex->getAngle());
    }
  }
  Serial.println();
}

/**
 * @brief Print the list of command station consists
 */
inline void printCSConsists() {
  Serial.println("Printing command station consists:");
  if (!csClient.csConsists->getFirst()) {
    Serial.println("(no CSConsists)");
  }
  for (CSConsist *csConsist = csClient.csConsists->getFirst(); csConsist; csConsist = csConsist->getNext()) {
    Serial.print("CSConsist members:");
    for (CSConsistMember *member = csConsist->getFirstMember(); member; member = member->next) {
      Serial.print(" ");
      Serial.print(member->address);
      if (member->reversed)
        Serial.print("(rev)");
    }
    Serial.print(" memberCount=");
    Serial.print(csConsist->getMemberCount());
    Serial.print(" valid=");
    Serial.print(csConsist->isValid() ? "true" : "false");
    Serial.print(" replicateFunctions=");
    Serial.print(csConsist->getReplicateFunctions() ? "true" : "false");
    Serial.print(" speed=");
    Serial.print(csConsist->getSpeed());
    Serial.print(" direction=");
    Serial.println(directionToString(csConsist->getDirection()));
  }
  Serial.println();
}

#endif // PRINT_HELPERS_H
