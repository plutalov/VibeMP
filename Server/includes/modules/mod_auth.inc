// =============================================================================
//  mod_auth.inc — In-memory account registration and login
//
//  Extracted from the original mygamemode.pwn. Handles:
//    - Player connect: show register or login dialog
//    - Dialog responses: validate password, create account
//    - Player disconnect: persist score/money to in-memory store
//
//  Emits:
//    EVT_PLAYER_LOGIN    — after successful login  (playerid in slot 0)
//    EVT_PLAYER_REGISTER — after successful register (playerid in slot 0)
//
//  Public API:
//    Auth_IsLoggedIn(playerid) — returns true if player is logged in
// =============================================================================

#if defined _mod_auth_included
    #endinput
#endif
#define _mod_auth_included

// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define MAX_ACCOUNTS        (MAX_PLAYERS)
#define MAX_PASSWORD_LEN    (32)
#define MAX_USERNAME_LEN    (MAX_PLAYER_NAME)

#define DIALOG_LOGIN        (1)
#define DIALOG_REGISTER     (2)

// -----------------------------------------------------------------------------
//  Account storage (in-memory, wiped on restart)
// -----------------------------------------------------------------------------

enum E_ACCOUNT
{
    bool:acc_Used,
    acc_Name[MAX_USERNAME_LEN + 1],
    acc_Password[MAX_PASSWORD_LEN + 1],
    acc_Score,
    acc_Money
}

static g_Accounts[MAX_ACCOUNTS][E_ACCOUNT];

// Per-player state
enum E_PLAYER_AUTH
{
    bool:pa_LoggedIn,
    bool:pa_Registered,
    pa_AccountIndex
}

static g_PlayerAuth[MAX_PLAYERS][E_PLAYER_AUTH];

// -----------------------------------------------------------------------------
//  Helpers (static — internal to this module)
// -----------------------------------------------------------------------------

static stock Auth_FindAccount(const name[])
{
    for (new i = 0; i < MAX_ACCOUNTS; i++)
    {
        if (g_Accounts[i][acc_Used] && strcmp(g_Accounts[i][acc_Name], name, true) == 0)
            return i;
    }
    return -1;
}

static stock Auth_FindFreeSlot()
{
    for (new i = 0; i < MAX_ACCOUNTS; i++)
    {
        if (!g_Accounts[i][acc_Used])
            return i;
    }
    return -1;
}

static stock Auth_ShowLoginDialog(playerid)
{
    new name[MAX_USERNAME_LEN + 1];
    GetPlayerName(playerid, name, sizeof(name));

    new caption[64];
    format(caption, sizeof(caption), "Login — %s", name);

    ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD,
        caption,
        "Enter your password to log in:",
        "Login", "Quit");
}

static stock Auth_ShowRegisterDialog(playerid)
{
    new name[MAX_USERNAME_LEN + 1];
    GetPlayerName(playerid, name, sizeof(name));

    new caption[64];
    format(caption, sizeof(caption), "Register — %s", name);

    ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD,
        caption,
        "This name is not registered.\nChoose a password to create an account:",
        "Register", "Quit");
}

// -----------------------------------------------------------------------------
//  Public API
// -----------------------------------------------------------------------------

stock bool:Auth_IsLoggedIn(playerid)
{
    if (playerid < 0 || playerid >= MAX_PLAYERS) return false;
    return g_PlayerAuth[playerid][pa_LoggedIn];
}

// -----------------------------------------------------------------------------
//  Init / Destroy
// -----------------------------------------------------------------------------

stock Auth_Init()
{
    EventBus_RegisterModule("Auth");

    EventBus_Subscribe(EVT_PLAYER_CONNECT,    "Auth_OnPlayerConnect");
    EventBus_Subscribe(EVT_PLAYER_DISCONNECT, "Auth_OnPlayerDisconnect");
    EventBus_Subscribe(EVT_DIALOG_RESPONSE,   "Auth_OnDialogResponse");
    EventBus_Subscribe(EVT_PLAYER_COMMAND,    "Auth_OnPlayerCommand");

    print("[MOD] Auth module initialized.");
}

stock Auth_Destroy()
{
    print("[MOD] Auth module destroyed.");
}

// -----------------------------------------------------------------------------
//  Event Handlers
// -----------------------------------------------------------------------------

forward Auth_OnPlayerConnect();
public Auth_OnPlayerConnect()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Reset per-player state
    g_PlayerAuth[playerid][pa_LoggedIn]     = false;
    g_PlayerAuth[playerid][pa_Registered]   = false;
    g_PlayerAuth[playerid][pa_AccountIndex] = -1;

    new name[MAX_USERNAME_LEN + 1];
    GetPlayerName(playerid, name, sizeof(name));

    new idx = Auth_FindAccount(name);
    if (idx == -1)
    {
        Auth_ShowRegisterDialog(playerid);
    }
    else
    {
        g_PlayerAuth[playerid][pa_Registered]   = true;
        g_PlayerAuth[playerid][pa_AccountIndex] = idx;
        Auth_ShowLoginDialog(playerid);
    }
    return 1;
}

