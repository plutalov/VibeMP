// =============================================================================
//  mod_player_inv.inc — Player ↔ inventory bridge
//
//  Manages the player's personal inventory container: creates/loads it on
//  login, saves on logout, frees on disconnect. Provides player-specific
//  convenience wrappers and commands.
//
//  Follows the same bridge pattern as mod_veh_trunk: subscribes to player
//  lifecycle events and calls generic inventory API.
//
//  Depends on: mod_inventory, mod_inventory_ui, mod_auth, mod_db
// =============================================================================

#if defined _mod_player_inv_included
    #endinput
#endif
#define _mod_player_inv_included

#define PLAYERINV_DEFAULT_WEIGHT    30.0

// -----------------------------------------------------------------------------
//  Per-player state
// -----------------------------------------------------------------------------

static g_PlayerContainerIdx[MAX_PLAYERS];
static bool:g_InvLoaded[MAX_PLAYERS];

// =============================================================================
//  Public API — used by UI, commands, domain modules (food, phone, etc.)
// =============================================================================

stock PlayerInv_GetContainerIdx(playerid)
{
    if (playerid < 0 || playerid >= MAX_PLAYERS) return INVALID_CONTAINER_IDX;
    return g_PlayerContainerIdx[playerid];
}

stock bool:PlayerInv_IsLoaded(playerid)
{
    if (playerid < 0 || playerid >= MAX_PLAYERS) return false;
    return g_InvLoaded[playerid];
}

stock PlayerInv_AddItem(playerid, templateIdx, quantity, const metadata[])
{
    new cIdx = PlayerInv_GetContainerIdx(playerid);
    if (cIdx == INVALID_CONTAINER_IDX) return -1;
    return Inv_AddItem(cIdx, templateIdx, quantity, metadata);
}

stock PlayerInv_RemoveItem(playerid, slotIdx, quantity)
{
    new cIdx = PlayerInv_GetContainerIdx(playerid);
    if (cIdx == INVALID_CONTAINER_IDX) return 0;
    return Inv_RemoveItem(cIdx, slotIdx, quantity);
}

/// Use an item from the player's own container.
stock PlayerInv_UseItem(playerid, slotIdx)
{
    new cIdx = PlayerInv_GetContainerIdx(playerid);
    if (cIdx == INVALID_CONTAINER_IDX) return 0;
    return Inv_UseItemFromContainer(playerid, cIdx, slotIdx);
}

// =============================================================================
//  Init / Destroy
// =============================================================================

stock PlayerInv_Init()
{
    EventBus_RegisterModule("PlayerInv");
    EventBus_Subscribe(EVT_PLAYER_LOGIN,      "PlayerInv_OnLogin");
    EventBus_Subscribe(EVT_PLAYER_LOGOUT,     "PlayerInv_OnLogout");
    EventBus_Subscribe(EVT_PLAYER_DISCONNECT, "PlayerInv_OnDisconn");
    EventBus_Subscribe(EVT_PLAYER_HELP,       "PlayerInv_OnHelp");

    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        g_PlayerContainerIdx[i] = INVALID_CONTAINER_IDX;
        g_InvLoaded[i] = false;
    }

    print("[MOD] PlayerInv module initialized.");
}

stock PlayerInv_Destroy()
{
    // Save all player inventories (blocking — server shutting down)
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && g_InvLoaded[i])
            Inv_SaveAndFreeContainer(g_PlayerContainerIdx[i], true);
    }
    print("[MOD] PlayerInv module destroyed.");
}

// =============================================================================
//  Event Handlers
// =============================================================================

forward PlayerInv_OnLogin();
public PlayerInv_OnLogin()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    new accountId = Auth_GetAccountId(playerid);
    if (accountId < 0) return 1;

    new query[256];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "SELECT id, max_weight FROM containers WHERE owner_type = 'player' AND owner_id = %d LIMIT 1",
        accountId);
    mysql_tquery(DB_GetHandle(), query, "PlayerInv_OnContainerLoad", "d", playerid);
    return 1;
}

forward PlayerInv_OnContainerLoad(playerid);
public PlayerInv_OnContainerLoad(playerid)
{
    if (!IsPlayerConnected(playerid)) return;

    new rows;
    cache_get_row_count(rows);

    if (rows > 0)
    {
        new containerIdx = Inv_AllocContainer();
        if (containerIdx == -1)
        {
            printf("[PLAYERINV] ERROR: No free container slots for playerid=%d", playerid);
            return;
        }

        Inv_SetContainerFromCache(containerIdx, 0, OWNER_PLAYER, Auth_GetAccountId(playerid));
        g_PlayerContainerIdx[playerid] = containerIdx;

        // Load items
        new dbId;
        cache_get_value_name_int(0, "id", dbId);
        new query[128];
        mysql_format(DB_GetHandle(), query, sizeof(query),
            "SELECT id, template_id, quantity, slot, metadata FROM container_items WHERE container_id = %d",
            dbId);
        mysql_tquery(DB_GetHandle(), query, "PlayerInv_OnItemsLoad", "dd", playerid, containerIdx);
    }
    else
    {
        // New player — create container
        new accountId = Auth_GetAccountId(playerid);
        new query[128];
        mysql_format(DB_GetHandle(), query, sizeof(query),
            "INSERT INTO containers (owner_type, owner_id, max_weight) VALUES ('player', %d, %f)",
            accountId, PLAYERINV_DEFAULT_WEIGHT);
        mysql_tquery(DB_GetHandle(), query, "PlayerInv_OnContainerCreated", "d", playerid);
    }
}

