// =============================================================================
//  mod_inventory_ui.inc — Dual-panel inventory UI with context menu
//
//  Renders one or two 4×5 grids with 3D model previews.
//  Click item → context menu (Use / Move / Drop).
//  Move mode: select source, click destination in either panel.
//
//  Layout mode (which panels visible) is separate from interaction state.
//  State machine: IDLE → CONTEXT_MENU → MOVE_MODE (orthogonal to layout).
//
//  Textdraw budget: 91 of 256 per-player
//    - 2 panels × (bg + title + weight + 20 slots + 20 qty labels) = 86
//    - 1 close button + 4 context menu = 5
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
#define INVUI_HEADER_H          22.0

// Panel origins
#define INVUI_SINGLE_X          320.0   // single panel position
#define INVUI_SINGLE_Y          100.0
#define INVUI_LEFT_X            75.0    // dual: left panel
#define INVUI_LEFT_Y            100.0
#define INVUI_RIGHT_X           330.0   // dual: right panel
#define INVUI_RIGHT_Y           100.0

// Colors
#define INVUI_BG_COLOR          0x000000CC
#define INVUI_SLOT_COLOR        0x222222CC
#define INVUI_SLOT_HIGHLIGHT    0x44FF4466
#define INVUI_HOVER_COLOR       0x44FF4466
#define INVUI_TEXT_COLOR        0xFFFFFFFF
#define INVUI_EMPTY_MODEL       19300   // invisible object for empty slots

// Interaction states
#define INVUI_STATE_IDLE        0
#define INVUI_STATE_CONTEXT     1
#define INVUI_STATE_MOVE        2

// Per-player UI state
enum E_INV_UI
{
    // Left panel (always player inventory)
    PlayerText:ui_LBg,
    PlayerText:ui_LTitle,
    PlayerText:ui_LWeight,
    PlayerText:ui_LSlot[INV_SLOTS],
    PlayerText:ui_LQty[INV_SLOTS],

    // Right panel (external container — hidden in single mode)
    PlayerText:ui_RBg,
    PlayerText:ui_RTitle,
    PlayerText:ui_RWeight,
    PlayerText:ui_RSlot[INV_SLOTS],
    PlayerText:ui_RQty[INV_SLOTS],

    // Close buttons (one per layout mode — textdraws can't be repositioned)
    PlayerText:ui_CloseL,   // for single mode (on left panel's right edge)
    PlayerText:ui_CloseR,   // for dual mode (on right panel's right edge)

    // Context menu
    PlayerText:ui_CtxBg,
    PlayerText:ui_CtxUse,
    PlayerText:ui_CtxMove,
    PlayerText:ui_CtxDrop,
    PlayerText:ui_CtxCancel,

    // State
    bool:ui_Open,
    bool:ui_Created,
    bool:ui_Dual,               // true = two panels, false = one
    ui_LeftContainer,
    ui_RightContainer,
    ui_State,                   // INVUI_STATE_*
    ui_CtxPanel,                // 0=left, 1=right
    ui_CtxSlot,                 // which slot context menu is for
    ui_MoveSrcPanel,
    ui_MoveSrcSlot
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
    EventBus_Subscribe(EVT_INV_ITEM_MOVED,          "InvUI_OnItemChanged");
    print("[MOD] InventoryUI module initialized.");
}

stock InvUI_Destroy()
{
    print("[MOD] InventoryUI module destroyed.");
}

// =============================================================================
//  Panel Creation Helper (creates one panel at given origin)
// =============================================================================

