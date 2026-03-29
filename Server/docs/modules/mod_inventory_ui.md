# mod_inventory_ui — Dual-Panel Inventory UI

Renders one or two 4x5 grids with 3D model previews. Context menu (Use/Move/Drop/Cancel) on item click. Move mode for dragging items between containers.

Owner-agnostic — doesn't know what a container belongs to. Callers provide container indices.

## Public API

| Function | Description |
|----------|-------------|
| `InvUI_Open(playerid, containerIdx)` | Open single-panel for given container |
| `InvUI_OpenDual(playerid, leftContainerIdx, rightContainerIdx, title[])` | Open dual-panel with custom title for right panel |
| `InvUI_Close(playerid)` | Close UI |

## Interaction State Machine

Layout (one/two panels) and state are independent.

```
IDLE → click occupied slot → CONTEXT_MENU (slots disabled)
CONTEXT_MENU → Use  → Inv_UseItemFromContainer → IDLE
CONTEXT_MENU → Move → MOVE_MODE (slots re-enabled, source highlighted)
CONTEXT_MENU → Drop → Inv_RemoveItem → IDLE
CONTEXT_MENU → Cancel / ESC / click elsewhere → IDLE
MOVE_MODE → click destination → Inv_MoveItem → IDLE
MOVE_MODE → ESC → IDLE
IDLE → ESC / X → close
```

Slots are non-selectable during CONTEXT_MENU state.

## Context Menu

| Button | Color | Action |
|--------|-------|--------|
| Use | Green | Emits EVT_INV_ITEM_USE with container in EVD_EXTRA |
| Move | Yellow | Enters MOVE_MODE |
| Drop | Red | Removes item |
| Cancel | Gray | Closes menu |

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_PLAYER_CLICK_PTEXTDRAW` | Click handler |
| Subscribes | `EVT_PLAYER_CONNECT` | Reset state |
| Subscribes | `EVT_PLAYER_DISCONNECT` | Cleanup |
| Subscribes | `EVT_INV_ITEM_ADDED/REMOVED/MOVED` | Refresh if open |

## Dependencies

- `mod_inventory` — data queries and `Inv_UseItemFromContainer`, `Inv_MoveItem`, `Inv_RemoveItem`

No player, vehicle, or auth dependencies.

## Textdraw Budget

93 of 256 per-player. Leaves 163 for other UI systems.
