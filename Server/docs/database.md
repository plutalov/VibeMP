# Database Guide

## Overview

The server uses **MySQL** for persistent data storage and **bcrypt** for password hashing. Both run as SA-MP legacy plugins loaded by OMP.

- **MySQL plugin**: pBlueG/SA-MP-MySQL R41
- **Bcrypt plugin**: Sreyas-Sreelal/samp-bcrypt v2.2.5

## Dev Setup

### Prerequisites

- Docker Desktop (with WSL2 backend on Windows)
- Plugin DLLs in `Server/plugins/` (installed via sampctl)

### Start MySQL

```bash
# From Server/ directory, via WSL:
wsl docker compose up -d mysql

# Run migrations:
wsl docker compose up flyway
```

Flyway runs all pending migrations from `Server/sql/` and exits. MySQL stays running on port 3306.

### Connection Details (dev)

| Setting  | Value      |
|----------|------------|
| Host     | 127.0.0.1  |
| Port     | 3306       |
| Database | samp_rpg   |
| User     | samp       |
| Password | samppass   |

These are hardcoded in `mod_db.inc` for now.

## Flyway Migrations

Schema changes use **Flyway** with versioned SQL files.

### Convention

Files in `Server/sql/` follow the naming pattern:

```
V<number>__<description>.sql
```

- `V1__create_accounts.sql` — first migration
- `V2__add_vehicles.sql` — next migration
- etc.

**Two underscores** between the version number and description.

### Adding a Migration

1. Create a new file: `Server/sql/V<next>__<description>.sql`
2. Write your SQL (CREATE TABLE, ALTER TABLE, etc.)
3. Run: `wsl docker compose up flyway`
4. Flyway applies only the new migration

### Rules

- **Never edit an existing migration** — Flyway checksums them. If you change a file after it ran, Flyway errors out.
- **Never reuse a version number** — each migration gets a unique V number.
- Flyway tracks applied migrations in `flyway_schema_history` table.

### Reset (nuke and rebuild)

```bash
wsl docker compose down -v   # destroys MySQL volume
wsl docker compose up -d     # recreates everything from scratch
```

## Schema

### accounts (V1)

| Column     | Type         | Default       | Notes                |
|------------|-------------|---------------|----------------------|
| id         | INT PK AUTO | —             | Account ID           |
| username   | VARCHAR(24) | —             | UNIQUE, player name  |
| password   | VARCHAR(72) | —             | bcrypt hash          |
| score      | INT         | 0             |                      |
| money      | INT         | 0             |                      |
| skin       | INT         | 0             |                      |
| pos_x      | FLOAT       | -2233.97      | Mount Chiliad        |
| pos_y      | FLOAT       | -1737.58      |                      |
| pos_z      | FLOAT       | 480.55        |                      |
| pos_a      | FLOAT       | 0.0           | Facing angle         |
| health     | FLOAT       | 100.0         |                      |
| armor      | FLOAT       | 0.0           |                      |
| interior   | INT         | 0             |                      |
| vworld     | INT         | 0             | Virtual world        |
| created_at | DATETIME    | NOW()         |                      |
| admin_level| INT         | 0             | 0=player,1=mod,2=admin,3=owner |
| last_login | DATETIME    | NULL          |                      |

### item_templates (V3)

| Column           | Type         | Default | Notes |
|------------------|-------------|---------|-------|
| id               | INT PK AUTO | —       | Template ID |
| name             | VARCHAR(32) | —       | Display name |
| category         | VARCHAR(16) | —       | Used by consumers to filter ("medical", "electronic", etc.) |
| weight           | FLOAT       | 1.0     | Weight per unit |
| max_stack        | INT         | 1       | 1 = unique (instanced), >1 = stackable |
| model_id         | INT         | 0       | SA-MP object model for 3D preview |
| default_metadata | VARCHAR(128)| ''      | Default metadata for new instances |

**Seed data:** Bandage (medical, stackable ×20, model 11738), Phone (electronic, unique, model 18868)

### containers (V3)

| Column     | Type         | Default | Notes |
|------------|-------------|---------|-------|
| id         | INT PK AUTO | —       | Container ID |
| owner_type | ENUM        | —       | 'player', 'vehicle', 'property', 'world' |
| owner_id   | INT         | —       | FK to accounts.id / vehicle id / etc. |
| max_weight | FLOAT       | 30.0    | Max carry weight |
| created_at | DATETIME    | NOW()   | |

### container_items (V3)

| Column       | Type         | Default | Notes |
|--------------|-------------|---------|-------|
| id           | INT PK AUTO | —       | Instance ID |
| container_id | INT FK      | —       | FK to containers.id (CASCADE delete) |
| slot         | INT         | -1      | Grid position (0-19), -1 = auto-assign |
| template_id  | INT FK      | —       | FK to item_templates.id |
| quantity     | INT         | 1       | Stack count |
| metadata     | VARCHAR(128)| ''      | Instance-specific state ("lock_id=47", "ammo=12") |

**Stacking rule:** Items with empty metadata stack up to `max_stack`. Items with metadata are always unique (own slot).

### vehicles (V4)

| Column | Type | Default | Notes |
|--------|------|---------|-------|
| id | INT PK AUTO | — | Persistent vehicle ID |
| model_id | INT | — | SA-MP vehicle model (400-611) |
| pos_x/y/z | FLOAT | 0.0 | World position |
| pos_a | FLOAT | 0.0 | Facing angle |
| color1/color2 | INT | -1 | Colors (-1 = random) |
| health | FLOAT | 1000.0 | Engine damage state |
| created_at | DATETIME | NOW() | |

