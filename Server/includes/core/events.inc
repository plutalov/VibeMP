// =============================================================================
//  events.inc — All event ID constants
//  ALL event IDs live here. Never define event IDs in module files.
//
//  Ranges:
//    0–99   : SA-MP/OMP bridged callbacks
//    100+   : Custom game events (modules emit these)
// =============================================================================

#if defined _events_included
    #endinput
#endif
#define _events_included

// -----------------------------------------------------------------------------
//  SA-MP Bridged Events (0–99)
// -----------------------------------------------------------------------------

#define EVT_GAME_INIT               0
#define EVT_GAME_EXIT               1
#define EVT_PLAYER_CONNECT          2
#define EVT_PLAYER_DISCONNECT       3
#define EVT_PLAYER_SPAWN            4
#define EVT_PLAYER_DEATH            5
#define EVT_PLAYER_TEXT             6
#define EVT_PLAYER_COMMAND          7
#define EVT_PLAYER_REQUEST_CLASS    8
#define EVT_PLAYER_REQUEST_SPAWN    9
#define EVT_DIALOG_RESPONSE         10
#define EVT_PLAYER_STATE_CHANGE     11
#define EVT_PLAYER_KEY_STATE_CHANGE 12
#define EVT_PLAYER_UPDATE           13
#define EVT_PLAYER_ENTER_VEHICLE    14
#define EVT_PLAYER_EXIT_VEHICLE     15
#define EVT_VEHICLE_SPAWN           16
#define EVT_VEHICLE_DEATH           17
#define EVT_PLAYER_TAKE_DAMAGE      18
#define EVT_PLAYER_GIVE_DAMAGE      19
#define EVT_PLAYER_CLICK_PLAYER     20
#define EVT_PLAYER_PICK_UP_PICKUP   21
#define EVT_PLAYER_ENTER_CP         22
#define EVT_PLAYER_LEAVE_CP         23
#define EVT_PLAYER_ENTER_RACE_CP    24
#define EVT_PLAYER_LEAVE_RACE_CP    25
#define EVT_PLAYER_STREAM_IN        26
#define EVT_PLAYER_STREAM_OUT       27
#define EVT_PLAYER_INTERIOR_CHANGE  28
#define EVT_PLAYER_CLICK_MAP        29
#define EVT_PLAYER_CLICK_PTEXTDRAW  30  // OnPlayerClickPlayerTextDraw
#define EVT_PLAYER_CLICK_TEXTDRAW   31  // OnPlayerClickTextDraw (ESC from SelectTextDraw)

// -----------------------------------------------------------------------------
//  Custom Game Events (100+)
// -----------------------------------------------------------------------------

#define EVT_PLAYER_LOGIN            100
#define EVT_PLAYER_REGISTER         101
#define EVT_PLAYER_LOGOUT           102
#define EVT_PLAYER_DATA_LOADED      103  // player data loaded from DB and applied
#define EVT_PLAYER_DATA_SAVED       104  // player data saved to DB
#define EVT_DB_CONNECTED            105
#define EVT_PLAYER_HELP             106  // player requested /help

// Inventory events (110-119)
#define EVT_INV_TEMPLATES_LOADED    110  // all item templates loaded from DB
#define EVT_INV_PLAYER_LOADED       111  // player's container + items loaded
#define EVT_INV_ITEM_ADDED          112  // item added to a container
#define EVT_INV_ITEM_REMOVED        113  // item removed from a container
#define EVT_INV_ITEM_USE            114  // player used an item
#define EVT_INV_ITEM_MOVED          115  // item moved between containers

// Vehicle events (120-129)
#define EVT_VEHICLE_CREATED         120  // persistent vehicle loaded/spawned
#define EVT_VEHICLE_DESTROYED       121  // persistent vehicle removed

// -----------------------------------------------------------------------------
//  Limits
// -----------------------------------------------------------------------------

#define MAX_EVENTS                  128

// -----------------------------------------------------------------------------
//  Event name registry (for debug output)
//  Populated by EventBus_InitNames() in eventbus.inc
// -----------------------------------------------------------------------------

new g_EventNames[MAX_EVENTS][32];

