const { EventEmitter } = require('events');
const native = require('../build/Release/samp');

class RakClient extends EventEmitter {
    constructor() {
        super();
        this._connected = false;
    }

    connect() {
        native.connect((eventName, data) => {
            this.emit(eventName, data);
        });
        this.once('gameInit', () => { this._connected = true; });
    }

    disconnect() {
        native.disconnect();
        this._connected = false;
        this.emit('disconnected');
    }

    sendCommand(cmd)                          { native.sendCommand(cmd); }
    sendChat(text)                            { native.sendChat(text); }
    respondDialog(id, button, listItem, text) { native.respondDialog(id, button, listItem, text); }
    spawn()                                   { native.spawn(); }
    requestClass(classId)                     { native.requestClass(classId || 0); }

    // Wait for an event, optionally filtered, with timeout
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
    waitForMessage(pattern, timeout = 5000) {
        return this.waitFor('clientMessage', timeout,
            msg => (pattern instanceof RegExp ? pattern.test(msg.text) : msg.text.includes(pattern)));
    }
    waitForPos(timeout = 5000)                 { return this.waitFor('setPos', timeout); }
}

module.exports = { RakClient };
