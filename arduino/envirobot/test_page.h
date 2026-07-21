#pragma once
#include <Arduino.h>

// AUTO-GENERATED from arduino/test_page/index.html — do not edit by hand.
static const char TEST_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>EnviroBot Diagnostics</title>
<style>
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
:root { --ease-out: cubic-bezier(0.23, 1, 0.32, 1); }
body {
  background: #060912; color: #F0F4FF;
  font-family: system-ui, -apple-system, sans-serif;
  display: flex; flex-direction: column;
  height: 100dvh; overflow: hidden;
  -webkit-user-select: none; user-select: none;
}
header {
  padding: 12px 20px;
  background: rgba(10,14,26,0.95);
  border-bottom: 1px solid #1F2D45;
  font-size: 13px; letter-spacing: 0.1em;
  display: flex; align-items: center; justify-content: space-between; gap: 12px;
}
#logo { color: #F0F4FF; }
#logo span { color: #00D4AA; }
.head-right { display: flex; align-items: center; gap: 14px; }
#status {
  font-size: 12px; color: #8899BB; transition: color 300ms;
  display: inline-flex; align-items: center; gap: 6px;
}
#status::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: currentColor; }
#status.ok  { color: #00D4AA; }
#status.err { color: #EF9A9A; }
a.back { color: #8899BB; text-decoration: none; font-size: 12px; }
a.back:active { color: #00D4AA; }

main {
  flex: 1; min-height: 0;
  display: flex; flex-direction: column; align-items: stretch;
  padding: 16px 20px; gap: 18px;
  overflow-y: auto;
}
main > * { flex-shrink: 0; }
.wrap { width: 100%; max-width: 460px; margin: 0 auto; }

h2 {
  font-size: 11px; letter-spacing: 0.1em; color: #4A5880;
  font-weight: 600; margin-bottom: 8px;
}

/* ── Test row ── */
.row {
  display: grid; grid-template-columns: auto 1fr auto; align-items: center;
  gap: 10px; padding: 10px 0; border-bottom: 1px solid #131C2E;
}
.row .name { font-size: 13px; color: #F0F4FF; }
.row .result {
  grid-column: 2 / 3; grid-row: 2;
  font-family: ui-monospace, monospace; font-size: 11px; color: #8899BB;
  -webkit-user-select: text; user-select: text;   /* readings copyable */
}
.run-btn {
  grid-row: 1 / 3;
  min-width: 64px; padding: 10px 12px; border-radius: 10px;
  background: #111827; border: 1px solid #1F2D45; color: #8899BB;
  font: inherit; font-size: 12px; font-weight: 600; cursor: pointer;
  transition: transform 80ms var(--ease-out), background 80ms var(--ease-out);
  -webkit-tap-highlight-color: transparent;
}
.run-btn:active { transform: scale(0.95); background: #1A2235; }
.run-btn:disabled { opacity: 0.4; pointer-events: none; }
.badge {
  grid-row: 1 / 3; width: 22px; text-align: center;
  font-size: 15px; color: #4A5880;
}
.badge.ok  { color: #00D4AA; }
.badge.err { color: #EF4444; }
.badge.run { color: #F59E0B; }

/* ── Run All ── */
.runall {
  width: 100%; margin-top: 14px; padding: 14px; border-radius: 12px;
  background: rgba(0,212,170,0.10); border: 1px solid rgba(0,212,170,0.4); color: #00D4AA;
  font: inherit; font-size: 14px; font-weight: 600; letter-spacing: 0.04em; cursor: pointer;
  transition: transform 80ms var(--ease-out), opacity 150ms;
  -webkit-tap-highlight-color: transparent;
}
.runall:active { transform: scale(0.98); }
.runall:disabled { opacity: 0.5; pointer-events: none; }

/* ── Arm toggle ── */
.arm {
  display: flex; align-items: center; gap: 12px;
  padding: 12px 14px; border-radius: 12px; margin-bottom: 12px;
  background: rgba(245,158,11,0.08); border: 1px solid rgba(245,158,11,0.3);
  cursor: pointer;
}
.arm input { width: 20px; height: 20px; accent-color: #F59E0B; flex-shrink: 0; }
.arm .txt { font-size: 12px; color: #F5C36B; line-height: 1.35; }
.arm.armed { border-color: rgba(245,158,11,0.7); background: rgba(245,158,11,0.16); }
.motion.locked { opacity: 0.45; }

/* ── Log ── */
#log {
  width: 100%; margin-top: 8px; height: 96px; overflow-y: auto;
  font-size: 11px; padding: 8px 12px; border-radius: 10px;
  background: #0D1420; border: 1px solid #1F2D45;
  -webkit-user-select: text; user-select: text;
}
.log-line       { padding: 1px 0; color: #8899BB; }
.log-line.ok    { color: #00D4AA; }
.log-line.err   { color: #EF9A9A; }

.hidden { display: none !important; }

/* ── Wizard launch buttons ── */
.wiz-launch {
  display: block; width: 100%; margin-top: 8px; padding: 12px 14px;
  border-radius: 10px; background: #111827; border: 1px solid #1F2D45;
  color: #F0F4FF; font: inherit; font-size: 13px; font-weight: 600; text-align: left;
  cursor: pointer; -webkit-tap-highlight-color: transparent;
}
.wiz-launch:active { background: #1A2235; }
.wiz-launch:disabled { opacity: 0.4; pointer-events: none; }

/* ── Wizard modal ── */
.wiz-overlay {
  position: fixed; inset: 0; z-index: 50;
  background: rgba(6,9,18,0.85); backdrop-filter: blur(2px);
  display: flex; align-items: flex-end; justify-content: center;
}
.wiz-modal {
  width: 100%; max-width: 460px; max-height: 88vh; overflow-y: auto;
  background: #0D1420; border: 1px solid #1F2D45; border-radius: 16px 16px 0 0;
  padding: 18px 20px 20px; display: flex; flex-direction: column; gap: 14px;
}
.wiz-head { display: flex; align-items: baseline; justify-content: space-between; }
.wiz-head #wizTitle { font-size: 14px; font-weight: 600; color: #F0F4FF; }
.wiz-head #wizStep { font-size: 11px; color: #4A5880; letter-spacing: 0.05em; }
.wiz-body { font-size: 13px; line-height: 1.5; color: #C7D2E8; }
.wiz-body p { margin-bottom: 6px; }
.wiz-body .wiz-err { color: #EF9A9A; }
.wiz-input {
  width: 100%; margin-top: 8px; padding: 10px 12px; border-radius: 8px;
  background: #111827; border: 1px solid #1F2D45; color: #F0F4FF; font: inherit; font-size: 14px;
}
.wiz-bar { margin-top: 10px; height: 6px; border-radius: 3px; background: #111827; overflow: hidden; }
.wiz-bar-fill { height: 100%; width: 0%; background: #00D4AA; }
.wiz-actions { display: flex; gap: 10px; }
.wiz-btn {
  flex: 1; padding: 12px; border-radius: 10px; font: inherit; font-size: 13px; font-weight: 600;
  cursor: pointer; -webkit-tap-highlight-color: transparent; border: 1px solid transparent;
}
.wiz-btn.primary { background: rgba(0,212,170,0.14); border-color: rgba(0,212,170,0.4); color: #00D4AA; }
.wiz-btn.ghost { background: #111827; border-color: #1F2D45; color: #8899BB; }
.wiz-btn:disabled { opacity: 0.4; pointer-events: none; }

/* ── Config editor ── */
.lock-banner {
  padding: 10px 12px; margin-bottom: 10px; border-radius: 10px;
  background: rgba(239,68,68,0.12); border: 1px solid rgba(239,68,68,0.4);
  color: #EF9A9A; font-size: 12px; font-weight: 600;
}
.cfg-group-title { font-size: 11px; letter-spacing: 0.08em; color: #4A5880; margin: 12px 0 4px; }
.cfg-row {
  display: flex; align-items: center; justify-content: space-between; gap: 10px;
  padding: 8px 0; border-bottom: 1px solid #131C2E;
}
.cfg-row label { font-size: 12px; color: #C7D2E8; display: flex; flex-direction: column; gap: 2px; }
.cfg-range { font-size: 10px; color: #4A5880; font-family: ui-monospace, monospace; }
.cfg-row input {
  width: 88px; padding: 8px 10px; border-radius: 8px; text-align: right;
  background: #111827; border: 1px solid #1F2D45; color: #F0F4FF; font: inherit; font-size: 13px;
}
.cfg-row.dirty input { border-color: #F59E0B; color: #F5C36B; }
.cfg-row input:disabled { opacity: 0.4; }
.cfg-actions { display: flex; gap: 10px; margin-top: 14px; }

@media (prefers-reduced-motion: reduce) {
  * { transition-duration: 1ms !important; }
  .run-btn:active, .runall:active { transform: none; }
}
</style>
</head>
<body>
<header>
  <span id="logo">ENVIRO<span>BOT</span> DIAG</span>
  <span class="head-right">
    <span id="status">connecting…</span>
    <a class="back" href="/view">📊 Data</a>
    <a class="back" href="/">← RC</a>
  </span>
</header>

<main>
  <!-- Read-only sensor self-tests -->
  <section class="wrap">
    <h2>SENSORS</h2>
    <div class="row" data-test="turbidity">
      <button class="run-btn">Run</button>
      <span class="name">Turbidity</span>
      <span class="result">not run</span>
      <span class="badge">·</span>
    </div>
    <div class="row" data-test="soil">
      <button class="run-btn">Run</button>
      <span class="name">Soil moisture</span>
      <span class="result">not run</span>
      <span class="badge">·</span>
    </div>
    <div class="row" data-test="zones">
      <button class="run-btn">Run</button>
      <span class="name">Colour zones</span>
      <span class="result">not run</span>
      <span class="badge">·</span>
    </div>
    <div class="row" data-test="wall">
      <button class="run-btn">Run</button>
      <span class="name">Wall distance</span>
      <span class="result">not run</span>
      <span class="badge">·</span>
    </div>
    <div class="row" data-test="i2c">
      <button class="run-btn">Run</button>
      <span class="name">I2C bus</span>
      <span class="result">not run</span>
      <span class="badge">·</span>
    </div>
    <button class="runall" id="runAll">▶ Run all sensor tests</button>
  </section>

  <!-- Motion tests — physically move the robot. Locked until armed. -->
  <section class="wrap">
    <h2>MOTION</h2>
    <label class="arm" id="armBox">
      <input type="checkbox" id="armChk">
      <span class="txt">⚠ ARM: robot on blocks, arm path clear. Enables tests that drive the wheels and sweep the arm.</span>
    </label>
    <div class="motion locked" id="motion">
      <div class="row" data-test="motors">
        <button class="run-btn" disabled>Run</button>
        <span class="name">Motor drive</span>
        <span class="result">fwd · bwd · left · right</span>
        <span class="badge">·</span>
      </div>
      <div class="row" data-test="servo">
        <button class="run-btn" disabled>Run</button>
        <span class="name">Servo arm sweep</span>
        <span class="result">water · neutral · soil</span>
        <span class="badge">·</span>
      </div>
    </div>
  </section>

  <!-- Dead-reckoned pose re-zero — after an official re-centres a stuck robot (rulebook 5.2.1) -->
  <section class="wrap">
    <h2>POSE</h2>
    <div class="cfg-row">
      <label for="rezeroH">Re-zero pose<span class="cfg-range">eyeballed heading 0–359 °</span></label>
      <input type="number" id="rezeroH" min="0" max="359" step="1" value="0">
    </div>
    <button class="wiz-btn primary" id="btnRezero" style="margin-top:10px">⌖ Re-zero pose to origin</button>
  </section>

  <!-- Calibration wizards -->
  <section class="wrap">
    <h2>CALIBRATION WIZARDS</h2>
    <button class="wiz-launch" id="wizSoil">Soil probe →</button>
    <button class="wiz-launch" id="wizTurb">Turbidity →</button>
    <button class="wiz-launch" id="wizMotion">Motion →</button>
    <button class="wiz-launch" id="wizColour" data-locked="1" disabled>Colour / zone → (autonomous build only)</button>
  </section>

  <!-- Config editor -->
  <section class="wrap">
    <h2>CONFIG</h2>
    <div id="runLockBanner" class="lock-banner hidden">Run in progress — config locked until END_RUN</div>
    <div class="cfg-group-title">Calibration</div>
    <div id="cfgCalGroup"></div>
    <div class="cfg-group-title">Tuning</div>
    <div id="cfgTuneGroup"></div>
    <div class="cfg-actions">
      <button id="cfgReset" class="wiz-btn ghost">Restore defaults</button>
      <button id="cfgSave" class="wiz-btn primary" disabled>Save</button>
    </div>
  </section>

  <div class="wrap">
    <div id="log" aria-live="polite"><div class="log-line">Diagnostics ready.</div></div>
  </div>
</main>

<div class="wiz-overlay hidden" id="wizOverlay">
  <div class="wiz-modal">
    <div class="wiz-head"><span id="wizTitle"></span><span id="wizStep"></span></div>
    <div class="wiz-body" id="wizBody"></div>
    <div class="wiz-actions">
      <button class="wiz-btn ghost" id="wizCancel">Cancel</button>
      <button class="wiz-btn primary" id="wizAction">Ready</button>
    </div>
  </div>
</div>

<script>
const statusEl = document.getElementById('status');
const logEl    = document.getElementById('log');
let armed = false;

function ts() { return new Date().toLocaleTimeString('en', { hour12: false }); }
function addLog(text, cls) {
  const el = document.createElement('div');
  el.className = 'log-line' + (cls ? ' ' + cls : '');
  el.textContent = ts() + '  ' + text;
  logEl.appendChild(el);
  logEl.scrollTop = logEl.scrollHeight;
  while (logEl.children.length > 40) logEl.removeChild(logEl.firstChild);
}

async function selftest(test) {
  const r = await fetch('/selftest', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ test, armed }),
  });
  statusEl.textContent = 'connected'; statusEl.className = 'ok';
  if (!r.ok) {
    const e = await r.json().catch(() => ({}));
    const err = new Error(e.error || ('HTTP ' + r.status));
    err.http = true;   // server answered (409 etc) — the link is fine, don't flag offline
    throw err;
  }
  return r.json();
}

function setRow(row, state, text) {
  const badge = row.querySelector('.badge');
  const res   = row.querySelector('.result');
  badge.className = 'badge' + (state ? ' ' + state : '');
  badge.textContent = state === 'ok' ? '✓' : state === 'err' ? '✗' : state === 'run' ? '…' : '·';
  if (text != null) res.textContent = text;
}

// Run one row's test; returns true on pass. Never throws (logs instead).
async function runRow(row) {
  const test = row.dataset.test;
  setRow(row, 'run');
  try {
    const d = await selftest(test);
    setRow(row, d.ok ? 'ok' : 'err', d.value || '');
    addLog(test + ': ' + (d.value || (d.ok ? 'ok' : 'fail')), d.ok ? 'ok' : 'err');
    return d.ok;
  } catch (e) {
    setRow(row, 'err', e.message);
    addLog(test + ': ' + e.message, 'err');
    statusEl.textContent = 'offline'; statusEl.className = 'err';
    return false;
  }
}

document.querySelectorAll('.row').forEach(row => {
  row.querySelector('.run-btn').addEventListener('click', () => runRow(row));
});

// Run all — read-only sensor tests only, in sequence.
document.getElementById('runAll').addEventListener('click', async function () {
  this.disabled = true;
  addLog('Running all sensor tests…');
  let pass = 0, total = 0;
  for (const row of document.querySelectorAll('section .row[data-test]')) {
    if (row.closest('#motion')) continue;   // never auto-run motion tests
    total++;
    if (await runRow(row)) pass++;
  }
  addLog('Sensor suite: ' + pass + '/' + total + ' passed', pass === total ? 'ok' : 'err');
  this.disabled = false;
});

// Arm toggle — unlocks the motion tests.
const armChk = document.getElementById('armChk');
armChk.addEventListener('change', () => {
  armed = armChk.checked;
  document.getElementById('armBox').classList.toggle('armed', armed);
  document.getElementById('motion').classList.toggle('locked', !armed);
  document.querySelectorAll('#motion .run-btn').forEach(b => b.disabled = !armed);
  addLog(armed ? 'Motion tests ARMED (robot on blocks)' : 'Motion tests disarmed');
});

// Heartbeat — connected/offline indicator (2Hz). Reuses GET /config so the
// same poll also carries the run-lock flag; never touches field values —
// only setFieldValues() (initial load / after Save / Reset) does that.
let runActive = false;
setInterval(async () => {
  try {
    const r = await fetch('/config', { signal: AbortSignal.timeout(1500) });
    const d = await r.json();
    statusEl.textContent = 'connected'; statusEl.className = 'ok';
    runActive = d.runActive;
    applyLockUI();
  } catch { statusEl.textContent = 'offline'; statusEl.className = 'err'; }
}, 500);

// ============================================================
// Config editor
// ============================================================
const FIELDS = [
  { key: 'soilDryVal',        label: 'Soil dry (ADC)',        group: 'cal',  min: 0,    max: 4095,  step: 1,    unit: '' },
  { key: 'soilWetVal',        label: 'Soil wet (ADC)',        group: 'cal',  min: 0,    max: 4095,  step: 1,    unit: '' },
  { key: 'turbidityZeroRaw',  label: 'Turbidity zero (ADC)',  group: 'cal',  min: 0,    max: 4095,  step: 1,    unit: '' },
  { key: 'mmPerSecAtDrive',   label: 'Drive speed',           group: 'cal',  min: 10,   max: 1000,  step: 0.1,  unit: 'mm/s' },
  { key: 'wheelBaseMm',       label: 'Wheel base',            group: 'cal',  min: 50,   max: 500,   step: 0.1,  unit: 'mm' },
  { key: 'waterClearMin',     label: 'Water clear min',       group: 'cal',  min: 0,    max: 65535, step: 1,    unit: '' },
  { key: 'waterRedRatioMax',  label: 'Water red ratio max',   group: 'cal',  min: 0,    max: 1,     step: 0.01, unit: '' },
  { key: 'soilRedRatioMin',   label: 'Soil red ratio min',    group: 'cal',  min: 0,    max: 1,     step: 0.01, unit: '' },
  { key: 'soilBlueRatioMax',  label: 'Soil blue ratio max',   group: 'cal',  min: 0,    max: 1,     step: 0.01, unit: '' },
  { key: 'driveSpeed',        label: 'Drive speed (PWM)',     group: 'tune', min: 0,    max: 255,   step: 1,    unit: '' },
  { key: 'turnSpeed',         label: 'Turn speed (PWM)',      group: 'tune', min: 0,    max: 255,   step: 1,    unit: '' },
  { key: 'wallStopMm',        label: 'Wall stop distance',    group: 'tune', min: 30,   max: 2000,  step: 1,    unit: 'mm' },
  { key: 'sampleSettleMs',    label: 'Sample settle',         group: 'tune', min: 200,  max: 10000, step: 1,    unit: 'ms' },
  { key: 'sampleWindowMs',    label: 'Sample window',         group: 'tune', min: 500,  max: 10000, step: 1,    unit: 'ms' },
  { key: 'servoNeutral',      label: 'Servo neutral',         group: 'tune', min: 0,    max: 180,   step: 1,    unit: 'deg' },
  { key: 'servoWater',        label: 'Servo water',           group: 'tune', min: 0,    max: 180,   step: 1,    unit: 'deg' },
  { key: 'servoSoil',         label: 'Servo soil',            group: 'tune', min: 0,    max: 180,   step: 1,    unit: 'deg' },
  { key: 'headingDeadbandRad',label: 'Heading deadband',      group: 'tune', min: 0.05, max: 1.5,   step: 0.01, unit: 'rad' },
];

let serverConfig = null;
const dirty = new Set();

function fieldRowHtml(f) {
  const unit = f.unit ? ' ' + f.unit : '';
  return '<div class="cfg-row" data-key="' + f.key + '">' +
    '<label for="cfg_' + f.key + '">' + f.label +
    '<span class="cfg-range">' + f.min + '–' + f.max + unit + '</span></label>' +
    '<input type="number" id="cfg_' + f.key + '" min="' + f.min + '" max="' + f.max + '" step="' + f.step + '">' +
    '</div>';
}

function buildConfigEditor() {
  document.getElementById('cfgCalGroup').innerHTML  = FIELDS.filter(f => f.group === 'cal').map(fieldRowHtml).join('');
  document.getElementById('cfgTuneGroup').innerHTML = FIELDS.filter(f => f.group === 'tune').map(fieldRowHtml).join('');
  FIELDS.forEach(f => {
    document.getElementById('cfg_' + f.key).addEventListener('input', () => markDirty(f.key));
  });
}

function markDirty(key) {
  dirty.add(key);
  document.querySelector('.cfg-row[data-key="' + key + '"]').classList.add('dirty');
  document.getElementById('cfgSave').disabled = runActive;
}

// Wizard fills — local only, not saved until the user hits Save.
function fillConfigFields(values) {
  Object.entries(values).forEach(([k, v]) => {
    const el = document.getElementById('cfg_' + k);
    if (el) { el.value = v; markDirty(k); }
  });
}

function setFieldValues(cfg) {
  serverConfig = cfg;
  FIELDS.forEach(f => { document.getElementById('cfg_' + f.key).value = cfg[f.key]; });
  dirty.clear();
  document.querySelectorAll('.cfg-row.dirty').forEach(r => r.classList.remove('dirty'));
  document.getElementById('cfgSave').disabled = true;
}

function applyLockUI() {
  document.getElementById('runLockBanner').classList.toggle('hidden', !runActive);
  FIELDS.forEach(f => { document.getElementById('cfg_' + f.key).disabled = runActive; });
  document.getElementById('cfgSave').disabled = runActive || dirty.size === 0;
  document.getElementById('cfgReset').disabled = runActive;
  document.querySelectorAll('.wiz-launch').forEach(b => {
    if (b.dataset.locked !== '1') b.disabled = runActive;
  });
}

async function loadConfigEditor() {
  const r = await fetch('/config');
  const d = await r.json();
  runActive = d.runActive;
  setFieldValues(d);
  applyLockUI();
}

document.getElementById('cfgSave').addEventListener('click', async () => {
  const body = {};
  FIELDS.forEach(f => { body[f.key] = parseFloat(document.getElementById('cfg_' + f.key).value); });
  try {
    const r = await fetch('/config', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
    const d = await r.json();
    if (!r.ok) { addLog('Config save failed: ' + (d.error || r.status), 'err'); return; }
    setFieldValues(d);
    addLog('Config saved', 'ok');
  } catch (e) { addLog('Config save failed: ' + e.message, 'err'); }
});

document.getElementById('cfgReset').addEventListener('click', async () => {
  if (!confirm('Restore all config fields to firmware defaults?')) return;
  try {
    const r = await fetch('/config/reset', { method: 'POST' });
    const d = await r.json();
    if (!r.ok) { addLog('Reset failed: ' + (d.error || r.status), 'err'); return; }
    setFieldValues(d);
    addLog('Config restored to defaults', 'ok');
  } catch (e) { addLog('Reset failed: ' + e.message, 'err'); }
});

buildConfigEditor();
loadConfigEditor();

// ============================================================
// Calibration wizards — shared modal shell
// ============================================================
const wizOverlay  = document.getElementById('wizOverlay');
const wizTitleEl  = document.getElementById('wizTitle');
const wizStepEl   = document.getElementById('wizStep');
const wizBody     = document.getElementById('wizBody');
const wizAction   = document.getElementById('wizAction');
const wizCancelBtn = document.getElementById('wizCancel');

function openWizard(title) {
  wizTitleEl.textContent = title;
  wizOverlay.classList.remove('hidden');
}
function closeWizard() {
  wizOverlay.classList.add('hidden');
  wizBody.innerHTML = '';
  wizAction.onclick = null;
}
wizCancelBtn.addEventListener('click', () => { closeWizard(); addLog('Wizard cancelled'); });

function wizSetStep(n, total, html, actionLabel, onAction) {
  wizStepEl.textContent = 'Step ' + n + ' / ' + total;
  wizBody.innerHTML = html;
  wizAction.textContent = actionLabel;
  wizAction.disabled = false;
  wizAction.onclick = onAction;
}

// Animates a progress bar over ms inside the current step body. Resolves
// when done — pair with Promise.all([captureCall, wizProgress(ms)]) so the
// bar always shows the full 5s even if the HTTP round-trip is faster.
function wizProgress(ms) {
  return new Promise(resolve => {
    const bar = document.createElement('div');
    bar.className = 'wiz-bar';
    const fill = document.createElement('div');
    fill.className = 'wiz-bar-fill';
    bar.appendChild(fill);
    wizBody.appendChild(bar);
    const start = Date.now();
    (function tick() {
      const pct = Math.min(100, (Date.now() - start) / ms * 100);
      fill.style.width = pct + '%';
      if (pct < 100) requestAnimationFrame(tick); else resolve();
    })();
  });
}

async function wizCapture(target) {
  const r = await fetch('/cal/capture', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ target }) });
  const d = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(d.error || ('HTTP ' + r.status));
  return d.raw;
}

async function rcCmd(cmd, extra) {
  const r = await fetch('/rc', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(Object.assign({ cmd }, extra || {})) });
  const d = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(d.error || ('HTTP ' + r.status));
  return d;
}

// ── Soil wizard ──────────────────────────────────────────────────
function soilWizard() {
  openWizard('Soil calibration');
  let dryVal, wetVal;

  function step1() {
    wizSetStep(1, 3, '<p>Hold the soil probe in open air, completely dry. Wipe it if damp.</p>', 'Capture', async () => {
      wizAction.disabled = true;
      try { [dryVal] = await Promise.all([wizCapture('soil_dry'), wizProgress(5000)]); step2(); }
      catch (e) { addLog('Capture failed: ' + e.message, 'err'); wizAction.disabled = false; }
    });
  }
  function step2() {
    wizSetStep(2, 3, '<p>Submerge the probe in water <b>up to the marked line only</b> — never past the electronics.</p>', 'Capture', async () => {
      wizAction.disabled = true;
      try { [wetVal] = await Promise.all([wizCapture('soil_wet'), wizProgress(5000)]); step3(); }
      catch (e) { addLog('Capture failed: ' + e.message, 'err'); wizAction.disabled = false; }
    });
  }
  function step3() {
    const span = Math.abs(dryVal - wetVal);
    if (span < 200) {
      wizSetStep(3, 3,
        '<p>Dry: ' + dryVal.toFixed(0) + '  ·  Wet: ' + wetVal.toFixed(0) + '  ·  span ' + span.toFixed(0) + '</p>' +
        '<p class="wiz-err">Span too small — probe may be faulty or not actually wet/dry.</p>',
        'Retry', step1);
      return;
    }
    wizSetStep(3, 3, '<p>Dry: ' + dryVal.toFixed(0) + '  ·  Wet: ' + wetVal.toFixed(0) + '  ·  span ' + span.toFixed(0) + '</p>', 'Save to editor', () => {
      fillConfigFields({ soilDryVal: Math.round(dryVal), soilWetVal: Math.round(wetVal) });
      closeWizard();
      addLog('Soil calibration filled — hit Save to apply', 'ok');
    });
  }
  step1();
}

// ── Turbidity wizard ─────────────────────────────────────────────
function turbidityWizard() {
  openWizard('Turbidity calibration');

  function step1() {
    wizSetStep(1, 2, '<p>Submerge the SEN0189 probe in clear tap water. No bubbles on the optical window.</p>', 'Capture', async () => {
      wizAction.disabled = true;
      try { const [raw] = await Promise.all([wizCapture('turb_zero'), wizProgress(5000)]); step2(raw); }
      catch (e) { addLog('Capture failed: ' + e.message, 'err'); wizAction.disabled = false; }
    });
  }
  function step2(raw) {
    wizSetStep(2, 2,
      '<p>Zero point: ' + raw.toFixed(0) + ' ADC.</p>' +
      '<p>Slope stays the SEN0189 datasheet curve — NTU = curve(raw) offset so clear water reads 0.</p>',
      'Save to editor', () => {
        fillConfigFields({ turbidityZeroRaw: Math.round(raw) });
        closeWizard();
        addLog('Turbidity zero filled — hit Save to apply', 'ok');
      });
  }
  step1();
}

// ── Motion wizard ────────────────────────────────────────────────
function motionWizard() {
  openWizard('Motion calibration');
  const FWD_MS = 3000, SPIN_MS = 2000;
  let mmPerSec, wheelBase;

  function step1() {
    wizSetStep(1, 4, '<p>Place the robot on the arena surface with 2m clear ahead. Mark the starting point.</p>', 'Run', async () => {
      wizAction.disabled = true;
      try { await Promise.all([rcCmd('CAL_FWD', { ms: FWD_MS }), wizProgress(FWD_MS)]); step2(); }
      catch (e) { addLog('Drive failed: ' + e.message, 'err'); wizAction.disabled = false; }
    });
  }
  function step2() {
    wizSetStep(2, 4, '<p>Measure the distance travelled.</p><input type="number" id="wizMm" class="wiz-input" placeholder="mm">', 'Next', () => {
      const mm = parseFloat(document.getElementById('wizMm').value);
      if (!(mm > 0)) { addLog('Enter a distance > 0', 'err'); return; }
      mmPerSec = mm / (FWD_MS / 1000);
      step3();
    });
  }
  function step3() {
    wizSetStep(3, 4, "<p>Mark the robot's heading. Robot will spin.</p>", 'Run', async () => {
      wizAction.disabled = true;
      try { await Promise.all([rcCmd('CAL_SPIN', { ms: SPIN_MS }), wizProgress(SPIN_MS)]); step4(); }
      catch (e) { addLog('Spin failed: ' + e.message, 'err'); wizAction.disabled = false; }
    });
  }
  function step4() {
    wizSetStep(4, 4, '<p>How many degrees did it actually turn?</p><input type="number" id="wizDeg" class="wiz-input" placeholder="deg">', 'Compute', () => {
      const deg = parseFloat(document.getElementById('wizDeg').value);
      if (!(deg > 0)) { addLog('Enter degrees > 0', 'err'); return; }
      // Same differential-drive model as position.cpp: v = mmPerSecAtDrive * turnSpeed/driveSpeed,
      // theta = 2 * v * t / wheelBase (in-place spin, both wheels at ±v).
      const v = mmPerSec * (serverConfig.turnSpeed / serverConfig.driveSpeed);
      const theta = deg * Math.PI / 180;
      wheelBase = 2 * v * (SPIN_MS / 1000) / theta;
      stepResult();
    });
  }
  function stepResult() {
    wizSetStep(4, 4, '<p>Drive: ' + mmPerSec.toFixed(1) + ' mm/s  ·  Wheel base: ' + wheelBase.toFixed(1) + ' mm</p>', 'Save to editor', () => {
      fillConfigFields({ mmPerSecAtDrive: +mmPerSec.toFixed(1), wheelBaseMm: +wheelBase.toFixed(1) });
      closeWizard();
      addLog('Motion calibration filled — hit Save to apply', 'ok');
    });
    const verifyBtn = document.createElement('button');
    verifyBtn.className = 'wiz-btn ghost';
    verifyBtn.style.marginTop = '8px';
    verifyBtn.textContent = 'Spin again to verify';
    verifyBtn.onclick = step3;
    wizBody.appendChild(verifyBtn);
  }
  step1();
}

// ── Colour / zone wizard — autonomous build only (greyed while ENABLE_AUTONOMOUS == 0) ──
function colourWizard() {
  openWizard('Colour / zone calibration');
  let water, soil, floor;

  function captureStep(n, total, text, target, onDone) {
    wizSetStep(n, total, '<p>' + text + '</p>', 'Capture', async () => {
      wizAction.disabled = true;
      try { const [raw] = await Promise.all([wizCapture(target), wizProgress(3000)]); onDone(raw); }
      catch (e) { addLog('Capture failed: ' + e.message, 'err'); wizAction.disabled = false; }
    });
  }
  function step1() { captureStep(1, 4, 'Park the colour sensor over a water zone.', 'color_water', raw => { water = raw; step2(); }); }
  function step2() { captureStep(2, 4, 'Park over a soil disc.', 'color_soil', raw => { soil = raw; step3(); }); }
  function step3() { captureStep(3, 4, 'Park over bare arena floor.', 'color_floor', raw => { floor = raw; step4(); }); }
  function step4() {
    const wRatio = water.r / water.c, sRatio = soil.r / soil.c, sBlue = soil.b / soil.c;
    const fRatio = floor.r / floor.c, fBlue = floor.b / floor.c;
    const overlap = !(water.c > floor.c) || !(wRatio < fRatio) || !(sRatio > fRatio) || !(sBlue < fBlue);
    if (overlap) {
      wizSetStep(4, 4, '<p class="wiz-err">Water and floor are not distinguishable by this sensor at this height/lighting.</p>', 'Retry', step1);
      return;
    }
    const waterClearMin = (water.c + floor.c) / 2;
    const waterRedRatioMax = (wRatio + fRatio) / 2;
    const soilRedRatioMin = (sRatio + fRatio) / 2;
    const soilBlueRatioMax = (sBlue + fBlue) / 2;
    wizSetStep(4, 4,
      '<p>clearMin ' + waterClearMin.toFixed(0) + ' · redMax ' + waterRedRatioMax.toFixed(2) +
      ' · soilRedMin ' + soilRedRatioMin.toFixed(2) + ' · soilBlueMax ' + soilBlueRatioMax.toFixed(2) + '</p>',
      'Save to editor', () => {
        fillConfigFields({
          waterClearMin: Math.round(waterClearMin), waterRedRatioMax: +waterRedRatioMax.toFixed(2),
          soilRedRatioMin: +soilRedRatioMin.toFixed(2), soilBlueRatioMax: +soilBlueRatioMax.toFixed(2),
        });
        closeWizard();
        addLog('Colour calibration filled — hit Save to apply', 'ok');
      });
  }
  step1();
}

document.getElementById('wizSoil').addEventListener('click', soilWizard);
document.getElementById('wizTurb').addEventListener('click', turbidityWizard);
document.getElementById('wizMotion').addEventListener('click', motionWizard);
// wizColour has no listener while ENABLE_AUTONOMOUS == 0 — button stays disabled (see robot_config comment).

// ── Pose re-zero — officials re-centre a stuck robot (rulebook 5.2.1); h = eyeballed heading ──
document.getElementById('btnRezero').addEventListener('click', async () => {
  const h = parseFloat(document.getElementById('rezeroH').value) || 0;
  try { await rcCmd('REZERO', { h }); addLog('Pose re-zeroed, heading ' + h + '°', 'ok'); }
  catch (e) { addLog('Re-zero failed: ' + e.message, 'err'); }
});
</script>
</body>
</html>
)rawliteral";
