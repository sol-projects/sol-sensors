window.App = window.App || {};

App.panels = {
  getChartHeight(group) {
    const d = App.config.DENSITY[App.state.density];
    const base = group.devs.length > 1 ? d.threadH : d.otherH;
    const w = window.innerWidth;
    if (w <= 360) return Math.round(base * 0.55);
    if (w <= 480) return Math.round(base * 0.65);
    if (w <= 768) return Math.round(base * 0.8);
    return base;
  },

  build(group) {
    const st = App.state;
    const height = this.getChartHeight(group);
    const showAvg = group.devs.length > 1;

    const panel = document.createElement('div');
    panel.className = 'panel';
    panel.dataset.groupKey = group.key;

    // Title bar: name · value · min avg max · ▾
    const titleBar = document.createElement('div');
    titleBar.className = 'panel-title';
    titleBar.innerHTML = `
      <span class="pt-name">${group.title}</span>
      <span class="pt-value">—</span>
      <span class="pt-stats">
        <span class="stat-min">min —</span>
        <span class="stat-avg">avg —</span>
        <span class="stat-max">max —</span>
      </span>
      <span class="pt-collapse">▾</span>
    `;
    panel.appendChild(titleBar);

    const chartWrap = document.createElement('div');
    chartWrap.className = 'chart-wrap';
    panel.appendChild(chartWrap);

    document.getElementById('panels').appendChild(panel);
    void panel.offsetHeight;   // force layout

    const width = chartWrap.clientWidth || (window.innerWidth - 24);
    const { opts, initialData, hasTemp } = App.chart.buildOpts(group, showAvg, width, height);

    const plot = new uPlot(opts, initialData, chartWrap);
    plot.root.dataset.groupKey = group.key;

    const entry = {
      plot, group, showAvg, hasTemp,
      panelEl: panel,
      titleEl: titleBar,
      valueEl: titleBar.querySelector('.pt-value'),
      statMinEl: titleBar.querySelector('.stat-min'),
      statAvgEl: titleBar.querySelector('.stat-avg'),
      statMaxEl: titleBar.querySelector('.stat-max'),
      chartWrap,
      collapsed: false,
    };
    st.plots[group.key] = entry;

    titleBar.addEventListener('click', () => this.toggleCollapse(entry));
    App.tooltip.attach(plot);
    this.applyFilterToPanel(entry);
    return entry;
  },

  toggleCollapse(entry) {
    entry.collapsed = !entry.collapsed;
    entry.panelEl.classList.toggle('collapsed', entry.collapsed);
    if (!entry.collapsed) {
      requestAnimationFrame(() => {
        const h = this.getChartHeight(entry.group);
        entry.plot.setSize({ width: entry.chartWrap.clientWidth, height: h });
      });
    }
  },

  applyFilterToPanel(entry) {
    const q = App.state.filter.toLowerCase().trim();
    if (!q) { entry.panelEl.style.display = ''; return; }
    const haystack = (
      entry.group.title + ' ' +
      entry.group.devs.map(d => (d.name || '') + ' ' + (d.displayName || '') + ' ' + d.id).join(' ')
    ).toLowerCase();
    entry.panelEl.style.display = haystack.includes(q) ? '' : 'none';
  },

  applyFilter() {
    for (const key of Object.keys(App.state.plots)) this.applyFilterToPanel(App.state.plots[key]);
  },

  updateValueLabels() {
    const st = App.state;
    const loadSource = st.ema ? st.emaSeries : st.seriesData;
    const tempSource = st.ema ? st.emaTemp : st.seriesTemp;
    const lastIdx = st.timestamps.length - 1;

    for (const key of Object.keys(st.plots)) {
      const entry = st.plots[key];
      const { group } = entry;
      const suffix = App.utils.suffixFor(group);

      let loadVal = null;
      if (lastIdx >= 0) {
        if (group.devs.length === 1) {
          loadVal = (loadSource[group.devs[0].id] || [])[lastIdx] ?? null;
        } else {
          let sum = 0, count = 0;
          for (const d of group.devs) {
            const v = (loadSource[d.id] || [])[lastIdx];
            if (v != null) { sum += v; count++; }
          }
          loadVal = count > 0 ? sum / count : null;
        }
      }

      let tempVal = null;
      if (entry.hasTemp && lastIdx >= 0) {
        for (const d of group.devs) {
          if (!d.supportsTemp) continue;
          const v = (tempSource[d.id] || [])[lastIdx];
          if (v != null) { tempVal = v; break; }
        }
      }

      let display = loadVal == null ? '—' : loadVal.toFixed(1) + suffix;
      if (tempVal != null) display += ' / ' + tempVal.toFixed(1) + (group.tempUnit || '°C');
      entry.valueEl.textContent = display;

      // Value text color only — no panel border tint
      const cls = App.utils.classifyLevel(group.devs);
      entry.valueEl.classList.remove('value-warn', 'value-crit');
      if (cls === 'warn') entry.valueEl.classList.add('value-warn');
      if (cls === 'crit') entry.valueEl.classList.add('value-crit');
    }
  },

  updateStats() {
    const st = App.state;
    const source = st.ema ? st.emaSeries : st.seriesData;
    if (!st.timestamps.length) return;

    for (const key of Object.keys(st.plots)) {
      const entry = st.plots[key];
      if (entry.panelEl.style.display === 'none') continue;
      const { group } = entry;
      const suffix = App.utils.suffixFor(group);

      let xMin, xMax;
      if (st.liveMode || st.mode === 'replay') {
        const last = st.timestamps[st.timestamps.length - 1];
        xMin = Math.max(st.timestamps[0], last - st.windowS);
        xMax = last;
      } else {
        xMin = entry.plot.scales.x.min;
        xMax = entry.plot.scales.x.max;
        if (xMin == null || xMax == null) {
          entry.statMinEl.textContent = 'min —';
          entry.statAvgEl.textContent = 'avg —';
          entry.statMaxEl.textContent = 'max —';
          continue;
        }
      }

      let min = Infinity, max = -Infinity, sum = 0, count = 0;
      const startIdx = App.utils.lowerBound(st.timestamps, xMin);
      for (const d of group.devs) {
        const arr = source[d.id] || [];
        for (let i = startIdx; i < st.timestamps.length; i++) {
          if (st.timestamps[i] > xMax) break;
          const v = arr[i];
          if (v == null) continue;
          if (v < min) min = v;
          if (v > max) max = v;
          sum += v; count++;
        }
      }

      if (count === 0) {
        entry.statMinEl.textContent = 'min —';
        entry.statAvgEl.textContent = 'avg —';
        entry.statMaxEl.textContent = 'max —';
      } else {
        entry.statMinEl.textContent = 'min ' + min.toFixed(1) + suffix;
        entry.statAvgEl.textContent = 'avg ' + (sum / count).toFixed(1) + suffix;
        entry.statMaxEl.textContent = 'max ' + max.toFixed(1) + suffix;
      }
    }
  },

  applyDensity() {
    for (const key of Object.keys(App.state.plots)) {
      const entry = App.state.plots[key];
      if (entry.collapsed) continue;
      const h = this.getChartHeight(entry.group);
      entry.plot.setSize({ width: entry.chartWrap.clientWidth, height: h });
    }
  },

  handleResize() { this.applyDensity(); },
};