static stock InvUI_CreatePanel(playerid,
    PlayerText:slotTDs[], PlayerText:qtyTDs[],
    &PlayerText:bgTD, &PlayerText:titleTD, &PlayerText:weightTD,
    Float:originX, Float:originY, const title[])
{
    new Float:totalW = INVUI_COLS * INVUI_CELL_W + (INVUI_COLS + 1) * INVUI_PADDING;
    new Float:contentL = originX + INVUI_PADDING;
    new Float:contentR = originX + totalW + INVUI_PADDING;

    // Background — sized to cover header + 5 rows of 55px cells + padding
    bgTD = CreatePlayerTextDraw(playerid, originX, originY,
        "~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~");
    PlayerTextDrawTextSize(playerid, bgTD, contentR, 0.0);
    PlayerTextDrawLetterSize(playerid, bgTD, 0.0, 1.05);
    PlayerTextDrawFont(playerid, bgTD, TEXT_DRAW_FONT:2);
    PlayerTextDrawUseBox(playerid, bgTD, true);
    PlayerTextDrawBoxColor(playerid, bgTD, INVUI_BG_COLOR);
    PlayerTextDrawColor(playerid, bgTD, 0x00000000);
    PlayerTextDrawSetOutline(playerid, bgTD, 0);
    PlayerTextDrawSetShadow(playerid, bgTD, 0);

    // Title — left-aligned with slots
    titleTD = CreatePlayerTextDraw(playerid, contentL, originY + 5.0, title);
    PlayerTextDrawFont(playerid, titleTD, TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, titleTD, 0.28, 1.2);
    PlayerTextDrawColor(playerid, titleTD, INVUI_TEXT_COLOR);
    PlayerTextDrawSetOutline(playerid, titleTD, 0);
    PlayerTextDrawSetShadow(playerid, titleTD, 0);

    // Weight — right-aligned, leaving room for X button on the right
    new Float:slotsRight = contentL + INVUI_COLS * (INVUI_CELL_W + INVUI_PADDING) - INVUI_PADDING;
    weightTD = CreatePlayerTextDraw(playerid, slotsRight - 15.0, originY + 7.0, "0.0 / 30.0 kg");
    PlayerTextDrawFont(playerid, weightTD, TEXT_DRAW_FONT:1);
    PlayerTextDrawLetterSize(playerid, weightTD, 0.2, 1.0);
    PlayerTextDrawColor(playerid, weightTD, 0xCCCCCCFF);
    PlayerTextDrawSetOutline(playerid, weightTD, 1);
    PlayerTextDrawAlignment(playerid, weightTD, TEXT_DRAW_ALIGN:3);

    // Slots (combined model preview + background in one textdraw)
    new Float:gridStartY = originY + INVUI_HEADER_H;
    for (new row = 0; row < INVUI_ROWS; row++)
    {
        for (new col = 0; col < INVUI_COLS; col++)
        {
            new slot = row * INVUI_COLS + col;
            new Float:x = contentL + col * (INVUI_CELL_W + INVUI_PADDING);
            new Float:y = gridStartY + row * (INVUI_CELL_H + INVUI_PADDING);

            // Model preview (doubles as background for empty slots)
            slotTDs[slot] = CreatePlayerTextDraw(playerid, x, y, "_");
            PlayerTextDrawTextSize(playerid, slotTDs[slot], INVUI_CELL_W, INVUI_CELL_H);
            PlayerTextDrawLetterSize(playerid, slotTDs[slot], 0.0, 0.0);
            PlayerTextDrawFont(playerid, slotTDs[slot], TEXT_DRAW_FONT_MODEL_PREVIEW);
            PlayerTextDrawSetPreviewModel(playerid, slotTDs[slot], INVUI_EMPTY_MODEL);
            PlayerTextDrawSetPreviewRot(playerid, slotTDs[slot], -16.0, 0.0, -55.0, 1.0);
            PlayerTextDrawBackgroundColor(playerid, slotTDs[slot], INVUI_SLOT_COLOR);
            PlayerTextDrawSetSelectable(playerid, slotTDs[slot], true);

            // Quantity label
            qtyTDs[slot] = CreatePlayerTextDraw(playerid, x + INVUI_CELL_W - 5.0, y + INVUI_CELL_H - 12.0, " ");
            PlayerTextDrawFont(playerid, qtyTDs[slot], TEXT_DRAW_FONT:1);
            PlayerTextDrawLetterSize(playerid, qtyTDs[slot], 0.2, 0.9);
            PlayerTextDrawColor(playerid, qtyTDs[slot], INVUI_TEXT_COLOR);
            PlayerTextDrawSetOutline(playerid, qtyTDs[slot], 1);
            PlayerTextDrawAlignment(playerid, qtyTDs[slot], TEXT_DRAW_ALIGN:3);
        }
    }
}

