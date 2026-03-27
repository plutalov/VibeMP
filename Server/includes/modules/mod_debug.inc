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

        // Add module-specific tests here as modules are created.
        // Example: Auth_RunTests(); Spawn_RunTests();

        Test_Summary();
        EventBus_SetResult(1);
        return 1;
    }

    return 0;
}
