# Debugging Guide

This guide catalogs every feedback channel available for debugging the server, the bot client, and the protocol between them.

---

## 1. Client-side RPC logging (RakClient)

**Location:** `RakClient/src/RakPeer.cpp`

Both incoming and outgoing RPCs are logged unconditionally to stdout (captured by Node.js). Every RPC that crosses the wire is visible.

**Incoming (server to client):**
```
[RPC-IN] id=61  bits=657  registered=yes     ← ScrDialogBox, handler exists
[RPC-IN] id=72  bits=48   registered=NO      ← SetPlayerColor, no handler
```
- `id` = SA-MP RPC ID (see `RakClient/src/SAMP/SAMPRPC.cpp` for the full map)
- `bits` = payload size
- `registered=NO` means no C++ handler exists — the RPC arrives but is silently ignored

**Outgoing (client to server):**
```
[RPC-OUT] id=62  bits=136     ← DialogResponse
[RPC-OUT] id=50  bits=80      ← ServerCommand (/stats)
```

**How to read:** Run any test script (`node tests/test-debug.js`) and the RPC log appears interleaved with JS events in stdout. C++ `printf` output may be buffered — it appears after JS `console.log` output but the ordering within each stream is correct.

**RPC ID reference (common ones):**

| ID | Name | Direction |
|----|------|-----------|
| 25 | ClientJoin | OUT |
| 50 | ServerCommand | OUT |
| 52 | Spawn | OUT |
| 61 | ScrDialogBox | IN |
| 62 | DialogResponse | OUT |
| 66 | SetPlayerArmour | IN |
| 68 | SetSpawnInfo | IN |
| 72 | SetPlayerColor | IN |
| 93 | ClientMessage | IN |
| 128 | RequestClass | IN/OUT |
| 129 | RequestSpawn | IN/OUT |
| 137 | ServerJoin | IN |
| 139 | InitGame | IN |
| 153 | SetPlayerSkin | IN |
| 155 | UpdateScoresPingsIPs | IN/OUT |

Full list: `RakClient/src/SAMP/SAMPRPC.cpp`

---

## 2. Server-side RPC logging (RpcLogger component)

**Location:** `Server/src/RpcLogger/` (source), `Server/components/RpcLogger.dll` (deployed)

Custom OMP component that hooks `NetworkInEventHandler` and `NetworkOutEventHandler` to log every RPC and packet.

**Output (in server console and log.txt):**
```
[RPC-RECV] player=0  id=25   bits=536     ← client sent ClientJoin
[RPC-SEND] player=0  id=139  bits=2243    ← server sent InitGame
[RPC-RECV] player=0  id=62   bits=136     ← client sent DialogResponse
[RPC-SEND] player=0  id=93   bits=376     ← server sent ClientMessage
[PKT-RECV] player=0  id=207  bits=320     ← client sent sync data
```

**Build:**
```bash
cd Server/src/RpcLogger/build
cmake .. -G "Visual Studio 16 2019" -A Win32
cmake --build . --config Release
cp Release/RpcLogger.dll ../../components/
```

Requires `omp-src` submodule with SDK initialized: `git submodule update --init --recursive omp-src`.

**Enable/disable:** Remove `RpcLogger.dll` from `Server/components/` to disable. No config needed.

---

## 3. Server-side application logging

### OMP main log (`Server/log.txt`)

Contains:
- Component/plugin loading at startup
- Player connect/disconnect with IP and player ID
- RpcLogger output (if loaded)
- Pawn `print()` / `printf()` output from gamemode and modules
- Chat messages (if `log_chat: true` in config.json)

**Example:**
```
[2026-03-28T12:04:01+0100] [Info] [join] TestBot has joined the server (0:127.0.0.1)
[2026-03-28T12:04:01+0100] [Info] [PDATA] OnPlayerLogin: loading data for playerid=0
```

### OMP error/warning logs (`Server/logs/errors.log`, `Server/logs/warnings.log`)

Pawn runtime errors and OMP warnings. Check these first when callbacks silently fail.

### MySQL plugin log (`Server/logs/plugins/mysql.log`)

Extremely detailed — logs every query, callback execution, parameter binding, and result set. Set to `ALL` level by `mod_db.inc` calling `mysql_log(ALL)`.

**What to look for:**
- `query "..." successfully executed` — confirms the query ran
- `Executing callback '...'` — confirms the async callback fired
- `cache_get_row_count: return value: '0'` — shows what Pawn code sees
- `AMX callback executed with error '0'` — no Pawn errors in the callback

**Example flow for registration:**
```
mysql_tquery → "INSERT INTO accounts ..." → Auth_OnAcctCreated callback → cache_insert_id()
```
If you see the query but NOT the callback, the MySQL thread pool might be stalled.

### Event bus debug logging (runtime toggle)

**Enable:** `/debug` command in-game (requires login), or call `EventBus_SetDebug(true)` from Pawn.

**Output format:**
```
[EVT HH:MM:SS] <EventName> (id=X) player=Y depth=Z handlers=W
```

Shows every event dispatch with handler count. Useful for verifying the event chain fires correctly (e.g., EVT_PLAYER_LOGIN -> EVT_PLAYER_DATA_LOADED -> spawn).