// =============================================================================
//  Full Creation (both panels + close + context menu)
// =============================================================================

static stock InvUI_Create(playerid)
{
    if (g_UI[playerid][ui_Created]) return;

    // Left panel (positioned for dual; single mode will use these same TDs)
    InvUI_CreatePanel(playerid,
        g_UI[playerid][ui_LSlot], g_UI[playerid][ui_LQty],
        g_UI[playerid][ui_LBg], g_UI[playerid][ui_LTitle], g_UI[playerid][ui_LWeight],
        INVUI_LEFT_X, INVUI_LEFT_Y, "INVENTORY");

    // Right panel
    InvUI_CreatePanel(playerid,
        g_UI[playerid][ui_RSlot], g_UI[playerid][ui_RQty],
        g_UI[playerid][ui_RBg], g_UI[playerid][ui_RTitle], g_UI[playerid][ui_RWeight],
        INVUI_RIGHT_X, INVUI_RIGHT_Y, "CONTAINER");

    // Close buttons — aligned with last slot column's right edge
    // slotsRight = contentL + COLS * (CELL_W + PAD) - PAD
    new Float:lSlotsR = INVUI_LEFT_X + INVUI_PADDING + INVUI_COLS * (INVUI_CELL_W + INVUI_PADDING) - INVUI_PADDING;
    new Float:rSlotsR = INVUI_RIGHT_X + INVUI_PADDING + INVUI_COLS * (INVUI_CELL_W + INVUI_PADDING) - INVUI_PADDING;

    // Single mode: X at left panel slot right edge
    g_UI[playerid][ui_CloseL] = CreatePlayerTextDraw(playerid, lSlotsR - 8.0, INVUI_LEFT_Y + 6.0, "X");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_CloseL], TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_CloseL], 0.25, 1.0);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_CloseL], 0xFF4444FF);
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_CloseL], lSlotsR + 5.0, 10.0);
    PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_CloseL], true);

    // Dual mode: X at right panel slot right edge
    g_UI[playerid][ui_CloseR] = CreatePlayerTextDraw(playerid, rSlotsR - 8.0, INVUI_RIGHT_Y + 6.0, "X");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_CloseR], TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_CloseR], 0.25, 1.0);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_CloseR], 0xFF4444FF);
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_CloseR], rSlotsR + 5.0, 10.0);
    PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_CloseR], true);

    // Context menu (fixed position, center screen, spaced out)
    new Float:ctxX = 275.0, Float:ctxY = 160.0;
    new Float:ctxW = 80.0;
    new Float:ctxItemH = 28.0;

    g_UI[playerid][ui_CtxBg] = CreatePlayerTextDraw(playerid, ctxX, ctxY, "~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~~n~");
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_CtxBg], ctxX + ctxW, 0.0);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_CtxBg], 0.0, 1.0);
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_CtxBg], TEXT_DRAW_FONT:2);
    PlayerTextDrawUseBox(playerid, g_UI[playerid][ui_CtxBg], true);
    PlayerTextDrawBoxColor(playerid, g_UI[playerid][ui_CtxBg], 0x000000EE);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_CtxBg], 0x00000000);

    g_UI[playerid][ui_CtxUse] = CreatePlayerTextDraw(playerid, ctxX + 10.0, ctxY + 8.0, "Use");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_CtxUse], TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_CtxUse], 0.35, 1.6);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_CtxUse], 0x44FF44FF);
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_CtxUse], ctxX + ctxW - 5.0, 15.0);
    PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_CtxUse], true);

    g_UI[playerid][ui_CtxMove] = CreatePlayerTextDraw(playerid, ctxX + 10.0, ctxY + 8.0 + ctxItemH, "Move");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_CtxMove], TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_CtxMove], 0.35, 1.6);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_CtxMove], 0xFFFF44FF);
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_CtxMove], ctxX + ctxW - 5.0, 15.0);
    PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_CtxMove], true);

    g_UI[playerid][ui_CtxDrop] = CreatePlayerTextDraw(playerid, ctxX + 10.0, ctxY + 8.0 + ctxItemH * 2, "Drop");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_CtxDrop], TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_CtxDrop], 0.35, 1.6);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_CtxDrop], 0xFF4444FF);
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_CtxDrop], ctxX + ctxW - 5.0, 15.0);
    PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_CtxDrop], true);

    g_UI[playerid][ui_CtxCancel] = CreatePlayerTextDraw(playerid, ctxX + 10.0, ctxY + 8.0 + ctxItemH * 3, "Cancel");
    PlayerTextDrawFont(playerid, g_UI[playerid][ui_CtxCancel], TEXT_DRAW_FONT:2);
    PlayerTextDrawLetterSize(playerid, g_UI[playerid][ui_CtxCancel], 0.35, 1.6);
    PlayerTextDrawColor(playerid, g_UI[playerid][ui_CtxCancel], 0xAAAAAAFF);
    PlayerTextDrawTextSize(playerid, g_UI[playerid][ui_CtxCancel], ctxX + ctxW - 5.0, 15.0);
    PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_CtxCancel], true);

    g_UI[playerid][ui_Created] = true;
}

