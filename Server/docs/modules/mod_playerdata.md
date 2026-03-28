# mod_playerdata — Player Data Persistence

Loads and saves player state (position, health, armor, score, money, skin, interior, virtual world) from the database. Loads after login, saves on disconnect, and runs a periodic auto-save timer.

## Public API

| Function | Returns | Description |
|----------|---------|-------------|
| `PData_IsLoaded(playerid)` | `bool` | Whether data has been loaded from DB |
| `PData_GetSavedPos(playerid, &x, &y, &z, &a)` | void | Get cached position (from DB, not live) |
| `PData_GetSavedHealth(playerid, &hp, &arm)` | void | Get cached health/armor (from DB) |
| `PData_Save(playerid)` | void | Force immediate save to DB |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_LOGIN` | Triggers data load from DB |
| Subscribes | `EVT_PLAYER_LOGOUT` | Saves data to DB |
| Subscribes | `EVT_PLAYER_DISCONNECT` | Cleanup per-player state |
| Emits | `EVT_PLAYER_DATA_LOADED` | After data loaded and cached — mod_spawn listens for this |

## Dependencies

- `mod_db` — `DB_GetHandle()`
- `mod_auth` — `Auth_GetAccountId()`, `Auth_IsLoggedIn()`
- `mod_spawn` — `Spawn_HasSpawned()` (guards save against writing 0,0,0 before first spawn)

## Database

**Table:** `accounts`

- Load: `SELECT score, money, skin, pos_x, pos_y, pos_z, pos_a, health, armor, interior, vworld WHERE id = ?`
- Save: `UPDATE accounts SET score=?, money=?, skin=?, pos_x=?, ... WHERE id = ?`

## Auto-Save

A repeating timer runs every 5 minutes, iterating all connected players and saving those who are logged in, have loaded data, and have spawned.

Data is also saved on:
- Player disconnect (`EVT_PLAYER_LOGOUT`)
- Server shutdown (`PData_Destroy()`)

## Save Guards

`PData_Save()` checks three conditions before writing to DB:
1. `Auth_IsLoggedIn(playerid)` — must be authenticated
2. `PData_IsLoaded(playerid)` — must have loaded data (prevents saving defaults)
3. `Spawn_HasSpawned(playerid)` — must have spawned (prevents saving 0,0,0 position)
