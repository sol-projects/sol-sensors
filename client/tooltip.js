window.App = window.App || {};

App.tooltip = {
  el: null,

  ensureEl() {
    if (!this.el) this.el = document.getElementById('tooltip');
    return this.el;
  },

  attach(plot) {
    plot.over.addEventListener('mousemove', (e) => this.show(plot, e));
    plot.over.addEventListener('mouseleave', () => this.hide());
  },

  show(plot, e) {
    if (!App.state.hover) { this.hide(); return; }
    const st = App.state;
    const idx = plot.cursor.idx;
    if (idx == null || idx < 0) { this.hide(); return; }
    const ts = st.timestamps[idx];
    if (ts == null) { this.hide(); return; }

    const loadSource = st.ema ? st.emaSeries : st.seriesData;
    const tempSource = st.ema ? st.emaTemp : st.seriesTemp;

    const lines = [`<div class="tt-time">${App.utils.fmtClock(ts)}</div>`];

    for (const g of st.groups) {
      const entry = st.plots[g.key];
      if (!entry) continue;
      // Skip filtered-out panels so the tooltip matches what's on screen
      if (entry.panelEl.style.display === 'none') continue;

      const suffix = App.utils.suffixFor(g);
      const tempSuffix = g.tempUnit || '°C';

      lines.push(`<div class="tt-group"><div class="tt-group-title">${g.title}</div>`);

      for (const d of g.devs) {
        const v = (loadSource[d.id] || [])[idx];
        const name = App.utils.shortName(d);
        lines.push(
          `<div class="tt-row"><span>${name}</span><span>${v == null ? '—' : v.toFixed(1) + suffix}</span></div>`
        );

        if (entry.hasTemp && d.supportsTemp) {
          const tv = (tempSource[d.id] || [])[idx];
          lines.push(
            `<div class="tt-row"><span>&nbsp;&nbsp;↳ temp</span><span>${tv == null ? '—' : tv.toFixed(1) + tempSuffix}</span></div>`
          );
        }
      }

      if (g.devs.length > 1) {
        let sum = 0, count = 0;
        for (const d of g.devs) {
          const v = (loadSource[d.id] || [])[idx];
          if (v != null) { sum += v; count++; }
        }
        const avg = count > 0 ? sum / count : null;
        lines.push(
          `<div class="tt-row tt-avg"><span>Average</span><span>${avg == null ? '—' : avg.toFixed(1) + suffix}</span></div>`
        );
      }

      lines.push(`</div>`);
    }

    const el = this.ensureEl();
    el.innerHTML = lines.join('');
    el.classList.add('show');

    let x = e.clientX + 12;
    let y = e.clientY + 12;
    const r = el.getBoundingClientRect();
    if (x + r.width > window.innerWidth - 8)  x = e.clientX - r.width - 12;
    if (y + r.height > window.innerHeight - 8) y = e.clientY - r.height - 12;
    el.style.left = x + 'px';
    el.style.top  = y + 'px';
  },

  hide() {
    if (this.el) this.el.classList.remove('show');
  },
};