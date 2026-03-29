# mod_inventory_ui — Inventory Visual Grid

Renders a 4×5 textdraw grid with 3D item model previews. Click a slot to use the item. ESC or X to close.

## Public API

| Function | Description |
|----------|-------------|
| `InvUI_Open(playerid)` | Show the inventory grid |
| `InvUI_Close(playerid)` | Hide the inventory grid |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_CLICK_PTEXTDRAW` | Handles slot clicks, close button, ESC |
| Subscribes | `EVT_PLAYER_CONNECT` | Reset UI state on connect |
| Subscribes | `EVT_PLAYER_DISCONNECT` | Cleanup (textdraws auto-destroyed) |
| Subscribes | `EVT_INV_ITEM_ADDED` | Refresh UI if open |
| Subscribes | `EVT_INV_ITEM_REMOVED` | Refresh UI if open |

## Dependencies

- `mod_inventory` — all data queries
- `mod_auth` — `Auth_IsLoggedIn()` (via mod_inventory)

## Textdraw Budget

44 of 256 per-player textdraws:
- 1 background panel
- 1 title ("INVENTORY")
- 1 weight display
- 1 close button (X)
- 20 slot backgrounds (dark cells for grid structure)
- 20 slot models (3D preview, only shown for occupied slots)
- 20 quantity labels (bottom-right of slot, only shown when qty > 1)

Leaves 212 textdraws for other UI systems.

## Layout

- Grid: 4 columns × 5 rows, each cell 55×55px, 4px padding
- Position: starts at (320, 120) — right side of screen
- Empty slots show dark background only (no model)
- Occupied slots show 3D model via `TEXT_DRAW_FONT_MODEL_PREVIEW`
- Click detection via `SelectTextDraw` + `OnPlayerClickPlayerTextDraw`

## Notes

- Textdraws are created once per player on first `/inv`, reused on subsequent opens
- On reconnect, `InvUI_OnConnect` resets UI state and cancels any lingering textdraw selection
- ESC close is handled via `OnPlayerClickTextDraw` (global) with `INVALID_TEXT_DRAW`, forwarded to the same event handler
