// =============================================================================
//  mod_playerdata.inc — Player data persistence (position, health, score, etc.)
//
//  Loads player data from DB after login, saves on disconnect + auto-save.
//  This module does NOT handle authentication — that's mod_auth's job.
//
//  Listens to:
//    EVT_PLAYER_LOGIN       — triggers data load from DB
//    EVT_PLAYER_LOGOUT      — triggers data save to DB
//    EVT_PLAYER_DISCONNECT  — resets per-player state
//
//  Emits:
//    EVT_PLAYER_DATA_LOADED — after data loaded from DB and applied
//
//  Public API:
//    PData_IsLoaded(playerid)                       — true if data loaded
//    PData_GetSavedPos(playerid, &x, &y, &z, &a)   — get cached position
//    PData_GetSavedHealth(playerid, &hp, &arm)      — get cached health/armor
//    PData_Save(playerid)                           — force save now
//
//  Depends on: mod_db (DB_GetHandle), mod_auth (Auth_GetAccountId, Auth_IsLoggedIn)
// =============================================================================

#if defined _mod_playerdata_included
    #endinput
#endif
#define _mod_playerdata_included

// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define AUTOSAVE_INTERVAL   (300000) // 5 minutes in ms

// -----------------------------------------------------------------------------
//  Per-player data cache (loaded from DB, saved back on disconnect/timer)
// -----------------------------------------------------------------------------

enum E_PLAYER_DATA
{
    pd_Score,
    pd_Money,
    pd_Skin,
    Float:pd_PosX,
    Float:pd_PosY,
    Float:pd_PosZ,
    Float:pd_PosA,
    Float:pd_Health,
    Float:pd_Armor,
    pd_Interior,
    pd_VWorld
}

static g_PlayerData[MAX_PLAYERS][E_PLAYER_DATA];
static bool:g_PlayerDataLoaded[MAX_PLAYERS];

// Auto-save timer
static g_AutoSaveTimer = -1;

// -----------------------------------------------------------------------------
//  Public API
// -----------------------------------------------------------------------------

stock bool:PData_IsLoaded(playerid)
{
    if (playerid < 0 || playerid >= MAX_PLAYERS) return false;
    return g_PlayerDataLoaded[playerid];
}

stock PData_GetSavedPos(playerid, &Float:x, &Float:y, &Float:z, &Float:a)
{
    x = g_PlayerData[playerid][pd_PosX];
    y = g_PlayerData[playerid][pd_PosY];
    z = g_PlayerData[playerid][pd_PosZ];
    a = g_PlayerData[playerid][pd_PosA];
}

stock PData_GetSavedHealth(playerid, &Float:hp, &Float:arm)
{
    hp  = g_PlayerData[playerid][pd_Health];
    arm = g_PlayerData[playerid][pd_Armor];
}

stock PData_Save(playerid)
{
    if (!Auth_IsLoggedIn(playerid) || !g_PlayerDataLoaded[playerid])
        return;

    // Read current state from player
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    // Don't save if player is at origin (not properly spawned yet)
    if (x == 0.0 && y == 0.0 && z == 0.0)
        return;

    new Float:hp, Float:arm;
    GetPlayerHealth(playerid, hp);
    GetPlayerArmour(playerid, arm);

    new query[512];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "UPDATE accounts SET score=%d, money=%d, skin=%d, \
         pos_x=%f, pos_y=%f, pos_z=%f, pos_a=%f, \
         health=%f, armor=%f, interior=%d, vworld=%d \
         WHERE id=%d",
        GetPlayerScore(playerid), GetPlayerMoney(playerid), GetPlayerSkin(playerid),
        x, y, z, a, hp, arm,
        GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid),
        Auth_GetAccountId(playerid));

    // Fire and forget
    mysql_tquery(DB_GetHandle(), query);
}

// -----------------------------------------------------------------------------
//  Internal: load player data from DB
// -----------------------------------------------------------------------------

static stock PData_Load(playerid)
{
    new query[256];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "SELECT score, money, skin, pos_x, pos_y, pos_z, pos_a, \
         health, armor, interior, vworld FROM accounts WHERE id = %d",
        Auth_GetAccountId(playerid));
    mysql_tquery(DB_GetHandle(), query, "PData_OnDataLoad", "d", playerid);
}

// -----------------------------------------------------------------------------
//  Init / Destroy
// -----------------------------------------------------------------------------

stock PData_Init()
{
    EventBus_RegisterModule("PlayerData");

    EventBus_Subscribe(EVT_PLAYER_LOGIN,      "PData_OnPlayerLogin");
    EventBus_Subscribe(EVT_PLAYER_LOGOUT,     "PData_OnPlayerLogout");
    EventBus_Subscribe(EVT_PLAYER_DISCONNECT, "PData_OnPlayerDisconn");

    // Auto-save timer
    g_AutoSaveTimer = SetTimer("PData_AutoSave", AUTOSAVE_INTERVAL, true);

    print("[MOD] PlayerData module initialized.");
}