forward PlayerInv_OnContainerCreated(playerid);
public PlayerInv_OnContainerCreated(playerid)
{
    if (!IsPlayerConnected(playerid)) return;

    new containerIdx = Inv_AllocContainer();
    if (containerIdx == -1)
    {
        printf("[PLAYERINV] ERROR: No free container slots for new player %d", playerid);
        return;
    }

    Inv_InitContainer(containerIdx, cache_insert_id(), OWNER_PLAYER,
        Auth_GetAccountId(playerid), PLAYERINV_DEFAULT_WEIGHT);

    g_PlayerContainerIdx[playerid] = containerIdx;
    g_InvLoaded[playerid] = true;

    printf("[PLAYERINV] Created container (db=%d) for playerid=%d",
        Inv_GetContainerDbId(containerIdx), playerid);

    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_INV_PLAYER_LOADED);
}

forward PlayerInv_OnItemsLoad(playerid, containerIdx);
public PlayerInv_OnItemsLoad(playerid, containerIdx)
{
    if (!IsPlayerConnected(playerid)) return;

    Inv_LoadItemsFromCache(containerIdx);

    g_InvLoaded[playerid] = true;

    new rows;
    cache_get_row_count(rows);
    printf("[PLAYERINV] Loaded %d items for playerid=%d", rows, playerid);

    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_INV_PLAYER_LOADED);
}

forward PlayerInv_OnLogout();
public PlayerInv_OnLogout()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    if (g_InvLoaded[playerid])
        Inv_Save(g_PlayerContainerIdx[playerid]);
    return 1;
}

forward PlayerInv_OnDisconn();
public PlayerInv_OnDisconn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new cIdx = g_PlayerContainerIdx[playerid];
    if (cIdx != INVALID_CONTAINER_IDX)
        Inv_SaveAndFreeContainer(cIdx);
    g_PlayerContainerIdx[playerid] = INVALID_CONTAINER_IDX;
    g_InvLoaded[playerid] = false;
    return 1;
}

forward PlayerInv_OnHelp();
public PlayerInv_OnHelp()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    SendClientMessage(playerid, 0xCCCCCCFF, "/inv  /giveitem  /useitem");
    return 1;
}

// =============================================================================
//  Commands
// =============================================================================

forward cmd_inv(playerid, params[]);
public cmd_inv(playerid, params[])
{
    #pragma unused params
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in."), 1;
    if (!g_InvLoaded[playerid])
        return SendClientMessage(playerid, 0xFF4444FF, "Inventory not loaded yet."), 1;

    InvUI_Open(playerid, g_PlayerContainerIdx[playerid]);
    return 1;
}

forward cmd_giveitem(playerid, params[]);
public cmd_giveitem(playerid, params[])
{
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in."), 1;
    if (!Admin_RequireLevel(playerid, 2)) return 1;
    if (!g_InvLoaded[playerid])
        return SendClientMessage(playerid, 0xFF4444FF, "Inventory not loaded yet."), 1;

    new templateDbId, quantity = 1;
    if (sscanf(params, "dD(1)", templateDbId, quantity))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /giveitem <template_db_id> [quantity]"), 1;

    new templateIdx = Inv_FindTemplateByDbId(templateDbId);
    if (templateIdx == INVALID_TEMPLATE_IDX)
        return SendClientMessage(playerid, 0xFF4444FF, "Unknown template ID."), 1;

    new slot = PlayerInv_AddItem(playerid, templateIdx, quantity, "");
    if (slot == -1)
        return SendClientMessage(playerid, 0xFF4444FF, "Could not add item (full or too heavy)."), 1;

    new name[MAX_ITEM_NAME_LEN];
    Inv_GetTemplateName(templateIdx, name, sizeof(name));

    new msg[128];
    format(msg, sizeof(msg), "Added %dx %s to your inventory.", quantity, name);
    SendClientMessage(playerid, 0x44FF44FF, msg);
    return 1;
}

forward cmd_useitem(playerid, params[]);
public cmd_useitem(playerid, params[])
{
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in."), 1;
    if (!g_InvLoaded[playerid])
        return SendClientMessage(playerid, 0xFF4444FF, "Inventory not loaded yet."), 1;

    new slotIdx;
    if (sscanf(params, "d", slotIdx))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /useitem <slot>"), 1;

    new cIdx = g_PlayerContainerIdx[playerid];
    if (!Inv_IsSlotActive(cIdx, slotIdx))
        return SendClientMessage(playerid, 0xFF4444FF, "Nothing in that slot."), 1;

    PlayerInv_UseItem(playerid, slotIdx);
    return 1;
}
