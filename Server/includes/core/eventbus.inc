// =============================================================================
//  eventbus.inc — Event dispatcher with cycle detection and debug logging
//
//  USAGE:
//    1. Call EventBus_Init() in OnGameModeInit before any module init.
//    2. Modules call EventBus_Subscribe() during their Init().
//    3. Bridge callbacks set event data then call EventBus_Emit().
//    4. Handlers read data via EventBus_GetInt/GetFloat/GetString.
//
//  CRITICAL RULE FOR HANDLERS:
//    Always read event data into LOCAL variables at the TOP of your handler
//    before doing anything that might emit another event. Sub-emits save and
//    restore the data, but if you hold a reference and emit, the restored
//    data after the sub-emit will overwrite what you set.
// =============================================================================

#if defined _eventbus_included
    #endinput
#endif
#define _eventbus_included

// Must be included after events.inc
#if !defined MAX_EVENTS
    #error "Include events.inc before eventbus.inc"
#endif

// -----------------------------------------------------------------------------
//  Configuration
// -----------------------------------------------------------------------------

#define MAX_MODULES             16
#define MAX_HANDLERS_PER_EVENT  8
#define MAX_EVENT_DEPTH         10
#define MAX_EVENT_DATA_SLOTS    8
#define MAX_HANDLER_NAME        32

// -----------------------------------------------------------------------------
//  Event data slots (convention)
// -----------------------------------------------------------------------------

#define EVD_PLAYER_ID           0
#define EVD_SECONDARY_ID        1
#define EVD_VALUE               2
#define EVD_EXTRA               3
#define EVD_EXTRA2              4
#define EVD_EXTRA3              5

// -----------------------------------------------------------------------------
//  Shared event data (the "payload")
// -----------------------------------------------------------------------------

enum E_EVENT_DATA
{
    evd_Int[MAX_EVENT_DATA_SLOTS],
    Float:evd_Float[MAX_EVENT_DATA_SLOTS],
    evd_String[128]
}

new g_EventData[E_EVENT_DATA];

// Save stack for nested emits — data is saved/restored automatically.
new g_SavedEventData[MAX_EVENT_DEPTH][E_EVENT_DATA];

// -----------------------------------------------------------------------------
//  Event result (for callbacks that need return values)
//
//  Merge modes control how multiple handlers' results combine:
//    MERGE_LAST    — last SetResult wins (default, for events where return
//                    value doesn't matter)
//    MERGE_ANY_ZERO — if ANY handler sets 0, result is 0
//                     (RequestSpawn, Text — 0 = block/suppress)
//    MERGE_ANY_ONE  — if ANY handler sets 1, result is 1
//                     (CommandText — 1 = handled)
// -----------------------------------------------------------------------------

#define MERGE_LAST      0
#define MERGE_ANY_ZERO  1
#define MERGE_ANY_ONE   2

new g_EventResult;
new g_EventMergeMode[MAX_EVENTS];
new g_EventDefaultResult[MAX_EVENTS];

// -----------------------------------------------------------------------------
//  Handler registry
// -----------------------------------------------------------------------------

new g_Handlers[MAX_EVENTS][MAX_HANDLERS_PER_EVENT][MAX_HANDLER_NAME];
new g_HandlerCount[MAX_EVENTS];

// -----------------------------------------------------------------------------
//  Module registry
// -----------------------------------------------------------------------------

enum E_MODULE
{
    mod_Name[24],
    bool:mod_Active
}

new g_Modules[MAX_MODULES][E_MODULE];
new g_ModuleCount;

// -----------------------------------------------------------------------------
//  Cycle detection
// -----------------------------------------------------------------------------

new g_DispatchStack[MAX_EVENT_DEPTH];
new g_DispatchDepth;

// -----------------------------------------------------------------------------
//  Debug flag
// -----------------------------------------------------------------------------

new bool:g_DebugEvents = false;

// -----------------------------------------------------------------------------
//  Data accessors
// -----------------------------------------------------------------------------

