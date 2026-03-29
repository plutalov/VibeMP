// =============================================================================
//  mod_inventory.inc — Generic container & item data layer
//
//  Manages item templates, containers, and item instances.
//  Owner-agnostic: does not know or care what owns a container (player,
//  vehicle, property, etc.). Bridge modules (mod_player_inv, mod_veh_trunk)
//  handle the mapping between domain objects and containers.
//
//  Emits:
//    EVT_INV_TEMPLATES_LOADED  — after templates loaded at startup
//    EVT_INV_ITEM_ADDED        — item added to any container
//    EVT_INV_ITEM_REMOVED      — item removed from any container
//    EVT_INV_ITEM_MOVED        — item moved between containers
//
//  Depends on: mod_db
// =============================================================================

#if defined _mod_inventory_included
    #endinput
#endif
#define _mod_inventory_included

// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define MAX_ITEM_TEMPLATES      128
#define MAX_TEMPLATE_DB_ID      256
#define MAX_CONTAINERS          500
#define MAX_ITEMS_PER_CONTAINER 20
#define INV_SLOTS               20      // player inventory grid (4x5)
#define MAX_METADATA_LEN        128
#define MAX_ITEM_NAME_LEN       32
#define MAX_CATEGORY_LEN        16
#define INV_DEFAULT_MAX_WEIGHT  30.0
#define INV_AUTOSAVE_INTERVAL   300000  // 5 minutes

// Owner types (mirrors DB ENUM order)
#define OWNER_PLAYER            0
#define OWNER_VEHICLE           1
#define OWNER_PROPERTY          2
#define OWNER_WORLD             3

#define INVALID_TEMPLATE_IDX    (-1)
#define INVALID_CONTAINER_IDX   (-1)

// -----------------------------------------------------------------------------
//  Item Templates (loaded from DB once at startup)
// -----------------------------------------------------------------------------

enum E_ITEM_TEMPLATE
{
    tpl_DbId,
    tpl_Name[MAX_ITEM_NAME_LEN],
    tpl_Category[MAX_CATEGORY_LEN],
    Float:tpl_Weight,
    tpl_MaxStack,
    tpl_ModelId,
    tpl_DefaultMeta[MAX_METADATA_LEN]
}

static g_Templates[MAX_ITEM_TEMPLATES][E_ITEM_TEMPLATE];
static g_TemplateCount;
static g_TemplateDbToIdx[MAX_TEMPLATE_DB_ID]; // DB id → array index
static bool:g_TemplatesLoaded;

// -----------------------------------------------------------------------------
//  Containers
// -----------------------------------------------------------------------------

enum E_CONTAINER
{
    cnt_DbId,
    cnt_OwnerType,
    cnt_OwnerId,
    Float:cnt_MaxWeight,
    bool:cnt_Active
}

static g_Containers[MAX_CONTAINERS][E_CONTAINER];

// -----------------------------------------------------------------------------
//  Container Items
// -----------------------------------------------------------------------------

enum E_CONTAINER_ITEM
{
    ci_DbId,            // DB container_items.id (0 = new, not yet persisted)
    ci_TemplateIdx,     // index into g_Templates (NOT db id)
    ci_Quantity,
    ci_Slot,            // grid position (0..INV_SLOTS-1)
    ci_Metadata[MAX_METADATA_LEN],
    bool:ci_Active,
    bool:ci_Dirty,      // needs DB write
    bool:ci_Deleted     // marked for DELETE on next save
}

static g_Items[MAX_CONTAINERS][MAX_ITEMS_PER_CONTAINER][E_CONTAINER_ITEM];

static g_AutoSaveTimer = -1;

// =============================================================================
//  Public API — Templates
// =============================================================================

stock Inv_GetTemplateCount()
    return g_TemplateCount;

stock Inv_GetTemplateName(templateIdx, dest[], maxlen)
{
    if (templateIdx < 0 || templateIdx >= g_TemplateCount) { dest[0] = EOS; return; }
    strcat((dest[0] = EOS, dest), g_Templates[templateIdx][tpl_Name], maxlen);
}

stock Inv_GetTemplateCategory(templateIdx, dest[], maxlen)
{
    if (templateIdx < 0 || templateIdx >= g_TemplateCount) { dest[0] = EOS; return; }
    strcat((dest[0] = EOS, dest), g_Templates[templateIdx][tpl_Category], maxlen);
}

stock Float:Inv_GetTemplateWeight(templateIdx)
{
    if (templateIdx < 0 || templateIdx >= g_TemplateCount) return 0.0;
    return g_Templates[templateIdx][tpl_Weight];
}

stock Inv_GetTemplateMaxStack(templateIdx)
{
    if (templateIdx < 0 || templateIdx >= g_TemplateCount) return 1;
    return g_Templates[templateIdx][tpl_MaxStack];
}

stock Inv_GetTemplateModelId(templateIdx)
{
    if (templateIdx < 0 || templateIdx >= g_TemplateCount) return 0;
    return g_Templates[templateIdx][tpl_ModelId];
}

stock Inv_FindTemplateByDbId(dbId)
{
    if (dbId >= 0 && dbId < MAX_TEMPLATE_DB_ID)
        return g_TemplateDbToIdx[dbId];
    // Fallback: linear scan
    for (new i = 0; i < g_TemplateCount; i++)
        if (g_Templates[i][tpl_DbId] == dbId) return i;
    return INVALID_TEMPLATE_IDX;
}

