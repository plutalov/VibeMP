# mod_phone_temp — Phone Item Handler (test module)

Temporary test consumer for the inventory system. Handles items with category "electronic" — using a phone shows a placeholder dialog.

**This is a test/example module.** Replace with a proper `mod_phone.inc` when building the real phone system.

## Public API

None.

## Events

| Direction | Event | Description |
|-----------|-------|-------------|
| Subscribes | `EVT_INV_ITEM_USE` | Checks category "electronic", shows phone dialog |
| Subscribes | `EVT_PLAYER_HELP` | Prints usage hint in /help |

## Dependencies

- `mod_inventory` — `Inv_GetTemplateCategory()`

## Dialog IDs

| ID | Constant | Purpose |
|----|----------|---------|
| 60 | `DIALOG_PHONE_MAIN` | Phone menu (placeholder list dialog) |