stock EventBus_SetInt(slot, value)
{
    if (slot < 0 || slot >= MAX_EVENT_DATA_SLOTS) return;
    g_EventData[evd_Int][slot] = value;
}

stock EventBus_GetInt(slot)
{
    if (slot < 0 || slot >= MAX_EVENT_DATA_SLOTS) return 0;
    return g_EventData[evd_Int][slot];
}

stock EventBus_SetFloat(slot, Float:value)
{
    if (slot < 0 || slot >= MAX_EVENT_DATA_SLOTS) return;
    g_EventData[evd_Float][slot] = value;
}

stock Float:EventBus_GetFloat(slot)
{
    if (slot < 0 || slot >= MAX_EVENT_DATA_SLOTS) return 0.0;
    return g_EventData[evd_Float][slot];
}

stock EventBus_SetString(const value[])
{
    g_EventData[evd_String][0] = EOS;
    strcat(g_EventData[evd_String], value, 128);
}

stock EventBus_GetString(dest[], maxlen)
{
    dest[0] = EOS;
    strcat(dest, g_EventData[evd_String], maxlen);
}

stock EventBus_SetResult(value)
{
    // Apply merge mode for the currently dispatching event.
    // If not inside a dispatch (depth 0), just set directly.
    if (g_DispatchDepth > 0)
    {
        new eventId = g_DispatchStack[g_DispatchDepth - 1];
        switch (g_EventMergeMode[eventId])
        {
            case MERGE_ANY_ZERO:
            {
                // Once 0, stays 0 — any handler can block
                if (value == 0)
                    g_EventResult = 0;
                return;
            }
            case MERGE_ANY_ONE:
            {
                // Once 1, stays 1 — any handler can claim "handled"
                if (value == 1)
                    g_EventResult = 1;
                return;
            }
            // MERGE_LAST: fall through to direct set
        }
    }
    g_EventResult = value;
}

stock EventBus_GetResult()
{
    return g_EventResult;
}

// Configure how an event's result is merged across multiple handlers.
// Call during init, before any emit.
stock EventBus_SetMergeMode(eventId, mergeMode, defaultResult)
{
    if (eventId < 0 || eventId >= MAX_EVENTS) return;
    g_EventMergeMode[eventId] = mergeMode;
    g_EventDefaultResult[eventId] = defaultResult;
}

// -----------------------------------------------------------------------------
//  Module registration
// -----------------------------------------------------------------------------

stock EventBus_RegisterModule(const name[])
{
    if (g_ModuleCount >= MAX_MODULES)
    {
        printf("[EVENTBUS ERROR] Cannot register module '%s': MAX_MODULES (%d) reached.", name, MAX_MODULES);
        return -1;
    }

    new idx = g_ModuleCount;
    g_Modules[idx][mod_Name][0] = EOS;
    strcat(g_Modules[idx][mod_Name], name, 24);
    g_Modules[idx][mod_Active] = true;
    g_ModuleCount++;

    printf("[EVENTBUS] Module registered: %s", name);
    return idx;
}

// -----------------------------------------------------------------------------
//  Handler subscription
// -----------------------------------------------------------------------------

stock EventBus_Subscribe(eventId, const handlerName[])
{
    if (eventId < 0 || eventId >= MAX_EVENTS)
    {
        printf("[EVENTBUS ERROR] Subscribe failed: invalid event ID %d for handler '%s'.", eventId, handlerName);
        return 0;
    }

    if (g_HandlerCount[eventId] >= MAX_HANDLERS_PER_EVENT)
    {
        printf("[EVENTBUS ERROR] Subscribe failed: event %s (id=%d) has max %d handlers. Cannot add '%s'.",
            g_EventNames[eventId], eventId, MAX_HANDLERS_PER_EVENT, handlerName);
        return 0;
    }

    // Validate function exists
    if (funcidx(handlerName) == -1)
    {
        printf("[EVENTBUS WARNING] Handler '%s' not found! Did you forget forward/public?", handlerName);
        // Still register it — the function might be defined later via
        // include ordering. But warn loudly.
    }

    new idx = g_HandlerCount[eventId];
    g_Handlers[eventId][idx][0] = EOS;
    strcat(g_Handlers[eventId][idx], handlerName, MAX_HANDLER_NAME);
    g_HandlerCount[eventId]++;

    if (g_DebugEvents)
    {
        printf("[EVENTBUS] Subscribed '%s' to event %s (id=%d, handler #%d)",
            handlerName, g_EventNames[eventId], eventId, idx);
    }

    return 1;
}

