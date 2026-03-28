# mod_spawn — Spawn Management

Manages the spawn lifecycle: bypasses class selection after auth, blocks spawn for unauthenticated players, applies saved position and health/armor from mod_playerdata after spawning.

## Public API

| Function | Returns | Description |
|----------|---------|-------------|
| `Spawn_HasSpawned(playerid)` | `bool` | Whether player has spawned at least once this session |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_CONNECT` | Reset first-spawn flag |
| Subscribes | `EVT_PLAYER_REQUEST_CLASS` | Skip class selection — SetSpawnInfo + SpawnPlayer |
| Subscribes | `EVT_PLAYER_REQUEST_SPAWN` | Block spawn if not logged in or data not loaded |
| Subscribes | `EVT_PLAYER_SPAWN` | Apply pending health/armor, send welcome message |
| Subscribes | `EVT_PLAYER_DATA_LOADED` | Set spawn position from DB, trigger SpawnPlayer |

## Dependencies

- `mod_auth` — `Auth_IsLoggedIn()`
- `mod_playerdata` — `PData_IsLoaded()`, `PData_GetSavedPos()`, `PData_GetSavedHealth()`

## Flow

### First spawn (after login)

1. `EVT_PLAYER_DATA_LOADED` fires (from mod_playerdata)
2. `Spawn_OnDataLoaded`: reads saved position via `PData_GetSavedPos()`
3. Calls `SetSpawnInfo()` with the correct position
4. Calls `SpawnPlayer()` — triggers `OnPlayerSpawn`
5. `Spawn_OnPlayerSpawn`: applies pending health/armor (engine resets these during spawn), sends "Welcome!" message

### Respawn after death

OnPlayerSpawn fires again — health/armor are re-applied from the last saved values. The spawn position was already set by SetSpawnInfo.

### Guards

- `OnPlayerRequestClass`: if player is logged in and data is loaded, immediately sets spawn info and spawns (skips class selection UI)
- `OnPlayerRequestSpawn`: returns 0 (blocks) if `!Auth_IsLoggedIn() || !PData_IsLoaded()`

## Notes

- Health/armor are stored as "pending" values because the SA-MP engine resets them to 100/0 during SpawnPlayer. They're applied in the OnPlayerSpawn handler.
- The server sends `SetSpawnInfo` (RPC 68) twice: once before spawn, once after (for future respawns). This is normal.
- `Spawn_HasSpawned()` is used by mod_playerdata as a save guard — prevents saving 0,0,0 coordinates before the player has actually spawned in-world.
