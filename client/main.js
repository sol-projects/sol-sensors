window.App = window.App || {};

App.main = {
  _es: null,

  async init() {
    const st = App.state;
    const statusEl = document.getElementById('status');
    statusEl.textContent = 'loading server config...';

    try {
      // 1. Fetch server config (groups, window options, interval, precision)
      const cfg = await App.api.fetchConfig();
      App.config.INTERVAL = cfg.interval;
      App.config.PRECISION = cfg.precision;
      App.config.HISTORY_MAX = cfg.historyMax;
      App.config.DEFAULT_WIN_S = cfg.defaultWindowS;
      App.config.WINDOW_OPTIONS = cfg.windowOptions;
      App.config.GROUP_DEFS = cfg.groups;

      // 2. Apply URL state (may override windowS, ema, etc.)
      this.loadURLState();
      if (!st.windowS) st.windowS = cfg.defaultWindowS;
      st.serverInterval = cfg.interval;

      // 3. Fetch devices
      statusEl.textContent = 'loading devices...';
      const devices = await App.api.fetchDevices();
      st.devices = devices;
      for (const d of devices) {
        st.seriesData[d.id] = [];
        st.seriesTemp[d.id] = [];
        st.emaSeries[d.id] = [];
        st.emaTemp[d.id] = [];
      }

      // 4. Build UI
      this.buildPanels();
      App.scrollbar.init();
      App.controls.init();
      App.keyboard.init();


      if (!this._urlSetWindow) {
        const sampleSec = App.config.INTERVAL / 1000;
        const wantSec = Math.max(sampleSec * 10, 10);
        if (st.windowS < wantSec) {
          const opts = App.config.WINDOW_OPTIONS;
          const fit = opts.find(o => o.seconds >= wantSec) || opts[opts.length - 1];
          st.windowS = fit.seconds;
          const sel = document.getElementById('windowSelect');
          if (sel) sel.value = st.windowS;
        }
      }

      this.updateStatus();
      this.updateStatusInfo();
      App.panels.applyFilter();
      this.startStream();
      window.addEventListener('resize', () => App.panels.handleResize());
    } catch (err) {
      statusEl.textContent = 'Error: ' + err.message;
      console.error(err);
    }
  },

  buildPanels() {
    const st = App.state;
    st.groups = App.config.GROUP_DEFS
      .map(def => ({
        ...def,
        devs: st.devices.filter(d => d.type === def.key && d.supportsLoad),
      }))
      .filter(g => g.devs.length > 0);

    document.getElementById('panels').innerHTML = '';
    st.plots = {};
    App.alerts.resetState();

    if (st.groups.length === 0) return;
    st.groups.forEach(g => App.panels.build(g));
  },

  stopStream() {
    if (this._es) { this._es.close(); this._es = null; }
  },

  startStream() {
    this.stopStream();
    this._es = App.api.openStream({
      onOpen: () => { if (App.state.liveMode) this.updateStatus(); },
      onError: () => { document.getElementById('status').textContent = 'connection lost, retrying...'; },
      onMessage: (e) => this.onMessage(e),
    });
  },

  updateStatus() {
    const st = App.state;
    const statusEl = document.getElementById('status');
    const badge = document.getElementById('liveBadge');
    const labelEl = badge.querySelector('.label');

    if (st.mode === 'replay') {
      statusEl.textContent = `REPLAY · ${st.replay.playing ? 'playing' : 'paused'} · Space = play/pause · Esc = exit`;
      statusEl.style.color = '#6a9fd4';
      badge.classList.remove('live'); labelEl.textContent = 'REPLAY';
      return;
    }
    if (st.liveMode) {
      statusEl.textContent = 'live · wheel/swipe = history · slider = scrub · Space = pause';
      statusEl.style.color = '#888';
      badge.classList.add('live'); labelEl.textContent = 'LIVE';
    } else {
      statusEl.textContent = 'PAUSED · wheel/slider/swipe = move · Space/End = live';
      statusEl.style.color = '#f0a020';
      badge.classList.remove('live'); labelEl.textContent = 'PAUSED';
    }
  },

  updateStatusInfo() {
    const st = App.state;
    const el = document.getElementById('statusInfo');
    if (!el) return;
    const buffer = `${st.timestamps.length}/${App.config.HISTORY_MAX}`;
    const rate = st.sampleRateMs ? `${st.sampleRateMs.toFixed(0)}ms` : '—';
    const devs = `${st.devices.length} dev`;
    el.textContent = st.mode === 'replay'
      ? `${buffer} · replay · ${devs}`
      : `${buffer} pts · ${rate}/sample · ${devs}`;
  },

  onMessage(e) {
    const st = App.state;
    const frame = JSON.parse(e.data);
    const ts = frame.ts / 1000;
    const alpha = App.config.EMA_ALPHA;

    if (frame.interval) st.serverInterval = frame.interval;
    if (frame.precision != null) App.config.PRECISION = frame.precision;

    if (st.timestamps.length > 0) {
      const prev = st.timestamps[st.timestamps.length - 1];
      const delta = (ts - prev) * 1000;
      if (delta > 0 && delta < 10000) {
        st._sampleDeltas.push(delta);
        if (st._sampleDeltas.length > 20) st._sampleDeltas.shift();
        st.sampleRateMs = st._sampleDeltas.reduce((a, b) => a + b, 0) / st._sampleDeltas.length;
      }
    }

    st.timestamps.push(ts);

    for (const d of st.devices) {
      const dev = frame.devices.find(x => x.id === d.id);
      const loadRaw = dev && dev.load !== null ? dev.load : null;
      const tempRaw = dev && dev.temp !== null ? dev.temp : null;

      st.seriesData[d.id].push(loadRaw);
      st.seriesTemp[d.id].push(tempRaw);

      const prevL = st.emaSeries[d.id].length ? st.emaSeries[d.id][st.emaSeries[d.id].length - 1] : null;
      st.emaSeries[d.id].push(loadRaw == null ? prevL : (prevL == null ? loadRaw : alpha * loadRaw + (1 - alpha) * prevL));
      const prevT = st.emaTemp[d.id].length ? st.emaTemp[d.id][st.emaTemp[d.id].length - 1] : null;
      st.emaTemp[d.id].push(tempRaw == null ? prevT : (prevT == null ? tempRaw : alpha * tempRaw + (1 - alpha) * prevT));

      // Server-provided level
      d.level = dev ? dev.level : null;
    }

    if (st.timestamps.length > App.config.HISTORY_MAX) {
      const drop = st.timestamps.length - App.config.HISTORY_MAX;
      st.timestamps.splice(0, drop);
      for (const d of st.devices) {
        st.seriesData[d.id].splice(0, drop);
        st.seriesTemp[d.id].splice(0, drop);
        st.emaSeries[d.id].splice(0, drop);
        st.emaTemp[d.id].splice(0, drop);
      }
    }

    this.redrawAll();
    App.panels.updateValueLabels();
    App.panels.updateStats();
    App.alerts.check();
    this.updateStatusInfo();

    if (st.liveMode) App.scrollbar.el.value = 1000;
  },

  redrawAll() {
    const st = App.state;
    if (!st.timestamps.length) return;
    const last = st.timestamps[st.timestamps.length - 1];
    const source = st.ema ? st.emaSeries : st.seriesData;

    for (const g of st.groups) {
      const entry = st.plots[g.key];
      if (!entry) continue;
      const data = App.chart.makeSeriesData(entry);
      const xMin = Math.max(st.timestamps[0], last - st.windowS);
      const xMax = last;

      entry.plot.batch(() => {
        entry.plot.setData(data, false);
        if (st.liveMode || st.mode === 'replay')
          entry.plot.setScale('x', { min: xMin, max: xMin + st.windowS });

        if (st.yAuto) {
          let yMin = Infinity, yMax = -Infinity;
          const startIdx = App.utils.lowerBound(st.timestamps, xMin);
          for (const d of g.devs) {
            const arr = source[d.id] || [];
            for (let i = startIdx; i < st.timestamps.length; i++) {
              if (st.timestamps[i] > xMax) break;
              const v = arr[i];
              if (v == null) continue;
              if (v < yMin) yMin = v;
              if (v > yMax) yMax = v;
            }
          }
          if (isFinite(yMin) && isFinite(yMax)) {
            if (yMin === yMax) { yMin = 0; yMax = Math.max(yMax, 1); }
            const pad = (yMax - yMin) * 0.1 || 1;
            entry.plot.setScale('y', { min: Math.max(0, yMin - pad), max: yMax + pad });
          }
        }
      });
    }
  },

  loadURLState() {
    const st = App.state;
    const params = new URLSearchParams(location.hash.slice(1));
    if (params.has('ws')) {
      st.windowS = Number(params.get('ws')) || App.config.DEFAULT_WIN_S;
      this._urlSetWindow = true;
    } else {
      st.windowS = App.config.DEFAULT_WIN_S;
    }
    if (params.has('ema')) st.ema = params.get('ema') === '1';
    if (params.has('d')) st.density = params.get('d') === 'compact' ? 'compact' : 'comfortable';
    if (params.has('f')) st.filter = params.get('f') || '';
    if (params.has('ya')) st.yAuto = params.get('ya') === '1';
    if (params.has('hov')) st.hover = params.get('hov') === '1';
  },

  saveURLState() {
    const st = App.state;
    const params = new URLSearchParams();
    if (st.windowS !== App.config.DEFAULT_WIN_S) params.set('ws', String(st.windowS));
    if (st.ema) params.set('ema', '1');
    if (st.density !== 'comfortable') params.set('d', st.density);
    if (st.filter) params.set('f', st.filter);
    if (st.yAuto) params.set('ya', '1');
    if (!st.hover) params.set('hov', '0');
    const hash = params.toString();
    history.replaceState(null, '', hash ? '#' + hash : location.pathname + location.search);
  },
};

App.main.init();