// -----------------------------------------------------------------------------
//  Event dispatch
// -----------------------------------------------------------------------------

stock EventBus_Emit(eventId)
{
    if (eventId < 0 || eventId >= MAX_EVENTS)
    {
        printf("[EVENTBUS ERROR] Emit failed: invalid event ID %d.", eventId);
        return;
    }

    // No handlers? Skip entirely.
    if (g_HandlerCount[eventId] == 0)
        return;

    // ----- CYCLE DETECTION -----

    // Check for exact duplicate (same event re-entered in the stack)
    for (new i = 0; i < g_DispatchDepth; i++)
    {
        if (g_DispatchStack[i] == eventId)
        {
            new msg[256];
            print("[EVENTBUS FATAL] Cycle detected! Event chain:");

            for (new j = 0; j < g_DispatchDepth; j++)
            {
                format(msg, sizeof(msg), "  [%d] %s (id=%d)",
                    j, g_EventNames[g_DispatchStack[j]], g_DispatchStack[j]);
                print(msg);
            }

            format(msg, sizeof(msg), "  [%d] -> %s (id=%d)  ** CYCLE **",
                g_DispatchDepth, g_EventNames[eventId], eventId);
            print(msg);

            print("[EVENTBUS FATAL] Dispatch HALTED. Fix the cycle above.");
            return;
        }
    }

    // Check max depth
    if (g_DispatchDepth >= MAX_EVENT_DEPTH)
    {
        new msg[256];
        format(msg, sizeof(msg),
            "[EVENTBUS FATAL] Max dispatch depth (%d) exceeded! Chain:",
            MAX_EVENT_DEPTH);
        print(msg);

        for (new i = 0; i < g_DispatchDepth; i++)
        {
            format(msg, sizeof(msg), "  [%d] %s (id=%d)",
                i, g_EventNames[g_DispatchStack[i]], g_DispatchStack[i]);
            print(msg);
        }

        format(msg, sizeof(msg), "  [%d] -> %s (id=%d)  ** DEPTH LIMIT **",
            g_DispatchDepth, g_EventNames[eventId], eventId);
        print(msg);

        print("[EVENTBUS FATAL] Dispatch HALTED. Fix the chain above.");
        return;
    }

    // ----- PUSH ONTO STACK -----

    new depth = g_DispatchDepth;
    g_DispatchStack[depth] = eventId;
    g_DispatchDepth++;

    // ----- DEBUG LOG -----

    if (g_DebugEvents)
    {
        new h, m, s;
        gettime(h, m, s);
        printf("[EVT %02d:%02d:%02d] %s (id=%d) player=%d depth=%d handlers=%d",
            h, m, s,
            g_EventNames[eventId], eventId,
            g_EventData[evd_Int][EVD_PLAYER_ID],
            g_DispatchDepth,
            g_HandlerCount[eventId]);
    }

    // ----- RESET RESULT (use per-event default) -----

    g_EventResult = g_EventDefaultResult[eventId];

    // ----- SAVE EVENT DATA (for nested emit protection) -----

    // Copy current event data to save stack
    for (new s2 = 0; s2 < MAX_EVENT_DATA_SLOTS; s2++)
    {
        g_SavedEventData[depth][evd_Int][s2] = g_EventData[evd_Int][s2];
        g_SavedEventData[depth][evd_Float][s2] = g_EventData[evd_Float][s2];
    }
    g_SavedEventData[depth][evd_String][0] = EOS;
    strcat(g_SavedEventData[depth][evd_String], g_EventData[evd_String], 128);

    // ----- DISPATCH ALL HANDLERS -----

    for (new i = 0; i < g_HandlerCount[eventId]; i++)
    {
        CallLocalFunction(g_Handlers[eventId][i], "");
    }

    // ----- RESTORE EVENT DATA -----

    for (new s3 = 0; s3 < MAX_EVENT_DATA_SLOTS; s3++)
    {
        g_EventData[evd_Int][s3] = g_SavedEventData[depth][evd_Int][s3];
        g_EventData[evd_Float][s3] = g_SavedEventData[depth][evd_Float][s3];
    }
    g_EventData[evd_String][0] = EOS;
    strcat(g_EventData[evd_String], g_SavedEventData[depth][evd_String], 128);

    // ----- POP FROM STACK -----

    g_DispatchDepth--;
}

