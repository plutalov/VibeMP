// Registration E2E test — mirrors real SA-MP client RPC sequence.
//
// Requires: fresh DB (no TestBot account). Delete first:
//   docker exec samp-mysql mysql -usamp -psamppass samp_rpg -e "DELETE FROM accounts WHERE username='TestBot';"

const { RakClient } = require('../lib/rak-client');
const assert = require('assert');

const SPAWN_X = -2233.97;
const SPAWN_Y = -1737.58;
const SPAWN_Z = 480.55;

async function testRegistration() {
    const bot = new RakClient();

    // ── Connect ───────────────────────────────────────────────
    // Set up all waiters before connect — events arrive in one batch
    const gameInitP = bot.waitForGameInit(10000);
    const dialogP   = bot.waitForDialog(10000);

    bot.connect();

    const gameInit = await gameInitP;
    console.log(`[PASS] Connected — playerId=${gameInit.playerId}`);

    // Real SA-MP client sends RequestClass right after InitGame.
    // OMP requires this before it will process dialog responses.
    bot.requestClass(0);

    // ── Register ──────────────────────────────────────────────
    const dialog = await dialogP;
    assert(dialog.title.includes('Register'), `Expected Register dialog, got: "${dialog.title}"`);
    assert.strictEqual(dialog.style, 3, 'Expected DIALOG_STYLE_PASSWORD');
    console.log('[PASS] Register dialog shown');

    // Set up waiters for everything that follows the dialog response —
    // server sends account confirmation, spawn info, and welcome in one batch
    const createdP  = bot.waitForMessage(/Account created/i, 15000);
    const spawnInfoP = bot.waitFor('spawnInfo', 15000);
    const welcomeP  = bot.waitForMessage(/Welcome/i, 15000);

    bot.respondDialog(dialog.id, 1, -1, 'testpass123');

    const created = await createdP;
    console.log(`[PASS] ${created.text}`);

    // ── Spawn ─────────────────────────────────────────────────
    const spawnInfo = await spawnInfoP;
    assert(Math.abs(spawnInfo.x - SPAWN_X) < 20, `Bad spawn X: ${spawnInfo.x}`);
    assert(Math.abs(spawnInfo.y - SPAWN_Y) < 20, `Bad spawn Y: ${spawnInfo.y}`);
    console.log(`[PASS] spawnInfo at (${spawnInfo.x.toFixed(1)}, ${spawnInfo.y.toFixed(1)}, ${spawnInfo.z.toFixed(1)})`);

    bot.spawn();

    const welcome = await welcomeP;
    console.log(`[PASS] ${welcome.text}`);

    // ── Walk ──────────────────────────────────────────────────
    await bot.walk(SPAWN_X, SPAWN_Y + 10, SPAWN_Z, { fromX: SPAWN_X, fromY: SPAWN_Y, fromZ: SPAWN_Z });
    console.log('[PASS] Walked 10 units north');

    // ── /help ─────────────────────────────────────────────────
    const helpP = bot.waitForMessage(/Commands/i, 5000);

    bot.sendCommand('/help');

    const help = await helpP;
    console.log(`[PASS] /help → "${help.text}"`);

    // ── Done ──────────────────────────────────────────────────
    bot.disconnect();
    console.log('\n✓ test-register PASSED');
}

testRegistration().catch(err => {
    console.error('\n✗ test-register FAILED:', err.message);
    process.exit(1);
});
