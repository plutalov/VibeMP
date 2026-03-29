# mod_food_temp — Medical Item Handler (test module)

Temporary test consumer for the inventory system. Handles items with category "medical" — bandage heals 25 HP and consumes 1 from the stack.

**This is a test/example module.** Replace with a proper `mod_medical.inc` when building the real medical system.

## Public API

None.

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_INV_ITEM_USE` | Checks category "medical", heals player, removes 1 item |
| Subscribes | `EVT_PLAYER_HELP` | Prints usage hint in /help |

## Dependencies

- `mod_inventory` — `Inv_GetTemplateCategory()`, `Inv_RemoveItem()`

## How it works

1. Player uses an item (via `/useitem` or clicking in the grid)
2. `EVT_INV_ITEM_USE` fires with template index and metadata
3. Handler checks `Inv_GetTemplateCategory()` — if not "medical", returns (not our item)
4. Heals player by 25 HP (capped at 100)
5. Removes 1 from the stack via `Inv_RemoveItem()`

## Example for module authors

This module demonstrates the pattern for writing an inventory item consumer:

```pawn
forward MyMod_OnItemUse();
public MyMod_OnItemUse()
{
    new templateIdx = EventBus_GetInt(EVD_SECONDARY_ID);
    new category[MAX_CATEGORY_LEN];
    Inv_GetTemplateCategory(templateIdx, category, sizeof(category));
    if (strcmp(category, "my_category") != 0) return 1; // not mine

    // Handle the item...
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new slotIdx  = EventBus_GetInt(EVD_VALUE);
    new cIdx     = Inv_GetPlayerContainerIdx(playerid);
    Inv_RemoveItem(cIdx, slotIdx, 1); // consume
    return 1;
}
```
