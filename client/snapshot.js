window.App = window.App || {};

App.snapshot = {
  save() {
    const st = App.state;
    if (!st.timestamps.length) { document.getElementById('status').textContent = 'Nothing to save yet.'; return; }
    const snap = {
      version: 1, savedAt: new Date().toISOString(),
      devices: st.devices,
      precision: App.config.PRECISION, interval: App.config.INTERVAL,
      windowS: st.windowS,
      timestamps: st.timestamps.slice(),
      seriesData: {}, seriesTemp: {},
    };
    for (const d of st.devices) {
      snap.seriesData[d.id] = (st.seriesData[d.id] || []).slice();
      snap.seriesTemp[d.id] = (st.seriesTemp[d.id] || []).slice();
    }
    const blob = new Blob([JSON.stringify(snap)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    const stamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
    a.href = url; a.download = `sensors-${stamp}.json`; a.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  },

  validate(snap) {
    if (!snap || typeof snap !== 'object')   throw new Error('Not a JSON object');
    if (!Array.isArray(snap.timestamps))     throw new Error('Missing timestamps');
    if (!Array.isArray(snap.devices))        throw new Error('Missing devices');
    if (!snap.seriesData)                    throw new Error('Missing seriesData');
    if (snap.timestamps.length === 0)        throw new Error('Empty snapshot');
  },

  async loadReplay(file) {
    try {
      const snap = JSON.parse(await file.text());
      this.validate(snap);
      this.enterReplay(snap, file.name);
    } catch (err) {
      document.getElementById('status').textContent = `Replay failed: ${err.message}`;
      console.error(err);
    }
  },

  enterReplay(snapshot, filename) {
    const st = App.state;
    App.main.stopStream();

    this._liveBackup = {
      devices: st.devices, timestamps: st.timestamps.slice(),
      seriesData: {}, seriesTemp: {}, windowS: st.windowS,
    };
    for (const d of st.devices) {
      this._liveBackup.seriesData[d.id] = (st.seriesData[d.id] || []).slice();
      this._liveBackup.seriesTemp[d.id] = (st.seriesTemp[d.id] || []).slice();
    }

    st.mode = 'replay';
    st.liveMode = false;
    st.devices = snapshot.devices;
    st.windowS = snapshot.windowS || App.config.DEFAULT_WIN_S;
    st.timestamps = [];
    st.seriesData = {}; st.seriesTemp = {}; st.emaSeries = {}; st.emaTemp = {};
    for (const d of st.devices) {
      st.seriesData[d.id] = [];
      st.seriesTemp[d.id] = [];
      st.emaSeries[d.id] = [];
      st.emaTemp[d.id]   = [];
    }
    st.replay = { active: true, playing: false, speed: 1, index: 0, snapshot, timerId: null, filename };

    this.clearCompare();
    App.main.buildPanels();
    App.alerts.resetState();

    document.getElementById('scrollbar').classList.add('hidden');
    document.getElementById('replayBar').classList.remove('hidden');
    document.getElementById('compareBar').classList.add('hidden');
    document.getElementById('replayBtn').disabled = true;
    document.getElementById('compareBtn').disabled = true;
    document.getElementById('saveBtn').disabled = true;
    document.getElementById('replayFilename').textContent = filename;
    document.getElementById('replayFilename').title = filename;

    this.replaySeek(0);
    this.replayPlay();
    App.main.updateStatus();
  },

  exitReplay() {
    if (App.state.replay.timerId) clearTimeout(App.state.replay.timerId);
    const backup = this._liveBackup;
    const st = App.state;

    st.mode = 'live'; st.liveMode = true;
    st.timestamps = []; st.seriesData = {}; st.seriesTemp = {}; st.emaSeries = {}; st.emaTemp = {};
    st.replay = { active: false, playing: false, speed: 1, index: 0, snapshot: null, timerId: null, filename: '' };
    App.alerts.resetState();

    document.getElementById('scrollbar').classList.remove('hidden');
    document.getElementById('replayBar').classList.add('hidden');
    document.getElementById('compareBar').classList.add('hidden');
    document.getElementById('replayBtn').disabled = false;
    document.getElementById('compareBtn').disabled = false;
    document.getElementById('saveBtn').disabled = false;

    if (backup) {
      st.devices = backup.devices;
      st.windowS = backup.windowS;
      for (const d of st.devices) {
        st.seriesData[d.id] = [];
        st.seriesTemp[d.id] = [];
        st.emaSeries[d.id] = [];
        st.emaTemp[d.id]   = [];
      }
      App.main.buildPanels();
    }
    App.main.startStream();
    App.main.updateStatus();
  },

  replayPlay() {
    const r = App.state.replay;
    if (!r.active || r.playing || !r.snapshot) return;
    if (r.index >= r.snapshot.timestamps.length) this.replaySeek(0);
    r.playing = true; this.updatePlayButton(); this.scheduleTick(); App.main.updateStatus();
  },

  replayPause() {
    const r = App.state.replay;
    if (!r.playing) return;
    r.playing = false;
    if (r.timerId) { clearTimeout(r.timerId); r.timerId = null; }
    this.updatePlayButton(); App.main.updateStatus();
  },

  scheduleTick() {
    const r = App.state.replay;
    if (!r.playing || !r.snapshot) return;
    const interval = r.snapshot.interval || App.config.INTERVAL;
    const tickMs = Math.max(10, interval / r.speed);
    r.timerId = setTimeout(() => {
      if (!App.state.replay.active || !App.state.replay.playing) return;
      if (r.index >= r.snapshot.timestamps.length) { this.replayPause(); return; }
      this.replayPushNext(); this.scheduleTick();
    }, tickMs);
  },

  replayPushNext() {
    const r = App.state.replay, snap = r.snapshot, idx = r.index;
    if (idx >= snap.timestamps.length) return;
    const st = App.state, alpha = App.config.EMA_ALPHA;
    st.timestamps.push(snap.timestamps[idx]);
    for (const d of st.devices) {
      const loadRaw = (snap.seriesData[d.id] || [])[idx] ?? null;
      const tempRaw = (snap.seriesTemp?.[d.id] || [])[idx] ?? null;
      st.seriesData[d.id].push(loadRaw);
      st.seriesTemp[d.id].push(tempRaw);

      const prevL = st.emaSeries[d.id].length ? st.emaSeries[d.id][st.emaSeries[d.id].length - 1] : null;
      st.emaSeries[d.id].push(loadRaw == null ? prevL : (prevL == null ? loadRaw : alpha * loadRaw + (1 - alpha) * prevL));
      const prevT = st.emaTemp[d.id].length ? st.emaTemp[d.id][st.emaTemp[d.id].length - 1] : null;
      st.emaTemp[d.id].push(tempRaw == null ? prevT : (prevT == null ? tempRaw : alpha * tempRaw + (1 - alpha) * prevT));
    }
    r.index++;
    App.main.redrawAll(); App.panels.updateValueLabels(); App.panels.updateStats();
    this.updateScrubber(); this.updateTimeLabel();
  },

  replaySeek(index) {
    const r = App.state.replay, snap = r.snapshot;
    if (!snap) return;
    index = App.utils.clamp(index, 0, snap.timestamps.length - 1);
    const st = App.state, alpha = App.config.EMA_ALPHA;
    st.timestamps = snap.timestamps.slice(0, index + 1);
    for (const d of st.devices) {
      const rawL = (snap.seriesData[d.id] || []).slice(0, index + 1);
      const rawT = (snap.seriesTemp?.[d.id] || []).slice(0, index + 1);
      st.seriesData[d.id] = rawL;
      st.seriesTemp[d.id] = rawT;
      const emaL = []; let pL = null;
      for (const v of rawL) { const s = v == null ? pL : (pL == null ? v : alpha * v + (1 - alpha) * pL); emaL.push(s); pL = s; }
      st.emaSeries[d.id] = emaL;
      const emaT = []; let pT = null;
      for (const v of rawT) { const s = v == null ? pT : (pT == null ? v : alpha * v + (1 - alpha) * pT); emaT.push(s); pT = s; }
      st.emaTemp[d.id] = emaT;
    }
    r.index = index + 1;
    App.main.redrawAll(); App.panels.updateValueLabels(); App.panels.updateStats();
    this.updateScrubber(); this.updateTimeLabel();
  },

  setSpeed(speed) {
    const r = App.state.replay;
    r.speed = speed;
    if (r.playing && r.timerId) { clearTimeout(r.timerId); r.timerId = null; this.scheduleTick(); }
  },

  updatePlayButton() { document.getElementById('replayPlay').textContent = App.state.replay.playing ? '⏸' : '▶'; },
  updateScrubber() {
    const r = App.state.replay;
    if (!r.active || !r.snapshot) return;
    const s = document.getElementById('replayScrub');
    s.max = Math.max(0, r.snapshot.timestamps.length - 1);
    s.value = Math.max(0, r.index - 1);
  },
  updateTimeLabel() {
    const r = App.state.replay;
    if (!r.active || !r.snapshot) return;
    const interval = (r.snapshot.interval || App.config.INTERVAL) / 1000;
    const total = r.snapshot.timestamps.length * interval;
    const current = Math.min(r.index, r.snapshot.timestamps.length) * interval;
    document.getElementById('replayTime').textContent = `${App.utils.fmtDuration(current)} / ${App.utils.fmtDuration(total)}`;
  },

  async loadCompare(file) {
    try {
      const snap = JSON.parse(await file.text());
      this.validate(snap);
      this.applyCompare(snap, file.name);
    } catch (err) {
      document.getElementById('status').textContent = `Compare failed: ${err.message}`;
      console.error(err);
    }
  },

  applyCompare(snapshot, filename) {
    App.state.compare = {
      active: true, filename, offset: 30,
      snapshotLastTs: snapshot.timestamps[snapshot.timestamps.length - 1],
      timestamps: snapshot.timestamps, seriesData: snapshot.seriesData, devices: snapshot.devices,
    };
    document.getElementById('compareBar').classList.remove('hidden');
    document.getElementById('compareFilename').textContent = filename;
    document.getElementById('compareFilename').title = filename;
    document.getElementById('compareOffset').value = 30;
    document.getElementById('compareOffsetVal').textContent = '30s';
    this.redrawGhosts();
  },

  clearCompare() {
    App.state.compare = { active: false, filename: '', offset: 30, snapshotLastTs: 0, timestamps: [], seriesData: {}, devices: [] };
    document.getElementById('compareBar').classList.add('hidden');
    this.redrawGhosts();
  },

  setCompareOffset(seconds) {
    App.state.compare.offset = seconds;
    document.getElementById('compareOffsetVal').textContent = seconds + 's';
    this.redrawGhosts();
  },

  redrawGhosts() {
    for (const key of Object.keys(App.state.plots)) {
      const entry = App.state.plots[key];
      if (entry && entry.plot) entry.plot.redraw();
    }
  },
};