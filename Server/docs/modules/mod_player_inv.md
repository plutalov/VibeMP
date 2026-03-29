# mod_player_inv — Player ↔ Inventory Bridge

Manages the player's personal inventory container: creates/loads on login, saves on logout, frees on disconnect. Provides player-specific convenience wrappers and owns player commands.

Follows the same bridge pattern as mod_veh_trunk.

## Public API

| Function | Returns | Description |
|----------|---------|-------------|
| `PlayerInv_GetContainerIdx(playerid)` | `int` | Container index for player (-1 if not loaded) |
| `PlayerInv_IsLoaded(playerid)` | `bool` | Whether inventory is loaded |
| `PlayerInv_AddItem(playerid, templateIdx, qty, metadata[])` | `int` | Add item (slot on success, -1 on failure) |
| `PlayerInv_RemoveItem(playerid, slotIdx, qty)` | `int` | Remove item (1 on success, 0 on failure) |
| `PlayerInv_UseItem(playerid, slotIdx)` | `int` | Use item from player's container |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_LOGIN` | Load/create player container from DB |
| Subscribes | `EVT_PLAYER_LOGOUT` | Save player container (async) |
| Subscribes | `EVT_PLAYER_DISCONNECT` | Save and free container |
| Subscribes | `EVT_PLAYER_HELP` | Print commands in /help |
| Emits | `EVT_INV_PLAYER_LOADED` | playerid(0) — container ready |

## Commands

| Command | Level | Description |
|---------|-------|-------------|
| `/inv` | 0 | Open player inventory UI |
| `/giveitem <id> [qty]` | 2 | Add item by template DB id (admin) |
| `/useitem <slot>` | 0 | Use item in slot |

## Dependencies

- `mod_inventory` — container/item API
- `mod_inventory_ui` — `InvUI_Open()`
- `mod_auth` — `Auth_IsLoggedIn()`, `Auth_GetAccountId()`
- `mod_db` — async queries

## Constants

| Constant | Value |
|----------|-------|
| `PLAYERINV_DEFAULT_WEIGHT` | 30.0 kg |
