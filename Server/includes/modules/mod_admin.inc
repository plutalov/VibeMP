// =============================================================================
//  mod_admin.inc — Admin system
//
//  Loads admin level from DB on login. Provides moderation and dev commands.
//  Admin levels: 0 = player, 1 = moderator, 2 = admin, 3 = owner
//
//  Listens to:
//    EVT_PLAYER_LOGIN        — load admin level from DB
//    EVT_PLAYER_DISCONNECT   — reset admin level
//    EVT_PLAYER_CLICK_MAP    — teleport admin to clicked position
//
//  Public API:
//    Admin_GetLevel(playerid)              — returns 0-3
//    Admin_RequireLevel(playerid, level)   — checks + sends error, returns bool
//
//  Commands:
//    Lvl 1: /kick, /freeze, /unfreeze
//    Lvl 2: /tp, /tphere, /setpos, /setskin, /veh
//    Lvl 3: /setadmin, /ban
//
//  Depends on: mod_db, mod_auth
// =============================================================================

#if defined _mod_admin_included
    #endinput
#endif
#define _mod_admin_included

// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define ADMIN_COLOR     0xFF8800FF  // orange for admin messages

// -----------------------------------------------------------------------------
//  State
// -----------------------------------------------------------------------------

static g_AdminLevel[MAX_PLAYERS];
static bool:g_Frozen[MAX_PLAYERS];

// -----------------------------------------------------------------------------
//  Public API
// -----------------------------------------------------------------------------

stock Admin_GetLevel(playerid)
{
    if (playerid < 0 || playerid >= MAX_PLAYERS) return 0;
    return g_AdminLevel[playerid];
}

// Check permission and send error if insufficient. Returns true if allowed.
stock bool:Admin_RequireLevel(playerid, level)
{
    if (!Auth_IsLoggedIn(playerid))
    {
        SendClientMessage(playerid, 0xFF4444FF, "You must be logged in.");
        return false;
    }
    if (g_AdminLevel[playerid] < level)
    {
        SendClientMessage(playerid, 0xFF4444FF, "You don't have permission to use this command.");
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
//  Param parsing helpers (static)
// -----------------------------------------------------------------------------

// Extract first token as integer, return chars consumed. -1 on failure.
static stock Admin_ParseInt(const params[], &value)
{
    if (params[0] == '\0') return -1;

    new i = 0;
    // Skip leading spaces
    while (params[i] == ' ') i++;
    if (params[i] == '\0') return -1;

    new neg = 0;
    if (params[i] == '-') { neg = 1; i++; }

    if (params[i] < '0' || params[i] > '9') return -1;

    value = 0;
    while (params[i] >= '0' && params[i] <= '9')
    {
        value = value * 10 + (params[i] - '0');
        i++;
    }
    if (neg) value = -value;
    return i;
}

// Extract first token as float, return chars consumed. -1 on failure.
static stock Admin_ParseFloat(const params[], &Float:value)
{
    if (params[0] == '\0') return -1;

    new i = 0;
    while (params[i] == ' ') i++;
    if (params[i] == '\0') return -1;

    // Use Pawn's floatstr on the substring
    value = floatstr(params[i]);

    // Advance past the number
    if (params[i] == '-') i++;
    if (params[i] < '0' || params[i] > '9')
    {
        if (params[i] != '.') return -1;
    }
    while ((params[i] >= '0' && params[i] <= '9') || params[i] == '.')
        i++;

    return i;
}

// Parse a playerid from params, validate connected. Returns true if valid.
static stock bool:Admin_ParsePlayer(const params[], &targetid, &offset)
{
    offset = Admin_ParseInt(params, targetid);
    if (offset == -1)
    {
        return false;
    }
    if (!IsPlayerConnected(targetid))
    {
        return false;
    }
    return true;
}

// Get remaining params after offset, skip leading space
static stock Admin_GetRemainder(const params[], offset, dest[], maxlen)
{
    new i = offset;
    while (params[i] == ' ') i++;
    new p = 0;
    while (params[i] != '\0' && p < maxlen - 1)
    {
        dest[p] = params[i];
        i++;
        p++;
    }
    dest[p] = '\0';
}

// -----------------------------------------------------------------------------
//  Init / Destroy
// -----------------------------------------------------------------------------

stock Admin_Init()
{
    EventBus_RegisterModule("Admin");

    EventBus_Subscribe(EVT_PLAYER_LOGIN,      "Admin_OnPlayerLogin");
    EventBus_Subscribe(EVT_PLAYER_DISCONNECT, "Admin_OnPlayerDisconn");
    EventBus_Subscribe(EVT_PLAYER_CLICK_MAP,  "Admin_OnClickMap");
    EventBus_Subscribe(EVT_PLAYER_HELP,       "Admin_OnPlayerHelp");

    print("[MOD] Admin module initialized.");
}

stock Admin_Destroy()
{
    print("[MOD] Admin module destroyed.");
}

// -----------------------------------------------------------------------------
//  Event Handlers
// -----------------------------------------------------------------------------

forward Admin_OnPlayerLogin();
public Admin_OnPlayerLogin()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);

    // Load admin level from DB
    new query[128];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "SELECT admin_level FROM accounts WHERE id = %d",
        Auth_GetAccountId(playerid));
    mysql_tquery(DB_GetHandle(), query, "Admin_OnLevelLoad", "d", playerid);
    return 1;
}

