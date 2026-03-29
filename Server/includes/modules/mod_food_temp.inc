// =============================================================================
//  mod_food_temp.inc — Temporary test consumer for inventory (medical items)
//
//  Subscribes to EVT_INV_ITEM_USE, handles items with category "medical".
//  Bandage: heals 25 HP, consumes 1 from stack.
//
//  This is a test/example module. Replace with a proper mod_medical.inc later.
//
//  Depends on: mod_inventory, mod_auth
// =============================================================================

#if defined _mod_food_temp_included
    #endinput
#endif
#define _mod_food_temp_included

#define BANDAGE_HEAL_AMOUNT     25.0

stock FoodTemp_Init()
{
    EventBus_RegisterModule("FoodTemp");
    EventBus_Subscribe(EVT_INV_ITEM_USE, "Food_OnItemUse");
    EventBus_Subscribe(EVT_PLAYER_HELP, "Food_OnPlayerHelp");
    print("[MOD] FoodTemp module initialized.");
}

stock FoodTemp_Destroy()
{
    print("[MOD] FoodTemp module destroyed.");
}

forward Food_OnItemUse();
public Food_OnItemUse()
{
    new playerid    = EventBus_GetInt(EVD_PLAYER_ID);
    new templateIdx = EventBus_GetInt(EVD_SECONDARY_ID);
    new slotIdx     = EventBus_GetInt(EVD_VALUE);

    // Check if this is our category
    new category[MAX_CATEGORY_LEN];
    Inv_GetTemplateCategory(templateIdx, category, sizeof(category));
    if (strcmp(category, "medical") != 0) return 1; // not ours

    // Get item name for the message
    new name[MAX_ITEM_NAME_LEN];
    Inv_GetTemplateName(templateIdx, name, sizeof(name));

    // Apply healing
    new Float:hp;
    GetPlayerHealth(playerid, hp);
    new Float:newHp = hp + BANDAGE_HEAL_AMOUNT;
    if (newHp > 100.0) newHp = 100.0;
    SetPlayerHealth(playerid, newHp);

    // Consume 1 from stack
    new cIdx = Inv_GetPlayerContainerIdx(playerid);
    Inv_RemoveItem(cIdx, slotIdx, 1);

    new msg[128];
    format(msg, sizeof(msg), "Used %s. Health: %.0f -> %.0f", name, hp, newHp);
    SendClientMessage(playerid, 0x44FF44FF, msg);
    return 1;
}

forward Food_OnPlayerHelp();
public Food_OnPlayerHelp()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    SendClientMessage(playerid, 0xCCCCCCFF, "Use bandages from /inv or /useitem <slot> to heal");
    return 1;
}
