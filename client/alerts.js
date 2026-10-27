window.App = window.App || {};

App.alerts = {
  check() {
    const st = App.state;
    if (st.mode !== 'live') return;
    const cfg = App.config.ALERTS;
    const lastIdx = st.timestamps.length - 1;
    if (lastIdx < 0) return;
    const ts = st.timestamps[lastIdx];

    for (const g of st.groups) {
      for (const d of g.devs) {
        const v = (st.seriesData[d.id] || [])[lastIdx];
        if (v == null) continue;

        // Server-provided level (server computed from per-device thresholds)
        const level = d.level || 'ok';
        const meetsMin = (cfg.minLevel === 'crit') ? level === 'crit'
                                                   : (level === 'warn' || level === 'crit');

        const key = d.id;
        if (!st.alertState[key]) st.alertState[key] = { level: 'ok', count: 0, lastLogged: 'ok' };
        const state = st.alertState[key];

        if (meetsMin) {
          if (state.level === level) state.count++;
          else { state.level = level; state.count = 1; }

          if (state.count >= cfg.consecutiveSamples && state.lastLogged !== level) {
            this.log({ ts, deviceId: d.id, name: App.utils.shortName(d), value: v, level, unit: App.utils.suffixFor(g) });
            state.lastLogged = level;
          }
        } else {
          state.level = level; state.count = 0; state.lastLogged = 'ok';
        }
      }
    }
  },

  log(entry) {
    const st = App.state;
    st.alerts.push(entry);
    if (st.alerts.length > App.config.ALERTS.maxEntries)
      st.alerts.splice(0, st.alerts.length - App.config.ALERTS.maxEntries);
    this.render();
    this.updateBadge();
  },

  clear() { App.state.alerts = []; this.render(); this.updateBadge(); },
  resetState() { App.state.alertState = {}; },
  setSensitivity(n) { App.config.ALERTS.consecutiveSamples = n; this.resetState(); },
  setLevel(level) { App.config.ALERTS.minLevel = level; this.resetState(); },

  render() {
    const list = document.getElementById('alertList');
    if (!list) return;
    if (!App.state.alerts.length) { list.innerHTML = '<div class="alert-empty">No alerts yet.</div>'; return; }
    list.innerHTML = App.state.alerts.slice().reverse().map(a => {
      const icon = a.level === 'crit' ? '🔴' : '⚠';
      return `<div class="alert-row alert-${a.level}">
        <span class="alert-time">${App.utils.fmtClock(a.ts)}</span>
        <span class="alert-icon">${icon}</span>
        <span class="alert-name">${a.name}</span>
        <span class="alert-value">${a.value.toFixed(1)}${a.unit}</span>
      </div>`;
    }).join('');
  },

  updateBadge() {
    const b = document.getElementById('alertCount');
    if (b) b.textContent = App.state.alerts.length;
    const btn = document.getElementById('alertBtn');
    if (btn) btn.classList.toggle('has-alerts', App.state.alerts.length > 0);
  },

  togglePanel() {
    App.state.alertsPanelOpen = !App.state.alertsPanelOpen;
    document.getElementById('alertPanel').classList.toggle('hidden', !App.state.alertsPanelOpen);
    document.getElementById('alertBtn').classList.toggle('active', App.state.alertsPanelOpen);
    document.body.classList.toggle('alert-open', App.state.alertsPanelOpen);
  },
};