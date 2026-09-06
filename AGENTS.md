# AGENTS.md

Guide for AI agents and humans making changes to this repository. Read this before editing. The goal of this project
is end-to-end hardware testing of the [DCCEXProtocol](https://github.com/DCC-EX/DCCEXProtocol) library against a real
EX-CommandStation, with a human operator watching both consoles.

## Project overview

- The device under test (DUT) is an Arduino running this sketch (STM32 Bluepill via `Serial1`, or ESP32 via WiFi).
  It connects to an EX-CommandStation loaded with `EX-CommandStation_Automation/myAutomation.h`.
- Testing is **operator assisted and asynchronous**: the DCC-EX protocol API has no guaranteed response timing. The
  operator is the final assertion engine and monitors BOTH the DUT serial console and the EX-CommandStation console.
- `DCCEXProtocolTesting.ino` -> `Globals.h` -> `PrintHelpers.h` -> `TestListener.h` -> `Console.h` -> `TestSequence.h`.
  The `.ino` boots straight to the menu driven test console (`runTestConsole()`) with **no auto-connect**: the operator
  connects to the EX-CommandStation manually with `<C>` (`connectToCommandStation()`), which brings up the physical
  link (ESP32 WiFi), requests the server version and object lists, waits up to `CONNECT_TIMEOUT` ms (default 10000,
  override in `myConfig.h`), then prints the connection summary and a `myAutomation.h` checklist.

## Test strategy (invariant)

Every public `DCCEXProtocol` method and every `DCCEXProtocolDelegate` callback **must** be exercised by some test.
When the library adds methods/callbacks, that is a test gap - add coverage; never silently skip it. Run the coverage
audit after any library or sketch change to verify (see README.md `## Developer: API coverage audit`) - it exits
non-zero on an unexpected gap.

One exception: `DCCEXProtocol::disconnect()` is currently a no-op stub in the library, so it is deliberately not
exercised. The deprecated `Consist *` overloads of `setThrottle()`, `functionOn()`, `functionOff()` and
`isFunctionOn()` ARE exercised in `<T 17>` via a local legacy `Consist`.

Two test classes:

- `<T id>` **local tests** - the DUT drives activity via the library API and monitors the resulting responses and
  broadcasts. Callbacks a client can solicit belong here (e.g. `receivedTurnoutAction` via `throwTurnout()`, CV
  callbacks via `writeCV()`/`validateCV()`/etc.).
- `<R id>` **route tests** - the DUT calls `DCCEXProtocol::startRoute()` (`< / START id>`) to start a command-station
  route that produces broadcasts a client cannot solicit (e.g. `receivedMessage`, `receivedScreenUpdate`,
  `receivedJMRISensorBroadcast`, true `receivedLocoBroadcast`). This also tests `startRoute()` itself.

## Framework rules

- `localTests[]` and `routeTests[]` in `TestSequence.h` are the **single source of truth** for menus, ids and dispatch.
- Adding a test = add one registry row + implement/reference the test function. **Never** hardcode menu text or add a
  dispatch `switch` mirroring the registry.
- Route observation is driven by the `RouteTest.observeMs` window and the expectations set by the route test's body
- `<C>` is a connect command, not a test: it never appears in the test registers, prints no TEST COMPLETE banner, and
  the `myAutomation.h` reminder appears only in its output. A successful `<C>` sets the global `csConnected` flag, which
  gates the `<T>`/`<R>` tests - they are refused with an error until a connection has been established.

## ID rules

- T and R ids are **sequential** (`<T 1..17>`, `<R 1..9>`) and are **independent of** the CS route ids. The mapping
  between an R id and its underlying CS route lives only in the `RouteTest.csRouteId` field.
- Never renumber without updating: the registry, the README matrices, and the `myAutomation.h` route comments.

## Validation engine

Tests use typed expectations set on the listener (e.g. `csListener.expectTurnoutAction(100, true)`) plus observation
windows that pump `csClient.check()` until all expectations are matched. A local test may have several
expectation->operate->window phases; each `waitForExpectations()` replaces raw `testDelay()` waits for that phase.
Route tests set their expectations up front and use one window (`RouteTest.observeMs`). Rule for each event:

- No expectation for that callback -> print normally, ignore.
- Expectation but **different object** (e.g. turnout 101 while expecting 100) -> print normally, ignore (never fails).
- Expectation, same object, **wrong value** -> **immediate FAIL** (`UNEXPECTED VALUE`), test marked failed.
- Match -> expectation satisfied.
- Window timeout with unmet expectations -> **FAIL** listing every missing expectation.

Values that cannot be known up front use `EXPECT_ANY` (e.g. track currents, CS version). Tests with no expectations
(for example momentum) simply observe for the window. The validation engine replaces raw `testDelay()` waits.

## Test hygiene

- Prefer self-restoring tests: read a value, then write the same value back (e.g. `readLoco()` then
  `writeLocoAddress(<same address>)`).
- Destructive operations (prog-track CV writes, track mode changes) must print a short WARNING marker and the shared
  `>> cross-check on the CS console` line, and give the operator time to observe. The per-phase detail the operator
  verifies lives in README.md `## Available tests` (the DUT prints `<< <phase> <op> <command>` markers that map to it).
- Never block inside a test without pumping `csClient.check()` - delayed/async responses require it.
- Clobbered client-side lists must be re-fetched within the same test (the library's `refresh*()`/`getLists()`), so
  no test leaves the DUT in a broken state for the next one.

## Command-station coupling (`myAutomation.h`)

The CS sketch may only create:

- the objects the tests rely on: roster (2004-2066), JMRI sensors (6000-6300), turnouts 100-105/110/120/121,
  turntables 2/3/4;
- the routes that are `<R id>` targets (CS routes 500, 700-706, 708) and AUTOMATION 301 (used by the handoff test).

No "basic" demo routes/automations (200, 300, 400). If an object shared with a test is renamed/removed on the CS
side, the corresponding tests must be updated in the same change.

## House style

- C++17, `-Wall -Os` (see `platformio.ini`). `-flto` has been validated for the Bluepill env only (it frees 8-11 KB of
  flash) but is NOT enabled - prebuilt ESP-IDF libraries are not LTO objects and the ESP32 env fails to link with it.
  Reuse of shared fragment literals and terse console markers is preferred over build flags; see the narration rule.
- Console narration is **terse markers, not sentences**: the DUT prints `<< <phase> <op> <command>` via the `phase()`
  helper in `TestSequence.h`, and the full operator instructions live in README.md `## Available tests`. Compose
  markers from a small pool of shared fragment literals streamed through `Serial.print` (the same fragment-reuse idea
  as DCCEXProtocol's `_cmdAppend()`), never one-off sentences. This keeps the Bluepill firmware inside its 64 KB flash.
- Code must comply with the project `.clang-format` file. Format changed/new C++ files with
  `clang-format -i <file>` (ColumnLimit 120, 2-space indent) before considering a change done.
- Helper code lives in headers as `static` functions; public-ish helpers use Doxygen `@brief`/`@param` comments.
- Never use `printf`/`sprintf`/`snprintf`/`sscanf` in the sketch or helper headers: they drag the whole libc formatter
  (and on STM32 its float machinery) into flash. The Bluepill firmware runs at ~99% of the 64 KB - keep string
  literals concise. Check with `grep -rn "printf" *.h *.ino`.
- Objects are declared in the `.ino` and `extern`ed via `Globals.h`.
- ESP32 vs STM32 behaviour is gated with `#if defined(ARDUINO_ARCH_...)`. Serial monitor: 115200 baud.
- All human-facing text uses **Australian English spelling**: documentation, comments, README, serial console
  output/operator instructions and any other display text. Key rules: `-ise` not `-ize` (customise, organise,
  recognise, summarise); `-our` not `-or` (colour, behaviour, favour, honour); doubled-l before suffixes
  (travelled, cancelled, labelled, modelling); licence (noun) vs license (verb); "program" is acceptable only in
  the computing sense, not "programme".
- User config is `myConfig.h` / `myWiFi.h` (both gitignored; edit the `.example.h` files, not the local ones).

## Verification gate

A change is not done until it compiles for **both** environments and the API coverage audit passes:

- `pio run -e bluepill_f103c8`
- `pio run -e wemos_d1_mini32`
- `.venv/bin/python tools/check_coverage.py` (exits non-zero on any unexpected gap)

The library is linked via `symlink:///home/pete/code/DCCEXProtocol` in `platformio.ini` (adjust the path if your
local clone lives elsewhere). `myConfig.h`/`myWiFi.h` must never be committed. Hardware behaviour must ultimately be
verified by the operator on the live rig.

## Developer tooling

- Python developer tools live in `tools/`. Any requirements are installed **only** into the project virtual
  environment (`.venv`/), never into the system interpreter: `python3 -m venv .venv` then
  `.venv/bin/pip install -r tools/requirements.txt`. Always invoke tools via `.venv/bin/python tools/...`.
- `.venv/` is gitignored; `tools/requirements.txt` is the manifest for the environment.

## Trust warning

The existing code contains AI-assisted work. Do not assume it is correct - question it against the library behaviour,
the hardware and the protocol. When in doubt, the DCC-EX sources and the DCCEXProtocol header are the references.