stock PData_Destroy()
{
    if (g_AutoSaveTimer != -1)
    {
        KillTimer(g_AutoSaveTimer);
        g_AutoSaveTimer = -1;
    }

    // Save all logged-in players before shutdown
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && Auth_IsLoggedIn(i) && g_PlayerDataLoaded[i])
            PData_Save(i);
    }

    print("[MOD] PlayerData module destroyed.");
}

// -----------------------------------------------------------------------------
//  Auto-save timer
// -----------------------------------------------------------------------------

forward PData_AutoSave();
public PData_AutoSave()
{
    new count = 0;
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && Auth_IsLoggedIn(i) && g_PlayerDataLoaded[i])
        {
            PData_Save(i);
            count++;
        }
    }
    if (count > 0)
        printf("[PDATA] Auto-save: saved %d player(s).", count);
}

// -----------------------------------------------------------------------------
//  Event Bus Handlers
// -----------------------------------------------------------------------------

// --- On login: load player data from DB ---
forward PData_OnPlayerLogin();
public PData_OnPlayerLogin()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    printf("[PDATA] OnPlayerLogin: loading data for playerid=%d", playerid);
    PData_Load(playerid);
    return 1;
}

// --- On logout: save player data ---
forward PData_OnPlayerLogout();
public PData_OnPlayerLogout()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    if (g_PlayerDataLoaded[playerid])
    {
        printf("[PDATA] OnPlayerLogout: saving data for playerid=%d", playerid);
        PData_Save(playerid);
    }
    return 1;
}

// --- On disconnect: reset state ---
forward PData_OnPlayerDisconn();
public PData_OnPlayerDisconn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    g_PlayerDataLoaded[playerid] = false;
    return 1;
}

// =============================================================================
//  ASYNC CALLBACK — called by MySQL plugin, NOT event bus
// =============================================================================

forward PData_OnDataLoad(playerid);
public PData_OnDataLoad(playerid)
{
    if (!IsPlayerConnected(playerid)) return;

    new rows;
    cache_get_row_count(rows);
    printf("[PDATA] OnDataLoad: playerid=%d, rows=%d", playerid, rows);

    if (rows == 0)
    {
        // New account — mark as loaded with defaults
        printf("[PDATA] OnDataLoad: no rows, using DB defaults");
        g_PlayerDataLoaded[playerid] = true;
        EventBus_SetInt(EVD_PLAYER_ID, playerid);
        EventBus_Emit(EVT_PLAYER_DATA_LOADED);
        return;
    }

    // Read from DB into cache
    cache_get_value_name_int(0, "score",    g_PlayerData[playerid][pd_Score]);
    cache_get_value_name_int(0, "money",    g_PlayerData[playerid][pd_Money]);
    cache_get_value_name_int(0, "skin",     g_PlayerData[playerid][pd_Skin]);
    cache_get_value_name_float(0, "pos_x",  g_PlayerData[playerid][pd_PosX]);
    cache_get_value_name_float(0, "pos_y",  g_PlayerData[playerid][pd_PosY]);
    cache_get_value_name_float(0, "pos_z",  g_PlayerData[playerid][pd_PosZ]);
    cache_get_value_name_float(0, "pos_a",  g_PlayerData[playerid][pd_PosA]);
    cache_get_value_name_float(0, "health", g_PlayerData[playerid][pd_Health]);
    cache_get_value_name_float(0, "armor",  g_PlayerData[playerid][pd_Armor]);
    cache_get_value_name_int(0, "interior", g_PlayerData[playerid][pd_Interior]);
    cache_get_value_name_int(0, "vworld",   g_PlayerData[playerid][pd_VWorld]);

    printf("[PDATA] OnDataLoad: pos=%.2f,%.2f,%.2f hp=%.1f",
        g_PlayerData[playerid][pd_PosX], g_PlayerData[playerid][pd_PosY],
        g_PlayerData[playerid][pd_PosZ], g_PlayerData[playerid][pd_Health]);

    // Apply non-positional data immediately
    SetPlayerScore(playerid, g_PlayerData[playerid][pd_Score]);
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, g_PlayerData[playerid][pd_Money]);
    SetPlayerSkin(playerid, g_PlayerData[playerid][pd_Skin]);
    SetPlayerInterior(playerid, g_PlayerData[playerid][pd_Interior]);
    SetPlayerVirtualWorld(playerid, g_PlayerData[playerid][pd_VWorld]);

    g_PlayerDataLoaded[playerid] = true;

    // Emit data loaded event — mod_spawn listens for this
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_PLAYER_DATA_LOADED);
}
