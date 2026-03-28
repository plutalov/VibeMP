# mod_help — Help & Stats Commands

Player-facing commands for help and stats. The `/help` command uses the event bus — each module prints its own section.

## Public API

None.

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Emits | `EVT_PLAYER_HELP` | Broadcast from /help — each module subscribes to print its commands |

## Commands

| Command | Description |
|---------|-------------|
| `/help` | Show command list (broadcasts EVT_PLAYER_HELP, each module prints its section) |
| `/stats` | Show player score and money |

## Dependencies

- `mod_auth` — `Auth_IsLoggedIn()` (all commands require login)

## Notes

- `/help` doesn't maintain a central command list. It emits `EVT_PLAYER_HELP` and every module that has commands subscribes and prints its own list. To add commands to `/help`, subscribe to `EVT_PLAYER_HELP` in your module's Init.
