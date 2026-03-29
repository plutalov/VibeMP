# mod_veh_trunk — Vehicle Trunk Bridge

Bridge module connecting vehicles to inventory. Creates trunk containers when vehicles spawn, frees them on destroy. Owns `/trunk` command.

## Public API

None.

## Events

| Direction | Event | Action |
|-----------|-------|--------|
| Subscribes | `EVT_VEHICLE_CREATED` | `Inv_LoadContainerByOwner(OWNER_VEHICLE, vehDbId, 50.0)` |
| Subscribes | `EVT_VEHICLE_DESTROYED` | `Inv_SaveAndFreeContainer(containerIdx)` |
| Subscribes | `EVT_PLAYER_HELP` | Print "/trunk" |

## Commands

| Command | Description |
|---------|-------------|
| `/trunk` | Find nearest persistent vehicle within 10m, open dual-panel UI |

## Dependencies

- `mod_vehicles` — `Veh_GetDbId()` (read-only)
- `mod_inventory` — `Inv_LoadContainerByOwner()`, `Inv_FindContainerByOwner()`, `Inv_SaveAndFreeContainer()`
- `mod_inventory_ui` — `InvUI_OpenDual(playerid, leftIdx, rightIdx, title)`
- `mod_player_inv` — `PlayerInv_GetContainerIdx()`, `PlayerInv_IsLoaded()`

## Constants

| Constant | Value |
|----------|-------|
| `VEH_TRUNK_MAX_WEIGHT` | 50.0 kg |
| `VEH_TRUNK_MAX_DISTANCE` | 10.0 meters |
