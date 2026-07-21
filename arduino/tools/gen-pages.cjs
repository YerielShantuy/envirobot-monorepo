#!/usr/bin/env node
// Regenerates the PROGMEM page headers from their HTML sources.
// Run after editing any page under arduino/*_page/index.html:
//   node arduino/tools/gen-pages.cjs
const fs = require('fs');
const path = require('path');

const arduino = path.resolve(__dirname, '..');

const PAGES = [
  { src: 'rc_page/index.html',   out: 'envirobot/rc_page.h',   symbol: 'RC_PAGE_HTML'   },
  { src: 'test_page/index.html', out: 'envirobot/test_page.h', symbol: 'TEST_PAGE_HTML' },
  { src: 'data_page/index.html', out: 'envirobot/data_page.h', symbol: 'DATA_PAGE_HTML' },
];

for (const p of PAGES) {
  const html = fs.readFileSync(path.join(arduino, p.src), 'utf8');
  // The C++ raw-string delimiter is )rawliteral" — the HTML must never contain it.
  if (html.includes(')rawliteral"')) {
    throw new Error(p.src + ' contains the rawliteral delimiter');
  }
  const body =
    '#pragma once\n#include <Arduino.h>\n\n' +
    '// AUTO-GENERATED from arduino/' + p.src + ' — do not edit by hand.\n' +
    'static const char ' + p.symbol + '[] PROGMEM = R"rawliteral(\n' +
    html + ')rawliteral";\n';
  fs.writeFileSync(path.join(arduino, p.out), body, 'utf8');
  console.log(p.out.padEnd(24), Buffer.byteLength(body, 'utf8'), 'bytes');
}
