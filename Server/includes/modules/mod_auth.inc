// =============================================================================
//  mod_auth.inc — MySQL-backed authentication with bcrypt
//
//  Handles:
//    - Player connect: async DB lookup, show register or login dialog
//    - Registration: bcrypt hash, INSERT into accounts
//    - Login: bcrypt verify against stored hash
//
//  Emits:
//    EVT_PLAYER_LOGIN       — after successful login  (playerid in slot 0)
//    EVT_PLAYER_REGISTER    — after successful register (playerid in slot 0)
//    EVT_PLAYER_LOGOUT      — on disconnect if was logged in
//
//  Public API:
//    Auth_IsLoggedIn(playerid)   — true if authenticated
//    Auth_GetAccountId(playerid) — DB row ID (-1 if not logged in)
//
//  IMPORTANT: MySQL and bcrypt callbacks are NOT event bus handlers.
//  They are called directly by the plugins. They emit events to notify
//  other modules. Every async callback checks IsPlayerConnected() first.
// =============================================================================

#if defined _mod_auth_included
    #endinput
#endif
#define _mod_auth_included

// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define MAX_PASSWORD_LEN    (32)
#define MAX_USERNAME_LEN    (MAX_PLAYER_NAME)

#define DIALOG_LOGIN        (1)
#define DIALOG_REGISTER     (2)

#define BCRYPT_COST         (12)
#define MAX_LOGIN_ATTEMPTS  (3)

// -----------------------------------------------------------------------------
//  Per-player session state (in-memory, not persisted)
// -----------------------------------------------------------------------------

enum E_PLAYER_AUTH
{
    bool:pa_LoggedIn,
    bool:pa_Registered,     // whether username exists in DB
    pa_AccountId,           // DB row ID
    pa_LoginAttempts
}

static g_PlayerAuth[MAX_PLAYERS][E_PLAYER_AUTH];

// Temp storage for password during async bcrypt (per-player)
static g_TempPassword[MAX_PLAYERS][MAX_PASSWORD_LEN + 1];

// -----------------------------------------------------------------------------
//  Dialog helpers (static — internal)
// -----------------------------------------------------------------------------

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
        "This name is not registered.\nChoose a password (4-32 characters):",
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

stock Auth_GetAccountId(playerid)
{
    if (playerid < 0 || playerid >= MAX_PLAYERS) return -1;
    return g_PlayerAuth[playerid][pa_AccountId];
}

// -----------------------------------------------------------------------------
//  Init / Destroy
// -----------------------------------------------------------------------------

stock Auth_Init()
{
    EventBus_RegisterModule("Auth");

    EventBus_Subscribe(EVT_PLAYER_CONNECT,    "Auth_OnPlayerConnect");
    EventBus_Subscribe(EVT_PLAYER_DISCONNECT, "Auth_OnPlayerDisconn");
    EventBus_Subscribe(EVT_DIALOG_RESPONSE,   "Auth_OnDialogResp");
    EventBus_Subscribe(EVT_PLAYER_COMMAND,    "Auth_OnPlayerCommand");

    print("[MOD] Auth module initialized (MySQL + bcrypt).");
}

stock Auth_Destroy()
{
    print("[MOD] Auth module destroyed.");
}

// -----------------------------------------------------------------------------
//  Event Bus Handlers
// -----------------------------------------------------------------------------

// --- Player Connect: check if account exists in DB ---
forward Auth_OnPlayerConnect();
public Auth_OnPlayerConnect()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Reset per-player state
    g_PlayerAuth[playerid][pa_LoggedIn]      = false;
    g_PlayerAuth[playerid][pa_Registered]    = false;
    g_PlayerAuth[playerid][pa_AccountId]     = -1;
    g_PlayerAuth[playerid][pa_LoginAttempts] = 0;
    g_TempPassword[playerid][0]              = EOS;

    // Check if DB is available
    if (!DB_IsConnected())
    {
        SendClientMessage(playerid, 0xFF4444FF, "Database is not available. Please try again later.");
        SetTimerEx("Auth_DelayKick", 1000, false, "d", playerid);
        return 1;
    }

    new name[MAX_USERNAME_LEN + 1];
    GetPlayerName(playerid, name, sizeof(name));

    new query[128];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "SELECT id FROM accounts WHERE username = '%e' LIMIT 1", name);
    mysql_tquery(DB_GetHandle(), query, "Auth_OnAcctCheck", "d", playerid);
    return 1;
}

// --- Player Disconnect: emit logout if logged in, cleanup ---
forward Auth_OnPlayerDisconn();
public Auth_OnPlayerDisconn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    if (g_PlayerAuth[playerid][pa_LoggedIn])
    {
        // Emit logout event — mod_playerdata listens and saves
        EventBus_SetInt(EVD_PLAYER_ID, playerid);
        EventBus_Emit(EVT_PLAYER_LOGOUT);
    }

    // Clear per-player state
    g_PlayerAuth[playerid][pa_LoggedIn]      = false;
    g_PlayerAuth[playerid][pa_Registered]    = false;
    g_PlayerAuth[playerid][pa_AccountId]     = -1;
    g_PlayerAuth[playerid][pa_LoginAttempts] = 0;
    g_TempPassword[playerid][0]              = EOS;
    return 1;
}

