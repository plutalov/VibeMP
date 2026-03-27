// =============================================================================
//  mod_spawn.inc — Spawn management
//
//  Handles:
//    - Class selection bypass for logged-in players
//    - Blocking spawn for unauthenticated players
//    - Setting player position/interior/world on spawn
//    - Triggering spawn after login/register
//
//  Depends on: mod_auth (Auth_IsLoggedIn)
// =============================================================================

#if defined _mod_spawn_included
    #endinput
#endif
#define _mod_spawn_included

// -----------------------------------------------------------------------------
//  Spawn configuration
// -----------------------------------------------------------------------------

#define SPAWN_X             (-2233.9700)
#define SPAWN_Y             (-1737.5800)
#define SPAWN_Z             (480.5500)
#define SPAWN_A             (0.0)
#define SPAWN_SKIN          (0)
#define SPAWN_INTERIOR      (0)
#define SPAWN_VIRTUAL_WORLD (0)

// -----------------------------------------------------------------------------
//  Init / Destroy
// -----------------------------------------------------------------------------

stock Spawn_Init()
{
    EventBus_RegisterModule("Spawn");

    EventBus_Subscribe(EVT_PLAYER_REQUEST_CLASS, "Spawn_OnRequestClass");
    EventBus_Subscribe(EVT_PLAYER_REQUEST_SPAWN, "Spawn_OnRequestSpawn");
    EventBus_Subscribe(EVT_PLAYER_SPAWN,         "Spawn_OnPlayerSpawn");
    EventBus_Subscribe(EVT_PLAYER_LOGIN,         "Spawn_OnPlayerLogin");
    EventBus_Subscribe(EVT_PLAYER_REGISTER,      "Spawn_OnPlayerRegister");

    print("[MOD] Spawn module initialized.");
}

stock Spawn_Destroy()
{
    print("[MOD] Spawn module destroyed.");
}

// -----------------------------------------------------------------------------
//  Event Handlers
// -----------------------------------------------------------------------------

forward Spawn_OnRequestClass();
public Spawn_OnRequestClass()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Skip class selection for logged-in players
    if (Auth_IsLoggedIn(playerid))
    {
        SetSpawnInfo(playerid, NO_TEAM, SPAWN_SKIN,
            SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A,
            WEAPON_FIST, 0, WEAPON_FIST, 0, WEAPON_FIST, 0);
        SpawnPlayer(playerid);
    }

    EventBus_SetResult(0);
    return 1;
}

forward Spawn_OnRequestSpawn();
public Spawn_OnRequestSpawn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Block spawning until logged in
    if (!Auth_IsLoggedIn(playerid))
    {
        EventBus_SetResult(0);
    }

    return 1;
}

forward Spawn_OnPlayerSpawn();
public Spawn_OnPlayerSpawn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    SetPlayerInterior(playerid, SPAWN_INTERIOR);
    SetPlayerVirtualWorld(playerid, SPAWN_VIRTUAL_WORLD);
    SetPlayerPos(playerid, SPAWN_X, SPAWN_Y, SPAWN_Z);
    SetPlayerFacingAngle(playerid, SPAWN_A);
    SetCameraBehindPlayer(playerid);

    SendClientMessage(playerid, 0xFFFFFFFF, "Welcome! Use /help for commands.");
    return 1;
}

forward Spawn_OnPlayerLogin();
public Spawn_OnPlayerLogin()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    SetPlayerSkin(playerid, SPAWN_SKIN);
    SpawnPlayer(playerid);
    return 1;
}

forward Spawn_OnPlayerRegister();
public Spawn_OnPlayerRegister()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    SetPlayerSkin(playerid, SPAWN_SKIN);
    SpawnPlayer(playerid);
    return 1;
}
