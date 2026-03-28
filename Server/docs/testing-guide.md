# End-to-End Testing Guide

## Overview

The E2E testing framework uses a forked SA-MP bot client (`RakClient/`) to test the server from the **player's perspective** — asserting on what the server actually sends (dialogs, messages, position corrections) rather than just inspecting server-side state.

This gives confidence that:
- The auth flow works end-to-end (registration, login, data persistence)
- Players receive the correct dialogs, messages, and spawn positions
- Async chains (bcrypt → MySQL → event bus → spawn) complete correctly

---

## Prerequisites

- OMP server running (`omp-launcher.exe` from `Server/`)
- MySQL container running (`docker-compose up -d` from `Server/`)
- Node 18 LTS installed (`nvm use 18`)
- RakClient built: `cd RakClient/app && npx node-gyp rebuild`

---

## Running Tests

```bash
cd RakClient/app

# Smoke test: verify bot connects and receives basic events
node tests/test-events.js

# Full registration flow: register, receive welcome message, verify spawn
node tests/test-register.js

# Debug mode: log all events for 20s (useful for diagnosing new flows)
node tests/test-debug.js
```

---

## Test Structure

Tests are async/await scripts using the `RakClient` wrapper class:

```js
const { RakClient } = require('../lib/rak-client');
const assert = require('assert');

async function testSomething() {
    const bot = new RakClient();

    // Register ALL listeners before connect — events can fire immediately
    const gameInitP = bot.waitForGameInit(10000);
    const dialogP   = bot.waitForDialog(10000);

    bot.connect();

    const gameInit = await gameInitP;
    console.log(`[PASS] gameInit — player ${gameInit.playerId} on "${gameInit.hostname}"`);

    const dialog = await dialogP;
    assert(dialog.title.includes('Register'), `Expected Register dialog`);

    // Set up next waiters BEFORE triggering the action
    const welcomeP = bot.waitForMessage(/logged in/i, 15000);
    const posP     = bot.waitForPos(15000);

    // IMPORTANT: listItem must be -1 for password/input/msgbox dialogs (OMP requirement)
    bot.respondDialog(dialog.id, 1, -1, 'testpass123');

    const welcome = await welcomeP;
    const pos = await posP;

    bot.disconnect();
    console.log('PASS');
}

testSomething().catch(err => {
    console.error('FAIL:', err.message);
    process.exit(1);
});
```

**Critical pattern**: Always create `waitFor*` promises **before** the action that triggers them. The event bus delivers events synchronously within the same tick.

**OMP dialog quirk**: For password/input/msgbox dialogs, `listItem` must be `-1`, not `0`. OMP silently drops dialog responses where listItem doesn't match the expected value for the dialog style. For list dialogs, use the actual row index.

---

## RakClient API

### Connecting

```js
const bot = new RakClient();
bot.connect();      // loads RakSAMPClient.xml, starts network timer
bot.disconnect();   // stops timer, closes connection
```

Connection settings (nick, host, port) are in `RakClient/app/RakSAMPClient.xml`.

### Sending Actions

```js
bot.sendChat("hello server");
bot.sendCommand("/stats");
bot.respondDialog(dialogId, buttonIndex, listItem, inputText);
  // buttonIndex: 1 = left button (OK/Login/Register), 0 = right button (Cancel/Quit)
  // listItem: -1 for PASSWORD/INPUT/MSGBOX dialogs, row index for LIST dialogs
bot.spawn();            // sends Spawn RPC (id=52) only
bot.requestClass(0);    // sends class selection — MUST be called after gameInit
bot.setPosition(x, y, z, angle);  // updates reported position (synced automatically)
```

### Waiting for Events

```js
// Built-in helpers (all return Promises)
await bot.waitForGameInit(timeoutMs);
await bot.waitForDialog(timeoutMs);
await bot.waitForPos(timeoutMs);
await bot.waitForMessage(pattern, timeoutMs);   // pattern: string or RegExp

// Generic waiter with optional filter
await bot.waitFor('eventName', timeoutMs, data => data.someField === value);
```

All waiters reject with a timeout error if the event doesn't arrive in time.

### Listening to Events

```js
bot.on('gameInit',      d => { /* d.playerId, d.hostname */ });
bot.on('dialog',        d => { /* d.id, d.style, d.title, d.body, d.button1, d.button2 */ });
bot.on('clientMessage', d => { /* d.color, d.text (color codes stripped) */ });
bot.on('chat',          d => { /* d.playerId, d.text */ });
bot.on('setPos',        d => { /* d.x, d.y, d.z */ });
bot.on('setHealth',     d => { /* d.health */ });
bot.on('setArmor',      d => { /* d.armor */ });
bot.on('setSkin',       d => { /* d.playerId, d.skinId */ });
bot.on('requestClass',  d => { /* d.outcome, d.skin, d.x, d.y, d.z */ });
bot.on('playerJoin',    d => { /* d.playerId, d.name, d.isNpc */ });
bot.on('playerQuit',    d => { /* d.playerId, d.reason */ });
bot.on('rejected',      d => { /* d.reason */ });
bot.on('spawnInfo',     d => { /* d.skin, d.x, d.y, d.z */ });
```

