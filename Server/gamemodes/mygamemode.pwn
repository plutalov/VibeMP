// =============================================================================
//  mygamemode.pwn - boilerplate OMP gamemode
//  Login: in-memory (accounts reset on server restart)
//  Spawn: Mount Chiliad peak
// =============================================================================

#define MIXED_SPELLINGS

#include <open.mp>

// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define MAX_ACCOUNTS        (MAX_PLAYERS)   // one slot per connected player max
#define MAX_PASSWORD_LEN    (32)
#define MAX_USERNAME_LEN    (MAX_PLAYER_NAME)

#define DIALOG_LOGIN        (1)
#define DIALOG_REGISTER     (2)

#define SPAWN_X             (-2233.9700)
#define SPAWN_Y             (-1737.5800)
#define SPAWN_Z             (480.5500)
#define SPAWN_A             (0.0)
#define SPAWN_SKIN          (0)
#define SPAWN_INTERIOR      (0)
#define SPAWN_VIRTUAL_WORLD (0)

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

new g_Accounts[MAX_ACCOUNTS][E_ACCOUNT];

// Per-player state
enum E_PLAYER
{
    bool:p_LoggedIn,
    bool:p_Registered,
    p_AccountIndex         // index into g_Accounts, -1 if none
}

new g_Player[MAX_PLAYERS][E_PLAYER];

// -----------------------------------------------------------------------------
//  Helpers
// -----------------------------------------------------------------------------

// Returns account index for a name, or -1 if not found.
stock FindAccount(const name[])
{
    for (new i = 0; i < MAX_ACCOUNTS; i++)
    {
        if (g_Accounts[i][acc_Used] && strcmp(g_Accounts[i][acc_Name], name, true) == 0)
            return i;
    }
    return -1;
}

// Returns a free account slot, or -1 if full.
stock FindFreeAccountSlot()
{
    for (new i = 0; i < MAX_ACCOUNTS; i++)
    {
        if (!g_Accounts[i][acc_Used])
            return i;
    }
    return -1;
}

stock ShowLoginDialog(playerid)
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

stock ShowRegisterDialog(playerid)
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

stock SpawnPlayerAtDefault(playerid)
{
    SetPlayerSkin(playerid, SPAWN_SKIN);
    SpawnPlayer(playerid);
}

// -----------------------------------------------------------------------------
//  Callbacks
// -----------------------------------------------------------------------------

public OnGameModeInit()
{
    SetGameModeText("MyGamemode");
    ShowPlayerMarkers(PLAYER_MARKERS_MODE_GLOBAL);
    ShowNameTags(true);
    EnableStuntBonusForAll(false);
    DisableInteriorEnterExits();

    // Single class — skin/position are overridden after login anyway.
    AddPlayerClass(SPAWN_SKIN, SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A,
        WEAPON_FIST, 0, WEAPON_FIST, 0, WEAPON_FIST, 0);

    print("[GM] MyGamemode initialised.");
    return 1;
}

public OnGameModeExit()
{
    print("[GM] MyGamemode exiting.");
    return 1;
}

public OnPlayerConnect(playerid)
{
    // Reset per-player state.
    g_Player[playerid][p_LoggedIn]    = false;
    g_Player[playerid][p_Registered]  = false;
    g_Player[playerid][p_AccountIndex] = -1;

    new name[MAX_USERNAME_LEN + 1];
    GetPlayerName(playerid, name, sizeof(name));

    new idx = FindAccount(name);
    if (idx == -1)
    {
        ShowRegisterDialog(playerid);
    }
    else
    {
        g_Player[playerid][p_Registered]  = true;
        g_Player[playerid][p_AccountIndex] = idx;
        ShowLoginDialog(playerid);
    }

    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    // Persist score/money back to the in-memory account.
    new idx = g_Player[playerid][p_AccountIndex];
    if (idx != -1 && g_Player[playerid][p_LoggedIn])
    {
        g_Accounts[idx][acc_Score] = GetPlayerScore(playerid);
        g_Accounts[idx][acc_Money] = GetPlayerMoney(playerid);
    }

    // Clear per-player state.
    g_Player[playerid][p_LoggedIn]    = false;
    g_Player[playerid][p_Registered]  = false;
    g_Player[playerid][p_AccountIndex] = -1;
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    if (g_Player[playerid][p_LoggedIn])
    {
        SetSpawnInfo(playerid, NO_TEAM, SPAWN_SKIN, SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A, WEAPON_FIST, 0, WEAPON_FIST, 0, WEAPON_FIST, 0);
        SpawnPlayer(playerid);
        return 0;
    }
    return 0;
}

