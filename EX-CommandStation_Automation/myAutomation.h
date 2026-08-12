/**
 * @file myAutomation.h
 * @brief Create objects and routes for testing the DCCEXProtocol library
 *
 * @details Load this myAutomation.h file with EX-CommandStation to create a number of dummy objects and routes that can
 * be used to exercise the various library functions for testing on physical devices.
 *
 * DCCEXProtocolTesting.ino runs a menu driven, operator assisted test suite. Each test is commenced by entering a
 * command in DCC-EX protocol style on the serial console of the device under test (DUT):
 *  - <R id> starts the route with this id on the command station via startRoute(), simulating a throttle, and the
 *    route activity triggers broadcasts that the DUT receives, enabling verification of the delegate callbacks
 *  - <T id> runs a local test on the DUT which drives activity directly via the library API
 *
 * The routes in this file are the <R id> targets (500, 700-708) plus the automation (301) used by handOffLoco().
 *
 * NOTE: Turntables are DCC only in this file, EX-Turntable (EXTT) requires a physical device to be attached so is not
 * exercised here.
 *
 * @author peteGSX
 */

/* --------------------------------------------------------------------------
 * Dummy objects created to exercise the library list retrieval
 * ------------------------------------------------------------------------ */

// Roster entries (15 total) - used to verify roster list retrieval and loco control
ROSTER(2004, "QR 2004", "Lights/*Horn/*Bell/*Whistle/*Brakes/Idiots/Crap Programming")
ROSTER(2006, "QR 2006", "Lights/*Horn")
ROSTER(2010, "QR 2010", "Lights/*Horn")
ROSTER(2014, "QR 2014", "Lights/*Horn")
ROSTER(2016, "QR 2016", "Lights/*Horn")
ROSTER(2020, "QR 2020", "Lights/*Horn")
ROSTER(2024, "QR 2024", "Lights/*Horn")
ROSTER(2026, "QR 2026", "Lights/*Horn")
ROSTER(2030, "QR 2030", "Lights/*Horn")
ROSTER(2040, "QR 2040", "Lights/*Horn/Bell")
ROSTER(2046, "QR 2046", "Lights/*Horn/Bell/Whistle")
ROSTER(2050, "QR 2050", "Lights")
ROSTER(2056, "QR 2056", "Lights/*Horn")
ROSTER(2060, "QR 2060", "Lights/*Horn/Bell/Whistle/*Brakes")
ROSTER(2066, "QR 2066", "Lights")

// JMRI sensors (21 total) - used to verify JMRI sensor broadcasts
// 6000-6004 with internal pullup, 6100-6104 without pullup, 6200-6209, and a single 6300
JMRI_SENSOR(6000, 5)
JMRI_SENSOR_NOPULLUP(6100, 5)
JMRI_SENSOR(6200, 10)
JMRI_SENSOR(6300, 1)

// DCC turntables - used to verify turntable list retrieval and rotateTurntable()
// Turntable 2 has 6 positions, turntable 3 is home only (0 defined positions), turntable 4 has 3 positions
DCC_TURNTABLE(2, 1800, "My DCC Turntable")
TT_ADDPOSITION(2, 1, 900, 800, "Stall 1")
TT_ADDPOSITION(2, 2, 1200, 900, "Stall 2")
TT_ADDPOSITION(2, 3, 1500, 1000, "Stall 3")
TT_ADDPOSITION(2, 4, 2700, 2600, "Rev stall 1")
TT_ADDPOSITION(2, 5, 3000, 2700, "Layout")
TT_ADDPOSITION(2, 6, 3300, 2800, "Rev stall 3")

DCC_TURNTABLE(3, 1800, "Home Only Turntable")

DCC_TURNTABLE(4, 1800, "Three Position Turntable")
TT_ADDPOSITION(4, 1, 900, 800, "Stall A")
TT_ADDPOSITION(4, 2, 1500, 1200, "Stall B")
TT_ADDPOSITION(4, 3, 2700, 2400, "Stall C")

// Turnouts (10 total) - used to verify turnout list retrieval and throw/close/toggle
// NOTE: Turnout 105 uses the HIDDEN keyword so it will NOT appear in the <jT> turnout list
// Turnout 130 requires a servo on vpin 130, comment out if you don't have one attached
PIN_TURNOUT(100, 100, "Turnout 100")
PIN_TURNOUT(101, 101, "Turnout 101")
PIN_TURNOUT(102, 102)
PIN_TURNOUT(103, 103, "Turnout 103")
PIN_TURNOUT(104, 104)
PIN_TURNOUT(105, 105, HIDDEN)
VIRTUAL_TURNOUT(110, "Virtual Turnout")
TURNOUT(120, 10, 1, "Legacy Turnout")
TURNOUTL(121, 500, "Linear Turnout")
// SERVO_TURNOUT(130, 130, 700, 1200, Slow, "Servo Turnout")