// =============================================================================
//  Refresh (update slots from data layer)
// =============================================================================

static stock InvUI_RefreshPanel(playerid, PlayerText:slotTDs[], PlayerText:qtyTDs[],
    PlayerText:weightTD, containerIdx)
{
    if (containerIdx == INVALID_CONTAINER_IDX) return;

    for (new i = 0; i < INV_SLOTS; i++)
    {
        if (Inv_IsSlotActive(containerIdx, i))
        {
            new tIdx = Inv_GetItemTemplateIdx(containerIdx, i);
            new modelId = Inv_GetTemplateModelId(tIdx);
            new qty = Inv_GetItemQuantity(containerIdx, i);

            PlayerTextDrawSetPreviewModel(playerid, slotTDs[i], modelId);
            PlayerTextDrawShow(playerid, slotTDs[i]);

            if (qty > 1)
            {
                new qstr[8];
                format(qstr, sizeof(qstr), "%d", qty);
                PlayerTextDrawSetString(playerid, qtyTDs[i], qstr);
            }
            else
            {
                PlayerTextDrawSetString(playerid, qtyTDs[i], " ");
            }
            PlayerTextDrawShow(playerid, qtyTDs[i]);
        }
        else
        {
            PlayerTextDrawSetPreviewModel(playerid, slotTDs[i], INVUI_EMPTY_MODEL);
            PlayerTextDrawShow(playerid, slotTDs[i]);
            PlayerTextDrawSetString(playerid, qtyTDs[i], " ");
            PlayerTextDrawShow(playerid, qtyTDs[i]);
        }
    }

    // Update weight
    new wstr[20];
    format(wstr, sizeof(wstr), "%.1f / %.1f kg",
        Inv_GetContainerWeight(containerIdx), Inv_GetContainerMaxWeight(containerIdx));
    PlayerTextDrawSetString(playerid, weightTD, wstr);
}

static stock InvUI_RefreshAll(playerid)
{
    InvUI_RefreshPanel(playerid,
        g_UI[playerid][ui_LSlot], g_UI[playerid][ui_LQty],
        g_UI[playerid][ui_LWeight], g_UI[playerid][ui_LeftContainer]);

    if (g_UI[playerid][ui_Dual])
    {
        InvUI_RefreshPanel(playerid,
            g_UI[playerid][ui_RSlot], g_UI[playerid][ui_RQty],
            g_UI[playerid][ui_RWeight], g_UI[playerid][ui_RightContainer]);
    }
}

