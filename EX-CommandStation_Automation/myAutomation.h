/**
 * @file myAutomation.h
 * @brief Create objects and routes for testing the DCCEXProtocol library
 *
 * @details Load this myAutomation.h file with EX-CommandStation to create a number of dummy objects and routes that can
 * be used to exercise the various library functions for testing on physical devices.
 *
 * @author peteGSX
 */

ROSTER(2004, "QR 2004", "Lights/*Horn/*Bell/*Whistle/*Brakes/Idiots/Crap Programming")
ROSTER(2006, "QR 2006", "Lights/*Horn")
ROSTER(2010, "QR 2010", "Lights/*Horn")
ROSTER(2014, "QR 2014", "Lights/*Horn")
ROSTER(2016, "QR 2016", "Lights/*Horn")
ROSTER(2020, "QR 2020", "Lights/*Horn")
ROSTER(2024, "QR 2024", "Lights/*Horn")
ROSTER(2026, "QR 2026", "Lights/*Horn")
ROSTER(2030, "QR 2030", "Lights/*Horn")

JMRI_SENSOR(6000, 5)
JMRI_SENSOR_NOPULLUP(6100, 5)

HAL(EXTurntable, 600, 1, 0x60)
EXTT_TURNTABLE(1, 600, 1800, "My EX-Turntable")
TT_ADDPOSITION(1, 1, 900, 800, "Stall 1")
TT_ADDPOSITION(1, 2, 1200, 900, "Stall 2")
TT_ADDPOSITION(1, 3, 1500, 1000, "Stall 3")
TT_ADDPOSITION(1, 4, 2700, 2600, "Rev stall 1")
TT_ADDPOSITION(1, 5, 3000, 2700, "Layout")
TT_ADDPOSITION(1, 6, 3300, 2800, "Rev stall 3")

DCC_TURNTABLE(2, 1800, "My DCC Turntable")
TT_ADDPOSITION(2, 1, 900, 800, "Stall 1")
TT_ADDPOSITION(2, 2, 1200, 900, "Stall 2")
TT_ADDPOSITION(2, 3, 1500, 1000, "Stall 3")
TT_ADDPOSITION(2, 4, 2700, 2600, "Rev stall 1")
TT_ADDPOSITION(2, 5, 3000, 2700, "Layout")
TT_ADDPOSITION(2, 6, 3300, 2800, "Rev stall 3")

PIN_TURNOUT(100, 100, "Turnout 100")
PIN_TURNOUT(101, 101, "Turnout 101")
PIN_TURNOUT(102, 102)

ROUTE(200, "Route 200")
PRINT("Route 200 activated")
DONE

    AUTOMATION(300, "Automation 300") PRINT("Automation 300 activated") DONE

    ROUTE(400, "Route 400") PRINT("Route 400 activated") DONE

    ROUTE(500, "Sensor Test") PRINT("Sensor testing activated") PRINT("Activate 6000") BROADCAST("<Q 6000>\n")
        DELAY(2000) PRINT("Deactivate 6000") BROADCAST("<q 6000>\n") DELAY(2000) PRINT("Activate 6001, 6100, 6101")
            BROADCAST("<Q 6001>\n") BROADCAST("<Q 6100>\n") BROADCAST("<Q 6101>\n") DELAY(2000) PRINT("Deactivate 6101")
                BROADCAST("<q 6101>\n") DELAY(2000) PRINT("Deactivate 6001, 6100") BROADCAST("<q 6001>\n")
                    BROADCAST("<q 6100>\n") PRINT("Sensor testing end") DONE
