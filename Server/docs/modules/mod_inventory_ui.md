# mod_inventory_ui — Dual-Panel Inventory UI

Renders one or two 4x5 grids with 3D model previews. Context menu (Use/Move/Drop/Cancel) on item click. Move mode for dragging items between containers.

## Layout Modes

| Mode | Panels | Trigger |
|------|--------|---------|
| Single | Player inventory only | `/inv` |
| Dual | Player inventory (left) + external container (right) | `/trunk` or any `InvUI_OpenDual` call |

## Interaction State Machine

Layout and state are independent — the state machine works identically with one or two panels.

```
IDLE → click occupied slot → CONTEXT_MENU (slots disabled)
CONTEXT_MENU → Use  → emit EVT_INV_ITEM_USE → IDLE
CONTEXT_MENU → Move → MOVE_MODE (slots re-enabled, source highlighted)
CONTEXT_MENU → Drop → remove item → IDLE
CONTEXT_MENU → Cancel / ESC / click elsewhere → IDLE
MOVE_MODE → click destination slot → Inv_MoveItem → IDLE
MOVE_MODE → ESC → IDLE
IDLE → ESC / X → close UI
```

Slots are made non-selectable during CONTEXT_MENU state via hide/SetSelectable/show cycle, preventing accidental slot clicks while the menu is visible.

## Public API

| Function | Description |
|----------|-------------|
| `InvUI_Open(playerid, containerIdx)` | Open single-panel (defaults to player's container) |
| `InvUI_OpenDual(playerid, containerIdx, title[])` | Open dual-panel (left=player, right=external) |
| `InvUI_Close(playerid)` | Close UI, cleanup state |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_CLICK_PTEXTDRAW` | State machine click handler |
| Subscribes | `EVT_PLAYER_CONNECT` | Reset UI state |
| Subscribes | `EVT_PLAYER_DISCONNECT` | Cleanup |
| Subscribes | `EVT_INV_ITEM_ADDED` | Refresh if open |
| Subscribes | `EVT_INV_ITEM_REMOVED` | Refresh if open |
| Subscribes | `EVT_INV_ITEM_MOVED` | Refresh if open |

## Context Menu

| Button | Color | Action |
|--------|-------|--------|
| Use | Green | Emits EVT_INV_ITEM_USE |
| Move | Yellow | Enters MOVE_MODE |
| Drop | Red | Removes item entirely |
| Cancel | Gray | Closes menu |

## Textdraw Budget

95 of 256 per-player textdraws:

| Component | Left | Right | Shared | Total |
|-----------|------|-------|--------|-------|
| Background | 1 | 1 | | 2 |
| Title | 1 | 1 | | 2 |
| Weight | 1 | 1 | | 2 |
| Close buttons | | | 2 | 2 |
| Slots (model) | 20 | 20 | | 40 |
| Qty labels | 20 | 20 | | 40 |
| Context menu | | | 5 | 5 |
| **Total** | | | | **93** |

Leaves 163 for other UI systems. Right panel created but hidden in single mode.

## Dependencies

- `mod_inventory` — all data queries and item operations

## Notes

- **Two close buttons:** One for single mode (left panel edge), one for dual mode (right panel edge). Textdraws can't be repositioned, so both are created and the correct one is shown.
- **Slot selectability toggle:** On context menu open, all slots are hidden, set non-selectable, re-shown. On close, reversed. This prevents slot clicks from interfering with menu buttons.
- **Empty slots:** Display model 19300 (invisible object) with dark background color.
