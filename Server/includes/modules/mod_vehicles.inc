// =============================================================================
//  mod_vehicles.inc — Persistent vehicle lifecycle
//
//  Loads vehicles from DB on server start, creates SA-MP vehicles,
//  saves position/health periodically and on shutdown.
//
//  Emits:
//    EVT_VEHICLE_CREATED   — after a persistent vehicle is created in-game
//    EVT_VEHICLE_DESTROYED — before a persistent vehicle is removed
//
//  Public API:
//    Veh_GetDbId(vehicleid)   — SA-MP vehicle id → DB id (-1 if not persistent)
//    Veh_GetSAMPId(vehDbId)   — DB id → SA-MP vehicle id (-1 if not loaded)
//
//  This module knows NOTHING about inventory, containers, trunks, or UI.
//  Other modules react to vehicle events to attach their own behavior.
//
//  Depends on: mod_db, mod_admin (for /vehcreate)
// =============================================================================

#if defined _mod_vehicles_included
    #endinput
#endif
#define _mod_vehicles_included

// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define MAX_PERSISTENT_VEHICLES     200
#define VEH_AUTOSAVE_INTERVAL       300000  // 5 minutes

// -----------------------------------------------------------------------------
//  Per-vehicle data (in-memory mirror of DB)
// -----------------------------------------------------------------------------

enum E_VEHICLE_DATA
{
    veh_DbId,           // DB vehicles.id
    veh_SAMPId,         // SA-MP vehicleid from CreateVehicle
    veh_ModelId,
    Float:veh_PosX,
    Float:veh_PosY,
    Float:veh_PosZ,
    Float:veh_PosA,
    veh_Color1,
    veh_Color2,
    Float:veh_Health,
    bool:veh_Active
}

static g_Vehicles[MAX_PERSISTENT_VEHICLES][E_VEHICLE_DATA];
static g_VehicleCount;
static g_AutoSaveTimer = -1;

// Reverse lookup: SA-MP vehicleid → index in g_Vehicles
// SA-MP vehicle IDs are 1-based, max 2000
#define MAX_SAMP_VEHICLES   2000
static g_SAMPToIdx[MAX_SAMP_VEHICLES + 1]; // -1 = not persistent

// =============================================================================
//  Public API (read-only)
// =============================================================================

stock Veh_GetDbId(vehicleid)
{
    if (vehicleid < 1 || vehicleid > MAX_SAMP_VEHICLES) return -1;
    new idx = g_SAMPToIdx[vehicleid];
    if (idx < 0 || !g_Vehicles[idx][veh_Active]) return -1;
    return g_Vehicles[idx][veh_DbId];
}

stock Veh_GetSAMPId(vehDbId)
{
    for (new i = 0; i < g_VehicleCount; i++)
    {
        if (g_Vehicles[i][veh_Active] && g_Vehicles[i][veh_DbId] == vehDbId)
            return g_Vehicles[i][veh_SAMPId];
    }
    return -1;
}

// =============================================================================
//  Init / Destroy
// =============================================================================

stock Veh_Init()
{
    EventBus_RegisterModule("Vehicles");
    EventBus_Subscribe(EVT_PLAYER_HELP, "Veh_OnPlayerHelp");

    // Reset state
    g_VehicleCount = 0;
    for (new i = 0; i <= MAX_SAMP_VEHICLES; i++)
        g_SAMPToIdx[i] = -1;
    for (new i = 0; i < MAX_PERSISTENT_VEHICLES; i++)
        g_Vehicles[i][veh_Active] = false;

    g_AutoSaveTimer = SetTimer("Veh_AutoSave", VEH_AUTOSAVE_INTERVAL, true);

    print("[MOD] Vehicles module initialized.");

    // Load vehicles from DB if connected
    if (DB_IsConnected())
    {
        mysql_tquery(DB_GetHandle(),
            "SELECT id, model_id, pos_x, pos_y, pos_z, pos_a, color1, color2, health FROM vehicles ORDER BY id",
            "Veh_OnAllLoaded", "");
    }
}

