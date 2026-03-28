# RPC Trace Reference — Real SA-MP Client Session

Recorded 2026-03-28. Fresh DB, registration flow.
Player: Pluto. Actions: connect → register → spawn → walk → /help → quit.

## Phase 1: Connect + Auth Dialog

```
RECV  id=25   ClientJoin              ← client sends join request
SEND  id=60   PlayerUpdate            ← server requests player state
SEND  id=139  InitGame                ← server sends game init (player ID, settings)
SEND  id=72   SetPlayerColor          ← server assigns color
SEND  id=61   ScrDialogBox            ← server shows Register dialog
```

## Phase 2: Class Selection (automatic, under the dialog)

```
RECV  id=128  RequestClass            ← client auto-selects class 0 (IMPORTANT: bot must do this too!)
SEND  id=69   SetPlayerTeam           ← server responds with team
SEND  id=128  RequestClass response   ← server sends spawn info for class
```

**Key finding**: The real client sends `RequestClass` (id=128) immediately after receiving `InitGame`, even while the dialog is visible on top. This happens BEFORE the dialog response.

## Phase 3: Registration Dialog Response

```
RECV  id=62   DialogResponse (96 bits) ← client submits password
SEND  id=93   ClientMessage (376 bits) ← "Account created! You are now logged in."
```

**Note on bit count**: Real client sends 96 bits for dialog response. Our bot sends 136 bits. The difference is the password length (real: shorter password, bot: "testpass123" = 11 chars).

## Phase 4: Post-Login Data Load + Spawn

```
[PDATA] OnPlayerLogin: loading data for playerid=0
[PDATA] OnDataLoad: pos=-2233.96,-1737.57,480.54 hp=100.0

SEND  id=20   ResetPlayerWeapons
SEND  id=18   HaveSomeMoney           ← set money
SEND  id=153  SetPlayerSkin
SEND  id=156  SetInterior
SEND  id=69   SetPlayerTeam
SEND  id=68   SetSpawnInfo            ← spawn position from DB (Mount Chiliad)
SEND  id=129  RequestSpawn            ← server tells client "you can spawn now"
```

## Phase 5: Client Spawns

```
RECV  id=52   Spawn                   ← client confirms spawn (NOT RequestSpawn+Spawn like bot!)
SEND  id=67   ???                     ← (unknown RPC, 32 bits)
SEND  id=14   SetPlayerHealth
SEND  id=66   SetPlayerArmour
SEND  id=69   SetPlayerTeam           ← set again after spawn
SEND  id=68   SetSpawnInfo            ← set again (for respawn after death)
SEND  id=93   ClientMessage           ← "Welcome! Use /help for commands."
SEND  id=60   PlayerUpdate            ← request sync data
```

**Key finding**: Real client sends ONLY `Spawn` (id=52), not `RequestSpawn` (id=129) + `Spawn` (id=52). The server already sent `RequestSpawn` (id=129) to the client as a "go ahead" signal. The client responds with just `Spawn`.

## Phase 6: Walking (sync data, RPCs filtered out)

Continuous `PKT-RECV id=207` (OnFootSync) + `PKT-RECV id=205` (AimSync) + `PKT-RECV id=203` (StatsUpdate).
Periodic `PKT-SEND id=208` (server sync broadcast).

## Phase 7: /help Command

```
RECV  id=50   ServerCommand (72 bits) ← "/help"
SEND  id=93   ClientMessage (208 bits) ← help header
SEND  id=93   ClientMessage (360 bits) ← help body
```

## Phase 8: Disconnect

```
[PDATA] OnPlayerLogout: saving data for playerid=0
SEND  id=138  ServerQuit (broadcast)  ← notify all players
[part] Pluto has left the server (0:1)
```

---

## Differences: Real Client vs Bot

| Step | Real Client | Bot | Status |
|------|-------------|-----|--------|
| After InitGame | Sends `RequestClass` (id=128) | Sends `requestClass(0)` | **FIXED** |
| Dialog response listItem | -1 for password dialogs | -1 | **FIXED** |
| Spawn | Sends only `Spawn` (id=52) | Sends only `Spawn` (id=52) | **FIXED** |
| Sync data | Sends `ID_PLAYER_SYNC` ~30fps | Auto-sends via `onFootUpdateAtNormalPos()` | **FIXED** |
| Dialog response bits | 96 bits | 136 bits | OK — password length difference |

All critical differences have been resolved. The bot's RPC sequence now matches the real client.
