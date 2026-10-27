window.App = window.App || {};

App.keyboard = {
  init() { window.addEventListener('keydown', (e) => this.onKey(e)); },

  onKey(e) {
    const tag = document.activeElement?.tagName;
    if (tag === 'INPUT' || tag === 'SELECT' || tag === 'TEXTAREA') return;

    const st = App.state;
    if (e.key === 'Escape' && st.mode === 'replay') { e.preventDefault(); App.snapshot.exitReplay(); return; }

    if (st.mode === 'replay') {
      if (e.key === ' ') {
        e.preventDefault();
        if (st.replay.playing) App.snapshot.replayPause();
        else App.snapshot.replayPlay();
      }
      return;
    }

    const step = st.windowS / 10;
    switch (e.key) {
      case ' ':
        e.preventDefault();
        st.liveMode = !st.liveMode;
        if (st.liveMode) App.scrollbar.goLive();
        App.main.updateStatus();
        break;
      case 'ArrowLeft':
        e.preventDefault(); App.scrollbar.panRelative(-step); App.scrollbar.evaluateLiveMode(); break;
      case 'ArrowRight':
        e.preventDefault(); App.scrollbar.panRelative(step);  App.scrollbar.evaluateLiveMode(); break;
      case 'Home':
        e.preventDefault();
        if (st.timestamps.length) { App.scrollbar.setWindow(st.timestamps[0]); App.scrollbar.evaluateLiveMode(); }
        break;
      case 'End':
        e.preventDefault();
        st.liveMode = true;
        App.scrollbar.goLive();
        App.main.updateStatus();
        break;
    }
  },
};