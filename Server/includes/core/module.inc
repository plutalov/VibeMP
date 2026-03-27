// =============================================================================
//  module.inc — Module conventions and helpers
//
//  Pawn's preprocessor cannot reliably expand a macro into both a forward
//  declaration and a public function. So every handler must be written as:
//
//      forward MyMod_OnPlayerConnect();
//      public MyMod_OnPlayerConnect()
//      {
//          new playerid = EventBus_GetInt(EVD_PLAYER_ID);
//          // ...
//          return 1;
//      }
//
//  See docs/module-guide.md for the full template.
// =============================================================================

#if defined _module_included
    #endinput
#endif
#define _module_included
