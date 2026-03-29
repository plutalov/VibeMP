// =============================================================================
//  mod_phone_temp.inc — Temporary test consumer for inventory (electronic items)
//
//  Subscribes to EVT_INV_ITEM_USE, handles items with category "electronic".
//  Phone: shows a placeholder dialog.
//
//  This is a test/example module. Replace with a proper mod_phone.inc later.
//
//  Depends on: mod_inventory, mod_auth
// =============================================================================

#if defined _mod_phone_temp_included
    #endinput
#endif
#define _mod_phone_temp_included

#define DIALOG_PHONE_MAIN       60

stock PhoneTemp_Init()
{
    EventBus_RegisterModule("PhoneTemp");
    EventBus_Subscribe(EVT_INV_ITEM_USE, "PhoneTemp_OnItemUse");
    EventBus_Subscribe(EVT_PLAYER_HELP, "PhoneTemp_OnPlayerHelp");
    print("[MOD] PhoneTemp module initialized.");
}

stock PhoneTemp_Destroy()
{
    print("[MOD] PhoneTemp module destroyed.");
}

forward PhoneTemp_OnItemUse();
public PhoneTemp_OnItemUse()
{
    new playerid    = EventBus_GetInt(EVD_PLAYER_ID);
    new templateIdx = EventBus_GetInt(EVD_SECONDARY_ID);

    // Check if this is our category
    new category[MAX_CATEGORY_LEN];
    Inv_GetTemplateCategory(templateIdx, category, sizeof(category));
    if (strcmp(category, "electronic") != 0) return 1; // not ours

    // Show phone dialog (placeholder)
    ShowPlayerDialog(playerid, DIALOG_PHONE_MAIN, DIALOG_STYLE_LIST,
        "Phone",
        "Call\nSMS\nContacts\nSettings",
        "Select", "Close");

    SendClientMessage(playerid, 0xFFFFFFFF, "You pull out your phone.");
    return 1;
}

forward PhoneTemp_OnPlayerHelp();
public PhoneTemp_OnPlayerHelp()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    SendClientMessage(playerid, 0xCCCCCCFF, "Use phone from /inv or /useitem <slot>");
    return 1;
}