forward Auth_OnPlayerDisconnect();
public Auth_OnPlayerDisconnect()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Persist score/money to in-memory account
    new idx = g_PlayerAuth[playerid][pa_AccountIndex];
    if (idx != -1 && g_PlayerAuth[playerid][pa_LoggedIn])
    {
        g_Accounts[idx][acc_Score] = GetPlayerScore(playerid);
        g_Accounts[idx][acc_Money] = GetPlayerMoney(playerid);
    }

    // Clear per-player state
    g_PlayerAuth[playerid][pa_LoggedIn]     = false;
    g_PlayerAuth[playerid][pa_Registered]   = false;
    g_PlayerAuth[playerid][pa_AccountIndex] = -1;
    return 1;
}

forward Auth_OnDialogResponse();
public Auth_OnDialogResponse()
{
    new playerid   = EventBus_GetInt(EVD_PLAYER_ID);
    new dialogid   = EventBus_GetInt(EVD_SECONDARY_ID);
    new response   = EventBus_GetInt(EVD_VALUE);
    new inputtext[128];
    EventBus_GetString(inputtext, sizeof(inputtext));

    switch (dialogid)
    {
        case DIALOG_REGISTER:
        {
            if (!response)
            {
                Kick(playerid);
                return 1;
            }

            if (strlen(inputtext) < 4)
            {
                SendClientMessage(playerid, 0xFF4444FF, "Password must be at least 4 characters.");
                Auth_ShowRegisterDialog(playerid);
                return 1;
            }

            if (strlen(inputtext) > MAX_PASSWORD_LEN)
            {
                SendClientMessage(playerid, 0xFF4444FF, "Password too long (max 32 characters).");
                Auth_ShowRegisterDialog(playerid);
                return 1;
            }

            new slot = Auth_FindFreeSlot();
            if (slot == -1)
            {
                SendClientMessage(playerid, 0xFF4444FF, "Server is full, cannot register. Try again later.");
                Kick(playerid);
                return 1;
            }

            new name[MAX_USERNAME_LEN + 1];
            GetPlayerName(playerid, name, sizeof(name));

            g_Accounts[slot][acc_Used] = true;
            g_Accounts[slot][acc_Name][0] = EOS;
            strcat(g_Accounts[slot][acc_Name], name, MAX_USERNAME_LEN + 1);
            g_Accounts[slot][acc_Password][0] = EOS;
            strcat(g_Accounts[slot][acc_Password], inputtext, MAX_PASSWORD_LEN + 1);
            g_Accounts[slot][acc_Score] = 0;
            g_Accounts[slot][acc_Money] = 0;

            g_PlayerAuth[playerid][pa_Registered]   = true;
            g_PlayerAuth[playerid][pa_LoggedIn]     = true;
            g_PlayerAuth[playerid][pa_AccountIndex] = slot;

            SendClientMessage(playerid, 0x44FF44FF, "Account created! You are now logged in.");

            // Emit register event — other modules can react
            EventBus_SetInt(EVD_PLAYER_ID, playerid);
            EventBus_Emit(EVT_PLAYER_REGISTER);
        }

        case DIALOG_LOGIN:
        {
            if (!response)
            {
                Kick(playerid);
                return 1;
            }

            new idx = g_PlayerAuth[playerid][pa_AccountIndex];
            if (idx == -1 || strcmp(g_Accounts[idx][acc_Password], inputtext) != 0)
            {
                SendClientMessage(playerid, 0xFF4444FF, "Wrong password. Try again.");
                Auth_ShowLoginDialog(playerid);
                return 1;
            }

            g_PlayerAuth[playerid][pa_LoggedIn] = true;
            SendClientMessage(playerid, 0x44FF44FF, "Logged in successfully!");

            // Emit login event — other modules can react
            EventBus_SetInt(EVD_PLAYER_ID, playerid);
            EventBus_Emit(EVT_PLAYER_LOGIN);
        }
    }
    return 1;
}

forward Auth_OnPlayerCommand();
public Auth_OnPlayerCommand()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Block all commands for unauthenticated players
    if (!g_PlayerAuth[playerid][pa_LoggedIn])
    {
        SendClientMessage(playerid, 0xFF4444FF, "You must be logged in to use commands.");
        EventBus_SetResult(1); // "handled" — don't show unknown command
        return 1;
    }

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
        new name[MAX_USERNAME_LEN + 1];
        GetPlayerName(playerid, name, sizeof(name));

        new msg[128];
        format(msg, sizeof(msg), "Stats for %s | Score: %d | Money: $%d",
            name, GetPlayerScore(playerid), GetPlayerMoney(playerid));
        SendClientMessage(playerid, 0xFFFFFFFF, msg);
        EventBus_SetResult(1);
        return 1;
    }

    return 0;
}
