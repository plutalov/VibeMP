// =============================================================================
//  testing.inc — Lightweight test harness
//
//  Usage:
//    - Modules expose <ModName>_RunTests() functions.
//    - Call Test_Init() before running tests, Test_Summary() after.
//    - Use Test_Assert / Test_AssertEqual inside test functions.
//    - Test_EmitEvent() fires synthetic events for testing handlers.
//    - Gate auto-run with: #define RUN_TESTS_ON_INIT
// =============================================================================

#if defined _testing_included
    #endinput
#endif
#define _testing_included

static g_TestsPassed;
static g_TestsFailed;
static g_TestsRun;

stock Test_Init()
{
    g_TestsPassed = 0;
    g_TestsFailed = 0;
    g_TestsRun = 0;
    print("[TEST] Test harness initialized.");
}

stock Test_Assert(const testName[], bool:condition)
{
    g_TestsRun++;
    if (condition)
    {
        g_TestsPassed++;
        printf("[TEST PASS] %s", testName);
    }
    else
    {
        g_TestsFailed++;
        printf("[TEST FAIL] %s", testName);
    }
}

stock Test_AssertEqual(const testName[], actual, expected)
{
    g_TestsRun++;
    if (actual == expected)
    {
        g_TestsPassed++;
        printf("[TEST PASS] %s", testName);
    }
    else
    {
        g_TestsFailed++;
        printf("[TEST FAIL] %s — expected=%d actual=%d", testName, expected, actual);
    }
}

stock Test_Summary()
{
    print("==============================");
    printf("  Tests: %d  Passed: %d  Failed: %d",
        g_TestsRun, g_TestsPassed, g_TestsFailed);
    print("==============================");

    if (g_TestsFailed > 0)
        print("[TEST] SOME TESTS FAILED. Check output above.");
    else if (g_TestsRun > 0)
        print("[TEST] All tests passed.");
    else
        print("[TEST] No tests were run.");
}

stock Test_GetFailCount()
{
    return g_TestsFailed;
}

// Fire a synthetic event for testing.
stock Test_EmitEvent(eventId, playerid = 0, secondaryId = 0, value = 0, extra = 0)
{
    EventBus_SetInt(EVD_PLAYER_ID, playerid);
    EventBus_SetInt(EVD_SECONDARY_ID, secondaryId);
    EventBus_SetInt(EVD_VALUE, value);
    EventBus_SetInt(EVD_EXTRA, extra);
    EventBus_Emit(eventId);
}
