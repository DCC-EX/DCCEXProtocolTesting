# DCC-EX Protocol Testing

This repository contains a simple Arduino sketch with PlatformIO support to perform basic hardware testing of the DCCEXProtocol Arduino library.

This supports both STM32 Bluepill devices via a serial connection and ESP32 based devices via a WiFi connection.

The goal is to ensure changes to the library don't introduce breaking changes to user's throttle projects and new features/changes don't introduce bugs.

Use this sketch as an augmentation to the existing native Google tests run with PlatformIO for the DCCEXProtocol library.

**Warning!** The code in this repository was started by a human, but in the interests of saving time was completed with AI assistance, so don't assume everything here is good and correct.

## Installation

This sketch uses the DCCEXProtocol library (version 1.4.0 and later) and should work with any DCC-EX EX-CommandStation running version 5.4.0 or later.

Either download the library from the Arduino Library Manager or clone the git repository to a suitable location. If using a local clone, update the `symlink:///home/pete/code/DCCEXProtocol` path in `platformio.ini` to point at your local copy.

The sketch uses split source files to keep the entry point small:

- `DCCEXProtocolTesting.ino` - entry point, globals, `setup()` and `loop()`
- `Globals.h` - extern declarations shared by the other files
- `PrintHelpers.h` - enum renderers and list printers for console output
- `TestListener.h` - implementation of all 25 `DCCEXProtocolDelegate` callbacks
- `Console.h` - interactive serial console used to enter `<T id>` / `<R id>` commands
- `TestSequence.h` - the menu driven operator assisted test suite

### User configuration

