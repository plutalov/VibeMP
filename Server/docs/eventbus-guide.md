# Event Bus Guide

## What It Is

The event bus is the communication backbone of the server. Instead of systems calling each other directly, they communicate through events. System A emits an event, systems B and C react to it — without knowing about each other.

## How Events Flow

```
SA-MP Callback (e.g., OnPlayerDeath)
  → mygamemode.pwn sets event data
  → EventBus_Emit(EVT_PLAYER_DEATH)
  → Dispatcher calls all subscribed handlers
  → Handlers can emit their own events (sub-events)
```

## Event Data

Events carry data through a global payload. **Do not pass data through CallLocalFunction args** — everything goes through the shared data slots.

### Integer Slots (0–7)

| Slot | Constant | Convention |
|------|----------|------------|
| 0 | `EVD_PLAYER_ID` | The player this event is about |
| 1 | `EVD_SECONDARY_ID` | Second entity (killerid, vehicleid, dialogid) |
| 2 | `EVD_VALUE` | Primary value (reason, response, amount) |
| 3 | `EVD_EXTRA` | Extra context (listitem, oldstate) |
| 4+ | `EVD_EXTRA2`, `EVD_EXTRA3` | Module-specific |

### Float Slots (0–7)

Same indices, used for coordinates, angles, etc.

### String Slot

One 128-char string. Used for command text, chat messages, passwords.

### Reading Data

```pawn
new playerid = EventBus_GetInt(EVD_PLAYER_ID);
new killerid = EventBus_GetInt(EVD_SECONDARY_ID);
new Float:x  = EventBus_GetFloat(0);
new cmd[128];
EventBus_GetString(cmd, sizeof(cmd));
```

### Setting Data (before emitting)

```pawn
EventBus_SetInt(EVD_PLAYER_ID, playerid);
EventBus_SetInt(EVD_VALUE, amount);
EventBus_SetFloat(0, x);
EventBus_SetString("some text");
EventBus_Emit(EVT_MY_EVENT);
```

## Event Result (Return Values)

Some SA-MP callbacks need return values (0 = block, 1 = allow). The event bus provides:

```pawn
EventBus_SetResult(0);  // block the action
EventBus_GetResult();   // read in the bridge callback
```

### Merge Modes

When multiple handlers set a result for the same event, the **merge mode** controls how they combine. This prevents handler ordering from silently changing behavior.

| Mode | Constant | Behavior | Use case |
|------|----------|----------|----------|
| Last wins | `MERGE_LAST` | Last `SetResult` call wins | Events where return doesn't matter |
| Any zero | `MERGE_ANY_ZERO` | Once 0, stays 0 — any handler can block | RequestSpawn, Text, Update |
| Any one | `MERGE_ANY_ONE` | Once 1, stays 1 — any handler can claim handled | CommandText |

### Configured SA-MP events

| Event | Merge Mode | Default | Meaning |
|-------|-----------|---------|---------|
| `EVT_PLAYER_REQUEST_SPAWN` | ANY_ZERO | 1 (allow) | Any handler can block spawn |
| `EVT_PLAYER_REQUEST_CLASS` | ANY_ZERO | 1 (allow) | Any handler can block |
| `EVT_PLAYER_TEXT` | ANY_ZERO | 1 (show) | Any handler can suppress chat |
| `EVT_PLAYER_UPDATE` | ANY_ZERO | 1 (sync) | Any handler can block sync |
| `EVT_PLAYER_COMMAND` | ANY_ONE | 0 (unhandled) | Any handler can claim handled |
| Everything else | LAST | 1 | Last `SetResult` wins |

All handlers always run regardless of result — no early exit. This ensures logging and analytics modules always see every event.

### Configuring merge mode for custom events

If your custom event needs a specific merge mode, set it in `EventBus_Init()` (in `eventbus.inc`):

```pawn
EventBus_SetMergeMode(EVT_MY_CUSTOM_EVENT, MERGE_ANY_ZERO, 1);
```

## Cycle Detection

The event bus tracks which events are currently being dispatched. If an event tries to fire while it's already in the dispatch stack, you get:

```
[EVENTBUS FATAL] Cycle detected! Event chain:
  [0] PLAYER_DEATH (id=5)
  [1] ITEM_DROP (id=110)
  [2] -> PLAYER_DEATH (id=5)  ** CYCLE **
[EVENTBUS FATAL] Dispatch HALTED. Fix the cycle above.
```

There's also a max depth of 10. If events cascade deeper than that (even without a direct cycle), dispatch halts with the same loud error.

**This is intentional. Cycles are bugs. Fix them, don't work around them.**

## Debug Commands

- `/debug` — Toggle event logging. When on, every event dispatch prints to console with timestamp, event name, playerid, depth, and handler count.
- `/events` — Dump all registered handlers to server console.
- `/test` — Run the test suite.

## Rules You Must Follow

### 1. Read Data Into Locals First

At the **top** of every handler, copy what you need into local variables:

```pawn
MODULE_HANDLER(MyMod, OnPlayerDeath)
{
    // GOOD — read into locals immediately
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new killerid = EventBus_GetInt(EVD_SECONDARY_ID);

    // Now it's safe to emit sub-events
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_Emit(EVT_PLAYER_LOGOUT);

    // playerid and killerid are still valid (they're locals)
    // EventBus_GetInt(EVD_PLAYER_ID) might return something different now
}
```

### 2. All Event IDs Go in events.inc

Never define event IDs in module files. All IDs live in `includes/core/events.inc` to prevent collisions.

### 3. Don't Emit Your Own Event

If you're handling EVT_PLAYER_DEATH, don't emit EVT_PLAYER_DEATH inside that handler. The cycle detector will catch it, but design your flow to be acyclic.

### 4. Handler Naming

`<ModuleName>_On<EventName>` — keep under 31 characters.

### 5. Init Order = Priority

First module to subscribe gets called first. If handler ordering matters between modules, document it in the module's Init() comment.
