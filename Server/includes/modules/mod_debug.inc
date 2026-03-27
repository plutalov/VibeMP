// =============================================================================
//  mod_debug.inc — Debug, introspection, and utility commands
//
//  Commands:
//    /help     — show command list
//    /stats    — show player stats
//    /debug    — toggle event bus debug logging
//    /events   — dump all registered handlers to console
//    /test     — run all module tests
// =============================================================================

#if defined _mod_debug_included
    #endinput
#endif
#define _mod_debug_included

stock Debug_Init()
{
    EventBus_RegisterModule("Debug");
    EventBus_Subscribe(EVT_PLAYER_HELP, "Debug_OnPlayerHelp");
    print("[MOD] Debug module initialized.");
}

// --- Help handler: print debug/utility commands ---
forward Debug_OnPlayerHelp();
public Debug_OnPlayerHelp()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    SendClientMessage(playerid, 0xCCCCCCFF, "/help  /stats  /debug  /events  /test");
    return 1;
}

stock Debug_Destroy()
{
    print("[MOD] Debug module destroyed.");
}

// -----------------------------------------------------------------------------
//  Commands
// -----------------------------------------------------------------------------

forward cmd_help(playerid, params[]);
public cmd_help(playerid, params[])
{
    #pragma unused params
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in to use commands."), 1;

    SendClientMessage(playerid, 0xFFFFFFFF, "---- Commands ----");

    // Each module prints its own commands
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_PLAYER_HELP);
    return 1;
}


forward cmd_stats(playerid, params[]);
public cmd_stats(playerid, params[])
{
    #pragma unused params
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in to use commands."), 1;

    new name[MAX_PLAYER_NAME + 1];
    GetPlayerName(playerid, name, sizeof(name));

    new msg[128];
    format(msg, sizeof(msg), "Stats for %s | Score: %d | Money: $%d",
        name, GetPlayerScore(playerid), GetPlayerMoney(playerid));
    SendClientMessage(playerid, 0xFFFFFFFF, msg);
    return 1;
}

forward cmd_debug(playerid, params[]);
public cmd_debug(playerid, params[])
{
    #pragma unused params
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in to use commands."), 1;

    g_DebugEvents = !g_DebugEvents;
    new msg[64];
    format(msg, sizeof(msg), "Event debug logging: %s",
        g_DebugEvents ? ("ON") : ("OFF"));
    SendClientMessage(playerid, 0xFFFF00FF, msg);
    return 1;
}

forward cmd_events(playerid, params[]);
public cmd_events(playerid, params[])
{
    #pragma unused params
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in to use commands."), 1;

    EventBus_DumpHandlers();
    SendClientMessage(playerid, 0xFFFF00FF, "Handler dump printed to server console.");
    return 1;
}

forward cmd_test(playerid, params[]);
public cmd_test(playerid, params[])
{
    #pragma unused params
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in to use commands."), 1;

    SendClientMessage(playerid, 0xFFFF00FF, "Running tests... check server console.");
    Test_Init();

    // -- EventBus self-tests --
    Test_Assert("EventBus: handlers registered for PLAYER_CONNECT",
        g_HandlerCount[EVT_PLAYER_CONNECT] > 0);

    EventBus_SetInt(0, 42);
    Test_AssertEqual("EventBus: SetInt/GetInt round-trip",
        EventBus_GetInt(0), 42);

    EventBus_SetFloat(0, 3.14);
    Test_Assert("EventBus: SetFloat/GetFloat round-trip",
        EventBus_GetFloat(0) > 3.0);

    EventBus_SetString("hello");
    new buf[32];
    EventBus_GetString(buf, sizeof(buf));
    Test_Assert("EventBus: SetString/GetString round-trip",
        strcmp(buf, "hello") == 0);

    // -- Database module tests --
    Test_Assert("DB: IsConnected returns true",
        DB_IsConnected());

    Test_Assert("DB: GetHandle is valid",
        _:DB_GetHandle() != 0);

    // -- Auth module tests --
    Test_Assert("Auth: IsLoggedIn(-1) returns false",
        !Auth_IsLoggedIn(-1));

    Test_Assert("PData: IsLoaded(-1) returns false",
        !PData_IsLoaded(-1));

    Test_Assert("Auth: GetAccountId(-1) returns -1",
        Auth_GetAccountId(-1) == -1);

    // -- Event subscription tests --
    Test_Assert("EventBus: handlers for PLAYER_DATA_LOADED",
        g_HandlerCount[EVT_PLAYER_DATA_LOADED] > 0);

    Test_Summary();
    return 1;
}
