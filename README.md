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

- 4 roster entries (2010, 2014, 2016, 2030)
- 21 JMRI sensors (6000-6300)
- 2 DCC turntables (2 with 6 positions, 4 with 3 positions)
- 5 turnouts (100/101/102, virtual 110 and linear 121 at accessory index 500)
- Routes 500, 700-706, 708 and automation 301 (all created by tests; there are no basic demo routes):

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
| `<R 1>` | CS Route 700 (Loco Drive) | `<< startRoute 700` then `receivedLocoBroadcast()`/`receivedLocoUpdate()` for roster loco 2010 (speed 30F, F0 on/off) - confirm the matching `<t 2010 ...>`/`<F ...>` traffic on the CS console |
| `<R 2>` | CS Route 701 (Local Loco Drive) | `receivedLocoBroadcast()` for non-roster loco 9999 (speed 25, F1 on/off) |
| `<R 3>` | CS Route 702 (Turnout Ops) | `receivedTurnoutAction()` for turnouts 100, 101 (on), 110, 102 with Thrown/Closed states matching the CS THROW/CLOSE commands |
| `<R 4>` | CS Route 703 (Turntable Ops) | `receivedTurntableAction()` for turntables 2 and 4 with position/moving changes matching the CS ROTATE/ROTATEU commands |
| `<R 5>` | CS Route 704 (Power Changes) | `receivedTrackPower()` (x3) and `receivedIndividualTrackPower()` for track B; confirm power state changes on the CS console |
| `<R 6>` | CS Route 705 (Messages) | `receivedMessage()` for the two messages and `receivedScreenUpdate()` for the screen update; the message text prints on the DUT console |
| `<R 7>` | CS Route 706 (Consist Ops) | `receivedCSConsist()` once for lead 2010 + members (2014 rev, 2016); the later BREAK_CONSIST may not broadcast - check no consist remains on the CS console |
| `<R 8>` | CS Route 500 (Sensor Test) | `receivedJMRISensorBroadcast()` for sensors 6000, 6001, 6100, 6101, 6200, 6201, 6300 with Activated/Deactivated matching the CS BROADCAST commands |
| `<R 9>` | CS Route 708 (Delayed Activity) | `receivedTurnoutAction()` for turnout 100 (thrown) early and 101 (thrown then closed) later over ~20s; watch the CS console show the delayed PRINT markers |

### Local tests on the DUT (`<T id>`)

