/**
 * @file DCCEXProtocolTesting.ino
 * @brief Arduino sketch for basic DCCEXProtocol library testing with hardware.
 *
 * @details Use this sketch with either a serially connected STM32 Bluepill or WiFi connected ESP32 device to connect to
 * EX-CommandStation with various objects created to ensure the protocol can be used to connect, retrieve objects, and
 * respond to broadcasts/responses.
 *
 * Connect the device being tested to an EX-CommandStation and monitor the serial console of the device under test to
 * observe it receiving the various object lists and then broadcasts/responses.
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

/*
Global objects
*/
DCCEXProtocol csClient;      // Client connection to the EX-CommandStation using DCCEXProtocol
unsigned long lastAlive = 0; // Last time alive was displayed
#if defined(ARDUINO_ARCH_ESP32)
WiFiClient wifiClient;
#endif

/**
 * @brief Helper method to print roster entries
 */
void printRoster() {
  Serial.println("Printing roster:");
  for (Loco *loco = csClient.roster->getFirst(); loco; loco = loco->getNext()) {
    int id = loco->getAddress();
    const char *name = loco->getName();
    Serial.print(id);
    Serial.print(" ~");
    Serial.print(name);
    Serial.println("~");
    for (int i = 0; i < 32; i++) {
      const char *fName = loco->getFunctionName(i);
      if (fName != nullptr) {
        Serial.print("loadFunctionLabels() ");
        Serial.print(fName);
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
 * @brief Helper method to print the turnout list
 */
void printTurnouts() {
  Serial.println("Printing turnout list:");
  for (Turnout *turnout = csClient.turnouts->getFirst(); turnout; turnout = turnout->getNext()) {
    int id = turnout->getId();
    const char *name = turnout->getName();
    Serial.print(id);
    Serial.print(" ~");
    Serial.print(name);
    Serial.println("~");
  }
  Serial.println();
}

/**
 * @brief Helper method to print the route list
 */
void printRoutes() {
  Serial.println("Printing route list:");
  for (Route *route = csClient.routes->getFirst(); route; route = route->getNext()) {
    int id = route->getId();
    const char *name = route->getName();
    Serial.print(id);
    Serial.print(" ~");
    Serial.print(name);
    Serial.println("~");
  }
  Serial.println();
}

/**
 * @brief Helper method to print the turntable list (including indexes)
 */
void printTurntables() {
  Serial.println("Printing turntable list:");
  for (Turntable *turntable = csClient.turntables->getFirst(); turntable; turntable = turntable->getNext()) {
    int id = turntable->getId();
    const char *name = turntable->getName();
    Serial.print(id);
    Serial.print(" ~");
    Serial.print(name);
    Serial.println("~");

    int j = 0;
    for (TurntableIndex *turntableIndex = turntable->getFirstIndex(); turntableIndex;
         turntableIndex = turntableIndex->getNextIndex()) {
      const char *indexName = turntableIndex->getName();
      Serial.print("  index");
      Serial.print(j);
      Serial.print(" ~");
      Serial.print(indexName);
      Serial.println("~");
      j++;
    }
  }
  Serial.println();
}

/*
Custom delegate to handle events
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
   * @brief Print the received track power state
   * @param state
   */
  void receivedTrackPower(TrackPower state) override {
    Serial.print("Received Track Power: ");
    Serial.println(state);
    Serial.println();
  }

  /**
   * @brief Print the received roster
   */
  void receivedRosterList() override { printRoster(); }

  /**
   * @brief Print the received turnout list
   */
  void receivedTurnoutList() override { printTurnouts(); }

  /**
   * @brief Print the received route list
   */
  void receivedRouteList() override { printRoutes(); }

  /**
   * @brief Print the received turntable list
   */
  void receivedTurntableList() override { printTurntables(); }
};

/**
 * @brief Create the csListener
 */
CSListener csListener; // Listener for EX-CommandStation broadcasts/responses using DCCEXProtocolDelegate

void setup() {
#if defined(ARDUINO_ARCH_STM32)
  // Disable JTAG and enable SWD by clearing the SWJ_CFG bits
  // Assuming the register is named AFIO_MAPR or AFIO_MAPR2
  AFIO->MAPR &= ~(AFIO_MAPR_SWJ_CFG);
  // or
  // AFIO->MAPR2 &= ~(AFIO_MAPR2_SWJ_CFG);
#endif

  Serial.begin(115200);

#ifdef STARTUP_DELAY
  delay(STARTUP_DELAY);
#endif

  Serial.println("DCCEXProtocol Testing");

  csClient.setLogStream(&Serial);    // Set up serial connection for the console logs
  csClient.setDelegate(&csListener); // Set up the listener for broadcasts/events

#if defined(ARDUINO_ARCH_STM32) // If using Bluepill, use Serial1 for the CS connection
  Serial1.begin(115200);
  csClient.connect(&Serial1);
#elif defined(ARDUINO_ARCH_ESP32) // If using ESP32, use WiFi for the CS connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(1000);

  Serial.print("WiFi connected with IP ");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to command station over WiFi");
  if (wifiClient.connect(serverAddress, serverPort)) {
    Serial.println("Connection failed");
    while (1)
      delay(1000);
  }
  Serial.println("Connected to command station");
  csClient.connect(&wifiClient);
#endif

  csClient.requestServerVersion();
}

void loop() {
  csClient.check();
  if (!csClient.receivedLists()) {
    csClient.getLists();
  }

#ifdef ALIVE_DELAY
  if (millis() - lastAlive > ALIVE_DELAY) {
    lastAlive = millis();
    Serial.println("Testing still live");
  }
#endif
}
