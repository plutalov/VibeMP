// =============================================================================
//  mod_inventory_ui.inc — Visual inventory grid using per-player textdraws
//
//  Renders a 4x5 grid of item slots with 3D model previews.
//  Click a slot to use the item. ESC or close button to dismiss.
//
//  Textdraw budget: 44 of 256 per-player textdraws
//    - 1 background, 1 title, 1 weight, 1 close button
//    - 20 slots × 2 (model preview + quantity label)
//
//  Depends on: mod_inventory, mod_auth
// =============================================================================

#if defined _mod_inventory_ui_included
    #endinput
#endif
#define _mod_inventory_ui_included

// Layout constants
#define INVUI_COLS              4
#define INVUI_ROWS              5
#define INVUI_CELL_W            55.0
#define INVUI_CELL_H            55.0
#define INVUI_PADDING           4.0
#define INVUI_START_X           320.0
#define INVUI_START_Y           120.0
#define INVUI_BG_COLOR          0x00000099
#define INVUI_SLOT_COLOR        0x222222CC
#define INVUI_HOVER_COLOR       0x44FF4466
#define INVUI_TEXT_COLOR        0xFFFFFFFF

// Per-player UI state
enum E_INV_UI
{
    PlayerText:ui_Bg,
    PlayerText:ui_Title,
    PlayerText:ui_Weight,
    PlayerText:ui_Close,
    PlayerText:ui_SlotBg[INV_SLOTS],
    PlayerText:ui_SlotModel[INV_SLOTS],
    PlayerText:ui_SlotQty[INV_SLOTS],
    bool:ui_Open,
    bool:ui_Created,
    ui_ContainerIdx             // which container is currently displayed
}

static g_UI[MAX_PLAYERS][E_INV_UI];

// =============================================================================
//  Init / Destroy
// =============================================================================

stock InvUI_Init()
{
    EventBus_RegisterModule("InvUI");
    EventBus_Subscribe(EVT_PLAYER_CLICK_PTEXTDRAW, "InvUI_OnClickTD");
    EventBus_Subscribe(EVT_PLAYER_CONNECT,          "InvUI_OnConnect");
    EventBus_Subscribe(EVT_PLAYER_DISCONNECT,       "InvUI_OnDisconn");
    EventBus_Subscribe(EVT_INV_ITEM_ADDED,          "InvUI_OnItemChanged");
    EventBus_Subscribe(EVT_INV_ITEM_REMOVED,        "InvUI_OnItemChanged");
    print("[MOD] InventoryUI module initialized.");
}

stock InvUI_Destroy()
{
    print("[MOD] InventoryUI module destroyed.");
}

// =============================================================================
//  Textdraw Creation (once per player, on first open)
// =============================================================================