// =============================================================================
//  Show / Hide Panel Helpers
// =============================================================================

static stock InvUI_ShowPanel(playerid, PlayerText:slotTDs[], PlayerText:qtyTDs[],
    PlayerText:bgTD, PlayerText:titleTD, PlayerText:weightTD)
{
    PlayerTextDrawShow(playerid, bgTD);
    PlayerTextDrawShow(playerid, titleTD);
    PlayerTextDrawShow(playerid, weightTD);
    for (new i = 0; i < INV_SLOTS; i++)
    {
        PlayerTextDrawShow(playerid, slotTDs[i]);
        PlayerTextDrawShow(playerid, qtyTDs[i]);
    }
}

static stock InvUI_HidePanel(playerid, PlayerText:slotTDs[], PlayerText:qtyTDs[],
    PlayerText:bgTD, PlayerText:titleTD, PlayerText:weightTD)
{
    PlayerTextDrawHide(playerid, bgTD);
    PlayerTextDrawHide(playerid, titleTD);
    PlayerTextDrawHide(playerid, weightTD);
    for (new i = 0; i < INV_SLOTS; i++)
    {
        PlayerTextDrawHide(playerid, slotTDs[i]);
        PlayerTextDrawHide(playerid, qtyTDs[i]);
    }
}

static stock InvUI_HideContextMenu(playerid)
{
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_CtxBg]);
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_CtxUse]);
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_CtxMove]);
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_CtxDrop]);
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_CtxCancel]);
}

static stock InvUI_SetSlotsSelectable(playerid, bool:selectable)
{
    for (new i = 0; i < INV_SLOTS; i++)
    {
        PlayerTextDrawHide(playerid, g_UI[playerid][ui_LSlot][i]);
        PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_LSlot][i], selectable);
        PlayerTextDrawShow(playerid, g_UI[playerid][ui_LSlot][i]);
    }
    if (g_UI[playerid][ui_Dual])
    {
        for (new i = 0; i < INV_SLOTS; i++)
        {
            PlayerTextDrawHide(playerid, g_UI[playerid][ui_RSlot][i]);
            PlayerTextDrawSetSelectable(playerid, g_UI[playerid][ui_RSlot][i], selectable);
            PlayerTextDrawShow(playerid, g_UI[playerid][ui_RSlot][i]);
        }
    }
}

// =============================================================================
//  Open / Close
// =============================================================================

stock InvUI_Open(playerid, containerIdx = INVALID_CONTAINER_IDX)
{
    if (containerIdx == INVALID_CONTAINER_IDX)
        containerIdx = Inv_GetPlayerContainerIdx(playerid);
    if (containerIdx == INVALID_CONTAINER_IDX) return;
    if (g_UI[playerid][ui_Open]) return;

    if (!g_UI[playerid][ui_Created])
        InvUI_Create(playerid);

    g_UI[playerid][ui_LeftContainer] = containerIdx;
    g_UI[playerid][ui_RightContainer] = INVALID_CONTAINER_IDX;
    g_UI[playerid][ui_Dual] = false;
    g_UI[playerid][ui_State] = INVUI_STATE_IDLE;

    // Show left panel only + close button for single mode
    InvUI_ShowPanel(playerid,
        g_UI[playerid][ui_LSlot], g_UI[playerid][ui_LQty],
        g_UI[playerid][ui_LBg], g_UI[playerid][ui_LTitle], g_UI[playerid][ui_LWeight]);

    PlayerTextDrawShow(playerid, g_UI[playerid][ui_CloseL]);

    InvUI_RefreshPanel(playerid,
        g_UI[playerid][ui_LSlot], g_UI[playerid][ui_LQty],
        g_UI[playerid][ui_LWeight], containerIdx);

    SelectTextDraw(playerid, INVUI_HOVER_COLOR);
    g_UI[playerid][ui_Open] = true;
}

