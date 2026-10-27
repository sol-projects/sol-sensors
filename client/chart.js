window.App = window.App || {};

App.chart = {
  // Choose a "nice" step so the number of ticks is ~5
  pickTimeStep(span) {
    const nice = [1, 2, 5, 10, 15, 20, 30, 60, 120, 300, 600];
    const target = span / 5;
    for (const c of nice) if (c >= target) return c;
    return 600;
  },

  buildOpts(group, showAvg, width, height) {
    const cfg = App.config;
    const hasTemp = !!(group.showTemp && group.devs.some(d => d.supportsTemp));
    const tempDevs = hasTemp ? group.devs.filter(d => d.supportsTemp) : [];
    const isMulti = group.devs.length > 1;
    const primaryColor = group.color || null;

    const series = [{ label: 'time' }];
    const initialData = [[]];

    group.devs.forEach((d, i) => {
      const color = (primaryColor && !isMulti)
        ? primaryColor
        : cfg.COLORS[i % cfg.COLORS.length];

      series.push({
        label: App.utils.shortName(d),
        stroke: color,
        width: isMulti ? 1.2 : 1.6,
        spanGaps: false,
        fill: group.fill && !isMulti ? color + '40' : undefined,
      });
      initialData.push([]);
    });

    if (showAvg) {
      series.push({
        label: 'Average',
        stroke: 'rgba(255,255,255,0.65)',
        width: 1.5,
        spanGaps: false,
        dash: [5, 4],
      });
      initialData.push([]);
    }

    tempDevs.forEach((d) => {
      series.push({
        label: App.utils.shortName(d) + ' temp',
        stroke: '#d08868',
        width: 1.2,
        spanGaps: false,
        scale: 'y2',
        dash: [2, 2],
      });
      initialData.push([]);
    });

    // ── X axis: static relative labels (-30s … -20s … 0) ─────────
    const xAxis = {
      stroke: '#666',
      grid: { show: false },
      ticks: { stroke: '#333', size: 3 },
      font: '10px Cantarell, Ubuntu, sans-serif',
      size: 34,
      splits: (u) => {
        const xMax = u.scales.x.max;
        const xMin = u.scales.x.min;
        if (xMax == null || xMin == null) return [];
        const span = xMax - xMin;
        if (span <= 0) return [];
        const step = this.pickTimeStep(span);
        const out = [];
        for (let offset = 0; offset <= span + 0.001; offset += step) {
          out.push(xMax - offset);
        }
        return out;
      },
      values: (u, ticks) => {
        const xMax = u.scales.x.max;
        if (xMax == null) return ticks.map(() => '');
        return ticks.map(t => {
          const delta = Math.round(t - xMax);
          if (delta === 0) return '0';
          const abs = Math.abs(delta);
          const sign = delta < 0 ? '-' : '+';
          if (abs >= 60) {
            const mins = Math.floor(abs / 60);
            const secs = abs % 60;
            return secs === 0 ? `${sign}${mins}m` : `${sign}${mins}m${secs}s`;
          }
          return `${sign}${abs}s`;
        });
      },
    };

    // ── Y axis: fixed ticks at 0/20/40/60/80/100 for percent panels ──
    const isPercent = group.yMax === 100;
    const yAxis = {
      stroke: '#666',
      grid: { stroke: '#2a2a2a', width: 1 },
      ticks: { stroke: '#333', size: 3 },
      font: '10px Cantarell, Ubuntu, sans-serif',
      size: 32,
      splits: isPercent
        ? () => [0, 20, 40, 60, 80, 100]
        : null,
      values: (u, ticks) => {
        const min = u.scales.y.min ?? 0;
        const max = u.scales.y.max ?? 100;
        const digits = (max - min) < 10 ? 1 : 0;
        return ticks.map(t => t.toFixed(digits));
      },
    };

    const axes = [xAxis, yAxis];

    if (hasTemp) {
      axes.push({
        scale: 'y2',
        side: 1,
        stroke: '#a06040',
        grid: { show: false },
        ticks: { stroke: '#4a2a18', size: 3 },
        font: '9px Cantarell, Ubuntu, sans-serif',
        size: 40,
        values: (u, ticks) => {
          const min = u.scales.y2?.min ?? 0;
          const max = u.scales.y2?.max ?? 100;
          const digits = (max - min) < 10 ? 1 : 0;
          return ticks.map(t => t.toFixed(digits) + '°');
        },
      });
    }

    const scales = {
      x: { time: true },
      y: group.yMax != null
        ? { range: () => [0, group.yMax] }
        : { range: (u, min, max) => [0, Math.max(max, 1) * 1.05] },
    };
    if (hasTemp) {
      scales.y2 = {
        range: (u, dMin, dMax) => {
          if (dMin == null || dMax == null) return [0, 100];
          if (dMin === dMax) return [Math.max(0, dMin - 5), dMax + 5];
          return [Math.max(0, dMin * 0.9), dMax * 1.1];
        },
      };
    }

    return {
      opts: {
        width, height,
        padding: [6, 10, 6, 6],
        cursor: {
          drag: { x: false, y: false, setScale: false },
          points: { show: false },
          sync: { key: cfg.CURSOR_SYNC_KEY },
        },
        legend: { show: isMulti, live: false },
        series, axes, scales,
        hooks: {
          draw: [
            (u) => App.chart.drawGhostLines(u, group),
          ],
        },
      },
      initialData,
      hasTemp,
    };
  },

  drawGhostLines(u, group) {
    const cmp = App.state.compare;
    if (!cmp.active || !cmp.timestamps.length || !App.state.timestamps.length) return;
    const { ctx, bbox } = u;
    const xMin = u.scales.x.min, xMax = u.scales.x.max;
    if (xMin == null || xMax == null) return;
    const liveLast = App.state.timestamps[App.state.timestamps.length - 1];
    const anchor = liveLast - cmp.offset;

    ctx.save();
    ctx.beginPath();
    ctx.rect(bbox.left, bbox.top, bbox.width, bbox.height);
    ctx.clip();
    ctx.setLineDash([3, 3]);
    ctx.lineWidth = 1;

    group.devs.forEach((d, i) => {
      const values = cmp.seriesData[d.id];
      if (!values || !values.length) return;
      const color = (group.color && group.devs.length === 1)
        ? group.color
        : App.config.COLORS[i % App.config.COLORS.length];
      ctx.strokeStyle = color + '80';
      ctx.beginPath();
      let started = false;
      for (let j = 0; j < cmp.timestamps.length; j++) {
        const mapped = cmp.timestamps[j] - cmp.snapshotLastTs + anchor;
        if (mapped < xMin) { started = false; continue; }
        if (mapped > xMax) break;
        const v = values[j];
        if (v == null) { started = false; continue; }
        const x = u.valToPos(mapped, 'x', true);
        const y = u.valToPos(v, 'y', true);
        if (!started) { ctx.moveTo(x, y); started = true; }
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
    });
    ctx.restore();
  },

  makeSeriesData(entry) {
    const st = App.state;
    const loadSource = st.ema ? st.emaSeries : st.seriesData;
    const tempSource = st.ema ? st.emaTemp : st.seriesTemp;
    const data = [st.timestamps];
    for (const d of entry.group.devs) data.push(loadSource[d.id] || []);
    if (entry.showAvg) {
      data.push(st.timestamps.map((_, i) => {
        let sum = 0, count = 0;
        for (const d of entry.group.devs) {
          const v = (loadSource[d.id] || [])[i];
          if (v != null) { sum += v; count++; }
        }
        return count > 0 ? sum / count : null;
      }));
    }
    if (entry.hasTemp) {
      for (const d of entry.group.devs) {
        if (!d.supportsTemp) continue;
        data.push(tempSource[d.id] || []);
      }
    }
    return data;
  },
};