static stock InvUI_Create(playerid)
{
    if (g_UI[playerid][ui_Created]) return;

    new Float:totalW = INVUI_COLS * INVUI_CELL_W + (INVUI_COLS + 1) * INVUI_PADDING;

    // Layout edges — slots start at INVUI_START_X + INVUI_PADDING
    // so the left content margin is at that same X
    new Float:contentL = INVUI_START_X + INVUI_PADDING;    // left edge of first slot
    new Float:contentR = INVUI_START_X + totalW;           // right edge of last slot

    // Header + background panel
    new Float:bgTop = INVUI_START_Y - 20.0;
    g_UI[playerid][ui_Bg] = CreatePlayerTextDraw(playerid, INVUI_START_X - INVUI_PADDING, bgTop,
        "~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~");
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_Bg], INVUI_START_X + totalW + INVUI_PADDING, 0.0);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_Bg], 0.0, 1.2);
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_Bg], TEXT_DRAW_FONT:2);
    PlayerTextDrawUseBox(playerid, g_UI[playerid][ui_Bg], true);
    PlayerTextDrawBoxColor(playerid, g_UI[playerid][ui_Bg], 0x000000CC);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_Bg], 0x00000000); // invisible text
    PlayerTextDrawSetOutline(playerid, g_UI[playerid][ui_Bg], 0);
    PlayerTextDrawSetShadow(playerid, g_UI[playerid][ui_Bg], 0);

    // Title — aligned with left edge of first slot
    g_UI[playerid][ui_Title] = CreatePlayerTextDraw(playerid, contentL, bgTop + 5.0, "INVENTORY");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_Title], TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_Title], 0.28, 1.2);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_Title], INVUI_TEXT_COLOR);
    PlayerTextDrawSetOutline(playerid, g_UI[playerid][ui_Title], 0);
    PlayerTextDrawSetShadow(playerid, g_UI[playerid][ui_Title], 0);

    // Weight — right-aligned, just left of the X button
    g_UI[playerid][ui_Weight] = CreatePlayerTextDraw(playerid, contentR - 15.0, bgTop + 7.0, "0.0 / 30.0 kg");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_Weight], TEXT_DRAW_FONT:1);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_Weight], 0.2, 1.0);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_Weight], 0xCCCCCCFF);
    PlayerTextDrawSetOutline(playerid, g_UI[playerid][ui_Weight], 1);
    PlayerTextDrawAlignment(playerid, g_UI[playerid][ui_Weight], TEXT_DRAW_ALIGN:3);

    // Close button — aligned with right edge of last slot
    g_UI[playerid][ui_Close] = CreatePlayerTextDraw(playerid, contentR - 10.0, bgTop + 6.0, "X");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_Close], TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_Close], 0.25, 1.0);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_Close], 0xFF4444FF);
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_Close], contentR, 15.0);
    PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_Close], true);

    // Item slots (4x5 grid)
    for (new row = 0; row < INVUI_ROWS; row++)
    {
        for (new col = 0; col < INVUI_COLS; col++)
        {
            new slot = row * INVUI_COLS + col;
            new Float:x = INVUI_START_X + INVUI_PADDING + col * (INVUI_CELL_W + INVUI_PADDING);
            new Float:y = INVUI_START_Y + INVUI_PADDING + row * (INVUI_CELL_H + INVUI_PADDING);

            // Slot background (always visible — gives the grid structure)
            // Use model preview font with model 0 and transparent preview for a clean box
            g_UI[playerid][ui_SlotBg][slot] = CreatePlayerTextDraw(playerid, x, y, "_");
            PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_SlotBg][slot], INVUI_CELL_W, INVUI_CELL_H);
            PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_SlotBg][slot], 0.0, 0.0);
            PlayerTextDrawFont(playerid, g_UI[playerid][ui_SlotBg][slot], TEXT_DRAW_FONT_MODEL_PREVIEW);
            PlayerTextDrawSetPreviewModel(playerid, g_UI[playerid][ui_SlotBg][slot], 19300); // invisible object
            PlayerTextDrawBackgroundColor(playerid, g_UI[playerid][ui_SlotBg][slot], INVUI_SLOT_COLOR);
            PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_SlotBg][slot], true);

            // 3D model preview (shown on top of bg, only for occupied slots)
            g_UI[playerid][ui_SlotModel][slot] = CreatePlayerTextDraw(playerid, x, y, "_");
            PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_SlotModel][slot], INVUI_CELL_W, INVUI_CELL_H);
            PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_SlotModel][slot], 0.0, 0.0);
            PlayerTextDrawFont(playerid, g_UI[playerid][ui_SlotModel][slot], TEXT_DRAW_FONT_MODEL_PREVIEW);
            PlayerTextDrawSetPreviewModel(playerid, g_UI[playerid][ui_SlotModel][slot], 0);
            PlayerTextDrawSetPreviewRot(playerid, g_UI[playerid][ui_SlotModel][slot], -16.0, 0.0, -55.0, 1.0);
            PlayerTextDrawBackgroundColor(playerid, g_UI[playerid][ui_SlotModel][slot], INVUI_SLOT_COLOR);
            PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_SlotModel][slot], true);

            // Quantity label (bottom-right corner of slot)
            g_UI[playerid][ui_SlotQty][slot] = CreatePlayerTextDraw(playerid, x + INVUI_CELL_W - 5.0, y + INVUI_CELL_H - 12.0, "");
            PlayerTextDrawFont(playerid, g_UI[playerid][ui_SlotQty][slot], TEXT_DRAW_FONT:1);
            PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_SlotQty][slot], 0.2, 0.9);
            PlayerTextDrawColor(playerid, g_UI[playerid][ui_SlotQty][slot], INVUI_TEXT_COLOR);
            PlayerTextDrawSetOutline(playerid, g_UI[playerid][ui_SlotQty][slot], 1);
            PlayerTextDrawAlignment(playerid, g_UI[playerid][ui_SlotQty][slot], TEXT_DRAW_ALIGN:3); // right-align
        }
    }

    g_UI[playerid][ui_Created] = true;
}

