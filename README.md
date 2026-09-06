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
- `TestListener.h` - implementation of all 25 `DCCEXProtocolDelegate` callbacks plus the typed expectation engine
- `Console.h` - interactive serial console used to enter `<C>`, `<T id>` and `<R id>` commands
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
- Routes 500, 700-708 and automation 301 (all created by tests; there are no basic demo routes)

## Running the test suite

1. Flash your command station with `myAutomation.h` loaded.
2. Flash the device under test (DUT) with this sketch (`pio run -t upload -e bluepill_f103c8` or `pio run -t upload -e wemos_d1_mini32`).
3. Open the DUT serial monitor at 115200 baud. The DUT boots straight to the test console with **no auto-connect**.
4. Enter `<C>` to connect to the command station. This brings up the physical link, requests the server version and
   all object lists, then prints a connection summary and a checklist of the objects expected from `myAutomation.h`.
5. The DUT then displays a menu of available tests and waits for a command. Enter a command in DCC-EX protocol style
   to commence each test, and the menu is displayed again after every test completes.

## Interactive console

The DUT serial console accepts commands framed with `<` and `>` like the DCC-EX protocol. Everything typed outside of a `<...>` frame (including carriage returns, line feeds and stray bytes) is ignored.

Three opcodes differentiate how a test is commenced:

- `<C>` - connect to the command station (brings up the physical link and requests the server version/object lists). This is a connect command, not a test: it prints no TEST COMPLETE banner.
- `<R id>` - start a **R**oute on the command station. The DUT sends `startRoute(id)` (`< / START id>`), the command station runs the route and broadcasts activity, and the DUT monitors and prints the received delegate callbacks.
- `<T id>` - run a **T**est locally on the DUT. The DUT drives activity directly via the DCCEXProtocol library API (throttles, turnouts, CV programming etc.) and monitors the responses/broadcasts.

Nothing is commenced automatically - every test is started by entering a command. There is no timeout while waiting for input, so the operator is present during testing.

## Available tests

### Routes on the command station (`<R id>`)

The R id is sequential 1-9 and is independent of the CS route id (shown in the Route column).

| Command | Route | What the DUT console should print |
| ------- | ----- | -------------------------------- |
| `<R 1>` | CS Route 700 (Loco Drive) | `receivedLocoBroadcast()` and `receivedLocoUpdate()` for roster loco 2010 (speed 30 forward, F0 on/off) |
| `<R 2>` | CS Route 701 (Local Loco Drive) | `receivedLocoBroadcast()` and `receivedLocoUpdate()` for non-roster loco 9999 (speed 25, F1 on/off) |
| `<R 3>` | CS Route 702 (Turnout Ops) | `receivedTurnoutAction()` for turnouts 100, 101, 110, 102 (Thrown/Closed states) |
| `<R 4>` | CS Route 703 (Turntable Ops) | `receivedTurntableAction()` for turntables 2 and 4 with position/moving changes |
| `<R 5>` | CS Route 704 (Power Changes) | `receivedTrackPower()` and `receivedIndividualTrackPower()` for PowerOff/PowerOn on all tracks and track B |
| `<R 6>` | CS Route 705 (Messages) | `receivedMessage()` and `receivedScreenUpdate()` for the two messages and screen update |
| `<R 7>` | CS Route 706 (Consist Ops) | `receivedCSConsist()` for lead 2010 with members 2014 (rev) and 2016, then 2010 alone after BREAK_CONSIST |
| `<R 8>` | CS Route 500 (Sensor Test) | `receivedJMRISensorBroadcast()` for sensors 6000, 6001, 6100, 6101, 6200, 6201, 6300 with Activated/Deactivated states matching the CS BROADCAST commands |
| `<R 9>` | CS Route 708 (Delayed Activity) | `receivedTurnoutAction()` for turnouts 100 (thrown) and 101 (thrown then closed) over ~20s |

### Local tests on the DUT (`<T id>`)

| Command | Test | What to verify |
| ------- | ---- | -------------- |
| `<T 1>` | Version, Lists & Refresh | Server version plus roster/turnout/route/turntable list counts, then `refreshAllLists()` re-fetches every list |
| `<T 2>` | Roster Loco Control | `receivedLocoBroadcast()` and `receivedLocoUpdate()` for roster loco 2010 while the DUT drives it (throttle 30F/60R/0, F0 on/off, `requestLocoUpdate()`) |
| `<T 3>` | Local Loco Control | `receivedLocoBroadcast()` for local loco 9999 while the DUT drives it (throttle 25F/0, F1 on/off), then list cleanup after delete |
| `<T 4>` | Turnout Control | `receivedTurnoutAction()` while the DUT throws/closes turnouts 100 and 102 and toggles turnout 101 |
| `<T 5>` | Turntable Control | `receivedTurntableAction()` while the DUT rotates turntable 2 (positions 2, 4 then home) and turntable 4 (positions 1, 3) |
| `<T 6>` | Track Power | `receivedTrackPower()` while the DUT powers all tracks on/off, the MAIN track off/on, track B off/on and `joinProg()` (PROG power is often not broadcast - check the CS console) |
| `<T 7>` | Track Types | `receivedTrackType()` while the DUT cycles track A through MAIN/PROG/DC/DCX and restores MAIN |
| `<T 8>` | Track Currents | `receivedTrackCurrentGauge()` and `receivedTrackCurrent()` responses for all tracks |
| `<T 9>` | Momentum | No delegate callbacks expected - check the CS console for responses (algorithm, default and per-loco momentum, including accelerating/braking overloads) |
| `<T 10>` | DCC Accessories | No delegate callbacks expected - check the CS console for accessory 10 and linear accessory 500 activation |
| `<T 11>` | Consist Operations | `receivedCSConsist()` and loco broadcasts while the DUT builds a consist (lead 2010, members 2014 rev/2016), drives it, looks it up, removes a member, deletes it both ways, and clears the client-side list |
| `<T 12>` | Automation Handoff | `receivedLocoBroadcast()` for loco 3001 (speed 15, F0 on/off) after `handOffLoco(3001, 301)` |
| `<T 13>` | JMRI Sensor List | `receivedJMRISensorBroadcast()` for all 21 sensors after `requestJMRISensorList()` |
| `<T 14>` | CV Programming (read/write) | `receivedReadLoco()`/`receivedWriteCV()`/`receivedValidateCV()`/`receivedValidateCVBit()`/`receivedWriteLoco()` while reading and writing CVs - self restoring (requires a loco on the PROG track, plus a layout loco for the on-main writes) |
| `<T 15>` | Fast Clock | `receivedSetFastClock()` and `receivedFastClockTime()` after set/request |
| `<T 16>` | Delayed Activity + Pause/Resume | `startRoute(708)` then `pauseRoutes()`/`resumeRoutes()`; verify the PRINT markers and pause/resume behaviour on the CS console |
| `<T 17>` | Miscellaneous | `getLibraryVersion()`, `clearLocalLocos()`, `getNumberSupportedLocos()`, `emergencyStop()` (with power restore), debug output with version re-request, then `refreshAllLists()` |

The DUT drives activity directly for the `<T id>` tests (throttles, turnouts, turntables, CV programming, consists etc.) so the CS console is expected to show matching output for those operations too. Routes are started by entering `<R id>` at the DUT serial console.
