# mod_db — Database Connection

Owns the MySQL connection handle. All other modules access the database through `DB_GetHandle()`.

## Public API

| Function | Returns | Description |
|----------|---------|-------------|
| `DB_GetHandle()` | `MySQL:` | The active MySQL connection handle |
| `DB_IsConnected()` | `bool` | Whether the connection is alive |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Emits | `EVT_DB_CONNECTED` | Fired after successful MySQL connection |

## Dependencies

None — this is the foundation module. Must init first.

## Init Order

First. All other modules depend on this.

## Notes

- Connection details are hardcoded in the module (host, user, password, database)
- Calls `mysql_log(ALL)` to enable verbose plugin logging to `logs/plugins/mysql.log`
- Auto-reconnect is enabled via `mysql_option(handle, AUTO_RECONNECT, true)`
