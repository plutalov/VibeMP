// =============================================================================
//  mod_spawn.inc — Spawn management
//
//  Handles:
//    - Class selection bypass (auto-spawn after data loaded)
//    - Blocking spawn for unauthenticated / unloaded players
//    - Applying saved position via SetSpawnInfo before SpawnPlayer
//    - Setting health/armor post-spawn (engine resets these)
//
//  Depends on: mod_auth (Auth_IsLoggedIn)
//              mod_playerdata (PData_IsLoaded, PData_GetSavedPos, PData_GetSavedHealth)
// =============================================================================

#if defined _mod_spawn_included
    #endinput
#endif
#define _mod_spawn_included

// -----------------------------------------------------------------------------
//  Default spawn configuration (used for new accounts and respawns)
// -----------------------------------------------------------------------------

#define SPAWN_X             (-2233.9700)
#define SPAWN_Y             (-1737.5800)
#define SPAWN_Z             (480.5500)
#define SPAWN_A             (0.0)
#define SPAWN_SKIN          (0)
#define SPAWN_INTERIOR      (0)
#define SPAWN_VIRTUAL_WORLD (0)

// Track whether this is the player's first spawn after login
static bool:g_FirstSpawn[MAX_PLAYERS];

// Pending health/armor to apply post-spawn (engine resets these on SpawnPlayer)
static Float:g_PendingHealth[MAX_PLAYERS];
static Float:g_PendingArmor[MAX_PLAYERS];

// -----------------------------------------------------------------------------
//  Init / Destroy
// -----------------------------------------------------------------------------

stock Spawn_Init()
{
    EventBus_RegisterModule("Spawn");

    EventBus_Subscribe(EVT_PLAYER_CONNECT,        "Spawn_OnPlayerConnect");
    EventBus_Subscribe(EVT_PLAYER_REQUEST_CLASS,   "Spawn_OnRequestClass");
    EventBus_Subscribe(EVT_PLAYER_REQUEST_SPAWN,   "Spawn_OnRequestSpawn");
    EventBus_Subscribe(EVT_PLAYER_SPAWN,           "Spawn_OnPlayerSpawn");
    EventBus_Subscribe(EVT_PLAYER_DATA_LOADED,     "Spawn_OnDataLoaded");

    print("[MOD] Spawn module initialized.");
}

stock Spawn_Destroy()
{
    print("[MOD] Spawn module destroyed.");
}

// -----------------------------------------------------------------------------
//  Event Handlers
// -----------------------------------------------------------------------------

forward Spawn_OnPlayerConnect();
public Spawn_OnPlayerConnect()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    g_FirstSpawn[playerid] = true;
    g_PendingHealth[playerid] = 100.0;
    g_PendingArmor[playerid] = 0.0;
    return 1;
}

forward Spawn_OnRequestClass();
public Spawn_OnRequestClass()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // If already logged in and data loaded (e.g. respawn after death),
    // skip class selection entirely
    if (Auth_IsLoggedIn(playerid) && PData_IsLoaded(playerid))
    {
        SpawnPlayer(playerid);
    }

    // Block the class selection screen from showing
    EventBus_SetResult(0);
    return 1;
}

forward Spawn_OnRequestSpawn();
public Spawn_OnRequestSpawn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Block spawning until logged in AND data loaded
    if (!Auth_IsLoggedIn(playerid) || !PData_IsLoaded(playerid))
    {
        EventBus_SetResult(0);
    }

    return 1;
}

// --- Data loaded: set correct spawn info, then spawn ---
forward Spawn_OnDataLoaded();
public Spawn_OnDataLoaded()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Get saved position from DB (or defaults for new accounts)
    new Float:x, Float:y, Float:z, Float:a;
    PData_GetSavedPos(playerid, x, y, z, a);

    // Store health/armor to apply after spawn (engine resets these)
    PData_GetSavedHealth(playerid, g_PendingHealth[playerid], g_PendingArmor[playerid]);

    // Set spawn info with the CORRECT position, then spawn
    SetSpawnInfo(playerid, NO_TEAM, SPAWN_SKIN,
        x, y, z, a,
        WEAPON_FIST, 0, WEAPON_FIST, 0, WEAPON_FIST, 0);
    SpawnPlayer(playerid);
    return 1;
}

// --- Post-spawn: apply health/armor and welcome message ---
forward Spawn_OnPlayerSpawn();
public Spawn_OnPlayerSpawn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    if (g_FirstSpawn[playerid])
    {
        // First spawn after login — apply saved health/armor
        SetPlayerHealth(playerid, g_PendingHealth[playerid]);
        SetPlayerArmour(playerid, g_PendingArmor[playerid]);
        g_FirstSpawn[playerid] = false;

        // Set spawn info for future respawns (death) — engine handles it natively
        SetSpawnInfo(playerid, NO_TEAM, SPAWN_SKIN,
            SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A,
            WEAPON_FIST, 0, WEAPON_FIST, 0, WEAPON_FIST, 0);
    }

    SendClientMessage(playerid, 0xFFFFFFFF, "Welcome! Use /help for commands.");
    return 1;
}