stock Inv_FindTemplateByName(const name[])
{
    for (new i = 0; i < g_TemplateCount; i++)
        if (!strcmp(g_Templates[i][tpl_Name], name, true)) return i;
    return INVALID_TEMPLATE_IDX;
}

// =============================================================================
//  Public API — Containers
// =============================================================================

stock Float:Inv_GetContainerWeight(containerIdx)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return 0.0;
    new Float:total = 0.0;
    for (new i = 0; i < MAX_ITEMS_PER_CONTAINER; i++)
    {
        if (!g_Items[containerIdx][i][ci_Active] || g_Items[containerIdx][i][ci_Deleted]) continue;
        new tIdx = g_Items[containerIdx][i][ci_TemplateIdx];
        total += g_Templates[tIdx][tpl_Weight] * float(g_Items[containerIdx][i][ci_Quantity]);
    }
    return total;
}

stock Float:Inv_GetContainerMaxWeight(containerIdx)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return 0.0;
    return g_Containers[containerIdx][cnt_MaxWeight];
}

// =============================================================================
//  Public API — Items
// =============================================================================

/// Use an item from any container. Emits EVT_INV_ITEM_USE with container index
/// in EVD_EXTRA so domain modules know which container to remove from.
stock Inv_UseItemFromContainer(playerid, containerIdx, slotIdx)
{
    if (!Inv_IsSlotActive(containerIdx, slotIdx)) return 0;

    new templateIdx = Inv_GetItemTemplateIdx(containerIdx, slotIdx);
    new meta[MAX_METADATA_LEN];
    Inv_GetItemMetadata(containerIdx, slotIdx, meta, sizeof(meta));

    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, templateIdx);
    EventBus_SetInt(EVD_VALUE, slotIdx);
    EventBus_SetInt(EVD_EXTRA, containerIdx);
    EventBus_SetString(meta);
    EventBus_Emit(EVT_INV_ITEM_USE);
    return 1;
}

stock Inv_GetItemTemplateIdx(containerIdx, slotIdx)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return INVALID_TEMPLATE_IDX;
    if (slotIdx < 0 || slotIdx >= MAX_ITEMS_PER_CONTAINER) return INVALID_TEMPLATE_IDX;
    if (!g_Items[containerIdx][slotIdx][ci_Active]) return INVALID_TEMPLATE_IDX;
    return g_Items[containerIdx][slotIdx][ci_TemplateIdx];
}

stock Inv_GetItemQuantity(containerIdx, slotIdx)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return 0;
    if (slotIdx < 0 || slotIdx >= MAX_ITEMS_PER_CONTAINER) return 0;
    if (!g_Items[containerIdx][slotIdx][ci_Active]) return 0;
    return g_Items[containerIdx][slotIdx][ci_Quantity];
}

stock Inv_GetItemMetadata(containerIdx, slotIdx, dest[], maxlen)
{
    dest[0] = EOS;
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return;
    if (slotIdx < 0 || slotIdx >= MAX_ITEMS_PER_CONTAINER) return;
    if (!g_Items[containerIdx][slotIdx][ci_Active]) return;
    strcat(dest, g_Items[containerIdx][slotIdx][ci_Metadata], maxlen);
}

stock bool:Inv_IsSlotActive(containerIdx, slotIdx)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return false;
    if (slotIdx < 0 || slotIdx >= MAX_ITEMS_PER_CONTAINER) return false;
    return g_Items[containerIdx][slotIdx][ci_Active] && !g_Items[containerIdx][slotIdx][ci_Deleted];
}

// Find first free slot in a container
static stock Inv_FindFreeSlot(containerIdx)
{
    for (new i = 0; i < MAX_ITEMS_PER_CONTAINER; i++)
        if (!g_Items[containerIdx][i][ci_Active] || g_Items[containerIdx][i][ci_Deleted])
            return i;
    return -1;
}

// Find existing stack of same template with room
static stock Inv_FindStackableSlot(containerIdx, templateIdx)
{
    new maxStack = g_Templates[templateIdx][tpl_MaxStack];
    if (maxStack <= 1) return -1;
    for (new i = 0; i < MAX_ITEMS_PER_CONTAINER; i++)
    {
        if (!g_Items[containerIdx][i][ci_Active] || g_Items[containerIdx][i][ci_Deleted]) continue;
        if (g_Items[containerIdx][i][ci_TemplateIdx] != templateIdx) continue;
        if (g_Items[containerIdx][i][ci_Metadata][0] != EOS) continue; // has metadata = unique
        if (g_Items[containerIdx][i][ci_Quantity] < maxStack) return i;
    }
    return -1;
}