/* --------------------------------------------------------------------------
 * Basic routes and automations
 * ------------------------------------------------------------------------ */

ROUTE(200, "Route 200")
PRINT("Route 200 activated")
DONE

    AUTOMATION(300, "Automation 300") PRINT("Automation 300 activated") DONE

    ROUTE(400, "Route 400") PRINT("Route 400 activated") DONE

/* --------------------------------------------------------------------------
 * ROUTE 500 - JMRI sensor broadcast test
 * --------------------------------------------------------------------------
 * Start this route by entering <R 500> on the DUT serial console to trigger JMRI sensor broadcasts. The BROADCAST()
 * macro sends the raw DCC-EX protocol command to all connected throttles, so the DUT will receive <Q id>/<q id>
 * sensor broadcasts and print them via receivedJMRISensorBroadcast().
 */
    ROUTE(500, "Sensor Test") PRINT("Sensor testing activated") PRINT("Activate 6000") BROADCAST("<Q 6000>\n")
        DELAY(2000) PRINT("Deactivate 6000") BROADCAST("<q 6000>\n") DELAY(2000) PRINT("Activate 6001, 6100, 6101")
            BROADCAST("<Q 6001>\n") BROADCAST("<Q 6100>\n") BROADCAST("<Q 6101>\n") DELAY(2000) PRINT("Deactivate 6101")
                BROADCAST("<q 6101>\n") DELAY(2000) PRINT("Activate 6200, 6201, 6300")
                    BROADCAST("<Q 6200>\n") BROADCAST("<Q 6201>\n") BROADCAST("<Q 6300>\n") DELAY(2000)
                        PRINT("Deactivate 6001, 6100, 6200, 6201, 6300")
                            BROADCAST("<q 6001>\n") BROADCAST("<q 6100>\n") BROADCAST("<q 6200>\n")
                                BROADCAST("<q 6201>\n") BROADCAST("<q 6300>\n") PRINT("Sensor testing end") DONE

/* --------------------------------------------------------------------------
 * Activity routes (700-708) - started by entering <R id> on the DUT serial
 * console to trigger broadcasts the DUT receives
 * ------------------------------------------------------------------------ */

/* ROUTE 700 - Loco Drive
 * Drives roster loco 2010 forward, toggling function 0 (lights). The DUT receives <l 2010 ...> broadcasts and
 * prints them via receivedLocoBroadcast() and receivedLocoUpdate(). Started by entering <R 700>.
 */
ROUTE(700, "Loco Drive")
PRINT("Route 700 (Loco Drive) activated")
SETLOCO(2010)
FON(0)
FWD(30)
DELAY(5000)
FOFF(0)
FWD(0)
PRINT("Route 700 complete")
DONE

/* ROUTE 701 - Local Loco Drive
 * Drives non-roster loco 9999 forward, toggling function 1. The DUT receives <l 9999 ...> broadcasts and prints
 * them via receivedLocoBroadcast(). Started by entering <R 701>.
 */
ROUTE(701, "Local Loco Drive")
PRINT("Route 701 (Local Loco Drive) activated")
SETLOCO(9999)
FON(1)
FWD(25)
DELAY(5000)
FOFF(1)
FWD(0)
PRINT("Route 701 complete")
DONE

/* ROUTE 702 - Turnout Ops
 * Throws/closes/toggles various turnouts. The DUT receives <H id state> broadcasts and prints them via
 * receivedTurnoutAction(). Started by entering <R 702>.
 */
ROUTE(702, "Turnout Ops")
PRINT("Route 702 (Turnout Ops) activated")
THROW(100)
DELAY(2000)
CLOSE(100)
DELAY(2000)
TOGGLE_TURNOUT(101)
DELAY(2000)
TOGGLE_TURNOUT(101)
DELAY(2000)
THROW(110)
DELAY(2000)
CLOSE(110)
DELAY(2000)
THROW(102)
DELAY(2000)
CLOSE(102)
PRINT("Route 702 complete")
DONE

/* ROUTE 703 - Turntable Ops
 * Rotates the DCC turntables 2 and 4. The DUT receives <I id position moving> broadcasts and prints them via
 * receivedTurntableAction(). Started by entering <R 703>.
 */
ROUTE(703, "Turntable Ops")
PRINT("Route 703 (Turntable Ops) activated")
ROTATE_DCC(2, 2)
DELAY(5000)
ROTATE_DCC(2, 4)
DELAY(5000)
ROTATE_DCC(2, 0)
DELAY(5000)
ROTATE_DCC(4, 1)
DELAY(5000)
ROTATE_DCC(4, 3)
DELAY(5000)
ROTATE_DCC(4, 0)
PRINT("Route 703 complete")
DONE

