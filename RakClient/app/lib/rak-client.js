const { EventEmitter } = require('events');
const native = require('../build/Release/samp');

class RakClient extends EventEmitter {
    constructor() {
        super();
        this._connected = false;
        this._spawned = false;
    }

    // ── Low-level actions ───────────────────────────────────────

    connect() {
        native.connect((eventName, data) => {
            this.emit(eventName, data);
        });
        this.once('gameInit', () => { this._connected = true; });
    }

    disconnect() {
        native.disconnect();
        this._connected = false;
        this._spawned = false;
        this.emit('disconnected');
    }

    sendCommand(cmd)                          { native.sendCommand(cmd); }
    sendChat(text)                            { native.sendChat(text); }
    respondDialog(id, button, listItem, text) { native.respondDialog(id, button, listItem, text); }
    spawn()                                   { native.spawn(); this._spawned = true; }
    requestClass(classId)                     { native.requestClass(classId || 0); }
    setPosition(x, y, z, angle)              { native.setPosition(x, y, z, angle || 0); }

    // ── Low-level waiters ───────────────────────────────────────

    waitFor(eventName, timeout = 5000, filter = null) {
        return new Promise((resolve, reject) => {
            const timer = setTimeout(() => {
                this.removeListener(eventName, handler);
                reject(new Error(`Timeout waiting for '${eventName}' after ${timeout}ms`));
            }, timeout);

            const handler = (data) => {
                if (!filter || filter(data)) {
                    clearTimeout(timer);
                    this.removeListener(eventName, handler);
                    resolve(data);
                }
            };
            this.on(eventName, handler);
        });
    }

    waitForDialog(timeout = 5000)              { return this.waitFor('dialog', timeout); }
    waitForGameInit(timeout = 5000)            { return this.waitFor('gameInit', timeout); }
    waitForPos(timeout = 5000)                 { return this.waitFor('setPos', timeout); }
    waitForMessage(pattern, timeout = 5000) {
        return this.waitFor('clientMessage', timeout,
            msg => (pattern instanceof RegExp ? pattern.test(msg.text) : msg.text.includes(pattern)));
    }

    // ── High-level helpers ──────────────────────────────────────

    /**
     * Connect, wait for gameInit, send requestClass(0), wait for first dialog.
     * All waiters are set up before connect() so nothing is missed.
     * Returns { gameInit, dialog }.
     */
    async connectAndInit(timeout = 10000) {
        const gameInitP = this.waitForGameInit(timeout);
        const dialogP   = this.waitForDialog(timeout);
        this.connect();
        const gameInit = await gameInitP;
        this.requestClass(0);
        const dialog = await dialogP;
        return { gameInit, dialog };
    }

    /**
     * Move the bot in a straight line from current position toward (x, y, z).
     * Takes `steps` discrete steps over `duration` ms.
     */
    async walk(toX, toY, toZ, { steps = 5, duration = 1000, fromX, fromY, fromZ } = {}) {
        const sx = fromX !== undefined ? fromX : toX;
        const sy = fromY !== undefined ? fromY : toY;
        const sz = fromZ !== undefined ? fromZ : toZ;
        const dx = (toX - sx) / steps;
        const dy = (toY - sy) / steps;
        const dz = (toZ - sz) / steps;
        const interval = duration / steps;

        return new Promise(resolve => {
            let step = 0;
            const timer = setInterval(() => {
                step++;
                this.setPosition(sx + dx * step, sy + dy * step, sz + dz * step, 0);
                if (step >= steps) {
                    clearInterval(timer);
                    resolve();
                }
            }, interval);
        });
    }

    /**
     * Pause for `ms` milliseconds.
     */
    sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}

module.exports = { RakClient };
