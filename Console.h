/**
 * @file Console.h
 * @brief Interactive serial console for the DCCEXProtocol testing sketch.
 *
 * @details The operator enters commands in DCC-EX protocol style to commence tests:
 *  - <C> connects to the EX-CommandStation manually (requests the version and object lists)
 *  - <R id> starts a ROUTE on the command station via startRoute(), the command station runs the route and broadcasts
 *  - <T id> starts a local test on the device under test, which drives activity via the library API
 *
 * After each test completes, the list of available tests is displayed again (see TestSequence.h).
 *
 * Commands are framed with < and > like the DCC-EX protocol, and everything outside a frame (carriage returns, line
 * feeds and stray bytes) is ignored.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include "Globals.h"

static const uint8_t consoleBufferSize = 32;
static char consoleFrameChars[consoleBufferSize];
static bool consoleFrameInProgress = false;
static bool consoleFrameNewData = false;
static uint8_t consoleFrameIndex = 0;

/**
 * @brief Process serial input, assembling complete <...> frames like the DCC-EX protocol
 * @param buffer Buffer to store the assembled frame content (must be at least consoleBufferSize)
 * @return true if a complete <...> frame was read
 */
static bool consoleGetFrame(char *buffer) {
  while (Serial.available() > 0) {
    char serialChar = (char)Serial.read();
    if (consoleFrameInProgress) {
      if (serialChar != '>') {
        if (consoleFrameIndex < (consoleBufferSize - 1)) {
          consoleFrameChars[consoleFrameIndex++] = serialChar;
        }
      } else {
        consoleFrameChars[consoleFrameIndex] = '\0';
        consoleFrameInProgress = false;
        consoleFrameIndex = 0;
        consoleFrameNewData = true;
      }
    } else if (serialChar == '<') {
      consoleFrameInProgress = true;
    }
  }
  if (consoleFrameNewData) {
    consoleFrameNewData = false;
    strcpy(buffer, consoleFrameChars);
    return true;
  }
  return false;
}

/**
 * @brief Blocking prompt requesting a <opcode id> command (e.g. <C>, <R 704> or <T 1>)
 * @param opcode Output opcode ('C', 'T' or 'R')
 * @param id Output test/route id (always 0 for <C>)
 * @return true once a valid command has been entered
 */
static bool consoleReadCommand(char *opcode, int *id) {
  Serial.println(">>> Enter <C> to connect, <T id> for a local test, or <R id> for a route on the command station:");
  while (true) {
    csClient.check();
    char frame[consoleBufferSize];
    if (consoleGetFrame(frame)) {
      char op = frame[0];
      *opcode = (op >= 'a' && op <= 'z') ? (op - ('a' - 'A')) : op;
      if (*opcode == 'C') {
        *id = 0;
        Serial.println("> <C>");
        return true;
      }
      if (*opcode == 'T' || *opcode == 'R') {
        *id = atoi(frame + 1);
        if (*id > 0) {
          Serial.print("> <");
          Serial.print(*opcode);
          Serial.print(" ");
          Serial.print(*id);
          Serial.println(">");
          return true;
        }
      }
      Serial.print(">>> Invalid command: <");
      Serial.print(frame);
      Serial.println(">");
      Serial.println(">>> Use <C> to connect, <T id> for a local test or <R id> for a route on the command station");
    }
  }
}

#endif // CONSOLE_H