stock InvUI_OpenDual(playerid, externalContainerIdx, const title[])
{
    new playerContainer = Inv_GetPlayerContainerIdx(playerid);
    if (playerContainer == INVALID_CONTAINER_IDX) return;
    if (externalContainerIdx == INVALID_CONTAINER_IDX) return;
    if (g_UI[playerid][ui_Open]) return;

    if (!g_UI[playerid][ui_Created])
        InvUI_Create(playerid);

    g_UI[playerid][ui_LeftContainer] = playerContainer;
    g_UI[playerid][ui_RightContainer] = externalContainerIdx;
    g_UI[playerid][ui_Dual] = true;
    g_UI[playerid][ui_State] = INVUI_STATE_IDLE;

    // Update right panel title
    PlayerTextDrawSetString(playerid, g_UI[playerid][ui_RTitle], title);

    // Show both panels
    InvUI_ShowPanel(playerid,
        g_UI[playerid][ui_LSlot], g_UI[playerid][ui_LQty],
        g_UI[playerid][ui_LBg], g_UI[playerid][ui_LTitle], g_UI[playerid][ui_LWeight]);
    InvUI_ShowPanel(playerid,
        g_UI[playerid][ui_RSlot], g_UI[playerid][ui_RQty],
        g_UI[playerid][ui_RBg], g_UI[playerid][ui_RTitle], g_UI[playerid][ui_RWeight]);

    PlayerTextDrawShow(playerid, g_UI[playerid][ui_CloseR]);

    InvUI_RefreshAll(playerid);
    SelectTextDraw(playerid, INVUI_HOVER_COLOR);
    g_UI[playerid][ui_Open] = true;
}

stock InvUI_Close(playerid)
{
    if (!g_UI[playerid][ui_Open]) return;

    // Re-enable slots if they were disabled by context menu
    if (g_UI[playerid][ui_State] == INVUI_STATE_CONTEXT)
        InvUI_SetSlotsSelectable(playerid, true);

    InvUI_HidePanel(playerid,
        g_UI[playerid][ui_LSlot], g_UI[playerid][ui_LQty],
        g_UI[playerid][ui_LBg], g_UI[playerid][ui_LTitle], g_UI[playerid][ui_LWeight]);

    if (g_UI[playerid][ui_Dual])
    {
        InvUI_HidePanel(playerid,
            g_UI[playerid][ui_RSlot], g_UI[playerid][ui_RQty],
            g_UI[playerid][ui_RBg], g_UI[playerid][ui_RTitle], g_UI[playerid][ui_RWeight]);
    }

    PlayerTextDrawHide(playerid, g_UI[playerid][ui_CloseL]);
    PlayerTextDrawHide(playerid, g_UI[playerid][ui_CloseR]);
    InvUI_HideContextMenu(playerid);

    CancelSelectTextDraw(playerid);
    g_UI[playerid][ui_Open] = false;
    g_UI[playerid][ui_State] = INVUI_STATE_IDLE;
}

// =============================================================================
//  Click Handler — State Machine
// =============================================================================