Copy the example files and fill in your own values (both are gitignored so you don't have to worry about committing them):

- `myConfig.example.h` -> `myConfig.h` - optional `STARTUP_DELAY` (delay before starting, to match the command station startup) and `ALIVE_DELAY` (print an alive message at this interval)
- `myWiFi.example.h` -> `myWiFi.h` - SSID, password and the IP address/port of your EX-CommandStation (ESP32 only)

### Hardware setup

| Device | Connection |
| ------ | ---------- |
| STM32 Bluepill | Serial1 (`PA9`/`PA10`) via USB-TTL adaptor, 115200 baud. JTAG on `PA13`/`PA14` is disabled in `setup()` so `PA13`/`PA14` can be used as normal GPIO |
| ESP32 Wemos D1 mini32 | WiFi, connects to your EX-CommandStation's IP and port |

Both devices use the serial monitor at 115200 baud for the console output.

## Loading myAutomation.h onto the Command Station

The `EX-CommandStation_Automation` directory contains a sample `myAutomation.h` that creates dummy objects and routes used to exercise the library functions.

Copy `myAutomation.h` into your EX-CommandStation build, recompile and flash the command station.

It creates:

- 15 roster entries (2004-2066)
- 21 JMRI sensors (6000-6300)
- 3 DCC turntables (2 with 6 positions, 3 home only, 4 with 3 positions)
- 10 turnouts (including a HIDDEN one, a virtual one and a legacy DCC one - see the file for details)
- Routes 200, 400, 500, 700-708 and automations 300, 301

## Running the test suite

1. Flash your command station with `myAutomation.h` loaded.
2. Flash the device under test (DUT) with this sketch (`pio run -t upload -e bluepill_f103c8` or `pio run -t upload -e wemos_d1_mini32`).
3. Open the DUT serial monitor at 115200 baud. The DUT will connect, request the lists and print the server version and all retrieved lists.
4. The DUT then displays a menu of available tests and waits for a command. Enter a command in DCC-EX protocol style to commence each test, and the menu is displayed again after every test completes.

## Interactive console

The DUT serial console accepts commands framed with `<` and `>` like the DCC-EX protocol. Everything typed outside of a `<...>` frame (including carriage returns, line feeds and stray bytes) is ignored.

Two opcodes differentiate how a test is commenced:

- `<R id>` - start a **R**oute on the command station. The DUT sends `startRoute(id)` (`< / START id>`), the command station runs the route and broadcasts activity, and the DUT monitors and prints the received delegate callbacks.
- `<T id>` - run a **T**est locally on the DUT. The DUT drives activity directly via the DCCEXProtocol library API (throttles, turnouts, CV programming etc.) and monitors the responses/broadcasts.

Nothing is commenced automatically - every test is started by entering a command. There is no timeout while waiting for input, so the operator is present during testing.

## Available tests

### Routes on the command station (`<R id>`)

| Command | Route | What the DUT console should print |
| ------- | ----- | -------------------------------- |
| `<R 500>` | Sensor Test | `receivedJMRISensorBroadcast()` for sensors 6000, 6001, 6100, 6101, 6200, 6201, 6300 with Activated/Deactivated states matching the CS BROADCAST commands |
| `<R 700>` | Loco Drive | `receivedLocoBroadcast()` and `receivedLocoUpdate()` for roster loco 2010 (speed 30, F0 on/off) |
| `<R 701>` | Local Loco Drive | `receivedLocoBroadcast()` for non-roster loco 9999 (speed 25, F1 on/off) |
| `<R 702>` | Turnout Ops | `receivedTurnoutAction()` for turnouts 100, 101, 102, 110 (Thrown/Closed states) |
| `<R 703>` | Turntable Ops | `receivedTurntableAction()` for turntables 2 and 4 with position/moving changes |
| `<R 704>` | Power Changes | `receivedTrackPower()` and `receivedIndividualTrackPower()` for PowerOff/PowerOn |
| `<R 705>` | Messages | `receivedMessage()` and `receivedScreenUpdate()` for the two messages and screen update |
| `<R 706>` | Consist Ops | `receivedCSConsist()` for lead 2010 with members 2014 (rev) and 2016, then 2010 alone after BREAK_CONSIST |
| `<R 708>` | Delayed Activity | No DUT output expected - verify PRINT markers and pause/resume behaviour on the CS console |

### Local tests on the DUT (`<T id>`)

| Command | Test | What to verify |
| ------- | ---- | -------------- |
| `<T 301>` | Automation Handoff | `receivedLocoBroadcast()` for loco 3001 (speed 15, F0 on/off) after `handOffLoco(3001, 301)` |
| `<T 500>` | JMRI Sensors | Sensor list request response |
| `<T 700>` | Roster Loco Control | `receivedLocoBroadcast()` and `receivedLocoUpdate()` for loco 2010 while the DUT drives it |
| `<T 701>` | Local Loco Control | `receivedLocoBroadcast()` for local loco 9999 while the DUT drives it, then list cleanup after delete |
| `<T 702>` | Turnout Control | `receivedTurnoutAction()` while the DUT throws/closes/toggles turnouts 100-102 |
| `<T 703>` | Turntable Control | `receivedTurntableAction()` while the DUT rotates turntables 2 and 4 |
| `<T 704>` | Track Power | `receivedTrackPower()` and `receivedIndividualTrackPower()` while the DUT powers tracks on/off |
| `<T 706>` | Consist Operations | `receivedCSConsist()` while the DUT builds/drives/breaks a consist with lead 2010 |
| `<T 708>` | Delayed Activity | Verify PRINT markers and pause/resume behaviour on the CS console after `startRoute(708)`, `pauseRoutes()` and `resumeRoutes()` |
| `<T 710>` | Track Types | Track mode changes - check the CS console for responses |
| `<T 711>` | Track Currents | Current gauge/current responses on the DUT console |
| `<T 712>` | Momentum | No delegate callbacks expected - check the CS console for responses |
| `<T 713>` | DCC Accessories | No delegate callbacks expected - check the CS console for responses |
| `<T 714>` | CV Programming | Read loco address response - requires a loco on the PROG track. No writes are performed |
| `<T 715>` | Fast Clock | Fast clock time response on the DUT console |
| `<T 716>` | List Refresh & Misc | Refreshed lists printed after `refreshAllLists()` |

The DUT drives activity directly for the `<T id>` tests (throttles, turnouts, turntables, CV programming, consists etc.) so the CS console is expected to show matching output for those operations too. Routes are started by entering `<R id>` at the DUT serial console.