### High-level Helpers

```js
// Connect, requestClass(0), wait for first dialog — for tests that don't need
// to test the connect/class-selection boundary explicitly
const { gameInit, dialog } = await bot.connectAndInit(timeout);

// Move in a straight line (async, resolves when done)
await bot.walk(toX, toY, toZ, { fromX, fromY, fromZ, steps: 5, duration: 1000 });

// Async pause
await bot.sleep(500);
```

### Handling Spawn After Login/Register

After successful auth, the server sends `spawnInfo` with the spawn position. Set up the waiter **before** responding to the dialog (events arrive in one batch), then call `spawn()` after receiving it:

```js
const spawnInfoP = bot.waitFor('spawnInfo', 15000);

bot.respondDialog(dialog.id, 1, -1, password);

const spawnInfo = await spawnInfoP;
bot.spawn();
```

The server sends `spawnInfo` twice (once for initial spawn, once for respawn-after-death config). Only call `spawn()` once.

---

## Writing Tests

### Test Isolation

Each test should use a **unique bot nick** that doesn't exist in the DB. Test cleanup: delete the account after the test, or use a naming convention (`TestBot_Register`, `TestBot_Login`).

For the registration test, the account must NOT exist. Delete it:
```sql
DELETE FROM accounts WHERE username = 'TestBot';
```
Or use a fresh nick per run (e.g., append a timestamp).

### Asserting Dialogs

```js
const dialog = await bot.waitForDialog(10000);
assert(dialog.title.includes('Register'), `Expected Register dialog, got: "${dialog.title}"`);
assert(dialog.body.includes('not registered'), `Expected registration body`);
assert.strictEqual(dialog.style, 3, 'Expected DIALOG_STYLE_PASSWORD');
```

### Asserting Messages

```js
const msg = await bot.waitForMessage(/logged in successfully/i, 15000);
// msg.text is the raw text (SA-MP color codes stripped)
// msg.color is the uint32 color value
```

### Asserting Spawn Position

```js
const pos = await bot.waitForPos(15000);
const SPAWN_X = -2233.97, SPAWN_Y = -1737.58;
assert(
    Math.abs(pos.x - SPAWN_X) < 20 && Math.abs(pos.y - SPAWN_Y) < 20,
    `Expected spawn near Mount Chiliad, got (${pos.x.toFixed(1)}, ${pos.y.toFixed(1)})`
);
```

### Handling Multiple Dialogs (sequential flows)

```js
// Dialog 1: auth
const authDialog = await bot.waitForDialog(10000);
bot.respondDialog(authDialog.id, 1, -1, password);

// Dialog 2: some follow-up prompt
const nextDialogP = bot.waitForDialog(10000);  // set up before action
bot.sendCommand('/some_command');
const nextDialog = await nextDialogP;
```

---

## Test Scenarios Checklist

### Auth

- [ ] **Registration**: New nick → Register dialog → submit password → welcome message → spawn at Mount Chiliad
- [ ] **Login**: Existing nick → Login dialog → correct password → welcome message → spawn at saved position
- [ ] **Wrong password**: Login → wrong password → error message → dialog shown again
- [ ] **Too many attempts**: 3 wrong passwords → kick message

### Player Data Persistence

- [ ] Register → move to a position → disconnect → reconnect → verify spawn at saved position
- [ ] Register → gain score/money → disconnect → reconnect → verify stats restored

### Admin System

- [ ] Non-admin player: `/kick` → "You don't have permission"
- [ ] Admin level 2+: `/tp 0 0 0` → position update received

---

## Debugging a Failing Test

**1. Run test-debug.js first** to see all raw events:
```bash
node tests/test-debug.js
```
This logs every event for 20 seconds and auto-responds to the first dialog.

**2. Check server MySQL log** for async chain completion:
```
Server/logs/plugins/mysql.log
```
Look for bcrypt callbacks, INSERT/SELECT queries completing after the bot responds to dialog.

**3. Check server errors**:
```
Server/logs/errors.log
Server/logs/warnings.log
```

**4. Add C++ diagnostics** to `RakClient/src/net/netrpc.cpp`:
```cpp
void SomeRpcHandler(RPCParameters *rpcParams) {
    Log("[RPC] SomeRpcHandler called");   // will appear in Node stdout
    ...
}
```
Then rebuild: `npx node-gyp rebuild`

**5. Timing issues**: If an event arrives before `waitFor()` is called, it's lost. Always set up waiters before triggering actions.

---

## Planned Tests

| Test File | What It Covers | Status |
|-----------|----------------|--------|
| `test-events.js` | gameInit + dialog arrive | Done |
| `test-debug.js` | All-event logger, 20s | Done |
| `test-register.js` | Full registration flow | In progress |
| `test-login.js` | Login + data restore | Planned |
| `test-admin.js` | Admin commands | Planned |
| `test-commands.js` | Basic command routing | Planned |
