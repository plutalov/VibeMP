# mod_admin — Admin System

Permission-based admin system with four levels. Loads admin level from DB on login. Provides moderation commands and map-click teleport for higher levels.

## Public API

| Function | Returns | Description |
|----------|---------|-------------|
| `Admin_GetLevel(playerid)` | `int` | Admin level 0-3 |
| `Admin_RequireLevel(playerid, level)` | `bool` | Check + auto-send error message if too low |

## Permission Levels

| Level | Role | Access |
|-------|------|--------|
| 0 | Player | No admin commands |
| 1 | Moderator | /kick, /freeze, /unfreeze |
| 2 | Admin | + /tp, /tphere, /setpos, /setskin, /veh, map-click teleport |
| 3 | Owner | + /setadmin, /ban |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_LOGIN` | Load admin_level from DB |
| Subscribes | `EVT_PLAYER_DISCONNECT` | Cleanup per-player state |
| Subscribes | `EVT_PLAYER_CLICK_MAP` | Teleport to clicked position (level 2+) |
| Subscribes | `EVT_PLAYER_HELP` | Print admin command list in /help |

## Commands

| Command | Level | Usage | Description |
|---------|-------|-------|-------------|
| `/kick` | 1 | `/kick [id] [reason]` | Kick player |
| `/freeze` | 1 | `/freeze [id]` | Toggle player controllable off |
| `/unfreeze` | 1 | `/unfreeze [id]` | Toggle player controllable on |
| `/tp` | 2 | `/tp [id]` | Teleport to player |
| `/tphere` | 2 | `/tphere [id]` | Teleport player to you |
| `/setpos` | 2 | `/setpos [id] x y z` | Set player position |
| `/setskin` | 2 | `/setskin [id] skinid` | Set player skin |
| `/veh` | 2 | `/veh modelid [c1] [c2]` | Spawn vehicle at your position |
| `/setadmin` | 3 | `/setadmin [id] [level]` | Set player's admin level (persists to DB) |
| `/ban` | 3 | `/ban [id] [reason]` | Ban player |
| `/gmx` | 3 | `/gmx` | Restart gamemode (GameModeExit — triggers all Destroy callbacks with blocking saves) |

## Dependencies

- `mod_db` — `DB_GetHandle()`
- `mod_auth` — `Auth_IsLoggedIn()`, `Auth_GetAccountId()`

## Database

**Table:** `accounts`

- Load: `SELECT admin_level FROM accounts WHERE id = ?`
- Update: `UPDATE accounts SET admin_level = ? WHERE id = ?` (via /setadmin)

## Map-Click Teleport

Level 2+ admins can click the map (pause menu) to teleport. If in a vehicle, the vehicle moves too. Uses `EVT_PLAYER_CLICK_MAP` which provides x, y, z via event data floats.