### Module-level logging

| Module | What it logs | Level |
|--------|-------------|-------|
| mod_db | Connection success/failure, mysql_log(ALL) | Always |
| mod_auth | Init/destroy only | Minimal |
| mod_playerdata | Data load/save with position and health values | Detailed |
| mod_admin | Init/destroy only | Minimal |
| mod_spawn | Init/destroy only | Minimal |
| mod_debug | Init/destroy only | Minimal |

---

## 4. Source code references

### RakClient source (`RakClient/`)

Our fork of SyncroIT/RakClient-NodeGYP. The full SA-MP 0.3.7 client protocol implementation.

Key files for debugging:
- `src/RakPeer.cpp` — RPC send/receive, central packet logger
- `src/net/netrpc.cpp` — All server-to-client RPC handlers (40+)
- `src/net/netgame.cpp` — Non-RPC packet handlers (sync data, connection state)
- `src/main/misc_funcs.cpp` — All client-to-server actions (sendChat, respondDialog, spawn, etc.)
- `src/SAMP/SAMPRPC.cpp` — RPC ID constants (the Rosetta Stone for protocol debugging)
- `src/main/localplayer.cpp` — Sync data sending (onfoot, incar, aim, passenger)

### OMP server source (`omp-src/` submodule)

The open.mp server C++ source. Not our code — reference only.

Key files for debugging:
- `Shared/NetCode/dialog.hpp` — Dialog RPC format + response validation rules
- `Shared/NetCode/core.hpp` — Core RPC formats (ClientJoin, ServerCommand, Chat, etc.)
- `Shared/Network/bitstream.hpp` — readDynStr8/16/32, readINT16, etc. (data type definitions)
- `Server/Components/Dialogs/dialog.cpp` — Dialog response validation logic (listItem checks, dialog ID matching)
- `Server/Components/Pawn/Scripting/Dialog/Events.hpp` — How dialog events reach Pawn callbacks

**Init submodule:** `git submodule update --init omp-src`
**Init nested submodules (network types):** `cd omp-src && git submodule update --init Shared/Network SDK`

### Gamemode source (`Server/`)

Our Pawn code:
- `gamemodes/mygamemode.pwn` — Thin bridge: routes SA-MP callbacks to event bus
- `includes/core/eventbus.inc` — Event dispatch, cycle detection, debug toggle
- `includes/core/events.inc` — Event ID definitions
- `includes/core/commands.inc` — ZCMD-style command routing
- `includes/modules/mod_*.inc` — All game modules

---

## 5. Debugging recipes

### "Server ignores my RPC"

1. Check `[RPC-OUT]` log — is the RPC actually being sent?
2. Check the RPC ID — does it match what the OMP source expects? (`omp-src/Shared/NetCode/*.hpp`)
3. Check the payload format — field types must match exactly (WORD vs BYTE vs INT)
4. Check OMP's validation logic — e.g., dialog responses require `listItem=-1` for non-list dialogs
5. Check `Server/log.txt` — does OMP log any warnings about the RPC?

### "Async callback never fires"

1. Check `Server/logs/plugins/mysql.log` — did the query execute?
2. Look for `Executing callback 'CallbackName'` — did the plugin call it?
3. Look for `AMX callback executed with error '0'` — did Pawn crash inside it?
4. Enable event bus debug (`/debug`) — does the expected event dispatch?
5. Check `Server/logs/errors.log` — Pawn runtime errors go here

### "Bot connects but nothing happens"

1. Check `[RPC-IN]` for `id=139` (InitGame) — confirms successful join
2. Check for `id=61` (ScrDialogBox) or `id=128` (RequestClass) — server must send one of these
3. If neither arrives, check `Server/log.txt` for `[join]` message — did OMP accept the connection?
4. Check if MySQL is running — auth module falls back to "Database not available" kick

### "Bot spawns twice"

The server sends `ScrSetSpawnInfo` (id=68) twice in our spawn flow. Guard against double-spawn in JS:
```js
let spawned = false;
bot.on('spawnInfo', () => {
    if (!spawned) { spawned = true; bot.spawn(); }
});
```

### "Registration works from real client but not bot"

Compare the `[RPC-OUT]` payload between bot and real client. Common issues:
- `listItem` field value (must be -1 for password dialogs in OMP)
- String encoding differences
- Missing intermediate RPCs the real client sends

---

## 6. Config switches

### Server config (`Server/config.json`)

```json
"logging": {
    "enable": true,
    "log_chat": true,
    "log_connection_messages": true,
    "log_deaths": true,
    "log_queries": false    // ← set true for query-level logging
}
```

### MySQL plugin logging

Set in `mod_db.inc`:
```pawn
mysql_log(ALL);   // logs everything to logs/plugins/mysql.log
mysql_log(NONE);  // silence (production)
```

### Event bus debug

```pawn
EventBus_SetDebug(true);   // from Pawn
```
Or `/debug` command in-game.

### Client RPC logging

Both `[RPC-IN]` and `[RPC-OUT]` are always active. To disable, comment out the `Log(...)` calls in `RakClient/src/RakPeer.cpp`:
- Incoming: `HandleRPCPacket()` (~line 2880)
- Outgoing: `RPC()` (~line 1168)
