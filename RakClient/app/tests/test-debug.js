// Debug: log ALL events for 20 seconds, respond to dialog if one appears
const { RakClient } = require('../lib/rak-client');

const bot = new RakClient();
let responded = false;
let spawned = false;

// Log everything
['gameInit','clientMessage','dialog','setPos','chat','playerJoin','playerQuit',
 'setHealth','setArmor','setSkin','spawnInfo','requestClass','rejected'].forEach(ev => {
    bot.on(ev, d => console.log(`[${ev}]`, JSON.stringify(d)));
});

// Real SA-MP client sends RequestClass right after InitGame, before dialog response.
// OMP requires this — without it, dialog responses are silently dropped.
bot.on('gameInit', () => {
    console.log('[ACTION] Sending requestClass(0) — required before dialog response');
    bot.requestClass(0);
});

bot.on('dialog', d => {
    if (!responded) {
        responded = true;
        // Small delay — real client takes ~3s (human typing), we just need to be after requestClass
        setTimeout(() => {
            console.log('[ACTION] Responding to dialog with password...');
            bot.respondDialog(d.id, 1, -1, 'testpass123');
        }, 200);
    }
});

// After login, server sends spawnInfo — respond with spawn (once only, server sends it twice)
bot.on('spawnInfo', () => {
    if (!spawned) {
        spawned = true;
        console.log('[ACTION] spawnInfo received, spawning...');
        bot.spawn();
    }
});

// Walk a bit after 2s (move position over time), then /help at 5s
setTimeout(() => {
    if (!spawned) return;
    console.log('[ACTION] Walking — moving position...');
    // Start at spawn, walk ~10 units north over 2 seconds
    const startX = -2233.97, startY = -1737.58, startZ = 480.55;
    let step = 0;
    const walkInterval = setInterval(() => {
        step++;
        bot.setPosition(startX, startY + step * 2, startZ, 0);
        if (step >= 5) clearInterval(walkInterval);
    }, 400);
}, 2000);

setTimeout(() => {
    console.log('[ACTION] Sending /help command...');
    bot.sendCommand('/help');
}, 5000);

bot.connect();

setTimeout(() => {
    console.log('[DONE] 20s elapsed, disconnecting.');
    bot.disconnect();
    process.exit(0);
}, 20000);
