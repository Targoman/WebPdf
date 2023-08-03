export default class clsEventTarget {
    constructor() {
        this.eventHandlers = [];
    }

    getEventListeners(type) {
        if (this.eventHandlers.hasOwnProperty(type)) return this.eventHandlers[type];
    }

    addEventListener(type, listener) {
        if (!this.eventHandlers.hasOwnProperty(type)) this.eventHandlers[type] = [];
        var listeners = this.eventHandlers[type];
        listeners.push(listener);
    }

    removeEventListener(type, listener) {
        var listeners = this.getEventListeners(type);
        if (!listeners) return;
        var index = listeners.indexOf(listener);
        if (index >= 0) listeners.splice(index, 1);
    }

    dispatchEvent(type) {
        var params = [].slice.call(arguments, 1);
        if (type instanceof Event) {
            params = [type];
            type = type.type;
        }
        var listeners = this.getEventListeners(type);
        if (!listeners) return;
        listeners.forEach((listener) => {
            listener.apply(this, params);
        });
    }
}