forward Admin_OnPlayerDisconn();
public Admin_OnPlayerDisconn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    g_AdminLevel[playerid] = 0;
    g_Frozen[playerid] = false;
    return 1;
}

// Map click: teleport admin to clicked position
forward Admin_OnClickMap();
public Admin_OnClickMap()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    if (g_AdminLevel[playerid] < 2) return 0;

    new Float:x = EventBus_GetFloat(0);
    new Float:y = EventBus_GetFloat(1);
    new Float:z = EventBus_GetFloat(2);

    if (IsPlayerInAnyVehicle(playerid))
    {
        SetVehiclePos(GetPlayerVehicleID(playerid), x, y, z);
    }
    else
    {
        SetPlayerPos(playerid, x, y, z);
    }
    SetCameraBehindPlayer(playerid);

    new msg[64];
    format(msg, sizeof(msg), "Teleported to %.1f, %.1f, %.1f", x, y, z);
    SendClientMessage(playerid, ADMIN_COLOR, msg);
    return 1;
}

// -----------------------------------------------------------------------------
//  Async Callbacks
// -----------------------------------------------------------------------------

forward Admin_OnLevelLoad(playerid);
public Admin_OnLevelLoad(playerid)
{
    if (!IsPlayerConnected(playerid)) return;

    new rows;
    cache_get_row_count(rows);
    if (rows > 0)
    {
        cache_get_value_name_int(0, "admin_level", g_AdminLevel[playerid]);
    }

    if (g_AdminLevel[playerid] > 0)
    {
        new msg[64];
        format(msg, sizeof(msg), "Admin level %d loaded.", g_AdminLevel[playerid]);
        SendClientMessage(playerid, ADMIN_COLOR, msg);
    }
}

// Help handler: print admin commands based on level
forward Admin_OnPlayerHelp();
public Admin_OnPlayerHelp()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new lvl = g_AdminLevel[playerid];

    if (lvl >= 1)
    {
        SendClientMessage(playerid, ADMIN_COLOR, "---- Moderator (Lvl 1) ----");
        SendClientMessage(playerid, 0xCCCCCCFF, "/kick [id] [reason]  /freeze [id]  /unfreeze [id]");
    }
    if (lvl >= 2)
    {
        SendClientMessage(playerid, ADMIN_COLOR, "---- Admin (Lvl 2) ----");
        SendClientMessage(playerid, 0xCCCCCCFF, "/tp [id]  /tphere [id]  /veh [model] [c1] [c2]");
        SendClientMessage(playerid, 0xCCCCCCFF, "/setpos [id] x y z  /setskin [id] skin");
        SendClientMessage(playerid, 0xCCCCCCFF, "Map click = teleport");
    }
    if (lvl >= 3)
    {
        SendClientMessage(playerid, ADMIN_COLOR, "---- Owner (Lvl 3) ----");
        SendClientMessage(playerid, 0xCCCCCCFF, "/setadmin [id] [level]  /ban [id] [reason]");
    }
    return 1;
}

