window.App = window.App || {};

App.api = {
  async fetchConfig() {
    const resp = await fetch(`${App.config.SERVER}/api/config`);
    if (!resp.ok) throw new Error(`/api/config HTTP ${resp.status}`);
    return resp.json();
  },

  async fetchDevices() {
    const resp = await fetch(`${App.config.SERVER}/devices`);
    if (!resp.ok) throw new Error(`/devices HTTP ${resp.status}`);
    const { devices } = await resp.json();
    return devices;
  },

  openStream({ onOpen, onError, onMessage }) {
    const url = `${App.config.SERVER}/stream?interval=${App.config.INTERVAL}&precision=${App.config.PRECISION}`;
    const es = new EventSource(url);
    es.onopen = onOpen;
    es.onerror = onError;
    es.onmessage = onMessage;
    return es;
  },
};