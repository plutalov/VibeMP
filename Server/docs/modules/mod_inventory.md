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

### Items

| Function | Returns | Description |
|----------|---------|-------------|
| `Inv_AddItem(containerIdx, templateIdx, qty, metadata[])` | `int` | Slot index on success, -1 on failure |
| `Inv_RemoveItem(containerIdx, slotIdx, qty)` | `int` | 1 on success, 0 on failure |
| `Inv_GetItemTemplateIdx(containerIdx, slotIdx)` | `int` | Template index (-1 if empty) |
| `Inv_GetItemQuantity(containerIdx, slotIdx)` | `int` | Quantity in slot |
| `Inv_GetItemMetadata(containerIdx, slotIdx, dest[], maxlen)` | void | Copy metadata string |
| `Inv_IsSlotActive(containerIdx, slotIdx)` | `bool` | Whether slot has an item |
| `Inv_PlayerAddItem(playerid, templateIdx, qty, metadata[])` | `int` | Convenience wrapper |
| `Inv_PlayerRemoveItem(playerid, slotIdx, qty)` | `int` | Convenience wrapper |
| `Inv_PlayerUseItem(playerid, slotIdx)` | `int` | Emits EVT_INV_ITEM_USE |
| `Inv_Save(containerIdx)` | void | Force save dirty items to DB |

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
| Emits | `EVT_INV_ITEM_USE` | playerid(0), templateIdx(1), slotIdx(2), string=metadata |

## Commands

| Command | Level | Description |
|---------|-------|-------------|
| `/inv` | 0 | Open inventory UI (textdraw grid) |
| `/giveitem <id> [qty]` | 2 | Add item by template DB id |
| `/useitem <slot>` | 0 | Use item in specified slot |

## Dependencies

- `mod_db` — `DB_GetHandle()`, `DB_IsConnected()`
- `mod_auth` — `Auth_IsLoggedIn()`, `Auth_GetAccountId()`
- `mod_inventory_ui` — `InvUI_Open()` (called by /inv command)

## Database

**Tables:** `item_templates`, `containers`, `container_items` (created by V3 migration)

- Templates loaded at startup: `SELECT * FROM item_templates`
- Player container loaded on login: `SELECT id, max_weight FROM containers WHERE owner_type='player' AND owner_id=?`
- Items loaded per container: `SELECT id, template_id, quantity, slot, metadata FROM container_items WHERE container_id=?`
- New player: `INSERT INTO containers (owner_type, owner_id, max_weight)`
- Save: INSERT/UPDATE/DELETE `container_items` based on dirty flags

## Stacking Rules

- `metadata == ""` AND `max_stack > 1` → stackable, merges with existing stack
- `metadata != ""` OR `max_stack == 1` → unique, gets own slot
- Split-on-use: consuming module removes 1 from stack, adds new item with metadata

## Auto-Save

5-minute timer saves all loaded player inventories. Also saves on logout and server shutdown.

## Memory Limits

| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_ITEM_TEMPLATES` | 128 | Max item types |
| `MAX_CONTAINERS` | 500 | Active containers in memory |
| `MAX_ITEMS_PER_CONTAINER` | 20 | Slots per container |
| `INV_SLOTS` | 20 | Player inventory grid (4×5) |