// =============================================================================
//  COMMANDS
// =============================================================================

// --- Level 1: Moderator ---

forward cmd_kick(playerid, params[]);
public cmd_kick(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 1)) return 1;

    new targetid, offset;
    if (!Admin_ParsePlayer(params, targetid, offset))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /kick [playerid] [reason]"), 1;

    new reason[64];
    Admin_GetRemainder(params, offset, reason, sizeof(reason));
    if (reason[0] == '\0')
        strcat(reason, "No reason given", sizeof(reason));

    new tname[MAX_PLAYER_NAME + 1], aname[MAX_PLAYER_NAME + 1];
    GetPlayerName(targetid, tname, sizeof(tname));
    GetPlayerName(playerid, aname, sizeof(aname));

    new msg[128];
    format(msg, sizeof(msg), "%s was kicked by %s. Reason: %s", tname, aname, reason);
    SendClientMessageToAll(ADMIN_COLOR, msg);

    // Delay kick so message is seen
    SetTimerEx("Admin_DelayKick", 500, false, "d", targetid);
    return 1;
}

forward cmd_freeze(playerid, params[]);
public cmd_freeze(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 1)) return 1;

    new targetid, offset;
    if (!Admin_ParsePlayer(params, targetid, offset))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /freeze [playerid]"), 1;

    TogglePlayerControllable(targetid, false);
    g_Frozen[targetid] = true;

    new tname[MAX_PLAYER_NAME + 1];
    GetPlayerName(targetid, tname, sizeof(tname));
    new msg[64];
    format(msg, sizeof(msg), "%s has been frozen.", tname);
    SendClientMessage(playerid, ADMIN_COLOR, msg);
    SendClientMessage(targetid, ADMIN_COLOR, "You have been frozen by an admin.");
    return 1;
}

forward cmd_unfreeze(playerid, params[]);
public cmd_unfreeze(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 1)) return 1;

    new targetid, offset;
    if (!Admin_ParsePlayer(params, targetid, offset))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /unfreeze [playerid]"), 1;

    TogglePlayerControllable(targetid, true);
    g_Frozen[targetid] = false;

    new tname[MAX_PLAYER_NAME + 1];
    GetPlayerName(targetid, tname, sizeof(tname));
    new msg[64];
    format(msg, sizeof(msg), "%s has been unfrozen.", tname);
    SendClientMessage(playerid, ADMIN_COLOR, msg);
    SendClientMessage(targetid, ADMIN_COLOR, "You have been unfrozen.");
    return 1;
}

// --- Level 2: Admin ---

forward cmd_tp(playerid, params[]);
public cmd_tp(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 2)) return 1;

    new targetid, offset;
    if (!Admin_ParsePlayer(params, targetid, offset))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /tp [playerid]"), 1;

    new Float:x, Float:y, Float:z;
    GetPlayerPos(targetid, x, y, z);
    SetPlayerPos(playerid, x, y, z);
    SetPlayerInterior(playerid, GetPlayerInterior(targetid));
    SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(targetid));
    SetCameraBehindPlayer(playerid);

    new tname[MAX_PLAYER_NAME + 1];
    GetPlayerName(targetid, tname, sizeof(tname));
    new msg[64];
    format(msg, sizeof(msg), "Teleported to %s.", tname);
    SendClientMessage(playerid, ADMIN_COLOR, msg);
    return 1;
}

forward cmd_tphere(playerid, params[]);
public cmd_tphere(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 2)) return 1;

    new targetid, offset;
    if (!Admin_ParsePlayer(params, targetid, offset))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /tphere [playerid]"), 1;

    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    SetPlayerPos(targetid, x, y, z);
    SetPlayerInterior(targetid, GetPlayerInterior(playerid));
    SetPlayerVirtualWorld(targetid, GetPlayerVirtualWorld(playerid));
    SetCameraBehindPlayer(targetid);

    new tname[MAX_PLAYER_NAME + 1];
    GetPlayerName(targetid, tname, sizeof(tname));
    new msg[64];
    format(msg, sizeof(msg), "Teleported %s to you.", tname);
    SendClientMessage(playerid, ADMIN_COLOR, msg);
    SendClientMessage(targetid, ADMIN_COLOR, "You have been teleported by an admin.");
    return 1;
}

