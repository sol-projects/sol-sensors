window.App = window.App || {};

App.scrollbar = {
  el: null,

  init() {
    this.el = document.getElementById('timeScroll');
    this.el.addEventListener('input', () => this.onInput());
    this.attachTouchPan();
  },

  attachTouchPan() {
    let startX = null, startMin = null, plot = null;

    const onStart = (e) => {
      if (e.touches.length !== 1) return;
      const over = e.target.closest('.u-over');
      if (!over) return;
      const key = over.closest('.panel')?.dataset.groupKey;
      if (!key) return;
      plot = App.state.plots[key]?.plot;
      if (!plot) return;
      startX   = e.touches[0].clientX;
      startMin = plot.scales.x.min;
    };

    const onMove = (e) => {
      if (startX == null || !plot || e.touches.length !== 1) return;
      const dx = e.touches[0].clientX - startX;
      if (Math.abs(dx) < 8) return;                 // allow vertical page scroll first
      const pxPerSec = plot.over.clientWidth / App.state.windowS;
      const dSec = dx / pxPerSec;
      const newMin = startMin - dSec;
      this.setWindow(newMin);
      this.evaluateLiveMode();
      e.preventDefault();
    };

    const onEnd = () => { startX = null; startMin = null; plot = null; };

    document.addEventListener('touchstart', onStart, { passive: true });
    document.addEventListener('touchmove',  onMove,  { passive: false });
    document.addEventListener('touchend',   onEnd);
    document.addEventListener('touchcancel', onEnd);
  },

  applyWindow(xMin, xMax) {
    for (const g of App.state.groups) {
      const entry = App.state.plots[g.key];
      if (entry) entry.plot.setScale('x', { min: xMin, max: xMax });
    }
  },

  setWindow(xMin) {
    const st = App.state;
    if (st.timestamps.length < 2) return;
    const span = st.windowS;
    const dataMin = st.timestamps[0];
    const dataMax = st.timestamps[st.timestamps.length - 1];
    const over = span * App.config.OVERSCROLL;

    if (dataMax - dataMin <= span) xMin = dataMin;
    else xMin = App.utils.clamp(xMin, dataMin - over, dataMax - span + over);

    this.applyWindow(xMin, xMin + span);
    this.updatePosition(xMin);
  },

  updatePosition(xMin) {
    const st = App.state;
    const span = st.windowS;
    const dataMin = st.timestamps[0];
    const dataMax = st.timestamps[st.timestamps.length - 1];
    const over = span * App.config.OVERSCROLL;
    if (dataMax - dataMin <= span) { this.el.value = 1000; return; }
    const minAllowed = dataMin - over;
    const maxAllowed = dataMax - span + over;
    const range = maxAllowed - minAllowed;
    const val = ((xMin - minAllowed) / range) * 1000;
    this.el.value = App.utils.clamp(val, 0, 1000);
  },

  onInput() {
    const st = App.state;
    if (st.timestamps.length < 2) return;
    const span = st.windowS;
    const dataMin = st.timestamps[0];
    const dataMax = st.timestamps[st.timestamps.length - 1];
    const over = span * App.config.OVERSCROLL;
    if (dataMax - dataMin <= span) return;

    const minAllowed = dataMin - over;
    const maxAllowed = dataMax - span + over;
    const range = maxAllowed - minAllowed;
    const xMin = minAllowed + (Number(this.el.value) / 1000) * range;
    this.applyWindow(xMin, xMin + span);

    const v = Number(this.el.value);
    const wantLive = v >= 999;
    if (wantLive && !st.liveMode) { st.liveMode = true; App.main.updateStatus(); }
    else if (!wantLive && st.liveMode) { st.liveMode = false; App.main.updateStatus(); }
  },

  goLive() {
    const st = App.state;
    if (st.timestamps.length < 2) return;
    const span = st.windowS;
    const dataMax = st.timestamps[st.timestamps.length - 1];
    const dataMin = st.timestamps[0];
    let xMin = dataMax - span;
    if (xMin < dataMin) xMin = dataMin;
    this.applyWindow(xMin, xMin + span);
    this.el.value = 1000;
  },

  panRelative(seconds) {
    const st = App.state;
    if (!st.groups.length) return;
    const entry = st.plots[st.groups[0].key];
    if (!entry) return;
    const xMin = entry.plot.scales.x.min;
    if (xMin == null) return;
    this.setWindow(xMin + seconds);
  },

  evaluateLiveMode() {
    const st = App.state;
    if (!st.groups.length || st.timestamps.length < 2) return;
    const entry = st.plots[st.groups[0].key];
    if (!entry) return;
    const dataMax = st.timestamps[st.timestamps.length - 1];
    const xMin = entry.plot.scales.x.min;
    const xMax = xMin + st.windowS;
    const atLiveEdge = xMax >= dataMax - 0.15;
    if (atLiveEdge && !st.liveMode)      { st.liveMode = true;  App.main.updateStatus(); }
    else if (!atLiveEdge && st.liveMode) { st.liveMode = false; App.main.updateStatus(); }
  },
};