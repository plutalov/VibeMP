# mod_auth — Authentication

MySQL-backed registration and login with bcrypt password hashing. Shows dialogs on connect, processes responses asynchronously, and emits login/logout events for other modules to react to.

## Public API

| Function | Returns | Description |
|----------|---------|-------------|
| `Auth_IsLoggedIn(playerid)` | `bool` | Whether player has authenticated |
| `Auth_GetAccountId(playerid)` | `int` | Database row ID (-1 if not logged in) |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_CONNECT` | Check if account exists, show register/login dialog |
| Subscribes | `EVT_PLAYER_DISCONNECT` | Emit logout if was logged in, cleanup state |
| Subscribes | `EVT_DIALOG_RESPONSE` | Process register/login dialog input |
| Emits | `EVT_PLAYER_REGISTER` | After successful registration (account created) |
| Emits | `EVT_PLAYER_LOGIN` | After successful login (password verified) |
| Emits | `EVT_PLAYER_LOGOUT` | On disconnect if player was logged in |

## Dependencies

- `mod_db` — `DB_GetHandle()`, `DB_IsConnected()`

## Database

**Table:** `accounts`

- Registration: `INSERT INTO accounts (username, password)`
- Login check: `SELECT id FROM accounts WHERE username = ?`
- Password fetch: `SELECT password FROM accounts WHERE id = ?`
- Last login update: `UPDATE accounts SET last_login = NOW() WHERE id = ?`

## Dialog IDs

| ID | Constant | Purpose |
|----|----------|---------|
| 1 | `DIALOG_LOGIN` | Password input for existing accounts |
| 2 | `DIALOG_REGISTER` | Password input for new accounts |

## Flow

1. Player connects → `EVT_PLAYER_CONNECT` → async MySQL check for username
2. Account exists → show login dialog (ID 1)
3. Account doesn't exist → show register dialog (ID 2)
4. **Register path:** dialog response → bcrypt hash (async) → INSERT → emit `EVT_PLAYER_REGISTER` + `EVT_PLAYER_LOGIN`
5. **Login path:** dialog response → fetch hash → bcrypt verify (async) → emit `EVT_PLAYER_LOGIN`
6. Wrong password → increment attempts → re-show dialog (max 3 attempts)
7. Player disconnects while logged in → emit `EVT_PLAYER_LOGOUT`

## Notes

- Bcrypt cost factor: 12 (defined as `BCRYPT_COST`)
- Max login attempts: 3 (then kicked)
- Password length: 4-32 characters
- All async callbacks check `IsPlayerConnected()` first