**Used by:** `mod_vehicles` loads all rows on startup; `mod_veh_trunk` uses vehicles as container owners.

## Async Query Patterns

All MySQL queries are **threaded** — they don't block the server. You issue a query with a callback function name, and the plugin calls that function when the result is ready.

### Basic pattern

```pawn
// Issue query with callback
new query[128];
mysql_format(DB_GetHandle(), query, sizeof(query),
    "SELECT * FROM accounts WHERE id = %d", accountId);
mysql_tquery(DB_GetHandle(), query, "MyCallback", "d", playerid);

// Callback — called by MySQL plugin, NOT by event bus
forward MyCallback(playerid);
public MyCallback(playerid)
{
    // ALWAYS check player is still connected
    if (!IsPlayerConnected(playerid)) return;

    new rows;
    cache_get_row_count(rows);
    if (rows == 0) return;

    // Read data
    new value;
    cache_get_value_name_int(0, "column_name", value);

    // Notify other modules via event bus
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_MY_EVENT);
}
```

### Key rules

1. **Always check `IsPlayerConnected()`** in every async callback. The player may have disconnected while the query was in flight.
2. **MySQL callbacks are NOT event bus handlers.** They're called directly by the plugin. Use `EventBus_Emit()` inside them to notify other modules.
3. **Use `mysql_format` with `%e` for strings** — it escapes SQL injection automatically.
4. **Fire-and-forget for writes** — `mysql_tquery` with no callback is fine for UPDATE/INSERT when you don't need the result.

## Bcrypt Usage

### Hashing (registration)

```pawn
bcrypt_hash(password, 12, "OnHashComplete", "d", playerid);

forward OnHashComplete(playerid);
public OnHashComplete(playerid)
{
    if (!IsPlayerConnected(playerid)) return;
    new hash[BCRYPT_HASH_LENGTH + 1];
    bcrypt_get_hash(hash);
    // INSERT hash into DB
}
```

### Verifying (login)

```pawn
bcrypt_check(input_password, stored_hash, "OnCheckComplete", "d", playerid);

forward OnCheckComplete(playerid);
public OnCheckComplete(playerid)
{
    if (!IsPlayerConnected(playerid)) return;
    if (bcrypt_is_equal())
    {
        // Password matches
    }
}
```

### Cost factor

We use cost 12 (defined as `BCRYPT_COST` in mod_auth.inc). Higher = slower but more secure. 12 is a good balance for a game server.

## Auto-Save

`mod_playerdata` runs a 5-minute timer (`PData_AutoSave`) that saves all logged-in players' data to the database. This protects against data loss if the server crashes.

Data is also saved on:
- Player disconnect (via `EVT_PLAYER_LOGOUT`)
- Server shutdown (in `PData_Destroy()`)

## Troubleshooting

### "Database is not available"

MySQL isn't running or the server can't connect.

```bash
wsl docker compose ps          # Check if MySQL container is running
wsl docker compose logs mysql  # Check for errors
```

### "Failed to connect to MySQL" (errno 2019)

This is an SSL error from `libmariadb.dll`. Known causes:

1. **MySQL 8.0**: R41-4's bundled `libmariadb.dll` is too old for MySQL 8's TLS. Use **MySQL 5.7** with `--skip-ssl` (already configured in docker-compose.yml).
2. **Missing DLLs**: `sampctl install` does NOT download `libmariadb.dll` or `log-core.dll`. Download the full release ZIP from [pBlueG/SA-MP-MySQL releases](https://github.com/pBlueG/SA-MP-MySQL/releases/tag/R41-4) and copy `libmariadb.dll` and `log-core.dll` to `Server/`.
3. **Docker in WSL**: MySQL runs in WSL but the OMP server runs on Windows. Port 3306 forwards correctly, but MySQL 8's TLS handshake fails through this bridge. MySQL 5.7 with `--skip-ssl` works.

Also check:
- MySQL container is running on port 3306
- Credentials in `mod_db.inc` match `docker-compose.yml`

### Plugin not loading

Check server console on startup for:
```
[Info] Loading plugin: mysql
[Info] Loading plugin: samp_bcrypt
```

If missing, verify:
- DLLs exist in `Server/plugins/` (mysql.dll, samp_bcrypt.dll)
- `config.json` has `"legacy_plugins": ["mysql", "samp_bcrypt"]`

### Flyway migration failed

```bash
wsl docker compose logs flyway  # See the error
```

Common causes:
- Syntax error in SQL file
- Edited an already-applied migration (checksum mismatch)
- Version number conflict

### sampctl pawn-stdlib conflict

`samp-bcrypt` depends on `pawn-stdlib` which conflicts with OMP's includes in `qawno/include/`. We keep an empty `dependencies/pawn-stdlib/` stub directory to satisfy sampctl without actually using those files.

### Bcrypt plugin API differences

We use **Sreyas-Sreelal/samp-bcrypt** (not lassir/bcrypt-samp). The API is different from most community tutorials:
- `bcrypt_hash(playerid, callback, input, cost)` — playerid is first arg
- `bcrypt_verify(playerid, callback, input, hash)` — named `verify` not `check`
- Hash callback: `public callback(playerid, hashid)`
- Verify callback: `public callback(playerid, bool:success)` — success directly, no `bcrypt_is_equal()`
- Include: `samp_bcrypt.inc` (not `bcrypt.inc`)
