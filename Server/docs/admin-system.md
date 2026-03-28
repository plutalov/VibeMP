# Admin System

## Overview

The admin system (`mod_admin.inc`) provides moderation and development tools. Admin levels are stored in the `accounts` table and loaded from the database on login.

## Admin Levels

| Level | Role | Description |
|-------|------|-------------|
| 0 | Player | No admin privileges |
| 1 | Moderator | Basic moderation (kick, freeze) |
| 2 | Admin | Full admin (teleport, vehicles, skins, positions) |
| 3 | Owner | Server management (set admin levels, ban) |

## Setting Admin Level

Admin levels are stored in the database. To set a player's level:

**In-game (requires level 3):**
```
/setadmin [playerid] [level]
```

**Via MySQL (for first admin):**
```sql
UPDATE accounts SET admin_level = 3 WHERE username = 'YourName';
```

## Commands

### Level 1 — Moderator

| Command | Usage | Description |
|---------|-------|-------------|
| `/kick` | `/kick [id] [reason]` | Kick a player with optional reason |
| `/freeze` | `/freeze [id]` | Freeze a player in place |
| `/unfreeze` | `/unfreeze [id]` | Unfreeze a player |

### Level 2 — Admin

| Command | Usage | Description |
|---------|-------|-------------|
| `/tp` | `/tp [id]` | Teleport yourself to a player |
| `/tphere` | `/tphere [id]` | Teleport a player to you |
| `/setpos` | `/setpos [x] [y] [z]` | Set your own position |
| `/setpos` | `/setpos [id] [x] [y] [z]` | Set another player's position |
| `/setskin` | `/setskin [skinid]` | Set your own skin (0-311) |
| `/setskin` | `/setskin [id] [skinid]` | Set another player's skin |
| `/veh` | `/veh [modelid] [color1] [color2]` | Spawn a vehicle and enter it (model 400-611) |
| Map click | Right-click pause menu map | Teleport to clicked location (moves vehicle too) |

### Level 3 — Owner

| Command | Usage | Description |
|---------|-------|-------------|
| `/setadmin` | `/setadmin [id] [level]` | Set a player's admin level (0-3), persisted to DB |
| `/ban` | `/ban [id] [reason]` | Ban a player |

## How It Works

### Architecture

- **State:** `g_AdminLevel[MAX_PLAYERS]` — in-memory, loaded from DB on login
- **Loading:** Subscribes to `EVT_PLAYER_LOGIN`, queries `accounts.admin_level` asynchronously
- **Cleanup:** Subscribes to `EVT_PLAYER_DISCONNECT`, resets level to 0
- **Map teleport:** Subscribes to `EVT_PLAYER_CLICK_MAP`, teleports if level >= 2

### Public API

Other modules can check admin status:

```pawn
// Get the raw level (0-3)
new level = Admin_GetLevel(playerid);

// Check permission with automatic error message — use in cmd_* handlers
if (!Admin_RequireLevel(playerid, 2)) return 1;
```

### Help Integration

mod_admin subscribes to `EVT_PLAYER_HELP` and prints commands appropriate to the player's level. When a player types `/help`, they only see commands they have access to.

## Adding New Admin Commands

1. Define the command in `mod_admin.inc`:
```pawn
forward cmd_mycommand(playerid, params[]);
public cmd_mycommand(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 2)) return 1;
    // command logic
    return 1;
}
```

2. Add it to `Admin_OnPlayerHelp` so it shows in `/help`

3. That's it — the command processor routes it automatically
