// =============================================================================
//  mygamemode.pwn — Thin bridge shell
//
//  This file does three things:
//    1. Includes the framework and all modules
//    2. Routes SA-MP callbacks into the event bus
//    3. Initializes/destroys modules in order
//
//  ALL game logic lives in modules (includes/modules/mod_*.inc).
//  Do not add game logic here.
// =============================================================================

#define MIXED_SPELLINGS

#include <open.mp>
#include <a_mysql>
#include <samp_bcrypt>

// --- Core framework ---
#include <core/events>
#include <core/eventbus>
#include <core/module>
#include <core/testing>
#include <core/commands>

// --- Modules (order matters: dependencies first) ---
#include <modules/mod_debug>
#include <modules/mod_db>
#include <modules/mod_auth>
#include <modules/mod_playerdata>
#include <modules/mod_admin>
#include <modules/mod_spawn>

// =============================================================================
//  SA-MP CALLBACK BRIDGE
//  Each callback sets event data, then emits the corresponding event.
// =============================================================================

public OnGameModeInit()
{
    // --- Framework init ---
    EventBus_Init();

    // --- Core init ---
    Cmd_Init();

    // --- Module init (debug first so it can log everything) ---
    Debug_Init();
    DB_Init();
    Auth_Init();
    PData_Init();
    Admin_Init();
    Spawn_Init();

    // --- Server setup ---
    SetGameModeText("RPG Server");
    ShowPlayerMarkers(PLAYER_MARKERS_MODE_GLOBAL);
    ShowNameTags(true);
    EnableStuntBonusForAll(false);
    DisableInteriorEnterExits();

    // Single class — spawn position is overridden by mod_spawn after login.
    AddPlayerClass(0, -2233.97, -1737.58, 480.55, 0.0,
        WEAPON_FIST, 0, WEAPON_FIST, 0, WEAPON_FIST, 0);

    // --- Emit init event ---
    EventBus_Emit(EVT_GAME_INIT);

    print("[GM] Gamemode initialized.");
    return 1;
}

public OnGameModeExit()
{
    EventBus_Emit(EVT_GAME_EXIT);

    // Destroy in reverse order
    Spawn_Destroy();
    Admin_Destroy();
    PData_Destroy();
    Auth_Destroy();
    DB_Destroy();
    Debug_Destroy();

    print("[GM] Gamemode exiting.");
    return 1;
}

public OnPlayerConnect(playerid)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_PLAYER_CONNECT);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_VALUE, reason);
    EventBus_Emit(EVT_PLAYER_DISCONNECT);
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, classid);
    EventBus_Emit(EVT_PLAYER_REQUEST_CLASS);
    return EventBus_GetResult();
}

public OnPlayerRequestSpawn(playerid)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_PLAYER_REQUEST_SPAWN);
    return EventBus_GetResult();
}

public OnPlayerSpawn(playerid)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_PLAYER_SPAWN);
    return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, killerid);
    EventBus_SetInt(EVD_VALUE, _:reason);
    EventBus_Emit(EVT_PLAYER_DEATH);
    return 1;
}

public OnPlayerText(playerid, text[])
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetString(text);
    EventBus_Emit(EVT_PLAYER_TEXT);
    return EventBus_GetResult();
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetString(cmdtext);
    EventBus_Emit(EVT_PLAYER_COMMAND);
    return EventBus_GetResult();
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, dialogid);
    EventBus_SetInt(EVD_VALUE, response);
    EventBus_SetInt(EVD_EXTRA, listitem);
    EventBus_SetString(inputtext);
    EventBus_Emit(EVT_DIALOG_RESPONSE);
    return 1;
}

public OnPlayerStateChange(playerid, PLAYER_STATE:newstate, PLAYER_STATE:oldstate)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_VALUE, _:newstate);
    EventBus_SetInt(EVD_EXTRA, _:oldstate);
    EventBus_Emit(EVT_PLAYER_STATE_CHANGE);
    return 1;
}

public OnPlayerEnterVehicle(playerid, vehicleid, ispassenger)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, vehicleid);
    EventBus_SetInt(EVD_VALUE, ispassenger);
    EventBus_Emit(EVT_PLAYER_ENTER_VEHICLE);
    return 1;
}

public OnPlayerExitVehicle(playerid, vehicleid)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, vehicleid);
    EventBus_Emit(EVT_PLAYER_EXIT_VEHICLE);
    return 1;
}

public OnPlayerInteriorChange(playerid, newinteriorid, oldinteriorid)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_VALUE, newinteriorid);
    EventBus_SetInt(EVD_EXTRA, oldinteriorid);
    EventBus_Emit(EVT_PLAYER_INTERIOR_CHANGE);
    return 1;
}

public OnPlayerKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_VALUE, _:newkeys);
    EventBus_SetInt(EVD_EXTRA, _:oldkeys);
    EventBus_Emit(EVT_PLAYER_KEY_STATE_CHANGE);
    return 1;
}

public OnPlayerStreamIn(playerid, forplayerid)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, forplayerid);
    EventBus_Emit(EVT_PLAYER_STREAM_IN);
    return 1;
}

public OnPlayerStreamOut(playerid, forplayerid)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, forplayerid);
    EventBus_Emit(EVT_PLAYER_STREAM_OUT);
    return 1;
}

public OnPlayerClickMap(playerid, Float:fX, Float:fY, Float:fZ)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetFloat(0, fX);
    EventBus_SetFloat(1, fY);
    EventBus_SetFloat(2, fZ);
    EventBus_Emit(EVT_PLAYER_CLICK_MAP);
    return 1;
}
