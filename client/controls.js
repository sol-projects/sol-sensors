window.App = window.App || {};

App.controls = {
  init() {
    this.buildWindowSelector();
    this.buildToggles();
    this.buildFilter();
    this.buildMoreMenu();
    this.buildSnapshotButtons();
    this.buildReplayBar();
    this.buildCompareBar();
    this.buildAlertPanel();
    this.syncUIToState();
  },

  buildWindowSelector() {
    const sel = document.getElementById('windowSelect');
    sel.innerHTML = '';
    for (const opt of App.config.WINDOW_OPTIONS) {
      const o = document.createElement('option');
      o.value = opt.seconds;
      o.textContent = opt.label;
      if (opt.seconds === App.state.windowS) o.selected = true;
      sel.appendChild(o);
    }
    sel.addEventListener('change', () => {
      App.state.windowS = Number(sel.value);
      if (App.state.liveMode) App.scrollbar.goLive();
      App.main.saveURLState();
    });
  },

  buildToggles() {
    document.getElementById('densityBtn').addEventListener('click', () => {
      App.state.density = App.state.density === 'comfortable' ? 'compact' : 'comfortable';
      this.syncUIToState();
      App.panels.applyDensity();
      App.main.saveURLState();
    });
    document.getElementById('emaBtn').addEventListener('click', () => {
      App.state.ema = !App.state.ema;
      this.syncUIToState();
      App.main.redrawAll();
      App.panels.updateValueLabels();
      App.panels.updateStats();
      App.main.saveURLState();
    });
    document.getElementById('yAutoBtn').addEventListener('click', () => {
      App.state.yAuto = !App.state.yAuto;
      this.syncUIToState();
      if (!App.state.yAuto) {
        for (const key of Object.keys(App.state.plots)) App.state.plots[key].plot.delScale('y');
      }
      App.main.redrawAll();
      App.panels.updateStats();
      App.main.saveURLState();
    });
    document.getElementById('hoverBtn').addEventListener('click', () => {
      App.state.hover = !App.state.hover;
      this.syncUIToState();
      document.body.classList.toggle('no-hover', !App.state.hover);
      if (!App.state.hover) App.tooltip.hide();
      App.main.saveURLState();
    });
  },

  buildFilter() {
    const input = document.getElementById('filterInput');
    input.value = App.state.filter || '';
    input.addEventListener('input', () => {
      App.state.filter = input.value;
      App.panels.applyFilter();
      App.panels.updateStats();
      App.main.saveURLState();
    });
  },

  buildMoreMenu() {
    const btn = document.getElementById('moreBtn');
    const menu = document.getElementById('moreMenu');
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      menu.classList.toggle('hidden');
    });
    document.addEventListener('click', () => menu.classList.add('hidden'));
    menu.addEventListener('click', (e) => e.stopPropagation());
  },

  buildSnapshotButtons() {
    document.getElementById('saveBtn').addEventListener('click', () => {
      document.getElementById('moreMenu').classList.add('hidden');
      App.snapshot.save();
    });

    const replayInput = document.getElementById('replayFileInput');
    document.getElementById('replayBtn').addEventListener('click', () => {
      document.getElementById('moreMenu').classList.add('hidden');
      replayInput.click();
    });
    replayInput.addEventListener('change', () => {
      const f = replayInput.files[0];
      if (f) App.snapshot.loadReplay(f);
      replayInput.value = '';
    });

    const compareInput = document.getElementById('compareFileInput');
    document.getElementById('compareBtn').addEventListener('click', () => {
      document.getElementById('moreMenu').classList.add('hidden');
      compareInput.click();
    });
    compareInput.addEventListener('change', () => {
      const f = compareInput.files[0];
      if (f) App.snapshot.loadCompare(f);
      compareInput.value = '';
    });
  },

  buildReplayBar() {
    document.getElementById('replayPlay').addEventListener('click', () => {
      const r = App.state.replay;
      if (r.playing) App.snapshot.replayPause();
      else App.snapshot.replayPlay();
    });
    document.getElementById('replayScrub').addEventListener('input', (e) => {
      if (!App.state.replay.active) return;
      App.snapshot.replayPause();
      App.snapshot.replaySeek(Number(e.target.value));
    });
    document.getElementById('replaySpeed').addEventListener('change', (e) => App.snapshot.setSpeed(Number(e.target.value)));
    document.getElementById('replayExit').addEventListener('click', () => App.snapshot.exitReplay());
  },

  buildCompareBar() {
    document.getElementById('compareOffset').addEventListener('input', (e) => {
      App.snapshot.setCompareOffset(Number(e.target.value));
    });
    document.getElementById('compareExit').addEventListener('click', () => App.snapshot.clearCompare());
  },

  buildAlertPanel() {
    document.getElementById('alertBtn').addEventListener('click', () => App.alerts.togglePanel());
    document.getElementById('alertClose').addEventListener('click', () => App.alerts.togglePanel());
    document.getElementById('alertClear').addEventListener('click', () => App.alerts.clear());

    const sens = document.getElementById('alertSensitivity');
    sens.value = String(App.config.ALERTS.consecutiveSamples);
    sens.addEventListener('change', () => App.alerts.setSensitivity(Number(sens.value)));

    const level = document.getElementById('alertLevel');
    level.value = App.config.ALERTS.minLevel;
    level.addEventListener('change', () => App.alerts.setLevel(level.value));

    App.alerts.render();
    App.alerts.updateBadge();
  },

  syncUIToState() {
    const st = App.state;
    const ema = document.getElementById('emaBtn');
    ema.textContent = 'EMA';
    ema.classList.toggle('active', st.ema);
    document.getElementById('yAutoBtn').classList.toggle('active', st.yAuto);

    const hov = document.getElementById('hoverBtn');
    hov.classList.toggle('active', st.hover);
    document.body.classList.toggle('no-hover', !st.hover);   // ← add

    document.getElementById('densityBtn').textContent = st.density === 'comfortable' ? '⊞' : '⊟';
    document.getElementById('densityBtn').title = st.density === 'comfortable'
      ? 'Comfortable (click for compact)'
      : 'Compact (click for comfortable)';
  },
};