stock EventBus_InitNames()
{
    // Zero all names first
    for (new i = 0; i < MAX_EVENTS; i++)
        g_EventNames[i][0] = EOS;

    // SA-MP bridged
    strcat(g_EventNames[EVT_GAME_INIT],               "GAME_INIT",               32);
    strcat(g_EventNames[EVT_GAME_EXIT],               "GAME_EXIT",               32);
    strcat(g_EventNames[EVT_PLAYER_CONNECT],           "PLAYER_CONNECT",          32);
    strcat(g_EventNames[EVT_PLAYER_DISCONNECT],        "PLAYER_DISCONNECT",       32);
    strcat(g_EventNames[EVT_PLAYER_SPAWN],             "PLAYER_SPAWN",            32);
    strcat(g_EventNames[EVT_PLAYER_DEATH],             "PLAYER_DEATH",            32);
    strcat(g_EventNames[EVT_PLAYER_TEXT],              "PLAYER_TEXT",             32);
    strcat(g_EventNames[EVT_PLAYER_COMMAND],           "PLAYER_COMMAND",          32);
    strcat(g_EventNames[EVT_PLAYER_REQUEST_CLASS],     "PLAYER_REQUEST_CLASS",    32);
    strcat(g_EventNames[EVT_PLAYER_REQUEST_SPAWN],     "PLAYER_REQUEST_SPAWN",    32);
    strcat(g_EventNames[EVT_DIALOG_RESPONSE],          "DIALOG_RESPONSE",         32);
    strcat(g_EventNames[EVT_PLAYER_STATE_CHANGE],      "PLAYER_STATE_CHANGE",     32);
    strcat(g_EventNames[EVT_PLAYER_KEY_STATE_CHANGE],  "PLAYER_KEY_STATE_CHANGE", 32);
    strcat(g_EventNames[EVT_PLAYER_UPDATE],            "PLAYER_UPDATE",           32);
    strcat(g_EventNames[EVT_PLAYER_ENTER_VEHICLE],     "PLAYER_ENTER_VEHICLE",    32);
    strcat(g_EventNames[EVT_PLAYER_EXIT_VEHICLE],      "PLAYER_EXIT_VEHICLE",     32);
    strcat(g_EventNames[EVT_VEHICLE_SPAWN],            "VEHICLE_SPAWN",           32);
    strcat(g_EventNames[EVT_VEHICLE_DEATH],            "VEHICLE_DEATH",           32);
    strcat(g_EventNames[EVT_PLAYER_TAKE_DAMAGE],       "PLAYER_TAKE_DAMAGE",      32);
    strcat(g_EventNames[EVT_PLAYER_GIVE_DAMAGE],       "PLAYER_GIVE_DAMAGE",      32);
    strcat(g_EventNames[EVT_PLAYER_CLICK_PLAYER],      "PLAYER_CLICK_PLAYER",     32);
    strcat(g_EventNames[EVT_PLAYER_PICK_UP_PICKUP],    "PLAYER_PICK_UP_PICKUP",   32);
    strcat(g_EventNames[EVT_PLAYER_ENTER_CP],          "PLAYER_ENTER_CP",         32);
    strcat(g_EventNames[EVT_PLAYER_LEAVE_CP],          "PLAYER_LEAVE_CP",         32);
    strcat(g_EventNames[EVT_PLAYER_ENTER_RACE_CP],     "PLAYER_ENTER_RACE_CP",    32);
    strcat(g_EventNames[EVT_PLAYER_LEAVE_RACE_CP],     "PLAYER_LEAVE_RACE_CP",    32);
    strcat(g_EventNames[EVT_PLAYER_STREAM_IN],         "PLAYER_STREAM_IN",        32);
    strcat(g_EventNames[EVT_PLAYER_STREAM_OUT],        "PLAYER_STREAM_OUT",       32);
    strcat(g_EventNames[EVT_PLAYER_INTERIOR_CHANGE],   "PLAYER_INTERIOR_CHANGE",  32);
    strcat(g_EventNames[EVT_PLAYER_CLICK_MAP],        "PLAYER_CLICK_MAP",        32);
    strcat(g_EventNames[EVT_PLAYER_CLICK_PTEXTDRAW],  "PLAYER_CLICK_PTEXTDRAW", 32);
    strcat(g_EventNames[EVT_PLAYER_CLICK_TEXTDRAW],  "PLAYER_CLICK_TEXTDRAW", 32);

    // Custom game events
    strcat(g_EventNames[EVT_PLAYER_LOGIN],             "PLAYER_LOGIN",            32);
    strcat(g_EventNames[EVT_PLAYER_REGISTER],          "PLAYER_REGISTER",         32);
    strcat(g_EventNames[EVT_PLAYER_LOGOUT],            "PLAYER_LOGOUT",           32);
    strcat(g_EventNames[EVT_PLAYER_DATA_LOADED],       "PLAYER_DATA_LOADED",      32);
    strcat(g_EventNames[EVT_PLAYER_DATA_SAVED],        "PLAYER_DATA_SAVED",       32);
    strcat(g_EventNames[EVT_DB_CONNECTED],             "DB_CONNECTED",            32);
    strcat(g_EventNames[EVT_PLAYER_HELP],              "PLAYER_HELP",             32);

    // Inventory events
    strcat(g_EventNames[EVT_INV_TEMPLATES_LOADED],     "INV_TEMPLATES_LOADED",    32);
    strcat(g_EventNames[EVT_INV_PLAYER_LOADED],        "INV_PLAYER_LOADED",       32);
    strcat(g_EventNames[EVT_INV_ITEM_ADDED],           "INV_ITEM_ADDED",          32);
    strcat(g_EventNames[EVT_INV_ITEM_REMOVED],         "INV_ITEM_REMOVED",        32);
    strcat(g_EventNames[EVT_INV_ITEM_USE],             "INV_ITEM_USE",            32);
    strcat(g_EventNames[EVT_INV_ITEM_MOVED],           "INV_ITEM_MOVED",          32);

    // Vehicle events
    strcat(g_EventNames[EVT_VEHICLE_CREATED],          "VEHICLE_CREATED",         32);
    strcat(g_EventNames[EVT_VEHICLE_DESTROYED],        "VEHICLE_DESTROYED",       32);
}
