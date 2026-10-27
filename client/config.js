window.App = window.App || {};

const origin = window.location.protocol === 'file:' ? 'http://127.0.0.1:8080' : '';

App.config = {
  SERVER: origin,

  INTERVAL: 100,
  PRECISION: 2,
  HISTORY_MAX: 3000,
  DEFAULT_WIN_S: 10,
  WINDOW_OPTIONS: [
    { label: '5s',  seconds: 5 },
    { label: '10s', seconds: 10 },
    { label: '30s', seconds: 30 },
    { label: '1m',  seconds: 60 },
    { label: '5m',  seconds: 300 },
  ],
  GROUP_DEFS: [],

  OVERSCROLL: 0.1,
  EMA_ALPHA: 0.7,

  DENSITY: {
    comfortable: { threadH: 260, otherH: 200 },
    compact:     { threadH: 150, otherH: 120 },
  },

  // Per-group primary color. `null` = use palette per series (multi-series only).
  GROUP_COLORS: {
    cpu_thread: null,
    cpu:  '#33d17a',
    ram:  '#9141ac',
    vram: '#a06ac9',
    gpu:  '#3584e4',
  },

  ALERTS: {
    consecutiveSamples: 5,
    minLevel: 'warn',
    maxEntries: 100,
  },

  NAME_MAX_LENGTH: 22,
  NAME_OVERRIDES: {},
  CURSOR_SYNC_KEY: 'sensors-monitor',

  // Adwaita-inspired muted palette
  COLORS: [
    '#33d17a', '#3584e4', '#f6d32d', '#e01b24',
    '#9141ac', '#00b6d0', '#ff7800', '#26a269',
    '#c061cb', '#62a0ea', '#f9c440', '#c64600',
  ],
};