public OnPlayerRequestSpawn(playerid)
{
    // Block spawning until logged in.
    if (!g_Player[playerid][p_LoggedIn])
        return 0;
    return 1;
}

public OnPlayerSpawn(playerid)
{
    SetPlayerInterior(playerid, SPAWN_INTERIOR);
    SetPlayerVirtualWorld(playerid, SPAWN_VIRTUAL_WORLD);
    SetPlayerPos(playerid, SPAWN_X, SPAWN_Y, SPAWN_Z);
    SetPlayerFacingAngle(playerid, SPAWN_A);
    SetCameraBehindPlayer(playerid);

    new idx = g_Player[playerid][p_AccountIndex];
    if (idx != -1)
    {
        ResetPlayerMoney(playerid);
        GivePlayerMoney(playerid, g_Accounts[idx][acc_Money]);
        SetPlayerScore(playerid, g_Accounts[idx][acc_Score]);
    }

    SendClientMessage(playerid, 0xFFFFFFFF, "Welcome! Use /help for commands.");
    return 1;
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    switch (dialogid)
    {
        // ----- REGISTER -----
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
                ShowRegisterDialog(playerid);
                return 1;
            }

            if (strlen(inputtext) > MAX_PASSWORD_LEN)
            {
                SendClientMessage(playerid, 0xFF4444FF, "Password too long (max 32 characters).");
                ShowRegisterDialog(playerid);
                return 1;
            }

            new slot = FindFreeAccountSlot();
            if (slot == -1)
            {
                SendClientMessage(playerid, 0xFF4444FF, "Server is full, cannot register. Try again later.");
                Kick(playerid);
                return 1;
            }

            new name[MAX_USERNAME_LEN + 1];
            GetPlayerName(playerid, name, sizeof(name));

            g_Accounts[slot][acc_Used] = true;
            strcat(g_Accounts[slot][acc_Name],     name,      MAX_USERNAME_LEN + 1);
            strcat(g_Accounts[slot][acc_Password],  inputtext, MAX_PASSWORD_LEN + 1);
            g_Accounts[slot][acc_Score] = 0;
            g_Accounts[slot][acc_Money] = 0;

            g_Player[playerid][p_Registered]   = true;
            g_Player[playerid][p_LoggedIn]     = true;
            g_Player[playerid][p_AccountIndex] = slot;

            SendClientMessage(playerid, 0x44FF44FF, "Account created! You are now logged in.");
            SpawnPlayerAtDefault(playerid);
        }

        // ----- LOGIN -----
        case DIALOG_LOGIN:
        {
            if (!response)
            {
                Kick(playerid);
                return 1;
            }

            new idx = g_Player[playerid][p_AccountIndex];
            if (idx == -1 || strcmp(g_Accounts[idx][acc_Password], inputtext) != 0)
            {
                SendClientMessage(playerid, 0xFF4444FF, "Wrong password. Try again.");
                ShowLoginDialog(playerid);
                return 1;
            }

            g_Player[playerid][p_LoggedIn] = true;
            SendClientMessage(playerid, 0x44FF44FF, "Logged in successfully!");
            SpawnPlayerAtDefault(playerid);
        }
    }
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!g_Player[playerid][p_LoggedIn])
    {
        SendClientMessage(playerid, 0xFF4444FF, "You must be logged in to use commands.");
        return 1;
    }

    if (strcmp(cmdtext, "/help", true) == 0)
    {
        SendClientMessage(playerid, 0xFFFFFFFF, "---- Commands ----");
        SendClientMessage(playerid, 0xCCCCCCFF, "/help  - show this list");
        SendClientMessage(playerid, 0xCCCCCCFF, "/stats - show your stats");
        return 1;
    }

    if (strcmp(cmdtext, "/stats", true) == 0)
    {
        new name[MAX_USERNAME_LEN + 1];
        GetPlayerName(playerid, name, sizeof(name));

        new msg[128];
        format(msg, sizeof(msg), "Stats for %s | Score: %d | Money: $%d",
            name, GetPlayerScore(playerid), GetPlayerMoney(playerid));
        SendClientMessage(playerid, 0xFFFFFFFF, msg);
        return 1;
    }

    return 0;
}