stock Veh_Destroy()
{
    if (g_AutoSaveTimer != -1)
    {
        KillTimer(g_AutoSaveTimer);
        g_AutoSaveTimer = -1;
    }

    // Save all + emit destroy events
    for (new i = 0; i < g_VehicleCount; i++)
    {
        if (!g_Vehicles[i][veh_Active]) continue;

        // Emit destroy event before cleanup
        EventBus_SetInt(EVD_SECONDARY_ID, g_Vehicles[i][veh_SAMPId]);
        EventBus_SetInt(EVD_VALUE, g_Vehicles[i][veh_DbId]);
        EventBus_Emit(EVT_VEHICLE_DESTROYED);

        Veh_SaveOne(i);
        DestroyVehicle(g_Vehicles[i][veh_SAMPId]);
        g_Vehicles[i][veh_Active] = false;
    }

    print("[MOD] Vehicles module destroyed.");
}

// =============================================================================
//  DB Load
// =============================================================================

forward Veh_OnAllLoaded();
public Veh_OnAllLoaded()
{
    new rows;
    cache_get_row_count(rows);

    for (new i = 0; i < rows && i < MAX_PERSISTENT_VEHICLES; i++)
    {
        cache_get_value_name_int(i, "id", g_Vehicles[i][veh_DbId]);
        cache_get_value_name_int(i, "model_id", g_Vehicles[i][veh_ModelId]);
        cache_get_value_name_float(i, "pos_x", g_Vehicles[i][veh_PosX]);
        cache_get_value_name_float(i, "pos_y", g_Vehicles[i][veh_PosY]);
        cache_get_value_name_float(i, "pos_z", g_Vehicles[i][veh_PosZ]);
        cache_get_value_name_float(i, "pos_a", g_Vehicles[i][veh_PosA]);
        cache_get_value_name_int(i, "color1", g_Vehicles[i][veh_Color1]);
        cache_get_value_name_int(i, "color2", g_Vehicles[i][veh_Color2]);
        cache_get_value_name_float(i, "health", g_Vehicles[i][veh_Health]);

        // Create SA-MP vehicle
        new vid = CreateVehicle(
            g_Vehicles[i][veh_ModelId],
            g_Vehicles[i][veh_PosX], g_Vehicles[i][veh_PosY],
            g_Vehicles[i][veh_PosZ], g_Vehicles[i][veh_PosA],
            g_Vehicles[i][veh_Color1], g_Vehicles[i][veh_Color2],
            -1 // respawn delay (-1 = never)
        );

        if (vid == INVALID_VEHICLE_ID)
        {
            printf("[VEH] WARNING: Failed to create vehicle dbId=%d model=%d",
                g_Vehicles[i][veh_DbId], g_Vehicles[i][veh_ModelId]);
            continue;
        }

        SetVehicleHealth(vid, g_Vehicles[i][veh_Health]);

        g_Vehicles[i][veh_SAMPId] = vid;
        g_Vehicles[i][veh_Active] = true;
        g_SAMPToIdx[vid] = i;
        g_VehicleCount++;

        // Emit created event
        EventBus_SetInt(EVD_SECONDARY_ID, vid);
        EventBus_SetInt(EVD_VALUE, g_Vehicles[i][veh_DbId]);
        EventBus_Emit(EVT_VEHICLE_CREATED);
    }

    printf("[VEH] Loaded %d persistent vehicles.", g_VehicleCount);
}

// =============================================================================
//  Save
// =============================================================================

static stock Veh_SaveOne(idx)
{
    if (!g_Vehicles[idx][veh_Active]) return;

    new vid = g_Vehicles[idx][veh_SAMPId];

    // Read current position and health from SA-MP
    new Float:x, Float:y, Float:z, Float:a, Float:hp;
    GetVehiclePos(vid, x, y, z);
    GetVehicleZAngle(vid, a);
    GetVehicleHealth(vid, hp);

    new query[256];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "UPDATE vehicles SET pos_x=%f, pos_y=%f, pos_z=%f, pos_a=%f, health=%f WHERE id=%d",
        x, y, z, a, hp, g_Vehicles[idx][veh_DbId]);
    mysql_tquery(DB_GetHandle(), query);
}

forward Veh_AutoSave();
public Veh_AutoSave()
{
    new count = 0;
    for (new i = 0; i < g_VehicleCount; i++)
    {
        if (g_Vehicles[i][veh_Active])
        {
            Veh_SaveOne(i);
            count++;
        }
    }
    if (count > 0)
        printf("[VEH] Auto-save: saved %d vehicle(s).", count);
}

