# mod_vehicles — Persistent Vehicle Lifecycle

Manages creation, loading, and persistence of SA-MP vehicles. Loads all vehicles from the DB on server startup and creates them as SA-MP vehicle objects. Position and health are saved periodically (5-minute auto-save) and on shutdown.

This module knows nothing about inventory, containers, or trunks. Other modules (like `mod_veh_trunk`) react to vehicle events to attach their own behavior.

## Public API

| Function | Returns | Description |
|----------|---------|-------------|
| `Veh_GetDbId(vehicleid)` | `int` | Map SA-MP vehicle ID to persistent DB id (-1 if transient) |
| `Veh_GetSAMPId(vehDbId)` | `int` | Map DB id to current SA-MP vehicle ID (-1 if not loaded) |

## Events

| Direction | Event | Data slots |
|-----------|-------|------------|
| Emits | `EVT_VEHICLE_CREATED` | vehicleid(1), vehDbId(2) |
| Emits | `EVT_VEHICLE_DESTROYED` | vehicleid(1), vehDbId(2) |
| Subscribes | `EVT_PLAYER_HELP` | Prints /vehcreate for admins |

## Commands

| Command | Level | Usage | Description |
|---------|-------|-------|-------------|
| `/vehcreate` | 2 | `/vehcreate <modelid> [color1] [color2]` | Create persistent vehicle at player position |

## Dependencies

- `mod_db` — `DB_GetHandle()`, `DB_IsConnected()`
- `mod_admin` — `Admin_RequireLevel()`, `Admin_GetLevel()`

## Database

**Table:** `vehicles` (V4 migration)

| Column | Type | Default | Notes |
|--------|------|---------|-------|
| id | INT PK AUTO | — | Persistent vehicle ID |
| model_id | INT | — | SA-MP vehicle model (400-611) |
| pos_x/y/z | FLOAT | 0.0 | World position |
| pos_a | FLOAT | 0.0 | Facing angle |
| color1/color2 | INT | -1 | Colors (-1 = random) |
| health | FLOAT | 1000.0 | Damage state |
| created_at | DATETIME | NOW() | |

## Auto-Save

5-minute timer saves all vehicles' position and health. Also saves on server shutdown.

## Memory Limits

| Constant | Value |
|----------|-------|
| `MAX_PERSISTENT_VEHICLES` | 200 |
| `MAX_SAMP_VEHICLES` | 2000 |
