// =============================================================================
//  mod_help.inc — Player-facing help and stats commands
//
//  Commands:
//    /help     — show command list (each module prints its own section)
//    /stats    — show player stats
// =============================================================================

#if defined _mod_help_included
    #endinput
#endif
#define _mod_help_included

stock Help_Init()
{
    EventBus_RegisterModule("Help");
    print("[MOD] Help module initialized.");
}

stock Help_Destroy()
{
    print("[MOD] Help module destroyed.");
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

    // Each module prints its own commands via EVT_PLAYER_HELP
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