// =============================================================================
//  Commands
// =============================================================================

forward cmd_vehcreate(playerid, params[]);
public cmd_vehcreate(playerid, params[])
{
    if (!Auth_IsLoggedIn(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "You must be logged in."), 1;
    if (!Admin_RequireLevel(playerid, 2)) return 1;

    new modelid, color1 = -1, color2 = -1;
    if (sscanf(params, "d", modelid))
        return SendClientMessage(playerid, 0xFF4444FF, "Usage: /vehcreate <modelid> [color1] [color2]"), 1;

    // Parse optional colors
    new pos = 0;
    while (params[pos] != EOS && params[pos] != ' ') pos++;
    if (params[pos] == ' ')
    {
        pos++;
        color1 = strval(params[pos]);
        while (params[pos] != EOS && params[pos] != ' ') pos++;
        if (params[pos] == ' ')
        {
            pos++;
            color2 = strval(params[pos]);
        }
    }

    if (modelid < 400 || modelid > 611)
        return SendClientMessage(playerid, 0xFF4444FF, "Invalid model ID (400-611)."), 1;

    if (g_VehicleCount >= MAX_PERSISTENT_VEHICLES)
        return SendClientMessage(playerid, 0xFF4444FF, "Max persistent vehicles reached."), 1;

    // Get player position
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    x += 3.0; // offset so vehicle doesn't spawn on top of player

    // Insert into DB
    new query[256];
    mysql_format(DB_GetHandle(), query, sizeof(query),
        "INSERT INTO vehicles (model_id, pos_x, pos_y, pos_z, pos_a, color1, color2) VALUES (%d, %f, %f, %f, %f, %d, %d)",
        modelid, x, y, z, a, color1, color2);
    mysql_tquery(DB_GetHandle(), query, "Veh_OnCreated", "ddfffffdd",
        playerid, modelid, x, y, z, a, 1000.0, color1, color2);
    return 1;
}

forward Veh_OnCreated(playerid, modelid, Float:x, Float:y, Float:z, Float:a, Float:hp, color1, color2);
public Veh_OnCreated(playerid, modelid, Float:x, Float:y, Float:z, Float:a, Float:hp, color1, color2)
{
    new dbId = cache_insert_id();
    new idx = g_VehicleCount;

    if (idx >= MAX_PERSISTENT_VEHICLES) return;

    new vid = CreateVehicle(modelid, x, y, z, a, color1, color2, -1);
    if (vid == INVALID_VEHICLE_ID)
    {
        if (IsPlayerConnected(playerid))
            SendClientMessage(playerid, 0xFF4444FF, "Failed to create vehicle.");
        return;
    }

    SetVehicleHealth(vid, hp);

    g_Vehicles[idx][veh_DbId]   = dbId;
    g_Vehicles[idx][veh_SAMPId] = vid;
    g_Vehicles[idx][veh_ModelId]= modelid;
    g_Vehicles[idx][veh_PosX]   = x;
    g_Vehicles[idx][veh_PosY]   = y;
    g_Vehicles[idx][veh_PosZ]   = z;
    g_Vehicles[idx][veh_PosA]   = a;
    g_Vehicles[idx][veh_Color1] = color1;
    g_Vehicles[idx][veh_Color2] = color2;
    g_Vehicles[idx][veh_Health] = hp;
    g_Vehicles[idx][veh_Active] = true;
    g_SAMPToIdx[vid] = idx;
    g_VehicleCount++;

    // Emit created event
    EventBus_SetInt(EVD_SECONDARY_ID, vid);
    EventBus_SetInt(EVD_VALUE, dbId);
    EventBus_Emit(EVT_VEHICLE_CREATED);

    if (IsPlayerConnected(playerid))
    {
        new msg[128];
        format(msg, sizeof(msg), "Created vehicle model %d (dbId=%d, vehId=%d)", modelid, dbId, vid);
        SendClientMessage(playerid, 0x44FF44FF, msg);
    }
}

// =============================================================================
//  Event Handlers
// =============================================================================

forward Veh_OnPlayerHelp();
public Veh_OnPlayerHelp()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    if (Admin_GetLevel(playerid) >= 2)
        SendClientMessage(playerid, 0xCCCCCCFF, "/vehcreate <model> [c1] [c2]");
    return 1;
}