forward cmd_setpos(playerid, params[]);
public cmd_setpos(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 2)) return 1;

    // Try parsing: [playerid] x y z
    // If first param + 3 floats work → target + position
    // If 3 floats work → self + position
    new Float:x, Float:y, Float:z;
    new targetid = playerid; // default to self
    new off = 0, off2;

    // Try to parse 4 values (targetid x y z)
    new testid, testoff;
    if (Admin_ParsePlayer(params, testid, testoff))
    {
        new Float:tx, Float:ty, Float:tz;
        new o1 = testoff;
        while (params[o1] == ' ') o1++;
        new o2 = Admin_ParseFloat(params[o1], tx);
        if (o2 != -1)
        {
            o1 += o2;
            while (params[o1] == ' ') o1++;
            o2 = Admin_ParseFloat(params[o1], ty);
            if (o2 != -1)
            {
                o1 += o2;
                while (params[o1] == ' ') o1++;
                o2 = Admin_ParseFloat(params[o1], tz);
                if (o2 != -1)
                {
                    // Got all 4: targetid x y z
                    targetid = testid;
                    x = tx; y = ty; z = tz;
                    goto apply_setpos;
                }
            }
        }
    }

    // Try to parse 3 values (x y z) for self
    off = 0;
    while (params[off] == ' ') off++;
    off2 = Admin_ParseFloat(params[off], x);
    if (off2 == -1)
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /setpos [playerid] x y z"), 1;
    off += off2;
    while (params[off] == ' ') off++;
    off2 = Admin_ParseFloat(params[off], y);
    if (off2 == -1)
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /setpos [playerid] x y z"), 1;
    off += off2;
    while (params[off] == ' ') off++;
    off2 = Admin_ParseFloat(params[off], z);
    if (off2 == -1)
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /setpos [playerid] x y z"), 1;

    apply_setpos:
    SetPlayerPos(targetid, x, y, z);
    SetCameraBehindPlayer(targetid);

    new msg[128];
    if (targetid == playerid)
        format(msg, sizeof(msg), "Position set to %.1f, %.1f, %.1f", x, y, z);
    else
    {
        new tname[MAX_PLAYER_NAME + 1];
        GetPlayerName(targetid, tname, sizeof(tname));
        format(msg, sizeof(msg), "Set %s position to %.1f, %.1f, %.1f", tname, x, y, z);
    }
    SendClientMessage(playerid, ADMIN_COLOR, msg);
    return 1;
}

forward cmd_setskin(playerid, params[]);
public cmd_setskin(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 2)) return 1;

    // Try: [playerid] skinid
    // If two ints → target + skin. If one int → self + skin.
    new val1, val2, off1, off2;
    off1 = Admin_ParseInt(params, val1);
    if (off1 == -1)
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /setskin [playerid] skinid"), 1;

    new temp[128];
    Admin_GetRemainder(params, off1, temp, sizeof(temp));
    off2 = Admin_ParseInt(temp, val2);

    new targetid, skinid;
    if (off2 != -1)
    {
        // Two values: playerid + skinid
        targetid = val1;
        skinid = val2;
        if (!IsPlayerConnected(targetid))
            return SendClientMessage(playerid, 0xFF4444FF, "Player not connected."), 1;
    }
    else
    {
        // One value: self + skinid
        targetid = playerid;
        skinid = val1;
    }

    if (skinid < 0 || skinid > 311)
        return SendClientMessage(playerid, 0xFF4444FF, "Invalid skin ID (0-311)."), 1;

    SetPlayerSkin(targetid, skinid);

    new msg[64];
    if (targetid == playerid)
        format(msg, sizeof(msg), "Skin set to %d.", skinid);
    else
    {
        new tname[MAX_PLAYER_NAME + 1];
        GetPlayerName(targetid, tname, sizeof(tname));
        format(msg, sizeof(msg), "Set %s skin to %d.", tname, skinid);
    }
    SendClientMessage(playerid, ADMIN_COLOR, msg);
    return 1;
}

