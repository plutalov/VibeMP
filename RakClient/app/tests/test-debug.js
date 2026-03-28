// Debug: log ALL events for 20 seconds, respond to dialog if one appears
const { RakClient } = require('../lib/rak-client');

const bot = new RakClient();
let responded = false;

// Log everything
['gameInit','clientMessage','dialog','setPos','chat','playerJoin','playerQuit',
 'setHealth','setArmor','setSkin','spawnInfo','requestClass','rejected'].forEach(ev => {
    bot.on(ev, d => console.log(`[${ev}]`, JSON.stringify(d)));
});

bot.on('dialog', d => {
    if (!responded) {
        responded = true;
        // Delay 500ms to let OMP finalize dialog state
        setTimeout(() => {
            console.log('[ACTION] Responding to dialog with password...');
            // OMP requires listItem=-1 for PASSWORD/INPUT/MSGBOX dialogs (not 0!)
            bot.respondDialog(d.id, 1, -1, 'testpass123');
        }, 200);
    }
});

// Also test if outgoing RPCs work at all — send a chat command after 5s
setTimeout(() => {
    console.log('[ACTION] Sending /stats command to test outgoing RPCs...');
    bot.sendCommand('/stats');
}, 5000);

bot.on('requestClass', d => {
    console.log('[ACTION] requestClass received, spawning...');
    bot.spawn();
});

bot.on('spawnInfo', d => {
    console.log('[ACTION] spawnInfo received, spawning...');
    bot.spawn();
});

bot.connect();

setTimeout(() => {
    console.log('[DONE] 20s elapsed, disconnecting.');
    bot.disconnect();
    process.exit(0);
}, 20000);
