// =============================================================================
//  mod_veh_trunk.inc — Bridge: vehicles ↔ inventory
//
//  Subscribes to vehicle lifecycle events and creates/frees trunk containers.
//  Owns the /trunk command (find nearest vehicle, open dual-panel inventory).
//
//  This is the only module that knows about both vehicles and inventory.
//  Pattern: for any new container type (house safe, backpack), create a
//  similar bridge module that subscribes to the relevant lifecycle events.
//
//  Depends on: mod_vehicles, mod_inventory, mod_inventory_ui
// =============================================================================

#if defined _mod_veh_trunk_included
    #endinput
#endif
#define _mod_veh_trunk_included

#define VEH_TRUNK_MAX_WEIGHT    50.0
#define VEH_TRUNK_MAX_DISTANCE  10.0

stock VehTrunk_Init()
{
    EventBus_RegisterModule("VehTrunk");
    EventBus_Subscribe(EVT_VEHICLE_CREATED,   "VehTrunk_OnVehCreated");
    EventBus_Subscribe(EVT_VEHICLE_DESTROYED, "VehTrunk_OnVehDestroyed");
    EventBus_Subscribe(EVT_PLAYER_HELP,       "VehTrunk_OnPlayerHelp");
    print("[MOD] VehTrunk module initialized.");
}

stock VehTrunk_Destroy()
{
    print("[MOD] VehTrunk module destroyed.");
}

// =============================================================================
//  Event Handlers
// =============================================================================

// Vehicle created → load/create its trunk container
forward VehTrunk_OnVehCreated();
public VehTrunk_OnVehCreated()
{
    new vehDbId = EventBus_GetInt(EVD_VALUE);
    Inv_LoadContainerByOwner(OWNER_VEHICLE, vehDbId, VEH_TRUNK_MAX_WEIGHT);
    return 1;
}

// Vehicle destroyed → save and free its trunk container
forward VehTrunk_OnVehDestroyed();
public VehTrunk_OnVehDestroyed()
{
    new vehDbId = EventBus_GetInt(EVD_VALUE);
    new containerIdx = Inv_FindContainerByOwner(OWNER_VEHICLE, vehDbId);
    if (containerIdx != INVALID_CONTAINER_IDX)
        Inv_SaveAndFreeContainer(containerIdx);
    return 1;
}

// Help
forward VehTrunk_OnPlayerHelp();
public VehTrunk_OnPlayerHelp()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    SendClientMessage(playerid, 0xCCCCCCFF, "/trunk — open nearby vehicle trunk");
    return 1;
}

// =============================================================================
//  Commands
// =============================================================================

forward cmd_trunk(playerid, params[]);
public cmd_trunk(playerid, params[])
{
    #pragma unused params
    if (!PlayerInv_IsLoaded(playerid))
        return SendClientMessage(playerid, 0xFF4444FF, "Inventory not loaded yet."), 1;

    // Find nearest persistent vehicle
    new Float:px, Float:py, Float:pz;
    GetPlayerPos(playerid, px, py, pz);

    new bestVeh = INVALID_VEHICLE_ID;
    new Float:bestDist = VEH_TRUNK_MAX_DISTANCE;

    // Check all SA-MP vehicles for proximity
    for (new vid = 1; vid <= MAX_VEHICLES; vid++)
    {
        // Only persistent vehicles have a DB id
        if (Veh_GetDbId(vid) == -1) continue;

        new Float:vx, Float:vy, Float:vz;
        GetVehiclePos(vid, vx, vy, vz);

        new Float:dist = floatsqroot(
            (px - vx) * (px - vx) +
            (py - vy) * (py - vy) +
            (pz - vz) * (pz - vz)
        );

        if (dist < bestDist)
        {
            bestDist = dist;
            bestVeh = vid;
        }
    }

    if (bestVeh == INVALID_VEHICLE_ID)
        return SendClientMessage(playerid, 0xFF4444FF, "No vehicle nearby."), 1;

    new vehDbId = Veh_GetDbId(bestVeh);
    new containerIdx = Inv_FindContainerByOwner(OWNER_VEHICLE, vehDbId);

    if (containerIdx == INVALID_CONTAINER_IDX)
        return SendClientMessage(playerid, 0xFF4444FF, "This vehicle has no trunk loaded."), 1;

    new playerContainer = PlayerInv_GetContainerIdx(playerid);
    if (playerContainer == INVALID_CONTAINER_IDX)
        return SendClientMessage(playerid, 0xFF4444FF, "Inventory not loaded yet."), 1;

    InvUI_OpenDual(playerid, playerContainer, containerIdx, "TRUNK");
    return 1;
}
