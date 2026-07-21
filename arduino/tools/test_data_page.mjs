#!/usr/bin/env node
// Unit test for the /view live-data page. Loads the REAL <script> from
// data_page/index.html into a stubbed DOM/canvas sandbox (no drift vs a copy),
// then asserts the pure helpers + that rendering never throws.
//   node arduino/tools/test_data_page.mjs
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import vm from 'node:vm';
import assert from 'node:assert/strict';

const here = path.dirname(fileURLToPath(import.meta.url));
const html = readFileSync(path.join(here, '..', 'data_page', 'index.html'), 'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];

// ── Minimal DOM / canvas / runtime stubs ──
function makeCtx(canvas) {
  return new Proxy({ canvas }, {
    get(t, p) { return p in t ? t[p] : () => {}; },   // any 2d-context method → no-op
    set(t, p, v) { t[p] = v; return true; },
  });
}
function makeEl() {
  const kids = [];
  const el = {
    width: 400, height: 300, textContent: '', className: '', scrollTop: 0, scrollHeight: 0,
    style: {}, classList: { toggle() {}, add() {}, remove() {} },
    getContext() { return makeCtx(el); },
    getBoundingClientRect() { return { width: 400, height: 300 }; },
    appendChild(c) { kids.push(c); },
    removeChild(c) { const i = kids.indexOf(c); if (i >= 0) kids.splice(i, 1); },
    get firstChild() { return kids[0]; },
    get children() { return kids; },
    set innerHTML(_v) {}, get innerHTML() { return ''; },
  };
  return el;
}
const els = {};
const sandbox = {
  console,
  Math, JSON, Set, Array, Date, Promise, AbortSignal, String, Number,
  setInterval: () => 0, clearInterval: () => {},   // no live timers in the test
  requestAnimationFrame: () => 0,
  fetch: () => Promise.reject(new Error('no network in test')),   // seed()/polls catch this
  window: { addEventListener() {}, devicePixelRatio: 1 },
  document: {
    getElementById: (id) => (els[id] ||= makeEl()),
    createElement: () => makeEl(),
  },
};
sandbox.window.fetch = sandbox.fetch;

// Running the script also exercises onResize()+render()+seed() on empty data (no-throw #1)
vm.createContext(sandbox);
vm.runInContext(script, sandbox, { filename: 'data_page.js' });

// ── Assertions on the real functions ──
assert.equal(sandbox.fmtClock(0), '--:--', 'clock: not started');
assert.equal(sandbox.fmtClock(65), '1:05', 'clock: mm:ss zero-pad');
assert.equal(sandbox.fmtClock(485), '8:05', 'clock: past 8 min');

assert.equal(sandbox.octagonPoints(0, 0, 10).length, 8, 'octagon has 8 points');

const ctx = makeCtx(makeEl());
const populated = {
  path: [
    { t_s: 0, x_mm: 0, y_mm: 0 },
    { t_s: 1, x_mm: 100, y_mm: 50 },
    { t_s: 2, x_mm: 200, y_mm: 40 },
  ],
  samples: [
    { id: 'W1', type: 'WATER', sector: 1, value_ntu: 42.3, value_pct: null, t_s: 1 },
    { id: 'S1', type: 'SOIL', sector: 2, value_ntu: null, value_pct: 0, t_s: 2 },
  ],
  total_duration_s: 2,
};
assert.doesNotThrow(() => sandbox.drawArena(ctx, populated), 'drawArena: populated run');
assert.doesNotThrow(() => sandbox.drawArena(ctx, { path: [], samples: [], total_duration_s: 0 }), 'drawArena: empty run');

const cv = makeEl();
assert.doesNotThrow(() => sandbox.drawBars(cv, [{ label: 'W1', value: 42.3 }], '#4FC3F7', 'water', null), 'drawBars: water');
assert.doesNotThrow(() => sandbox.drawBars(cv, [{ label: 'S1', value: 0 }], '#D9A03F', 'soil', 100), 'drawBars: all-zero (no div-by-zero)');
assert.doesNotThrow(() => sandbox.drawBars(cv, [], '#4FC3F7', 'water', null), 'drawBars: empty');

console.log('PASS: data_page.js — clock, octagon, drawArena, drawBars (7 assertions)');
