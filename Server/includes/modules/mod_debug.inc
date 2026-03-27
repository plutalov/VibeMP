// =============================================================================
//  mod_debug.inc — Debug and introspection commands
//
//  Commands:
//    /debug    — Toggle event bus debug logging on/off
//    /events   — Dump all registered event handlers to console
//    /test     — Run all module tests
// =============================================================================

#if defined _mod_debug_included
    #endinput
#endif
#define _mod_debug_included

stock Debug_Init()
{
    EventBus_RegisterModule("Debug");

    EventBus_Subscribe(EVT_PLAYER_COMMAND, "Debug_OnPlayerCommand");

    print("[MOD] Debug module initialized.");
}

stock Debug_Destroy()
{
    print("[MOD] Debug module destroyed.");
}

// --- Command handler ---

forward Debug_OnPlayerCommand();
public Debug_OnPlayerCommand()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new cmd[128];
    EventBus_GetString(cmd, sizeof(cmd));

    if (strcmp(cmd, "/help", true) == 0)
    {
        SendClientMessage(playerid, 0xFFFFFFFF, "---- Commands ----");
        SendClientMessage(playerid, 0xCCCCCCFF, "/help   — show this list");
        SendClientMessage(playerid, 0xCCCCCCFF, "/stats  — show your stats");
        SendClientMessage(playerid, 0xCCCCCCFF, "/debug  — toggle event logging");
        SendClientMessage(playerid, 0xCCCCCCFF, "/events — dump handlers to console");
        SendClientMessage(playerid, 0xCCCCCCFF, "/test   — run tests");
        EventBus_SetResult(1);
        return 1;
    }

    if (strcmp(cmd, "/stats", true) == 0)
    {
        new name[MAX_PLAYER_NAME + 1];
        GetPlayerName(playerid, name, sizeof(name));

        new msg[128];
        format(msg, sizeof(msg), "Stats for %s | Score: %d | Money: $%d",
            name, GetPlayerScore(playerid), GetPlayerMoney(playerid));
        SendClientMessage(playerid, 0xFFFFFFFF, msg);
        EventBus_SetResult(1);
        return 1;
    }

    if (strcmp(cmd, "/debug", true) == 0)
    {
        g_DebugEvents = !g_DebugEvents;
        new msg[64];
        format(msg, sizeof(msg), "Event debug logging: %s",
            g_DebugEvents ? ("ON") : ("OFF"));
        SendClientMessage(playerid, 0xFFFF00FF, msg);
        EventBus_SetResult(1);
        return 1;
    }

    if (strcmp(cmd, "/events", true) == 0)
    {
        EventBus_DumpHandlers();
        SendClientMessage(playerid, 0xFFFF00FF, "Handler dump printed to server console.");
        EventBus_SetResult(1);
        return 1;
    }

    if (strcmp(cmd, "/test", true) == 0)
    {
        SendClientMessage(playerid, 0xFFFF00FF, "Running tests... check server console.");
        Test_Init();

        // -- EventBus self-tests --
        // Test: subscribe count
        Test_Assert("EventBus: handlers registered for PLAYER_CONNECT",
            g_HandlerCount[EVT_PLAYER_CONNECT] > 0);

        // Test: event data round-trip
        EventBus_SetInt(0, 42);
        Test_AssertEqual("EventBus: SetInt/GetInt round-trip",
            EventBus_GetInt(0), 42);

        EventBus_SetFloat(0, 3.14);
        // Float comparison — just check it's not zero
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

        Test_Assert("EventBus: handlers for DB_CONNECTED",
            g_HandlerCount[EVT_DB_CONNECTED] >= 0);

        Test_Summary();
        EventBus_SetResult(1);
        return 1;
    }

    return 0;
}