forward InvUI_OnClickTD();
public InvUI_OnClickTD()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    new PlayerText:tdid = PlayerText:EventBus_GetInt(EVD_SECONDARY_ID);

    if (!g_UI[playerid][ui_Open]) return 1;

    // ESC
    if (tdid == PlayerText:INVALID_PLAYER_TEXT_DRAW)
    {
        if (g_UI[playerid][ui_State] == INVUI_STATE_CONTEXT)
        {
            InvUI_HideContextMenu(playerid);
            InvUI_SetSlotsSelectable(playerid, true);
            g_UI[playerid][ui_State] = INVUI_STATE_IDLE;
            SelectTextDraw(playerid, INVUI_HOVER_COLOR);
        }
        else if (g_UI[playerid][ui_State] == INVUI_STATE_MOVE)
        {
            g_UI[playerid][ui_State] = INVUI_STATE_IDLE;
            InvUI_RefreshAll(playerid); // un-highlight
            SelectTextDraw(playerid, INVUI_HOVER_COLOR);
        }
        else
        {
            InvUI_Close(playerid);
        }
        return 1;
    }

    // Close button (either one)
    if (tdid == g_UI[playerid][ui_CloseL] || tdid == g_UI[playerid][ui_CloseR])
    {
        InvUI_Close(playerid);
        return 1;
    }

    // Context menu buttons
    if (g_UI[playerid][ui_State] == INVUI_STATE_CONTEXT)
    {
        new panel = g_UI[playerid][ui_CtxPanel];
        new slot  = g_UI[playerid][ui_CtxSlot];
        new cIdx  = (panel == 0) ? g_UI[playerid][ui_LeftContainer] : g_UI[playerid][ui_RightContainer];

        if (tdid == g_UI[playerid][ui_CtxUse])
        {
            InvUI_HideContextMenu(playerid);
            InvUI_SetSlotsSelectable(playerid, true);
            Inv_PlayerUseItem(playerid, slot);
            g_UI[playerid][ui_State] = INVUI_STATE_IDLE;
            InvUI_RefreshAll(playerid);
            SelectTextDraw(playerid, INVUI_HOVER_COLOR);
            return 1;
        }
        else if (tdid == g_UI[playerid][ui_CtxMove])
        {
            InvUI_HideContextMenu(playerid);
            InvUI_SetSlotsSelectable(playerid, true);
            g_UI[playerid][ui_State] = INVUI_STATE_MOVE;
            g_UI[playerid][ui_MoveSrcPanel] = panel;
            g_UI[playerid][ui_MoveSrcSlot] = slot;
            new PlayerText:srcTD = (panel == 0) ? g_UI[playerid][ui_LSlot][slot] : g_UI[playerid][ui_RSlot][slot];
            PlayerTextDrawBackgroundColor(playerid, srcTD, INVUI_SLOT_HIGHLIGHT);
            PlayerTextDrawShow(playerid, srcTD);
            SendClientMessage(playerid, 0xFFFF44FF, "Click destination slot to move item, ESC to cancel.");
            SelectTextDraw(playerid, INVUI_HOVER_COLOR);
            return 1;
        }
        else if (tdid == g_UI[playerid][ui_CtxDrop])
        {
            InvUI_HideContextMenu(playerid);
            InvUI_SetSlotsSelectable(playerid, true);
            new name[MAX_ITEM_NAME_LEN];
            new tIdx = Inv_GetItemTemplateIdx(cIdx, slot);
            Inv_GetTemplateName(tIdx, name, sizeof(name));
            Inv_RemoveItem(cIdx, slot, Inv_GetItemQuantity(cIdx, slot));
            new msg[64];
            format(msg, sizeof(msg), "You dropped %s.", name);
            SendClientMessage(playerid, 0xFF4444FF, msg);
            g_UI[playerid][ui_State] = INVUI_STATE_IDLE;
            InvUI_RefreshAll(playerid);
            SelectTextDraw(playerid, INVUI_HOVER_COLOR);
            return 1;
        }

        // Cancel button or clicked elsewhere — dismiss context menu
        InvUI_HideContextMenu(playerid);
        InvUI_SetSlotsSelectable(playerid, true);
        g_UI[playerid][ui_State] = INVUI_STATE_IDLE;
        SelectTextDraw(playerid, INVUI_HOVER_COLOR);
        return 1;
    }

    // Identify which slot was clicked and on which panel
    new clickedPanel = -1, clickedSlot = -1;
    for (new i = 0; i < INV_SLOTS; i++)
    {
        if (tdid == g_UI[playerid][ui_LSlot][i])
            { clickedPanel = 0; clickedSlot = i; break; }
        if (g_UI[playerid][ui_Dual] && tdid == g_UI[playerid][ui_RSlot][i])
            { clickedPanel = 1; clickedSlot = i; break; }
    }

    if (clickedPanel == -1) {
        // Clicked something else — re-enable selection
        SelectTextDraw(playerid, INVUI_HOVER_COLOR);
        return 1;
    }

    new cIdx = (clickedPanel == 0) ? g_UI[playerid][ui_LeftContainer] : g_UI[playerid][ui_RightContainer];

    // MOVE MODE: destination selected
    if (g_UI[playerid][ui_State] == INVUI_STATE_MOVE)
    {
        new srcPanel = g_UI[playerid][ui_MoveSrcPanel];
        new srcSlot  = g_UI[playerid][ui_MoveSrcSlot];
        new srcCIdx  = (srcPanel == 0) ? g_UI[playerid][ui_LeftContainer] : g_UI[playerid][ui_RightContainer];

        new result = Inv_MoveItem(srcCIdx, srcSlot, cIdx, clickedSlot);
        if (!result)
            SendClientMessage(playerid, 0xFF4444FF, "Cannot move item there (full or too heavy).");

        // Un-highlight source
        new PlayerText:srcTD = (srcPanel == 0) ? g_UI[playerid][ui_LSlot][srcSlot] : g_UI[playerid][ui_RSlot][srcSlot];
        PlayerTextDrawBackgroundColor(playerid, srcTD, INVUI_SLOT_COLOR);

        g_UI[playerid][ui_State] = INVUI_STATE_IDLE;
        InvUI_RefreshAll(playerid);
        SelectTextDraw(playerid, INVUI_HOVER_COLOR);
        return 1;
    }

    // IDLE: click on occupied slot → show context menu, disable slots
    if (Inv_IsSlotActive(cIdx, clickedSlot))
    {
        g_UI[playerid][ui_CtxPanel] = clickedPanel;
        g_UI[playerid][ui_CtxSlot] = clickedSlot;
        g_UI[playerid][ui_State] = INVUI_STATE_CONTEXT;

        InvUI_SetSlotsSelectable(playerid, false);

        PlayerTextDrawShow(playerid, g_UI[playerid][ui_CtxBg]);
        PlayerTextDrawShow(playerid, g_UI[playerid][ui_CtxUse]);
        PlayerTextDrawShow(playerid, g_UI[playerid][ui_CtxMove]);
        PlayerTextDrawShow(playerid, g_UI[playerid][ui_CtxDrop]);
        PlayerTextDrawShow(playerid, g_UI[playerid][ui_CtxCancel]);

        SelectTextDraw(playerid, INVUI_HOVER_COLOR);
        return 1;
    }

    // Clicked empty slot — just re-enable selection
    SelectTextDraw(playerid, INVUI_HOVER_COLOR);
    return 1;
}

// =============================================================================
//  Event: Item changed → refresh if open
// =============================================================================

forward InvUI_OnItemChanged();
public InvUI_OnItemChanged()
{
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (g_UI[i][ui_Open])
            InvUI_RefreshAll(i);
    }
    return 1;
}

// --- Connect: reset ---
forward InvUI_OnConnect();
public InvUI_OnConnect()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    g_UI[playerid][ui_Open] = false;
    g_UI[playerid][ui_Created] = false;
    g_UI[playerid][ui_LeftContainer] = INVALID_CONTAINER_IDX;
    g_UI[playerid][ui_RightContainer] = INVALID_CONTAINER_IDX;
    g_UI[playerid][ui_State] = INVUI_STATE_IDLE;
    CancelSelectTextDraw(playerid);
    return 1;
}

// --- Disconnect: cleanup ---
forward InvUI_OnDisconn();
public InvUI_OnDisconn()
{
    new playerid = EventBus_GetInt(EVD_PLAYER_ID);
    g_UI[playerid][ui_Open] = false;
    g_UI[playerid][ui_Created] = false;
    return 1;
}
