# mod_inventory — Item & Container Data Layer

Universal inventory system based on templates (item types) and containers (storage). Player inventory, vehicle trunks, property storage, and ground drops all use the same container abstraction.

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

### Containers

| Function | Returns | Description |
|----------|---------|-------------|
| `Inv_GetPlayerContainerIdx(playerid)` | `int` | Container index for player (-1 if not loaded) |
| `Inv_IsPlayerLoaded(playerid)` | `bool` | Whether inventory data is loaded |
| `Inv_GetContainerWeight(containerIdx)` | `Float` | Current total weight |
| `Inv_GetContainerMaxWeight(containerIdx)` | `Float` | Max capacity |
| `Inv_FindContainerByOwner(ownerType, ownerId)` | `int` | Look up loaded container by owner (-1 if not in memory) |
| `Inv_LoadContainerByOwner(ownerType, ownerId, maxWeight)` | void | Async load/create container from DB for any owner type |
| `Inv_SaveAndFreeContainer(containerIdx)` | void | Save dirty items and release memory slot |

### Items

| Function | Returns | Description |
|----------|---------|-------------|
| `Inv_AddItem(containerIdx, templateIdx, qty, metadata[])` | `int` | Slot index on success, -1 on failure |
| `Inv_RemoveItem(containerIdx, slotIdx, qty)` | `int` | 1 on success, 0 on failure |
| `Inv_MoveItem(srcContainer, srcSlot, dstContainer, dstSlot)` | `int` | Move/swap between containers with weight checks |
| `Inv_GetItemTemplateIdx(containerIdx, slotIdx)` | `int` | Template index (-1 if empty) |
| `Inv_GetItemQuantity(containerIdx, slotIdx)` | `int` | Quantity in slot |
| `Inv_GetItemMetadata(containerIdx, slotIdx, dest[], maxlen)` | void | Copy metadata string |
| `Inv_IsSlotActive(containerIdx, slotIdx)` | `bool` | Whether slot has an item |
| `Inv_PlayerAddItem(playerid, templateIdx, qty, metadata[])` | `int` | Convenience wrapper |
| `Inv_PlayerRemoveItem(playerid, slotIdx, qty)` | `int` | Convenience wrapper |
| `Inv_PlayerUseItem(playerid, slotIdx)` | `int` | Emits EVT_INV_ITEM_USE |
| `Inv_Save(containerIdx)` | void | Force save dirty items to DB |

## Move Semantics

`Inv_MoveItem` handles three cases:
1. **Empty destination:** Item moves directly
2. **Same stackable template (no metadata):** Merge stacks up to max_stack
3. **Different item or metadata:** Swap positions

Weight checked for cross-container moves.

## Events

| Direction | Event | Data slots |
|-----------|-------|------------|
| Subscribes | `EVT_DB_CONNECTED` | — |
| Subscribes | `EVT_PLAYER_LOGIN` | playerid(0) |
| Subscribes | `EVT_PLAYER_LOGOUT` | playerid(0) |
| Subscribes | `EVT_PLAYER_DISCONNECT` | playerid(0) |
| Emits | `EVT_INV_TEMPLATES_LOADED` | — |
| Emits | `EVT_INV_PLAYER_LOADED` | playerid(0) |
| Emits | `EVT_INV_ITEM_ADDED` | playerid(0), containerIdx(1), templateIdx(2), quantity(3) |
| Emits | `EVT_INV_ITEM_REMOVED` | playerid(0), containerIdx(1), templateIdx(2), quantity(3) |
| Emits | `EVT_INV_ITEM_MOVED` | playerid(0), srcContainer(1), dstContainer(2), templateIdx(3) |
| Emits | `EVT_INV_ITEM_USE` | playerid(0), templateIdx(1), slotIdx(2), string=metadata |

## Commands

| Command | Level | Description |
|---------|-------|-------------|
| `/inv` | 0 | Open inventory UI |
| `/giveitem <id> [qty]` | 2 | Add item by template DB id |
| `/useitem <slot>` | 0 | Use item in slot |

## Dependencies

- `mod_db` — `DB_GetHandle()`, `DB_IsConnected()`
- `mod_auth` — `Auth_IsLoggedIn()`, `Auth_GetAccountId()`
- `mod_inventory_ui` — `InvUI_Open()` (called by /inv)

## Stacking Rules

- `metadata == ""` AND `max_stack > 1` → stackable, merges with existing stack
- `metadata != ""` OR `max_stack == 1` → unique, gets own slot

## Owner Types

| Type | Value | DB Value | Used By |
|------|-------|----------|---------|
| `OWNER_PLAYER` | 0 | 'player' | Player inventories |
| `OWNER_VEHICLE` | 1 | 'vehicle' | Vehicle trunks (mod_veh_trunk) |
| `OWNER_PROPERTY` | 2 | 'property' | Future |
| `OWNER_WORLD` | 3 | 'world' | Future |

## Memory Limits

| Constant | Value |
|----------|-------|
| `MAX_ITEM_TEMPLATES` | 128 |
| `MAX_CONTAINERS` | 500 |
| `MAX_ITEMS_PER_CONTAINER` | 20 |
| `INV_SLOTS` | 20 |