// --- Core add ---
stock Inv_AddItem(containerIdx, templateIdx, quantity, const metadata[])
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return -1;
    if (!g_Containers[containerIdx][cnt_Active]) return -1;
    if (templateIdx < 0 || templateIdx >= g_TemplateCount) return -1;
    if (quantity <= 0) return -1;

    // Weight check
    new Float:addWeight = g_Templates[templateIdx][tpl_Weight] * float(quantity);
    if (Inv_GetContainerWeight(containerIdx) + addWeight > g_Containers[containerIdx][cnt_MaxWeight])
        return -1; // too heavy

    new remaining = quantity;
    new lastSlot = -1;

    // Try stacking first (only if no metadata)
    if (metadata[0] == EOS && g_Templates[templateIdx][tpl_MaxStack] > 1)
    {
        new maxStack = g_Templates[templateIdx][tpl_MaxStack];
        while (remaining > 0)
        {
            new slot = Inv_FindStackableSlot(containerIdx, templateIdx);
            if (slot == -1) break;

            new canAdd = maxStack - g_Items[containerIdx][slot][ci_Quantity];
            if (canAdd > remaining) canAdd = remaining;

            g_Items[containerIdx][slot][ci_Quantity] += canAdd;
            g_Items[containerIdx][slot][ci_Dirty] = true;
            remaining -= canAdd;
            lastSlot = slot;
        }
    }

    // Place remainder in new slots
    while (remaining > 0)
    {
        new slot = Inv_FindFreeSlot(containerIdx);
        if (slot == -1) break; // no room

        new maxStack = g_Templates[templateIdx][tpl_MaxStack];
        if (metadata[0] != EOS) maxStack = 1; // unique items don't stack

        new toPlace = remaining;
        if (toPlace > maxStack) toPlace = maxStack;

        g_Items[containerIdx][slot][ci_DbId]        = 0; // new, needs INSERT
        g_Items[containerIdx][slot][ci_TemplateIdx]  = templateIdx;
        g_Items[containerIdx][slot][ci_Quantity]     = toPlace;
        g_Items[containerIdx][slot][ci_Slot]         = slot;
        g_Items[containerIdx][slot][ci_Metadata][0]  = EOS;
        strcat(g_Items[containerIdx][slot][ci_Metadata], metadata, MAX_METADATA_LEN);
        g_Items[containerIdx][slot][ci_Active]       = true;
        g_Items[containerIdx][slot][ci_Dirty]        = true;
        g_Items[containerIdx][slot][ci_Deleted]      = false;

        remaining -= toPlace;
        lastSlot = slot;
    }

    if (lastSlot != -1)
    {
        // Emit event
        EventBus_SetInt(EVD_PLAYER_ID, -1); // generic container, no player
        EventBus_SetInt(EVD_SECONDARY_ID, containerIdx);
        EventBus_SetInt(EVD_VALUE, templateIdx);
        EventBus_SetInt(EVD_EXTRA, quantity - remaining);
        EventBus_Emit(EVT_INV_ITEM_ADDED);
    }

    return lastSlot;
}

// --- Core remove ---
stock Inv_RemoveItem(containerIdx, slotIdx, quantity)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return 0;
    if (slotIdx < 0 || slotIdx >= MAX_ITEMS_PER_CONTAINER) return 0;
    if (!g_Items[containerIdx][slotIdx][ci_Active]) return 0;

    new templateIdx = g_Items[containerIdx][slotIdx][ci_TemplateIdx];

    if (quantity >= g_Items[containerIdx][slotIdx][ci_Quantity])
    {
        // Remove entire stack
        new removed = g_Items[containerIdx][slotIdx][ci_Quantity];
        g_Items[containerIdx][slotIdx][ci_Quantity] = 0;
        if (g_Items[containerIdx][slotIdx][ci_DbId] > 0)
        {
            g_Items[containerIdx][slotIdx][ci_Deleted] = true;
            g_Items[containerIdx][slotIdx][ci_Dirty] = true;
        }
        else
        {
            g_Items[containerIdx][slotIdx][ci_Active] = false;
        }

        EventBus_SetInt(EVD_PLAYER_ID, -1);
        EventBus_SetInt(EVD_SECONDARY_ID, containerIdx);
        EventBus_SetInt(EVD_VALUE, templateIdx);
        EventBus_SetInt(EVD_EXTRA, removed);
        EventBus_Emit(EVT_INV_ITEM_REMOVED);
        return 1;
    }
    else
    {
        // Partial removal
        g_Items[containerIdx][slotIdx][ci_Quantity] -= quantity;
        g_Items[containerIdx][slotIdx][ci_Dirty] = true;

        EventBus_SetInt(EVD_PLAYER_ID, -1);
        EventBus_SetInt(EVD_SECONDARY_ID, containerIdx);
        EventBus_SetInt(EVD_VALUE, templateIdx);
        EventBus_SetInt(EVD_EXTRA, quantity);
        EventBus_Emit(EVT_INV_ITEM_REMOVED);
        return 1;
    }
}

// =============================================================================
//  DB Save
// =============================================================================

