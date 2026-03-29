# mod_inventory — Generic Container & Item Data Layer

Owner-agnostic container and item system. Does not know what owns a container (player, vehicle, property). Bridge modules (mod_player_inv, mod_veh_trunk) handle domain-specific mapping.

## Public API

### Templates

| Function | Returns | Description |
|----------|---------|-------------|
| `Inv_GetTemplateCount()` | `int` | Number of loaded templates |
| `Inv_GetTemplateName(idx, dest[], maxlen)` | void | Copy template name |
| `Inv_GetTemplateCategory(idx, dest[], maxlen)` | void | Copy template category |
| `Inv_GetTemplateWeight(idx)` | `Float` | Weight per unit |
| `Inv_GetTemplateMaxStack(idx)` | `int` | Max stack size (1 = unique) |
| `Inv_GetTemplateModelId(idx)` | `int` | SA-MP object model for 3D preview |
| `Inv_FindTemplateByDbId(dbId)` | `int` | Template index from DB id (-1 if not found) |
| `Inv_FindTemplateByName(name[])` | `int` | Template index from name (-1 if not found) |

### Container Lifecycle (for bridge modules)

| Function | Returns | Description |
|----------|---------|-------------|
| `Inv_AllocContainer()` | `int` | Allocate free memory slot (-1 if full) |
| `Inv_InitContainer(idx, dbId, ownerType, ownerId, maxWeight)` | void | Initialize slot with known values |
| `Inv_SetContainerFromCache(idx, row, ownerType, ownerId)` | void | Populate slot from MySQL cache |
| `Inv_LoadItemsFromCache(containerIdx)` | void | Load items from MySQL cache into slots |
| `Inv_GetContainerDbId(containerIdx)` | `int` | Get DB id of a container |
| `Inv_FindContainerByOwner(ownerType, ownerId)` | `int` | Look up loaded container by owner |
| `Inv_LoadContainerByOwner(ownerType, ownerId, maxWeight)` | void | Async load/create from DB |
| `Inv_SaveAndFreeContainer(containerIdx, blocking)` | void | Save + release memory |

### Container Queries

| Function | Returns | Description |
|----------|---------|-------------|
| `Inv_GetContainerWeight(containerIdx)` | `Float` | Current total weight |
| `Inv_GetContainerMaxWeight(containerIdx)` | `Float` | Max capacity |

### Items

| Function | Returns | Description |
|----------|---------|-------------|
| `Inv_AddItem(containerIdx, templateIdx, qty, metadata[])` | `int` | Slot on success, -1 on failure |
| `Inv_RemoveItem(containerIdx, slotIdx, qty)` | `int` | 1 on success, 0 on failure |
| `Inv_MoveItem(srcContainer, srcSlot, dstContainer, dstSlot)` | `int` | Move/swap with weight checks |
| `Inv_UseItemFromContainer(playerid, containerIdx, slotIdx)` | `int` | Emit EVT_INV_ITEM_USE with container in EVD_EXTRA |
| `Inv_GetItemTemplateIdx(containerIdx, slotIdx)` | `int` | Template index (-1 if empty) |
| `Inv_GetItemQuantity(containerIdx, slotIdx)` | `int` | Quantity in slot |
| `Inv_GetItemMetadata(containerIdx, slotIdx, dest[], maxlen)` | void | Copy metadata string |
| `Inv_IsSlotActive(containerIdx, slotIdx)` | `bool` | Whether slot has an item |
| `Inv_Save(containerIdx, blocking)` | void | Save dirty items (blocking=true for shutdown) |

## Events

| Direction | Event | Data slots |
|-----------|-------|------------|
| Subscribes | `EVT_DB_CONNECTED` | — |
| Emits | `EVT_INV_TEMPLATES_LOADED` | — |
| Emits | `EVT_INV_ITEM_ADDED` | playerid(0), containerIdx(1), templateIdx(2), quantity(3) |
| Emits | `EVT_INV_ITEM_REMOVED` | playerid(0), containerIdx(1), templateIdx(2), quantity(3) |
| Emits | `EVT_INV_ITEM_MOVED` | playerid(0), srcContainer(1), dstContainer(2), templateIdx(3) |
| Emits | `EVT_INV_ITEM_USE` | playerid(0), templateIdx(1), slotIdx(2), containerIdx(extra), metadata(string) |

## Dependencies

- `mod_db` — `DB_GetHandle()`, `DB_IsConnected()`

No player, vehicle, or UI dependencies.

## Auto-Save

5-minute timer iterates ALL active containers and saves dirty items (async). On shutdown, `Inv_Destroy` saves all containers with blocking queries.

## Owner Types

| Type | Value | DB Value |
|------|-------|----------|
| `OWNER_PLAYER` | 0 | 'player' |
| `OWNER_VEHICLE` | 1 | 'vehicle' |
| `OWNER_PROPERTY` | 2 | 'property' |
| `OWNER_WORLD` | 3 | 'world' |

These are metadata — the module doesn't have different behavior per owner type.
