# Module Guide — How to Create a Module

## Step by Step

### 1. Create the file

Create `includes/modules/mod_<name>.inc`. Use lowercase, underscores for multi-word names.

### 2. Add the include guard

```pawn
#if defined _mod_<name>_included
    #endinput
#endif
#define _mod_<name>_included
```

### 3. Define module-local state

Use `static` for all module-internal variables. This prevents other modules from accessing them directly.

```pawn
static g_MyData[MAX_PLAYERS];
static g_MyCounter;
```

### 4. Write Init and Destroy

```pawn
stock MyMod_Init()
{
    EventBus_RegisterModule("MyMod");

    // Subscribe to events you care about
    EventBus_Subscribe(EVT_PLAYER_CONNECT, "MyMod_OnPlayerConnect");
    EventBus_Subscribe(EVT_PLAYER_DEATH,   "MyMod_OnPlayerDeath");

    print("[MOD] MyMod module initialized.");
}

stock MyMod_Destroy()
{
    print("[MOD] MyMod module destroyed.");
}
```

### 5. Write event handlers

Use the `MODULE_HANDLER` macro to cut boilerplate:

```pawn
MODULE_HANDLER(MyMod, OnPlayerConnect)
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    g_MyData[playerid] = 0;
    return 1;
}

MODULE_HANDLER(MyMod, OnPlayerDeath)
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new killerid = EventBus_GetInt(EVD_SECONDARY_ID);

    g_MyData[playerid]++;

    // Emit a custom event if needed
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_VALUE, g_MyData[playerid]);
    EventBus_Emit(EVT_MY_CUSTOM_EVENT);

    return 1;
}
```

### 6. Expose a public API (optional)

If other modules need to query your module's state, expose stock functions:

```pawn
stock MyMod_GetDeathCount(playerid)
{
    if (playerid < 0 || playerid >= MAX_PLAYERS) return 0;
    return g_MyData[playerid];
}
```

### 7. Add custom events (if needed)

Add new event IDs in `includes/core/events.inc`:

```pawn
#define EVT_MY_CUSTOM_EVENT  130
```

Add the name in `EventBus_InitNames()`:

```pawn
strcat(g_EventNames[EVT_MY_CUSTOM_EVENT], "MY_CUSTOM_EVENT", 32);
```

### 8. Wire it into mygamemode.pwn

Add three lines:

```pawn
// At the top, with other includes:
#include <modules/mod_mymod>

// In OnGameModeInit, after other Init() calls:
MyMod_Init();

// In OnGameModeExit, before other Destroy() calls (reverse order):
MyMod_Destroy();
```

### 9. Update the compile command if needed

The compile command already includes `-i./includes`, so no change needed unless you added a new include directory.

### 10. Add tests (optional but encouraged)

```pawn
stock MyMod_RunTests()
{
    Test_EmitEvent(EVT_PLAYER_CONNECT, .playerid = 0);
    Test_AssertEqual("MyMod: connect resets counter", MyMod_GetDeathCount(0), 0);
}
```

Wire into the test runner in `mod_debug.inc`.

## Complete Template

```pawn
// =============================================================================
//  mod_example.inc — One-line description
// =============================================================================

#if defined _mod_example_included
    #endinput
#endif
#define _mod_example_included

// --- Module state ---
static g_ExampleData[MAX_PLAYERS];

// --- Public API ---
stock Example_GetData(playerid)
{
    if (playerid < 0 || playerid >= MAX_PLAYERS) return 0;
    return g_ExampleData[playerid];
}

// --- Init / Destroy ---
stock Example_Init()
{
    EventBus_RegisterModule("Example");
    EventBus_Subscribe(EVT_PLAYER_CONNECT, "Example_OnConnect");
    print("[MOD] Example module initialized.");
}

stock Example_Destroy()
{
    print("[MOD] Example module destroyed.");
}

// --- Handlers ---
forward Example_OnConnect();
public Example_OnConnect()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    g_ExampleData[playerid] = 0;
    return 1;
}
```

## Pawn Gotchas

### No multi-line macros for forward/public

Pawn's preprocessor cannot expand a macro into both `forward` and `public` declarations. Always write them explicitly:

```pawn
// WRONG — does not compile
MODULE_HANDLER(MyMod, OnPlayerConnect) { ... }

// CORRECT
forward MyMod_OnPlayerConnect();
public MyMod_OnPlayerConnect() { ... }
```

### OMP tag mismatches on callbacks

OMP includes use tagged types on some callback parameters (e.g., `WEAPON:reason`). When passing tagged values to the event bus (which uses plain integers), cast with `_:`:

```pawn
// In the bridge callback:
public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    EventBus_SetInt(EVD_VALUE, _:reason);  // cast WEAPON tag to int
    ...
}
```

Similarly, use `WEAPON_FIST` instead of `0` for weapon parameters in `AddPlayerClass`.

## Common Mistakes

| Mistake | What Happens | Fix |
|---------|-------------|-----|
| Forgot `forward`/`public` on handler | `funcidx` warning at subscribe, handler never called | Write explicit `forward` + `public` |
| Reading event data after a sub-emit | You get stale/restored data | Read into locals at handler top |
| Defining event ID in module file | ID collision with another module | All IDs go in `events.inc` |
| Handler name > 31 chars | Silently truncated, subscribe fails | Shorten the name |
| Using `new` instead of `static` for module state | Other modules can see your variables | Always use `static` |
| Emitting the same event you're handling | Cycle detection halts dispatch | Design acyclic event flows |
