# RakClient Fork — Development Guide

## What This Is

A fork of [SyncroIT/RakClient-NodeGYP](https://github.com/SyncroIT/RakClient-NodeGYP), which implements the full SA-MP 0.3.7 client protocol in C++ with node-gyp/NAN bindings. Our fork extends it into a proper event-emitter-based testing client for OMP servers.

The original project had the full protocol stack (40+ RPC handlers, all outbound actions) but only piped chat to Node.js via a blocking loop. We replaced the blocking loop with a libuv timer and wired all RPCs through an event emitter.

---

## Repository Layout

```
RakClient/
├── src/
│   ├── main/
│   │   ├── main.cpp          ← Node.js module init, uv_timer, EmitEvent, exported methods
│   │   ├── main.h            ← Declarations shared across translation units
│   │   ├── misc_funcs.cpp/h  ← sampSpawn, sampRequestClass, sampDisconnect, etc.
│   │   ├── localplayer.cpp   ← Player state tracking
│   │   ├── xmlsets.cpp/h     ← Loads RakSAMPClient.xml config
│   │   └── log.cpp/h         ← Log() → printf
│   ├── net/
│   │   ├── netrpc.cpp        ← All RPC handlers (server → client)
│   │   └── netgame.cpp       ← Packet handlers, UpdateNetwork() main loop
│   ├── SAMP/
│   │   ├── SAMPRPC.cpp/h     ← RPC ID constants + sampSpawn/sampRequestClass defs
│   │   └── samp_auth.cpp/h   ← SA-MP auth key generation
│   └── ... (RakNet, tinyxml, etc.)
├── app/
│   ├── binding.gyp           ← node-gyp build definition
│   ├── RakSAMPClient.xml     ← Bot connection settings (nick, server, port)
│   ├── build/Release/samp.node  ← Compiled native module
│   ├── lib/
│   │   └── rak-client.js     ← JS EventEmitter wrapper around native module
│   ├── tests/
│   │   ├── test-events.js    ← Smoke test: connect, get gameInit + dialog
│   │   ├── test-debug.js     ← Log all events for 20s, auto-respond to dialogs
│   │   └── test-register.js  ← Full registration flow test
│   └── package.json
```

---

## Build

**Requirements:**
- Node 18 LTS (node-gyp + NAN don't support Node 24 with VS2019 — C++20 issue)
- VS2019 Build Tools with "Desktop development with C++"
- Python 3 (for node-gyp)

**Commands (run from `app/`):**
```bash
npx node-gyp configure   # generate .vcxproj
npx node-gyp build       # incremental build
npx node-gyp rebuild     # clean + build
```

On success: `build/Release/samp.node`

**Known warnings** (harmless):
- `tinyxml.cpp`: `sscanf_s` format string mismatches — pre-existing, not our code
- All 1367 functions recompile on every build (no IPDB caching) — slow but correct

---

## Architecture

### C++ Side

**main.cpp** is the Node.js module entry point. Key pieces:

```cpp
// Persistent JS callback set by connect()
static Nan::Persistent<v8::Function> g_emitCallback;

// libuv timer — fires every 10ms on the JS main thread
static uv_timer_t g_networkTimer;

// Call this from any RPC handler to push an event to JS
void EmitEvent(const char* name, v8::Local<v8::Object> data);
```

The `NetworkTick` timer:
1. Calls `UpdateNetwork(pRakClient)` — drains the RakNet packet queue
2. Initiates connection if `IsConnectionRequired` flag is set

**Why uv_timer instead of AsyncWorker?**
`UpdateNetwork` triggers RPC callbacks synchronously during the drain loop. Since the timer fires on the JS main thread (libuv event loop), those RPC callbacks can safely call `EmitEvent` → V8 → JS. No locking needed.

If we used a background thread, all RPC callbacks would run on the network thread and would need synchronization before touching V8.

**netrpc.cpp** contains all server→client RPC handlers. Each handler:
1. Parses the RPC payload from `RPCParameters*`
2. Builds a `v8::Local<v8::Object>` with the parsed fields
3. Calls `EmitEvent("eventName", data)`

**Log() output** goes to `printf` → stdout → captured by Node.js. Useful for C++-side diagnostics.

### JS Side

**rak-client.js** wraps the native module as an EventEmitter:

```js
class RakClient extends EventEmitter {
    connect() {
        native.connect((eventName, data) => {
            this.emit(eventName, data);   // bridge C++ → EventEmitter
        });
    }
    // waitFor, waitForDialog, waitForMessage, waitForPos helpers
}
```

The `connect(emitCallback)` on the native side stores the callback persistently and calls it from `EmitEvent`.

---

## Event Reference

| JS Event | RPC | Data |
|----------|-----|------|
| `gameInit` | `InitGame` (139) | `{ playerId, hostname }` |
| `clientMessage` | `ClientMessage` (93) | `{ color, text }` (color codes stripped) |
| `chat` | `Chat` (101) | `{ playerId, text }` |
| `dialog` | `ScrDialogBox` (61) | `{ id, style, title, body, button1, button2 }` |
| `setPos` | `ScrSetPlayerPos` (12) | `{ x, y, z }` |
| `setHealth` | `ScrSetPlayerHealth` (14) | `{ health }` |
| `setArmor` | `ScrSetPlayerArmour` (66) | `{ armor }` |
| `setSkin` | `ScrSetPlayerSkin` (153) | `{ playerId, skinId }` |
| `requestClass` | `RequestClass` (128) | `{ outcome, skin, x, y, z }` |
| `playerJoin` | `ServerJoin` (137) | `{ playerId, name, isNpc }` |
| `playerQuit` | `ServerQuit` (138) | `{ playerId, reason }` |
| `rejected` | `ConnectionRejected` (130) | `{ reason }` |
| `spawnInfo` | `ScrSetSpawnInfo` (68) | *(not yet emitted — see TODO)* |

**Note:** `requestClass.outcome` — `1` = class accepted (bot can spawn), `0` = rejected.

---

## Outbound Actions (JS → Server)

| JS Method | RPC sent | Notes |
|-----------|----------|-------|
| `connect()` | `RPC_ClientJoin` | Loads XML, starts timer |
| `disconnect()` | *(RakNet disconnect)* | Stops timer, destroys interface |
| `sendChat(text)` | `RPC_Chat` | |
| `sendCommand(cmd)` | `RPC_ServerCommand` | Must include leading `/` |
| `respondDialog(id, btn, item, text)` | `RPC_DialogResponse` | |
| `spawn()` | `RPC_RequestSpawn` + `RPC_Spawn` | Also uses SpawnInfo from last RequestClass |
| `requestClass(classId)` | `RPC_RequestClass` | Triggers server OnPlayerRequestClass |

---

## Configuration (RakSAMPClient.xml)

The bot reads `RakSAMPClient.xml` from the working directory at `connect()` time. Key fields:

```xml
<RakSAMPClient manual_spawn="1" runmode="3" select_classid="0">
    <server nickname="TestBot" password="">127.0.0.1:7777</server>
    <normal_pos position="325.35 2512.09 16.56" rotation="0.0" force="0" />
</RakSAMPClient>
```

- `manual_spawn="1"` — don't auto-spawn; JS code controls spawn timing
- `runmode="3"` — normal mode (stays at position)
- `select_classid="0"` — send class 0 when selecting
- `normal_pos force="0"` — server can override position via SetSpawnInfo

---

## OMP Protocol Quirks (critical for test authors)

### Dialog response: listItem must be -1 for non-list dialogs

OMP validates dialog responses more strictly than SA-MP. For `DIALOG_STYLE_PASSWORD`, `DIALOG_STYLE_INPUT`, and `DIALOG_STYLE_MSGBOX`, the `listItem` field **must be -1**. Sending `0` causes OMP to **silently drop** the response — no error, no warning, no callback.

```js
// WRONG — OMP silently drops this:
bot.respondDialog(d.id, 1, 0, 'password');

// CORRECT:
bot.respondDialog(d.id, 1, -1, 'password');
```

For `DIALOG_STYLE_LIST`, `DIALOG_STYLE_TABLIST`, and `DIALOG_STYLE_TABLIST_HEADERS`, use the actual selected row index (0-based).

Source: `open.mp/Server/Components/Dialogs/dialog.cpp`, line ~160.

### RPC central logger

A global RPC logger exists in `RakPeer.cpp:HandleRPCPacket()` that logs every incoming RPC ID. Enable/disable by editing the `Log("[RPC-IN] ...")` call. Useful for diagnosing what the server sends.

---

## Spawn Protocol (SA-MP 0.3.7 / OMP)

Confirmed sequence after successful registration on our OMP server:

1. **Client connects** → sends `RPC_ClientJoin`
2. **Server** → sends `InitGame` (id=139) → bot gets `gameInit`
3. **Server** → sends `ScrDialogBox` (id=61, auth dialog) → bot gets `dialog`
4. **Bot responds** with `respondDialog(id, 1, -1, password)` — **listItem must be -1**
5. **Server** processes async (bcrypt hash → MySQL INSERT → login → data load)
6. **Server** sends back:
   - `ClientMessage` (id=93) — "Account created! You are now logged in."
   - `ResetPlayerWeapons` (id=20)
   - `HaveSomeMoney` (id=18)
   - `SetPlayerSkin` (id=153)
   - `ScrSetSpawnInfo` (id=68) — spawn position from DB
   - `RequestSpawn` (id=129) — server requesting client to spawn
7. **Bot** calls `spawn()` (sends `RPC_RequestSpawn` + `RPC_Spawn`) → server triggers `OnPlayerSpawn`
8. **Server** sends position, health updates → bot gets `setPos`, `setHealth`, etc.

---

## Active Bugs / Known Issues

### Sleep() in resetPools()

`netgame.cpp:resetPools(1, ...)` calls `Sleep(dwTimeReconnect)` — blocks the uv_timer thread for 2 seconds on disconnect/reconnect. Since we manage the connection lifecycle ourselves in JS (and never expect the reconnect path), this hasn't caused problems yet, but it will block the event loop if the server closes the connection unexpectedly.

---

## Debugging Tips

- **See all C++ logs**: Run with `node tests/test-debug.js` — Log() calls go to stdout
- **Check if RPC fires**: Add `Log("[RPC] HandlerName called")` at top of handler, rebuild
- **See server side**: OMP logs in `Server/logs/`, MySQL plugin in `Server/logs/plugins/mysql.log`
- **Verify DB state**: Connect to MySQL on port 3307 (`root`/`root`, database `samprpg`)
- **Race condition check**: Always register event listeners BEFORE calling `bot.connect()` — early events arrive synchronously on first tick

---

## Adding New Events

1. Find the RPC handler in `netrpc.cpp` (search for the SA-MP RPC ID in `SAMPRPC.h`)
2. Add EmitEvent call:
   ```cpp
   {
       Nan::HandleScope scope;
       v8::Local<v8::Object> data = Nan::New<v8::Object>();
       Nan::Set(data, Nan::New("fieldName").ToLocalChecked(), Nan::New(value));
       EmitEvent("eventName", data);
   }
   ```
3. Add to the event reference table above
4. `npx node-gyp rebuild`

---

## Adding New Outbound Actions

1. Find or implement the function in `misc_funcs.cpp` / `SAMPRPC.h`
2. Add a `NAN_METHOD` wrapper in `main.cpp`
3. Register it in `NAN_MODULE_INIT(Init)` with `Nan::Set`
4. Add a method to `RakClient` in `lib/rak-client.js`
5. Rebuild