stock Inv_Save(containerIdx, bool:blocking = false)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return;
    if (!g_Containers[containerIdx][cnt_Active]) return;

    new cntDbId = g_Containers[containerIdx][cnt_DbId];
    new query[512];
    new MySQL:db = DB_GetHandle();

    for (new i = 0; i < MAX_ITEMS_PER_CONTAINER; i++)
    {
        if (!g_Items[containerIdx][i][ci_Dirty]) continue;

        if (g_Items[containerIdx][i][ci_Deleted] && g_Items[containerIdx][i][ci_DbId] > 0)
        {
            // DELETE
            mysql_format(db, query, sizeof(query),
                "DELETE FROM container_items WHERE id = %d",
                g_Items[containerIdx][i][ci_DbId]);
            if (blocking) mysql_query(db, query);
            else mysql_tquery(db, query);
            g_Items[containerIdx][i][ci_Active] = false;
            g_Items[containerIdx][i][ci_Dirty] = false;
            g_Items[containerIdx][i][ci_Deleted] = false;
        }
        else if (g_Items[containerIdx][i][ci_DbId] == 0 && g_Items[containerIdx][i][ci_Active])
        {
            // INSERT
            new tplDbId = g_Templates[g_Items[containerIdx][i][ci_TemplateIdx]][tpl_DbId];
            mysql_format(db, query, sizeof(query),
                "INSERT INTO container_items (container_id, slot, template_id, quantity, metadata) \
                 VALUES (%d, %d, %d, %d, '%e')",
                cntDbId, i, tplDbId,
                g_Items[containerIdx][i][ci_Quantity],
                g_Items[containerIdx][i][ci_Metadata]);
            if (blocking)
            {
                new Cache:result = mysql_query(db, query);
                g_Items[containerIdx][i][ci_DbId] = cache_insert_id();
                cache_delete(result);
            }
            else
                mysql_tquery(db, query, "Inv_OnItemInserted", "dd", containerIdx, i);
        }
        else if (g_Items[containerIdx][i][ci_DbId] > 0 && g_Items[containerIdx][i][ci_Active])
        {
            // UPDATE
            mysql_format(db, query, sizeof(query),
                "UPDATE container_items SET slot=%d, quantity=%d, metadata='%e' WHERE id=%d",
                i, g_Items[containerIdx][i][ci_Quantity],
                g_Items[containerIdx][i][ci_Metadata],
                g_Items[containerIdx][i][ci_DbId]);
            if (blocking) mysql_query(db, query);
            else mysql_tquery(db, query);
        }

        g_Items[containerIdx][i][ci_Dirty] = false;
    }
}

forward Inv_OnItemInserted(containerIdx, slotIdx);
public Inv_OnItemInserted(containerIdx, slotIdx)
{
    // Store the auto-generated DB id for future UPDATEs
    g_Items[containerIdx][slotIdx][ci_DbId] = cache_insert_id();
}

// =============================================================================
//  Public API — Generic Container Operations
//  Used by bridge modules (mod_veh_trunk, etc.) to manage non-player containers
// =============================================================================

/// Look up a loaded container by owner type and id. Returns containerIdx or -1.
stock Inv_FindContainerByOwner(ownerType, ownerId)
{
    for (new i = 0; i < MAX_CONTAINERS; i++)
    {
        if (g_Containers[i][cnt_Active]
            && g_Containers[i][cnt_OwnerType] == ownerType
            && g_Containers[i][cnt_OwnerId] == ownerId)
            return i;
    }
    return INVALID_CONTAINER_IDX;
}

/// Save dirty items and release a container's memory slot.
/// Does NOT delete the container from DB — data persists for next load.
stock Inv_SaveAndFreeContainer(containerIdx, bool:blocking = false)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return;
    if (!g_Containers[containerIdx][cnt_Active]) return;

    Inv_Save(containerIdx, blocking);

    // Clear items
    for (new i = 0; i < MAX_ITEMS_PER_CONTAINER; i++)
    {
        g_Items[containerIdx][i][ci_Active] = false;
        g_Items[containerIdx][i][ci_Dirty] = false;
        g_Items[containerIdx][i][ci_Deleted] = false;
    }

    g_Containers[containerIdx][cnt_Active] = false;
}

/// Load or create a container for any owner type. Async — items available after
/// EVT_INV_CONTAINER_READY fires (not yet implemented, for now just loads silently).
/// Returns immediately; container is populated when DB callbacks complete.
stock Inv_LoadContainerByOwner(ownerType, ownerId, Float:maxWeight = INV_DEFAULT_MAX_WEIGHT)
{
    // Check if already loaded
    new existing = Inv_FindContainerByOwner(ownerType, ownerId);
    if (existing != INVALID_CONTAINER_IDX) return; // already in memory

    new ownerTypeStr[16];
    switch (ownerType)
    {
        case OWNER_PLAYER:   strcat(ownerTypeStr, "player", sizeof(ownerTypeStr));
        case OWNER_VEHICLE:  strcat(ownerTypeStr, "vehicle", sizeof(ownerTypeStr));
        case OWNER_PROPERTY: strcat(ownerTypeStr, "property", sizeof(ownerTypeStr));
        case OWNER_WORLD:    strcat(ownerTypeStr, "world", sizeof(ownerTypeStr));
        default: return;
    }

    new query[256];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "SELECT id, max_weight FROM containers WHERE owner_type = '%e' AND owner_id = %d LIMIT 1",
        ownerTypeStr, ownerId);
    mysql_tquery(DB_GetHandle(), query, "Inv_OnGenericContainerLoad", "ddf",
        ownerType, ownerId, maxWeight);
}

