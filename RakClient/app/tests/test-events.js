// Quick smoke test: verify bot connects and emits events from the server.
// Not a full registration test — just confirms the event pipe works.
const { RakClient } = require('../lib/rak-client');

const bot = new RakClient();

// Log all incoming events
bot.on('gameInit',       d => console.log('[EVENT] gameInit', d));
bot.on('clientMessage',  d => console.log('[EVENT] clientMessage', d));
bot.on('dialog',         d => console.log('[EVENT] dialog', d));
bot.on('setPos',         d => console.log('[EVENT] setPos', d));
bot.on('chat',           d => console.log('[EVENT] chat', d));

async function run() {
    console.log('[TEST] Connecting...');

    // Start waiting before connect so we don't miss early events
    const gameInitP = bot.waitForGameInit(10000);
    const dialogP   = bot.waitForDialog(10000);

    bot.connect();

    const gameInit = await gameInitP;
    console.log(`[PASS] gameInit received — playerId=${gameInit.playerId}, hostname="${gameInit.hostname}"`);

    const dialog = await dialogP;
    console.log(`[PASS] dialog received — id=${dialog.id}, title="${dialog.title}", body="${dialog.body}"`);

    // Clean disconnect
    bot.disconnect();
    console.log('[DONE] Disconnected.');
    process.exit(0);
}

run().catch(err => {
    console.error('[FAIL]', err.message);
    bot.disconnect();
    process.exit(1);
});
