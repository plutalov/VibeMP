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

1. `EVT_INV_ITEM_USE` fires with template index, slot, and **container index in EVD_EXTRA**
2. Handler checks category — if not "medical", returns
3. Heals 25 HP, removes 1 from stack using the container from EVD_EXTRA

**Key:** Uses `EVD_EXTRA` for container index — works whether item is in player inventory, vehicle trunk, or any other container.

## Example for module authors

```pawn
forward MyMod_OnItemUse();
public MyMod_OnItemUse()
{
    new playerid    = EventBus_GetInt(EVD_PLAYER_ID);
    new templateIdx = EventBus_GetInt(EVD_SECONDARY_ID);
    new slotIdx     = EventBus_GetInt(EVD_VALUE);
    new cIdx        = EventBus_GetInt(EVD_EXTRA); // container the item is in

    new category[MAX_CATEGORY_LEN];
    Inv_GetTemplateCategory(templateIdx, category, sizeof(category));
    if (strcmp(category, "my_category") != 0) return 1; // not mine

    Inv_RemoveItem(cIdx, slotIdx, 1); // consume from whichever container
    return 1;
}
```