forward Inv_OnGenericContainerLoad(ownerType, ownerId, Float:maxWeight);
public Inv_OnGenericContainerLoad(ownerType, ownerId, Float:maxWeight)
{
    // Check if something else already loaded it while we were querying
    if (Inv_FindContainerByOwner(ownerType, ownerId) != INVALID_CONTAINER_IDX) return;

    new rows;
    cache_get_row_count(rows);

    new containerIdx = Inv_AllocContainer();
    if (containerIdx == -1)
    {
        printf("[INV] ERROR: No free container slots for owner type=%d id=%d", ownerType, ownerId);
        return;
    }

    if (rows > 0)
    {
        // Container exists in DB
        cache_get_value_name_int(0, "id", g_Containers[containerIdx][cnt_DbId]);
        cache_get_value_name_float(0, "max_weight", g_Containers[containerIdx][cnt_MaxWeight]);
    }
    else
    {
        // Create new container in DB
        new ownerTypeStr[16];
        switch (ownerType)
        {
            case OWNER_PLAYER:   strcat(ownerTypeStr, "player", sizeof(ownerTypeStr));
            case OWNER_VEHICLE:  strcat(ownerTypeStr, "vehicle", sizeof(ownerTypeStr));
            case OWNER_PROPERTY: strcat(ownerTypeStr, "property", sizeof(ownerTypeStr));
            case OWNER_WORLD:    strcat(ownerTypeStr, "world", sizeof(ownerTypeStr));
        }

        new query[128];
        mysql_format(DB_GetHandle(), query, sizeof(query),
            "INSERT INTO containers (owner_type, owner_id, max_weight) VALUES ('%e', %d, %f)",
            ownerTypeStr, ownerId, maxWeight);
        mysql_tquery(DB_GetHandle(), query, "Inv_OnGenericContainerInserted", "d", containerIdx);

        g_Containers[containerIdx][cnt_MaxWeight] = maxWeight;
    }

    g_Containers[containerIdx][cnt_OwnerType] = ownerType;
    g_Containers[containerIdx][cnt_OwnerId] = ownerId;
    g_Containers[containerIdx][cnt_Active] = true;

    // Load items if container existed
    if (rows > 0)
    {
        new query[128];
        mysql_format(DB_GetHandle(), query, sizeof(query),
            "SELECT id, template_id, quantity, slot, metadata FROM container_items WHERE container_id = %d",
            g_Containers[containerIdx][cnt_DbId]);
        mysql_tquery(DB_GetHandle(), query, "Inv_OnGenericItemsLoad", "d", containerIdx);
    }
    else
    {
        printf("[INV] Created container for owner type=%d id=%d", ownerType, ownerId);
    }
}

forward Inv_OnGenericContainerInserted(containerIdx);
public Inv_OnGenericContainerInserted(containerIdx)
{
    if (containerIdx >= 0 && containerIdx < MAX_CONTAINERS && g_Containers[containerIdx][cnt_Active])
        g_Containers[containerIdx][cnt_DbId] = cache_insert_id();
}

forward Inv_OnGenericItemsLoad(containerIdx);
public Inv_OnGenericItemsLoad(containerIdx)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return;
    if (!g_Containers[containerIdx][cnt_Active]) return;

    new rows;
    cache_get_row_count(rows);

    for (new i = 0; i < rows && i < MAX_ITEMS_PER_CONTAINER; i++)
    {
        new dbId, tplDbId, quantity, slot;
        new meta[MAX_METADATA_LEN];

        cache_get_value_name_int(i, "id", dbId);
        cache_get_value_name_int(i, "template_id", tplDbId);
        cache_get_value_name_int(i, "quantity", quantity);
        cache_get_value_name_int(i, "slot", slot);
        cache_get_value_name(i, "metadata", meta, MAX_METADATA_LEN);

        new tplIdx = Inv_FindTemplateByDbId(tplDbId);
        if (tplIdx == INVALID_TEMPLATE_IDX) continue;

        if (slot < 0 || slot >= MAX_ITEMS_PER_CONTAINER)
            slot = Inv_FindFreeSlot(containerIdx);
        if (slot == -1) continue;

        g_Items[containerIdx][slot][ci_DbId]        = dbId;
        g_Items[containerIdx][slot][ci_TemplateIdx]  = tplIdx;
        g_Items[containerIdx][slot][ci_Quantity]     = quantity;
        g_Items[containerIdx][slot][ci_Slot]         = slot;
        g_Items[containerIdx][slot][ci_Metadata][0]  = EOS;
        strcat(g_Items[containerIdx][slot][ci_Metadata], meta, MAX_METADATA_LEN);
        g_Items[containerIdx][slot][ci_Active]       = true;
        g_Items[containerIdx][slot][ci_Dirty]        = false;
        g_Items[containerIdx][slot][ci_Deleted]      = false;
    }

    printf("[INV] Loaded %d items for container owner=%d:%d",
        rows, g_Containers[containerIdx][cnt_OwnerType], g_Containers[containerIdx][cnt_OwnerId]);
}

