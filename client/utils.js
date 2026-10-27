window.App = window.App || {};

App.utils = {
  // Server's displayName wins; regex fallback only for legacy snapshots.
  shortName(deviceOrName) {
    const isObj = deviceOrName && typeof deviceOrName === 'object';
    if (isObj && deviceOrName.displayName) return deviceOrName.displayName;

    const fullName = isObj ? (deviceOrName.name || '') : (deviceOrName || '');
    const deviceId = isObj ? deviceOrName.id : null;
    if (!fullName) return deviceId || '';

    const override = deviceId && (App.config.NAME_OVERRIDES || {})[deviceId];
    if (override) return override;

    let base = fullName, suffix = '';
    const m = base.match(/\s+thread\s*#?\s*(\d+)\s*$/i);
    if (m) { suffix = ' #' + m[1]; base = base.slice(0, m.index); }

    base = base.replace(/\s+with\s+.*$/i, '')
               .replace(/\s+CPU\s+@\s*\S+$/i, '')
               .replace(/\s+@\s*\S+$/i, '')
               .replace(/\s+\d+[- ]Core\s+Processor$/i, '')
               .replace(/\s+Processor$/i, '')
               .replace(/^(AMD|Intel|NVIDIA|Apple|Qualcomm|ARM|MediaTek)\s+/i, '')
               .replace(/\s+/g, ' ').trim();
    if (!base) base = fullName.replace(/\s+/g, ' ').trim();

    const MAX = App.config.NAME_MAX_LENGTH || 22;
    if (base.length > MAX) base = base.slice(0, MAX - 1).trimEnd() + '…';
    return base + suffix;
  },

  pad2(n) { return String(n).padStart(2, '0'); },

  fmtClock(ts) {
    const d = new Date(ts * 1000);
    return `${this.pad2(d.getHours())}:${this.pad2(d.getMinutes())}:${this.pad2(d.getSeconds())}`;
  },

  fmtDuration(seconds) {
    const s = Math.floor(seconds);
    return `${Math.floor(s / 60)}:${this.pad2(s % 60)}`;
  },

  clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); },

  lowerBound(arr, value) {
    let lo = 0, hi = arr.length;
    while (lo < hi) {
      const mid = (lo + hi) >> 1;
      if (arr[mid] < value) lo = mid + 1; else hi = mid;
    }
    return lo;
  },

  // Uses the level that came from the server (set on the device object).
  classifyLevel(devices) {
    let cls = 'ok';
    for (const d of devices) {
      if (d.level === 'crit') return 'crit';
      if (d.level === 'warn') cls = 'warn';
    }
    return cls;
  },

  suffixFor(group) {
    if (group.unit !== undefined) return group.unit;
    return group.yMax === 100 ? '%' : '';
  },
};