forward cmd_veh(playerid, params[]);
public cmd_veh(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 2)) return 1;

    new modelid, off;
    off = Admin_ParseInt(params, modelid);
    if (off == -1 || modelid < 400 || modelid > 611)
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /veh modelid [color1] [color2] (400-611)"), 1;

    new color1 = -1, color2 = -1;
    new temp[64];
    Admin_GetRemainder(params, off, temp, sizeof(temp));
    new off2 = Admin_ParseInt(temp, color1);
    if (off2 != -1)
    {
        new temp2[64];
        Admin_GetRemainder(temp, off2, temp2, sizeof(temp2));
        Admin_ParseInt(temp2, color2);
    }

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    new vehicleid = CreateVehicle(modelid, x, y, z, a, color1, color2, -1);
    if (vehicleid == INVALID_VEHICLE_ID)
        return SendClientMessage(playerid, 0xFF4444FF, "Failed to create vehicle."), 1;

    PutPlayerInVehicle(playerid, vehicleid, 0);

    new msg[64];
    format(msg, sizeof(msg), "Spawned vehicle %d (id: %d).", modelid, vehicleid);
    SendClientMessage(playerid, ADMIN_COLOR, msg);
    return 1;
}

// --- Level 3: Owner ---

forward cmd_setadmin(playerid, params[]);
public cmd_setadmin(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 3)) return 1;

    new targetid, offset;
    if (!Admin_ParsePlayer(params, targetid, offset))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /setadmin [playerid] [level 0-3]"), 1;

    new temp[64];
    Admin_GetRemainder(params, offset, temp, sizeof(temp));
    new level;
    if (Admin_ParseInt(temp, level) == -1 || level < 0 || level > 3)
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /setadmin [playerid] [level 0-3]"), 1;

    // Update in memory
    g_AdminLevel[targetid] = level;

    // Update in DB
    new query[128];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "UPDATE accounts SET admin_level = %d WHERE id = %d",
        level, Auth_GetAccountId(targetid));
    mysql_tquery(DB_GetHandle(), query);

    new tname[MAX_PLAYER_NAME + 1], aname[MAX_PLAYER_NAME + 1];
    GetPlayerName(targetid, tname, sizeof(tname));
    GetPlayerName(playerid, aname, sizeof(aname));

    new msg[128];
    format(msg, sizeof(msg), "%s set %s admin level to %d.", aname, tname, level);
    SendClientMessage(playerid, ADMIN_COLOR, msg);

    format(msg, sizeof(msg), "Your admin level has been set to %d by %s.", level, aname);
    SendClientMessage(targetid, ADMIN_COLOR, msg);
    return 1;
}

forward cmd_ban(playerid, params[]);
public cmd_ban(playerid, params[])
{
    if (!Admin_RequireLevel(playerid, 3)) return 1;

    new targetid, offset;
    if (!Admin_ParsePlayer(params, targetid, offset))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /ban [playerid] [reason]"), 1;

    new reason[64];
    Admin_GetRemainder(params, offset, reason, sizeof(reason));
    if (reason[0] == '\0')
        strcat(reason, "No reason given", sizeof(reason));

    new tname[MAX_PLAYER_NAME + 1], aname[MAX_PLAYER_NAME + 1];
    GetPlayerName(targetid, tname, sizeof(tname));
    GetPlayerName(playerid, aname, sizeof(aname));

    new msg[128];
    format(msg, sizeof(msg), "%s was banned by %s. Reason: %s", tname, aname, reason);
    SendClientMessageToAll(ADMIN_COLOR, msg);

    // Use OMP's ban system
    Ban(targetid);
    return 1;
}

// --- Helper: delayed kick ---
forward Admin_DelayKick(targetid);
public Admin_DelayKick(targetid)
{
    if (IsPlayerConnected(targetid))
        Kick(targetid);
}