/// Move an item from one container/slot to another. Handles weight checks.
/// If destination slot is occupied with same template + stackable: merge.
/// If destination slot is occupied with different item: swap.
/// Returns 1 on success, 0 on failure.
stock Inv_MoveItem(srcContainer, srcSlot, dstContainer, dstSlot)
{
    if (srcContainer < 0 || srcContainer >= MAX_CONTAINERS) return 0;
    if (dstContainer < 0 || dstContainer >= MAX_CONTAINERS) return 0;
    if (srcSlot < 0 || srcSlot >= MAX_ITEMS_PER_CONTAINER) return 0;
    if (dstSlot < 0 || dstSlot >= MAX_ITEMS_PER_CONTAINER) return 0;
    if (!g_Containers[srcContainer][cnt_Active]) return 0;
    if (!g_Containers[dstContainer][cnt_Active]) return 0;
    if (!g_Items[srcContainer][srcSlot][ci_Active]) return 0;

    // Same slot, same container — no-op
    if (srcContainer == dstContainer && srcSlot == dstSlot) return 1;

    new srcTpl = g_Items[srcContainer][srcSlot][ci_TemplateIdx];
    new srcQty = g_Items[srcContainer][srcSlot][ci_Quantity];
    new Float:srcItemWeight = g_Templates[srcTpl][tpl_Weight] * float(srcQty);

    if (g_Items[dstContainer][dstSlot][ci_Active] && !g_Items[dstContainer][dstSlot][ci_Deleted])
    {
        new dstTpl = g_Items[dstContainer][dstSlot][ci_TemplateIdx];
        new dstQty = g_Items[dstContainer][dstSlot][ci_Quantity];

        if (srcTpl == dstTpl
            && g_Items[srcContainer][srcSlot][ci_Metadata][0] == EOS
            && g_Items[dstContainer][dstSlot][ci_Metadata][0] == EOS
            && g_Templates[srcTpl][tpl_MaxStack] > 1)
        {
            // Same stackable template — merge
            new maxStack = g_Templates[srcTpl][tpl_MaxStack];
            new canFit = maxStack - dstQty;
            if (canFit <= 0) return 0; // destination stack full

            new toMove = srcQty;
            if (toMove > canFit) toMove = canFit;

            g_Items[dstContainer][dstSlot][ci_Quantity] += toMove;
            g_Items[dstContainer][dstSlot][ci_Dirty] = true;

            g_Items[srcContainer][srcSlot][ci_Quantity] -= toMove;
            if (g_Items[srcContainer][srcSlot][ci_Quantity] <= 0)
            {
                if (g_Items[srcContainer][srcSlot][ci_DbId] > 0)
                {
                    g_Items[srcContainer][srcSlot][ci_Deleted] = true;
                    g_Items[srcContainer][srcSlot][ci_Dirty] = true;
                }
                else
                    g_Items[srcContainer][srcSlot][ci_Active] = false;
            }
            else
                g_Items[srcContainer][srcSlot][ci_Dirty] = true;
        }
        else
        {
            // Different items or unique — swap
            if (srcContainer != dstContainer)
            {
                // Weight check for cross-container swap
                new Float:dstItemWeight = g_Templates[dstTpl][tpl_Weight] * float(dstQty);
                new Float:srcContWeight = Inv_GetContainerWeight(srcContainer);
                new Float:dstContWeight = Inv_GetContainerWeight(dstContainer);

                // After swap: src loses srcItem, gains dstItem
                if (srcContWeight - srcItemWeight + dstItemWeight > g_Containers[srcContainer][cnt_MaxWeight])
                    return 0;
                // After swap: dst loses dstItem, gains srcItem
                if (dstContWeight - dstItemWeight + srcItemWeight > g_Containers[dstContainer][cnt_MaxWeight])
                    return 0;
            }

            // Perform swap by copying fields
            new tmpDbId = g_Items[dstContainer][dstSlot][ci_DbId];
            new tmpTpl  = g_Items[dstContainer][dstSlot][ci_TemplateIdx];
            new tmpQty  = g_Items[dstContainer][dstSlot][ci_Quantity];
            new tmpMeta[MAX_METADATA_LEN];
            strcat(tmpMeta, g_Items[dstContainer][dstSlot][ci_Metadata], MAX_METADATA_LEN);

            g_Items[dstContainer][dstSlot][ci_DbId]       = g_Items[srcContainer][srcSlot][ci_DbId];
            g_Items[dstContainer][dstSlot][ci_TemplateIdx] = srcTpl;
            g_Items[dstContainer][dstSlot][ci_Quantity]    = srcQty;
            g_Items[dstContainer][dstSlot][ci_Metadata][0] = EOS;
            strcat(g_Items[dstContainer][dstSlot][ci_Metadata],
                g_Items[srcContainer][srcSlot][ci_Metadata], MAX_METADATA_LEN);
            g_Items[dstContainer][dstSlot][ci_Dirty]       = true;

            g_Items[srcContainer][srcSlot][ci_DbId]       = tmpDbId;
            g_Items[srcContainer][srcSlot][ci_TemplateIdx] = tmpTpl;
            g_Items[srcContainer][srcSlot][ci_Quantity]    = tmpQty;
            g_Items[srcContainer][srcSlot][ci_Metadata][0] = EOS;
            strcat(g_Items[srcContainer][srcSlot][ci_Metadata], tmpMeta, MAX_METADATA_LEN);
            g_Items[srcContainer][srcSlot][ci_Dirty]       = true;
        }
    }
    else
    {
        // Destination empty — simple move
        if (srcContainer != dstContainer)
        {
            // Weight check
            new Float:dstContWeight = Inv_GetContainerWeight(dstContainer);
            if (dstContWeight + srcItemWeight > g_Containers[dstContainer][cnt_MaxWeight])
                return 0;
        }

        // Copy to destination
        g_Items[dstContainer][dstSlot][ci_DbId]       = g_Items[srcContainer][srcSlot][ci_DbId];
        g_Items[dstContainer][dstSlot][ci_TemplateIdx] = srcTpl;
        g_Items[dstContainer][dstSlot][ci_Quantity]    = srcQty;
        g_Items[dstContainer][dstSlot][ci_Slot]        = dstSlot;
        g_Items[dstContainer][dstSlot][ci_Metadata][0] = EOS;
        strcat(g_Items[dstContainer][dstSlot][ci_Metadata],
            g_Items[srcContainer][srcSlot][ci_Metadata], MAX_METADATA_LEN);
        g_Items[dstContainer][dstSlot][ci_Active]      = true;
        g_Items[dstContainer][dstSlot][ci_Dirty]       = true;
        g_Items[dstContainer][dstSlot][ci_Deleted]     = false;

        // Clear source
        if (g_Items[srcContainer][srcSlot][ci_DbId] > 0)
        {
            // DB item moved — old row needs delete, new row needs insert
            // Simplest: mark old as deleted, new as dbId=0 (will INSERT on save)
            g_Items[srcContainer][srcSlot][ci_Deleted] = true;
            g_Items[srcContainer][srcSlot][ci_Dirty] = true;
            g_Items[dstContainer][dstSlot][ci_DbId] = 0; // will get new DB id on save
        }
        else
        {
            g_Items[srcContainer][srcSlot][ci_Active] = false;
        }
    }

    // Emit move event
    EventBus_SetInt(EVD_PLAYER_ID, -1);
    EventBus_SetInt(EVD_SECONDARY_ID, srcContainer);
    EventBus_SetInt(EVD_VALUE, dstContainer);
    EventBus_SetInt(EVD_EXTRA, srcTpl);
    EventBus_Emit(EVT_INV_ITEM_MOVED);

    return 1;
}

