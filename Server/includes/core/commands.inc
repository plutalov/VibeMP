// =============================================================================
//  commands.inc — Command processor (ZCMD-style routing via event bus)
//
//  Subscribes to EVT_PLAYER_COMMAND. Parses the command name from the input
//  string, then calls cmd_<name>(playerid, params[]) via CallLocalFunction.
//
//  This is pure plumbing — no auth awareness, no permission checks.
//  Each cmd_* handler is responsible for its own validation.
//
//  USAGE:
//    1. Include this after eventbus.inc
//    2. Call Cmd_Init() in OnGameModeInit after EventBus_Init()
//    3. Define commands as:
//         forward cmd_kick(playerid, params[]);
//         public cmd_kick(playerid, params[])
//         {
//             // handler code
//             return 1;
//         }
//    4. Command names are case-insensitive, max 30 chars
//    5. Return 1 from handler = command was handled
//       Return 0 = not handled (shows "unknown command" to player)
//
//  HOW IT WORKS:
//    Player types "/kick 5" →
//      OnPlayerCommandText → EVT_PLAYER_COMMAND →
//        Cmd_OnPlayerCommand:
//          - extracts "kick" from "/kick 5"
//          - params = "5"
//          - calls cmd_kick(playerid, "5") via CallLocalFunction
//          - if function exists → sets result to return value
//          - if not → result stays 0 (unknown command)
// =============================================================================

#if defined _commands_included
    #endinput
#endif
#define _commands_included

#define MAX_CMD_NAME    (30)

// -----------------------------------------------------------------------------
//  Init
// -----------------------------------------------------------------------------

stock Cmd_Init()
{
    EventBus_Subscribe(EVT_PLAYER_COMMAND, "Cmd_OnPlayerCommand");
    print("[CMD] Command processor initialized.");
}

// -----------------------------------------------------------------------------
//  Event Handler
// -----------------------------------------------------------------------------

forward Cmd_OnPlayerCommand();
public Cmd_OnPlayerCommand()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new input[128];
    EventBus_GetString(input, sizeof(input));

    // Must start with /
    if (input[0] != '/')
        return 0;

    // Extract command name (without /) and params
    new cmdname[MAX_CMD_NAME + 5]; // "cmd_" prefix + name
    new params[128];
    new i = 1; // skip the /

    // Read command name
    new c = 0;
    while (input[i] != '\0' && input[i] != ' ' && c < MAX_CMD_NAME)
    {
        cmdname[4 + c] = tolower(input[i]); // lowercase for case-insensitivity
        i++;
        c++;
    }
    cmdname[4 + c] = '\0';

    // Prepend "cmd_"
    cmdname[0] = 'c';
    cmdname[1] = 'm';
    cmdname[2] = 'd';
    cmdname[3] = '_';

    // Skip space between command and params
    if (input[i] == ' ')
        i++;

    // Copy params (everything after the command name)
    new p = 0;
    while (input[i] != '\0' && p < sizeof(params) - 1)
    {
        params[p] = input[i];
        i++;
        p++;
    }
    params[p] = '\0';

    // Check if function exists
    if (funcidx(cmdname) == -1)
        return 0; // unknown command

    // Call the handler
    new ret = CallLocalFunction(cmdname, "ds", playerid, params);
    if (ret == 1)
    {
        EventBus_SetResult(1); // command was handled
    }

    return ret;
}
