# mod_veh_trunk — Vehicle Trunk Bridge

Bridge module connecting vehicles to inventory. When a persistent vehicle is created, a trunk container is loaded/created. When destroyed, the trunk is saved and freed.

This is the canonical example of the **bridge pattern**: a small module that knows two domains (vehicles + inventory) and connects them via events.

## Pattern for New Container Types

```
mod_house_storage  → subscribes to house events → creates container → /safe command
mod_backpack       → subscribes to equip events → creates container → /backpack command
```

Each bridge module is self-contained. mod_inventory and mod_inventory_ui never change.

## Public API

None. Only the `/trunk` command and event handlers.

## Events

| Direction | Event | Action |
|-----------|-------|--------|
| Subscribes | `EVT_VEHICLE_CREATED` | Call `Inv_LoadContainerByOwner(OWNER_VEHICLE, vehDbId, 50.0)` |
| Subscribes | `EVT_VEHICLE_DESTROYED` | Call `Inv_SaveAndFreeContainer(containerIdx)` |
| Subscribes | `EVT_PLAYER_HELP` | Print "/trunk" in help list |

## Commands

| Command | Description |
|---------|-------------|
| `/trunk` | Find nearest persistent vehicle within 10m, open dual-panel inventory UI |

## Dependencies

- `mod_vehicles` — `Veh_GetDbId()` (read-only)
- `mod_inventory` — `Inv_LoadContainerByOwner()`, `Inv_FindContainerByOwner()`, `Inv_SaveAndFreeContainer()`
- `mod_inventory_ui` — `InvUI_OpenDual()`

## Constants

| Constant | Value |
|----------|-------|
| `VEH_TRUNK_MAX_WEIGHT` | 50.0 kg |
| `VEH_TRUNK_MAX_DISTANCE` | 10.0 meters |