// -----------------------------------------------------------------------------
//  Debug helpers
// -----------------------------------------------------------------------------

stock EventBus_SetDebug(bool:enabled)
{
    g_DebugEvents = enabled;
}

stock EventBus_DumpHandlers()
{
    print("=== EVENT BUS HANDLER DUMP ===");

    for (new e = 0; e < MAX_EVENTS; e++)
    {
        if (g_HandlerCount[e] == 0)
            continue;

        new msg[256];
        format(msg, sizeof(msg), "  %s (id=%d): %d handler(s)",
            g_EventNames[e], e, g_HandlerCount[e]);
        print(msg);

        for (new h = 0; h < g_HandlerCount[e]; h++)
        {
            format(msg, sizeof(msg), "    [%d] %s", h, g_Handlers[e][h]);
            print(msg);
        }
    }

    printf("  Modules: %d registered", g_ModuleCount);
    for (new m = 0; m < g_ModuleCount; m++)
    {
        printf("    [%d] %s (active=%d)", m, g_Modules[m][mod_Name], g_Modules[m][mod_Active]);
    }

    print("=== END DUMP ===");
}

// -----------------------------------------------------------------------------
//  Initialization
// -----------------------------------------------------------------------------

stock EventBus_Init()
{
    g_DispatchDepth = 0;
    g_ModuleCount = 0;
    g_EventResult = 1;
    g_DebugEvents = false;

    // Zero handler counts and set default merge modes
    for (new i = 0; i < MAX_EVENTS; i++)
    {
        g_HandlerCount[i] = 0;
        g_EventMergeMode[i] = MERGE_LAST;
        g_EventDefaultResult[i] = 1;
    }

    // Configure merge modes for SA-MP callbacks with return value semantics.
    //
    // MERGE_ANY_ZERO (default 1): any handler can block (set 0)
    //   - RequestSpawn: 0 = block spawn, 1 = allow
    //   - RequestClass: 0 = block, 1 = allow
    //   - PlayerText:   0 = suppress message, 1 = show
    //   - PlayerUpdate: 0 = don't sync, 1 = sync
    EventBus_SetMergeMode(EVT_PLAYER_REQUEST_SPAWN, MERGE_ANY_ZERO, 1);
    EventBus_SetMergeMode(EVT_PLAYER_REQUEST_CLASS, MERGE_ANY_ZERO, 1);
    EventBus_SetMergeMode(EVT_PLAYER_TEXT,          MERGE_ANY_ZERO, 1);
    EventBus_SetMergeMode(EVT_PLAYER_UPDATE,        MERGE_ANY_ZERO, 1);

    // MERGE_ANY_ONE (default 0): any handler can claim "handled" (set 1)
    //   - CommandText: 0 = not handled (shows "unknown command"), 1 = handled
    EventBus_SetMergeMode(EVT_PLAYER_COMMAND, MERGE_ANY_ONE, 0);

    // Populate event names for debug output
    EventBus_InitNames();

    print("[EVENTBUS] Initialized.");
}
