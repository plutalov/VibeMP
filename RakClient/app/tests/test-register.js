// Full registration flow test:
// 1. Bot connects
// 2. Server shows Register dialog
// 3. Bot submits password
// 4. Server sends "Logged in successfully!" message
// 5. Server sets player position (Mount Chiliad spawn)
// 6. Bot disconnects
// Run against a fresh account name (not already registered).

const { RakClient } = require('../lib/rak-client');
const assert = require('assert');

const NICK = 'TestBot';
const PASS = 'testpass123';
const SPAWN_X = -2233.97;
const SPAWN_Y = -1737.58;
const POS_TOLERANCE = 20;

async function testRegistration() {
    const bot = new RakClient();

    // Start listening before connect so we don't miss early events
    const gameInitP = bot.waitForGameInit(10000);
    const dialogP   = bot.waitForDialog(10000);

    bot.connect();

    const gameInit = await gameInitP;
    console.log(`[PASS] gameInit — playerId=${gameInit.playerId}, hostname="${gameInit.hostname}"`);

    const dialog = await dialogP;
    console.log(`[INFO] dialog — "${dialog.title}" style=${dialog.style}`);
    assert(dialog.title.includes('Register'), `Expected Register dialog, got: "${dialog.title}"`);
    assert(dialog.body.includes('not registered'), `Expected 'not registered' in body`);
    console.log('[PASS] Register dialog shown correctly');

    // Start listening BEFORE responding so we don't miss fast responses
    const welcomeP = bot.waitForMessage(/Logged in successfully/i, 15000);
    const posP     = bot.waitForPos(15000);

    // Submit password
    bot.respondDialog(dialog.id, 1, 0, PASS);

    const welcome = await welcomeP;
    console.log(`[PASS] Welcome message received: "${welcome.text}"`);

    const pos = await posP;
    console.log(`[INFO] Spawn position: x=${pos.x.toFixed(2)}, y=${pos.y.toFixed(2)}, z=${pos.z.toFixed(2)}`);
    assert(
        Math.abs(pos.x - SPAWN_X) < POS_TOLERANCE && Math.abs(pos.y - SPAWN_Y) < POS_TOLERANCE,
        `Expected spawn near Mount Chiliad (${SPAWN_X}, ${SPAWN_Y}), got (${pos.x.toFixed(2)}, ${pos.y.toFixed(2)})`
    );
    console.log('[PASS] Spawned at Mount Chiliad');

    bot.disconnect();
    console.log('\n✓ test-register PASSED');
}

testRegistration().catch(err => {
    console.error('\n✗ test-register FAILED:', err.message);
    process.exit(1);
});
