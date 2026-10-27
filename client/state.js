window.App = window.App || {};

App.state = {
  mode: 'live',
  liveMode: true,
  windowS: 10,
  density: 'comfortable',
  ema: false,
  yAuto: false,
  hover: true,
  filter: '',

  devices: [],
  timestamps: [],
  seriesData: {},
  seriesTemp: {},
  emaSeries: {},
  emaTemp: {},
  plots: {},
  groups: [],

  alerts: [],
  alertState: {},
  alertsPanelOpen: false,

  sampleRateMs: 0,
  serverInterval: 100,
  _sampleDeltas: [],

  compare: { active: false, filename: '', offset: 30, snapshotLastTs: 0, timestamps: [], seriesData: {}, devices: [] },

  replay: { active: false, playing: false, speed: 1, index: 0, snapshot: null, timerId: null, filename: '' },
};