/* ROUTE 704 - Power Changes
 * Cycles global and individual track power. The DUT receives <p...> broadcasts and prints them via
 * receivedTrackPower() and receivedIndividualTrackPower(). Started by entering <R 704>.
 */
ROUTE(704, "Power Changes")
PRINT("Route 704 (Power Changes) activated")
POWEROFF
DELAY(3000)
POWERON
DELAY(3000)
SET_POWER(B, OFF)
DELAY(3000)
SET_POWER(B, ON)
DELAY(3000)
POWERON
PRINT("Route 704 complete")
DONE

/* ROUTE 705 - Messages
 * Sends broadcast messages and an LCD/OLED screen update. The DUT receives <m "message"> and <@ screen row
 * "message"> broadcasts and prints them via receivedMessage() and receivedScreenUpdate(). Started by entering
 * <R 705>.
 */
ROUTE(705, "Messages")
PRINT("Route 705 (Messages) activated")
MESSAGE("Route 705 message to all throttles")
SCREEN(0, 1, "Route 705 screen update")
DELAY(3000)
MESSAGE("Route 705 second message")
DELAY(3000)
PRINT("Route 705 complete")
DONE

/* ROUTE 706 - Consist Ops
 * Builds a command station consist with lead 2010 and members 2014 (reversed) and 2016, drives it, then breaks it.
 * The DUT receives <^ lead member...> broadcasts and prints them via receivedCSConsist(). Started by entering
 * <R 706>.
 */
ROUTE(706, "Consist Ops")
PRINT("Route 706 (Consist Ops) activated")
SETLOCO(2010)
BUILD_CONSIST(2014)
BUILD_CONSIST(2016)
DELAY(5000)
FON(0)
FWD(20)
DELAY(5000)
FWD(0)
FOFF(0)
BREAK_CONSIST
DELAY(2000)
PRINT("Route 706 complete")
DONE

/* ROUTE 708 - Delayed Activity
 * A long running route with delays. Started by entering <R 708>, or use the <T 708> local test to verify
 * startRoute(), pauseRoutes(), and resumeRoutes().
 * When paused, the route should stop at the current DELAY and PRINT marker on the CS console until resumed.
 */
ROUTE(708, "Delayed Activity")
PRINT("Route 708 (Delayed Activity) started")
THROW(100)
DELAY(10000)
PRINT("Route 708 step 2")
THROW(101)
DELAY(10000)
PRINT("Route 708 step 3")
CLOSE(100)
CLOSE(101)
PRINT("Route 708 complete")
DONE

/* --------------------------------------------------------------------------
 * AUTOMATION 301 - used for the <T 301> local test which sends handOffLoco()
 * --------------------------------------------------------------------------
 * The DUT sends handOffLoco(3001, 301) which starts this automation with loco 3001. It drives loco 3001 forward,
 * toggling function 0, and the DUT receives the resulting <l 3001 ...> broadcasts.
 */
AUTOMATION(301, "Automation 301")
PRINT("Automation 301 started")
FON(0)
FWD(15)
DELAY(10000)
FWD(0)
FOFF(0)
PRINT("Automation 301 complete")
DONE

/* --------------------------------------------------------------------------
 * Route to expected DUT output test matrix
 * --------------------------------------------------------------------------
 * The following matrix shows what the DUT serial console should print when each route is started by entering <R id>
 * on the DUT serial console. The CS console will show the matching PRINT() markers.
 *
 * Route 500 (Sensor Test)   -> receivedJMRISensorBroadcast() for sensors 6000, 6001, 6100, 6101, 6200, 6201, 6300
 *                              with Activated/Deactivated states matching the BROADCAST() commands
 * Route 700 (Loco Drive)    -> receivedLocoBroadcast() and receivedLocoUpdate() for loco 2010 (speed 30, F0 on/off)
 * Route 701 (Local Loco)    -> receivedLocoBroadcast() for loco 9999 (speed 25, F1 on/off)
 * Route 702 (Turnout Ops)   -> receivedTurnoutAction() for turnouts 100, 101, 102, 110 (Thrown/Closed states)
 * Route 703 (Turntable Ops) -> receivedTurntableAction() for turntables 2 and 4 with position/moving changes
 * Route 704 (Power Changes) -> receivedTrackPower() and receivedIndividualTrackPower() for PowerOff/PowerOn
 * Route 705 (Messages)      -> receivedMessage() and receivedScreenUpdate() for the two messages and screen update
 * Route 706 (Consist Ops)   -> receivedCSConsist() for lead 2010 with members 2014 (rev) and 2016, then 2010 alone
 * Route 708 (Delayed Act.)  -> no DUT output expected, verify PRINT markers and pause/resume behaviour on CS console
 * Automation 301            -> receivedLocoBroadcast() for loco 3001 (speed 15, F0 on/off) after handOffLoco()
 */