// =============================================================================
//  Init / Destroy
// =============================================================================

stock Inv_Init()
{
    EventBus_RegisterModule("Inventory");
    EventBus_Subscribe(EVT_DB_CONNECTED, "Inv_OnDBConnected");

    // Reset state
    g_TemplateCount = 0;
    g_TemplatesLoaded = false;
    for (new i = 0; i < MAX_TEMPLATE_DB_ID; i++)
        g_TemplateDbToIdx[i] = INVALID_TEMPLATE_IDX;
    for (new i = 0; i < MAX_CONTAINERS; i++)
        g_Containers[i][cnt_Active] = false;

    g_AutoSaveTimer = SetTimer("Inv_AutoSave", INV_AUTOSAVE_INTERVAL, true);
    print("[MOD] Inventory module initialized.");

    // Load templates immediately if DB is already connected
    // (DB_Init runs before Inv_Init, so EVT_DB_CONNECTED was already emitted)
    if (DB_IsConnected())
    {
        mysql_tquery(DB_GetHandle(),
            "SELECT id, name, category, weight, max_stack, model_id, default_metadata FROM item_templates ORDER BY id",
            "Inv_OnTemplatesLoaded", "");
    }
}

stock Inv_Destroy()
{
    if (g_AutoSaveTimer != -1)
    {
        KillTimer(g_AutoSaveTimer);
        g_AutoSaveTimer = -1;
    }

    // Save all active containers (blocking — server is shutting down)
    for (new i = 0; i < MAX_CONTAINERS; i++)
    {
        if (g_Containers[i][cnt_Active])
            Inv_Save(i, true);
    }

    print("[MOD] Inventory module destroyed.");
}

// =============================================================================
//  Event Handlers
// =============================================================================

// --- DB Connected: load item templates ---
forward Inv_OnDBConnected();
public Inv_OnDBConnected()
{
    mysql_tquery(DB_GetHandle(),
        "SELECT id, name, category, weight, max_stack, model_id, default_metadata FROM item_templates ORDER BY id",
        "Inv_OnTemplatesLoaded", "");
    return 1;
}

forward Inv_OnTemplatesLoaded();
public Inv_OnTemplatesLoaded()
{
    new rows;
    cache_get_row_count(rows);
    g_TemplateCount = 0;

    for (new i = 0; i < rows && i < MAX_ITEM_TEMPLATES; i++)
    {
        cache_get_value_name_int(i, "id", g_Templates[i][tpl_DbId]);
        cache_get_value_name(i, "name", g_Templates[i][tpl_Name], MAX_ITEM_NAME_LEN);
        cache_get_value_name(i, "category", g_Templates[i][tpl_Category], MAX_CATEGORY_LEN);
        cache_get_value_name_float(i, "weight", g_Templates[i][tpl_Weight]);
        cache_get_value_name_int(i, "max_stack", g_Templates[i][tpl_MaxStack]);
        cache_get_value_name_int(i, "model_id", g_Templates[i][tpl_ModelId]);
        cache_get_value_name(i, "default_metadata", g_Templates[i][tpl_DefaultMeta], MAX_METADATA_LEN);

        // Populate reverse lookup
        new dbId = g_Templates[i][tpl_DbId];
        if (dbId >= 0 && dbId < MAX_TEMPLATE_DB_ID)
            g_TemplateDbToIdx[dbId] = i;

        g_TemplateCount++;
    }

    g_TemplatesLoaded = true;
    printf("[INV] Loaded %d item templates.", g_TemplateCount);

    EventBus_Emit(EVT_INV_TEMPLATES_LOADED);
}

