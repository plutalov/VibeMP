# mod_debug — Debug & Introspection

Developer tools for debugging and testing. Provides event bus diagnostics and module self-tests.

## Public API

None.

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_HELP` | Prints debug command list in /help output |

## Commands

| Command | Description |
|---------|-------------|
| `/debug` | Toggles event bus debug logging (shows every event dispatch) |
| `/events` | Dumps all registered event handlers to server console |
| `/test` | Runs module self-tests (assertions on EventBus, DB, Auth, PData) |

## Dependencies

- `mod_auth` — `Auth_IsLoggedIn()` (all commands require login)

## Notes

- `/debug` toggles `EventBus_SetDebug(true/false)` which prints `[EVT HH:MM:SS] <EventName> (id=X) player=Y depth=Z handlers=W` for every event dispatch.
- `/events` is a console-only dump — output goes to server log, not to the player's chat.
- `/test` runs quick in-process assertions. For E2E testing, use the RakClient bot framework instead.