// =============================================================================
//  Refresh (update slot models and quantities from data layer)
// =============================================================================

static stock InvUI_Refresh(playerid)
{
    new cIdx = g_UI[playerid][ui_ContainerIdx];
    if (cIdx == INVALID_CONTAINER_IDX) return;

    for (new i = 0; i < INV_SLOTS; i++)
    {
        if (Inv_IsSlotActive(cIdx, i))
        {
            new tIdx = Inv_GetItemTemplateIdx(cIdx, i);
            new modelId = Inv_GetTemplateModelId(tIdx);
            new qty = Inv_GetItemQuantity(cIdx, i);

            PlayerTextDrawSetPreviewModel(playerid, g_UI[playerid][ui_SlotModel][i], modelId);
            PlayerTextDrawShow(playerid, g_UI[playerid][ui_SlotModel][i]);

            if (qty > 1)
            {
                new qstr[8];
                format(qstr, sizeof(qstr), "%d", qty);
                PlayerTextDrawSetString(playerid, g_UI[playerid][ui_SlotQty][i], qstr);
            }
            else
            {
                PlayerTextDrawSetString(playerid, g_UI[playerid][ui_SlotQty][i], " ");
            }
            PlayerTextDrawShow(playerid, g_UI[playerid][ui_SlotQty][i]);
        }
        else
        {
            // Empty slot — hide model, show empty background
            PlayerTextDrawHide(playerid, g_UI[playerid][ui_SlotModel][i]);
            PlayerTextDrawHide(playerid, g_UI[playerid][ui_SlotQty][i]);
        }
    }

    // Update weight
    new wstr[20];
    format(wstr, sizeof(wstr), "%.1f / %.1f kg",
        Inv_GetContainerWeight(cIdx), Inv_GetContainerMaxWeight(cIdx));
    PlayerTextDrawSetString(playerid, g_UI[playerid][ui_Weight], wstr);
}

// =============================================================================
//  Open / Close
// =============================================================================

/// Open dual-panel view: player inventory (left) + external container (right).
/// Full dual-panel rendering comes in Phase 4 — for now opens the external container.
stock InvUI_OpenDual(playerid, externalContainerIdx, const title[])
{
    #pragma unused title
    // TODO Phase 4: render two panels side by side
    // For now, open the external container in the single-panel view
    InvUI_Open(playerid, externalContainerIdx);
}