| Command | Test | Phases (`<< n` markers) and what to verify |
| ------- | ---- | ------------------------------------------- |
| `<T 1>` | Version, Lists & Refresh | `<< 1` reset lists + request version; `<< 2` refresh all lists; `<< 3` received flags. Expect version, then roster/turnout/route/turntable counts, the full lists and the four `received*List()` flags to print; verify the version string on the CS console |
| `<T 2>` | Roster Loco Control | `<< 1` roster loco 2010; `<< 2` throttle 30F `<t 2010 30 1>`; `<< 3` fn 0 on / `<< 4` fn 0 off; `<< 5` throttle 60R `<t 2010 60 0>`; `<< 6` stop `<t 2010 0 1>`; `<< 7` update `<t 2010>`. Expect `receivedLocoBroadcast()`/`receivedLocoUpdate()` after each |
| `<T 3>` | Local Loco Control | `<< 1` new local loco 9999; `<< 2` throttle 25F `<t 9999 25 1>`; `<< 3` fn 1 on; `<< 4` stop; `<< 5` delete local loco. Expect `receivedLocoBroadcast()`/`receivedLocoUpdate()` after the throttle/function phases |
| `<T 4>` | Turnout Control | `<< 1` turnout 100 throw; `<< 2` 100 close `<T 100 0>`; `<< 3`/`<< 4` toggle 101; `<< 5` turnout 102 throw. Expect `receivedTurnoutAction()` each time, states matching |
| `<T 5>` | Turntable Control | `<< 1`-`<< 5` rotate turntable 2 (->2, ->4, ->home) and 4 (->1, ->3). Expect `receivedTurntableAction()` with matching positions (home = 0) |
| `<T 6>` | Track Power | `<< 1`-`<< 3` power all on/off `<1>`/`<0>`; `<< 4` MAIN `<0/1 MAIN>`; `<< 5` PROG `<0/1 PROG>` (often no broadcast - check the PROG track); `<< 6` track B `<1/0 B>`; `<< 7` join PROG `<1 JOIN>` (may not broadcast). Expect `receivedTrackPower()`/`receivedIndividualTrackPower()` where broadcast |
| `<T 7>` | Track Types | `<< 1`-`<< 5` cycle track A MAIN/PROG/DC 5/DCX 6/MAIN. Expect `receivedTrackType()` each time with matching type/address where applicable |
| `<T 8>` | Track Currents | `<< 1` gauges `<jG>`; `<< 2` currents `<jI>`. Expect `receivedTrackCurrentGauge()` and `receivedTrackCurrent()` (8 reports of each) |
| `<T 9>` | Momentum | No callback expected - check the CS console: `<< 1` algorithm -> Linear; `<< 2` default 10; `<< 3` default 10/5; `<< 4` loco 2010 20; `<< 5` 2010 10/5; `<< 6`/`<< 7` loco 2014 15 and 15/10 - confirm `<m>` responses |
| `<T 10>` | DCC Accessories | No callback expected - check the CS console: `<< 1`/`<< 2` accessory 10 sub 1 on/off `<A 10 1 1/0>`; `<< 3`/`<< 4` linear accessory 500 on/off `<a 500 1/0>` |
| `<T 11>` | Consist Operations | `<< 1` consist lead 2010; `<< 2` member 2014 rev; `<< 3` member 2016; `<< 4` consist throttle 20F; `<< 5`/`<< 6` consist fn 0 on/off; `<< 7` lookup by lead/member; `<< 8` consist stop; `<< 9` remove member 2016; `<< 10` delete consist 2010; `<< 11` replicated-functions consist lead 2030 (+member 2014); `<< 12` delete consist 2030; `<< 13` clear consist list; `<< 14` req consist list `<^>`. Expect `receivedCSConsist()` where the CS broadcasts - deletion may not broadcast |
| `<T 12>` | Automation Handoff | `<< 1` handoff 3001 -> automation 301. Expect `receivedLocoBroadcast()` x4 for loco 3001 (speed 15, F0 on/off) - confirm the automation drives it on the CS console |
| `<T 13>` | JMRI Sensor List | `<< 1` sensor list `<Q>`; `<< 2` expect 21 broadcasts. Expect `receivedJMRISensorBroadcast()` for all 21 sensors while the CS PRINTS them |
| `<T 14>` | CV Programming (read/write) | Self-restoring: `<< 1` read addr `<R>`; `<< 2` write addr back; `<< 3` read cv 29; `<< 4` validate cv 29; `<< 5` cv bit 29:5 -> 1; `<< 6` validate bit 29:5; `<< 7` cv 29 on main 2010; `<< 8` cv bit 29:5 on main 2010; `<< 9` restore main cv 29; `<< 10` restore cv 29. Requires a loco on the PROG track (a failed read skips the write phases). WARNINGs print first - drives the PROG track and later writes CVs on the main |
| `<T 15>` | Fast Clock | `<< 1` clock 07:00 x4 `<jT 420 4>`; `<< 2` clock time `<jC>`. Expect `receivedSetFastClock()` and `receivedFastClockTime()`; confirm the CS clock display |
| `<T 16>` | Delayed Activity + Pause/Resume | `<< 1` start route 708; `<< 2` pause routes `< / PAUSE>`; `<< 3` resume routes `< / RESUME>`. Expect `receivedTurnoutAction()` for 100 (thrown) then 101/100/101 and `>> route 708 should be complete now` - watch pause/resume on the CS console |
| `<T 17>` | Miscellaneous | `<< 1` clear local locos; `<< 2` supported locos `<#>`; `<< 3` emergency stop `< !>` (WARNING - powers off all tracks, restored at `<< 4` power all on); `<< 5` legacy `Consist` API loco 2015 (local speed + `<F 2015 1 1/0>` traffic); `<< 6` debug on + request version; `<< 7` debug off; `<< 8` clear all lists; `<< 9` refresh all lists (re-fetched lists print); `<< 10` `sendCommand("J C")` - raw command via the public API, expect the fast clock time response |
| `<T 18>` | List Maintenance | `<< 1`/`<< 2` clear+fetch roster; `<< 3`/`<< 4` turnouts; `<< 5`/`<< 6` routes; `<< 7`/`<< 8` turntables. Expect each `refresh*List()` to re-fetch that list (count prints after each fetch); verify the repopulated lists on the CS console |

The DUT drives activity directly for the `<T id>` tests (throttles, turnouts, turntables, CV programming, consists etc.) so the CS console is expected to show matching output for those operations too. Routes are started by entering `<R id>` at the DUT serial console.

## Developer: API coverage audit

The project invariant (see AGENTS.md) is that **every public `DCCEXProtocol` method and every
`DCCEXProtocolDelegate` callback is exercised by some test**. `tools/check_coverage.py` audits this against the live
library header and the sketch sources, so it catches the moment the library grows a method or callback that no test
covers.

Run it (always from the Python virtual environment - the tool itself is standard-library only, but the venv keeps
every developer on a known interpreter):

```sh
python3 -m venv .venv
.venv/bin/pip install -r tools/requirements.txt
.venv/bin/python tools/check_coverage.py
```

- Pass `--lib /path/to/DCCEXProtocol` if the library is not a sibling of this repository (the default matches the
  `symlink:///home/pete/code/DCCEXProtocol` used by `platformio.ini`).
- Output is one row per symbol: `covered`, `skipped` (documented exception: `disconnect()`, a no-op stub) or
  `*** GAP ***`. The exit code is non-zero when an unexpected gap is found.
- The sketch, the headers and the library are all one translation unit, and comment text is stripped before scanning,
  so documentation mentions never count as coverage. Public methods count only when called through an object
  (`csClient.method(...)`); delegate callbacks count when their `expect...()` counterpart is used by a test.