// Player-specific handlers moved to mod_player_inv.inc

// =============================================================================
//  Container lifecycle helpers (used by bridge modules)
// =============================================================================

/// Allocate a free container slot in memory. Returns index or -1.
stock Inv_AllocContainer()
{
    for (new i = 0; i < MAX_CONTAINERS; i++)
        if (!g_Containers[i][cnt_Active]) return i;
    return -1;
}

/// Initialize a container slot with known values (after DB INSERT).
stock Inv_InitContainer(containerIdx, dbId, ownerType, ownerId, Float:maxWeight)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return;
    g_Containers[containerIdx][cnt_DbId] = dbId;
    g_Containers[containerIdx][cnt_OwnerType] = ownerType;
    g_Containers[containerIdx][cnt_OwnerId] = ownerId;
    g_Containers[containerIdx][cnt_MaxWeight] = maxWeight;
    g_Containers[containerIdx][cnt_Active] = true;
}

/// Populate a container slot from MySQL cache row (after SELECT).
stock Inv_SetContainerFromCache(containerIdx, row, ownerType, ownerId)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return;
    cache_get_value_name_int(row, "id", g_Containers[containerIdx][cnt_DbId]);
    cache_get_value_name_float(row, "max_weight", g_Containers[containerIdx][cnt_MaxWeight]);
    g_Containers[containerIdx][cnt_OwnerType] = ownerType;
    g_Containers[containerIdx][cnt_OwnerId] = ownerId;
    g_Containers[containerIdx][cnt_Active] = true;
}

/// Load items from current MySQL cache into a container's slots.
/// Call this inside an async callback after querying container_items.
stock Inv_LoadItemsFromCache(containerIdx)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return;

    new rows;
    cache_get_row_count(rows);

    for (new i = 0; i < rows && i < MAX_ITEMS_PER_CONTAINER; i++)
    {
        new dbId, tplDbId, quantity, slot;
        new meta[MAX_METADATA_LEN];

        cache_get_value_name_int(i, "id", dbId);
        cache_get_value_name_int(i, "template_id", tplDbId);
        cache_get_value_name_int(i, "quantity", quantity);
        cache_get_value_name_int(i, "slot", slot);
        cache_get_value_name(i, "metadata", meta, MAX_METADATA_LEN);

        new tplIdx = Inv_FindTemplateByDbId(tplDbId);
        if (tplIdx == INVALID_TEMPLATE_IDX) continue;

        if (slot < 0 || slot >= MAX_ITEMS_PER_CONTAINER)
            slot = Inv_FindFreeSlot(containerIdx);
        if (slot == -1) continue;

        g_Items[containerIdx][slot][ci_DbId]        = dbId;
        g_Items[containerIdx][slot][ci_TemplateIdx]  = tplIdx;
        g_Items[containerIdx][slot][ci_Quantity]     = quantity;
        g_Items[containerIdx][slot][ci_Slot]         = slot;
        g_Items[containerIdx][slot][ci_Metadata][0]  = EOS;
        strcat(g_Items[containerIdx][slot][ci_Metadata], meta, MAX_METADATA_LEN);
        g_Items[containerIdx][slot][ci_Active]       = true;
        g_Items[containerIdx][slot][ci_Dirty]        = false;
        g_Items[containerIdx][slot][ci_Deleted]      = false;
    }
}

/// Get the DB id of a container (for storing in bridge module's own mapping).
stock Inv_GetContainerDbId(containerIdx)
{
    if (containerIdx < 0 || containerIdx >= MAX_CONTAINERS) return -1;
    if (!g_Containers[containerIdx][cnt_Active]) return -1;
    return g_Containers[containerIdx][cnt_DbId];
}

// =============================================================================
//  Auto-save timer
// =============================================================================

forward Inv_AutoSave();
public Inv_AutoSave()
{
    new count = 0;
    for (new i = 0; i < MAX_CONTAINERS; i++)
    {
        if (g_Containers[i][cnt_Active])
        {
            Inv_Save(i);
            count++;
        }
    }
    if (count > 0)
        printf("[INV] Auto-save: saved %d container(s).", count);
}

// Player commands moved to mod_player_inv.inc

// sscanf-lite: parse "d" and "dD(default)" from params
// Pawn doesn't have sscanf natively — this is a minimal parser
stock sscanf(const params[], const format[], &val1, &val2 = 0)
{
    if (format[0] == 'd' && format[1] == EOS)
    {
        if (params[0] == EOS) return 1;
        val1 = strval(params);
        return 0;
    }
    if (format[0] == 'd' && format[1] == 'D')
    {
        if (params[0] == EOS) return 1;
        val1 = strval(params);
        new pos = 0;
        while (params[pos] != EOS && params[pos] != ' ') pos++;
        if (params[pos] == ' ')
        {
            pos++;
            while (params[pos] == ' ') pos++;
            if (params[pos] != EOS)
                val2 = strval(params[pos]);
        }
        return 0;
    }
    return 1;
}