stock InvUI_Open(playerid, containerIdx = INVALID_CONTAINER_IDX)
{
    // Default to player's own inventory
    if (containerIdx == INVALID_CONTAINER_IDX)
        containerIdx = Inv_GetPlayerContainerIdx(playerid);
    if (containerIdx == INVALID_CONTAINER_IDX) return;
    if (g_UI[playerid][ui_Open]) return;

    g_UI[playerid][ui_ContainerIdx] = containerIdx;

    if (!g_UI[playerid][ui_Created])
        InvUI_Create(playerid);

    // Show chrome
    PlayerTextDrawShow(playerid, g_UI[playerid][ui_Bg]);
    PlayerTextDrawShow(playerid, g_UI[playerid][ui_Title]);
    PlayerTextDrawShow(playerid, g_UI[playerid][ui_Weight]);
    PlayerTextDrawShow(playerid, g_UI[playerid][ui_Close]);

    // Show all slot backgrounds (always visible for grid structure)
    for (new i = 0; i < INV_SLOTS; i++)
        PlayerTextDrawShow(playerid, g_UI[playerid][ui_SlotBg][i]);

    // Refresh item models/quantities on top
    InvUI_Refresh(playerid);

    // Enable click selection
    SelectTextDraw(playerid, INVUI_HOVER_COLOR);

    g_UI[playerid][ui_Open] = true;
}

stock InvUI_Close(playerid)
{
    if (!g_UI[playerid][ui_Open]) return;

    // Hide everything
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_Bg]);
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_Title]);
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_Weight]);
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_Close]);

    for (new i = 0; i < INV_SLOTS; i++)
    {
        PlayerTextDrawHide(playerid, g_UI[playerid][ui_SlotBg][i]);
        PlayerTextDrawHide(playerid, g_UI[playerid][ui_SlotModel][i]);
        PlayerTextDrawHide(playerid, g_UI[playerid][ui_SlotQty][i]);
    }

    CancelSelectTextDraw(playerid);
    g_UI[playerid][ui_Open] = false;
}

// =============================================================================
//  Event Handlers
// =============================================================================

// --- Textdraw click ---
forward InvUI_OnClickTD();
public InvUI_OnClickTD()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new PlayerText:tdid = PlayerText:EventBus_GetInt(EVD_SECONDARY_ID);

    if (!g_UI[playerid][ui_Open]) return 1;

    // ESC pressed (invalid textdraw)
    if (tdid == PlayerText:INVALID_PLAYER_TEXT_DRAW)
    {
        InvUI_Close(playerid);
        return 1;
    }

    // Close button
    if (tdid == g_UI[playerid][ui_Close])
    {
        InvUI_Close(playerid);
        return 1;
    }

    // Check item slots (model click or background click)
    for (new i = 0; i < INV_SLOTS; i++)
    {
        if (tdid == g_UI[playerid][ui_SlotModel][i] || tdid == g_UI[playerid][ui_SlotBg][i])
        {
            new cIdx = g_UI[playerid][ui_ContainerIdx];
            if (Inv_IsSlotActive(cIdx, i))
            {
                Inv_PlayerUseItem(playerid, i);
                // Refresh UI after use (item may have been consumed)
                InvUI_Refresh(playerid);
            }
            // Re-enable selection (gets cancelled after click)
            SelectTextDraw(playerid, INVUI_HOVER_COLOR);
            return 1;
        }
    }

    return 1;
}

// --- Item changed: refresh if UI is open ---
forward InvUI_OnItemChanged();
public InvUI_OnItemChanged()
{
    // Refresh all open UIs (simple approach — only players with UI open)
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (g_UI[i][ui_Open])
            InvUI_Refresh(i);
    }
    return 1;
}

// --- Connect: reset UI state ---
forward InvUI_OnConnect();
public InvUI_OnConnect()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    g_UI[playerid][ui_Open] = false;
    g_UI[playerid][ui_Created] = false;
    g_UI[playerid][ui_ContainerIdx] = INVALID_CONTAINER_IDX;
    CancelSelectTextDraw(playerid);
    return 1;
}

// --- Disconnect: cleanup ---
forward InvUI_OnDisconn();
public InvUI_OnDisconn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    // Player textdraws are auto-destroyed on disconnect
    g_UI[playerid][ui_Open] = false;
    g_UI[playerid][ui_Created] = false;
    return 1;
}