// --- Dialog Response: handle login/register inputs ---
forward Auth_OnDialogResp();
public Auth_OnDialogResp()
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

            // Start async bcrypt hash
            bcrypt_hash(playerid, "Auth_OnBcryptHash", inputtext, BCRYPT_COST);
        }

        case DIALOG_LOGIN:
        {
            if (!response)
            {
                Kick(playerid);
                return 1;
            }

            // Store password temporarily (needed after DB fetch), then fetch hash
            g_TempPassword[playerid][0] = EOS;
            strcat(g_TempPassword[playerid], inputtext, MAX_PASSWORD_LEN + 1);

            new query[128];
            mysql_format(DB_GetHandle(), query, sizeof(query),
                "SELECT password FROM accounts WHERE id = %d LIMIT 1",
                g_PlayerAuth[playerid][pa_AccountId]);
            mysql_tquery(DB_GetHandle(), query, "Auth_OnPwFetch", "d", playerid);
        }
    }
    return 1;
}

// --- Command handler: block commands for unauthenticated players ---
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

    return 0;
}

// =============================================================================
//  ASYNC CALLBACKS — called directly by MySQL/bcrypt plugins, NOT event bus
//  Every callback MUST check IsPlayerConnected() before acting.
// =============================================================================

// --- Helper: delayed kick ---
forward Auth_DelayKick(playerid);
public Auth_DelayKick(playerid)
{
    if (IsPlayerConnected(playerid))
        Kick(playerid);
}

// --- Account check on connect ---
forward Auth_OnAcctCheck(playerid);
public Auth_OnAcctCheck(playerid)
{
    if (!IsPlayerConnected(playerid)) return;

    new rows;
    cache_get_row_count(rows);

    if (rows > 0)
    {
        // Account exists — store ID, show login dialog
        cache_get_value_name_int(0, "id", g_PlayerAuth[playerid][pa_AccountId]);
        g_PlayerAuth[playerid][pa_Registered] = true;
        Auth_ShowLoginDialog(playerid);
    }
    else
    {
        // New name — show register dialog
        g_PlayerAuth[playerid][pa_Registered] = false;
        Auth_ShowRegisterDialog(playerid);
    }
}

// --- Password fetch for login ---
forward Auth_OnPwFetch(playerid);
public Auth_OnPwFetch(playerid)
{
    if (!IsPlayerConnected(playerid)) return;

    new rows;
    cache_get_row_count(rows);

    if (rows == 0)
    {
        SendClientMessage(playerid, 0xFF4444FF, "Account error. Please reconnect.");
        SetTimerEx("Auth_DelayKick", 1000, false, "d", playerid);
        return;
    }

    new stored_hash[BCRYPT_HASH_LENGTH + 1];
    cache_get_value_name(0, "password", stored_hash, sizeof(stored_hash));

    // Async bcrypt verify
    bcrypt_verify(playerid, "Auth_OnBcryptChk", g_TempPassword[playerid], stored_hash);
}

// --- Bcrypt verify result (login) ---
forward Auth_OnBcryptChk(playerid, bool:success);
public Auth_OnBcryptChk(playerid, bool:success)
{
    if (!IsPlayerConnected(playerid)) return;

    // Clear temp password
    g_TempPassword[playerid][0] = EOS;

    if (success)
    {
        // Login success
        g_PlayerAuth[playerid][pa_LoggedIn] = true;
        SendClientMessage(playerid, 0x44FF44FF, "Logged in successfully!");

        // Update last_login
        new query[128];
        mysql_format(DB_GetHandle(), query, sizeof(query),
            "UPDATE accounts SET last_login = NOW() WHERE id = %d",
            g_PlayerAuth[playerid][pa_AccountId]);
        mysql_tquery(DB_GetHandle(), query);

        // Emit login event — mod_playerdata listens and loads data
        EventBus_SetInt(EVD_PLAYER_ID, playerid);
        EventBus_Emit(EVT_PLAYER_LOGIN);
    }
    else
    {
        // Wrong password
        g_PlayerAuth[playerid][pa_LoginAttempts]++;
        if (g_PlayerAuth[playerid][pa_LoginAttempts] >= MAX_LOGIN_ATTEMPTS)
        {
            SendClientMessage(playerid, 0xFF4444FF, "Too many failed attempts. Disconnecting.");
            SetTimerEx("Auth_DelayKick", 1000, false, "d", playerid);
        }
        else
        {
            new msg[64];
            format(msg, sizeof(msg), "Wrong password. Attempts: %d/%d",
                g_PlayerAuth[playerid][pa_LoginAttempts], MAX_LOGIN_ATTEMPTS);
            SendClientMessage(playerid, 0xFF4444FF, msg);
            Auth_ShowLoginDialog(playerid);
        }
    }
}

// --- Bcrypt hash result (register) ---
forward Auth_OnBcryptHash(playerid, hashid);
public Auth_OnBcryptHash(playerid, hashid)
{
    #pragma unused hashid
    if (!IsPlayerConnected(playerid)) return;

    // Get the hash
    new hash[BCRYPT_HASH_LENGTH];
    bcrypt_get_hash(hash);

    new name[MAX_USERNAME_LEN + 1];
    GetPlayerName(playerid, name, sizeof(name));

    // INSERT new account
    new query[256];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "INSERT INTO accounts (username, password) VALUES ('%e', '%e')",
        name, hash);
    mysql_tquery(DB_GetHandle(), query, "Auth_OnAcctCreated", "d", playerid);
}

// --- Account created callback ---
forward Auth_OnAcctCreated(playerid);
public Auth_OnAcctCreated(playerid)
{
    if (!IsPlayerConnected(playerid)) return;

    g_PlayerAuth[playerid][pa_AccountId]  = cache_insert_id();
    g_PlayerAuth[playerid][pa_Registered] = true;
    g_PlayerAuth[playerid][pa_LoggedIn]   = true;

    SendClientMessage(playerid, 0x44FF44FF, "Account created! You are now logged in.");

    // Emit register event
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_PLAYER_REGISTER);

    // Emit login event — mod_playerdata listens and loads data
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_PLAYER_LOGIN);
}
