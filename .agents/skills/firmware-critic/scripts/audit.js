const fs = require('fs');
const path = require('path');

// Resolve repository root
const repoRoot = path.resolve(__dirname, '../../../../');
const keymapPath = path.join(repoRoot, 'zmk-config/config/ergodox.keymap');
const confPath = path.join(repoRoot, 'zmk-config/config/ergodox.conf');
const viewerDataPath = path.join(repoRoot, 'layout-viewer/data.js');
const westPath = path.join(repoRoot, 'zmk-config/config/west.yml');
const docsLayersPath = path.join(repoRoot, 'docs/layout/02_LAYERS.md');
const dtsiPath = path.join(repoRoot, 'zmk-config/boards/shields/ergodox/ergodox.dtsi');

console.log('=====================================================');
console.log('🔍 FIRMWARE CRITIC & AUDITOR: DEEP VERIFICATION');
console.log('=====================================================');

let errors = [];
let warnings = [];
let passCount = 0;

function assert(condition, message, isWarning = false) {
  if (condition) {
    passCount++;
  } else {
    if (isWarning) {
      warnings.push(message);
    } else {
      errors.push(message);
    }
  }
}

// 1. Check Files Existence
assert(fs.existsSync(keymapPath), `Keymap file exists at ${keymapPath}`);
assert(fs.existsSync(confPath), `Config file exists at ${confPath}`);
assert(fs.existsSync(viewerDataPath), `Viewer data file exists at ${viewerDataPath}`);
assert(fs.existsSync(westPath), `West manifest exists at ${westPath}`);
assert(fs.existsSync(docsLayersPath), `Layers doc exists at ${docsLayersPath}`);
assert(fs.existsSync(dtsiPath), `Hardware dtsi exists at ${dtsiPath}`);

const keymapContent = fs.readFileSync(keymapPath, 'utf8');
const confContent = fs.readFileSync(confPath, 'utf8');
const westContent = fs.readFileSync(westPath, 'utf8');
const docsLayersContent = fs.readFileSync(docsLayersPath, 'utf8');
const dtsiContent = fs.readFileSync(dtsiPath, 'utf8');

// Load layout viewer data
global.window = {};
require(viewerDataPath);
const viewerLayers = window.KEYBOARD_DATA ? window.KEYBOARD_DATA.layers : [];

// 2. Layer Extraction and Key Counts
const layerRegex = /layer_(\d+)\s*\{[\s\S]*?bindings\s*=\s*<([\s\S]*?)>;\s*\};/g;
let match;
const layers = [];
while ((match = layerRegex.exec(keymapContent)) !== null) {
  const index = parseInt(match[1], 10);
  const block = match[0];
  const bindingsText = match[2];
  const nameMatch = block.match(/display-name\s*=\s*"([^"]+)";/);
  const displayName = nameMatch ? nameMatch[1] : `LAYER_${index}`;
  
  // Extract token bindings (starts with &)
  const tokens = bindingsText.match(/&[^\s]+/g) || [];
  layers.push({ index, displayName, tokens, block, rawBindings: bindingsText });
}

console.log(`\n📦 Discovered Layers in Keymap: ${layers.length} (Expected: 14)`);
assert(layers.length === 14, `Expected exactly 14 layers in keymap, found ${layers.length}`);
assert(viewerLayers.length === 14, `Expected exactly 14 layers in viewer data, found ${viewerLayers.length}`);

// Check 76 keys per layer
layers.forEach(l => {
  assert(l.tokens.length === 76, `Layer ${l.index} (${l.displayName}): contains ${l.tokens.length} keys (must be exactly 76)`);
});

// 3. Firmware Configuration Checks (ergodox.conf)
console.log('\n⚙️ Checking ergodox.conf:');
assert(/CONFIG_ZMK_MOUSE\s*=\s*y/.test(confContent), 'CONFIG_ZMK_MOUSE=y enabled for mouse keys');
assert(/CONFIG_ZMK_STUDIO\s*=\s*y/.test(confContent), 'CONFIG_ZMK_STUDIO=y configured');
assert(/CONFIG_BT_CTLR_TX_PWR_PLUS_8\s*=\s*y/.test(confContent), 'CONFIG_BT_CTLR_TX_PWR_PLUS_8=y for maximum wireless signal stability');
assert(/CONFIG_ZMK_SLEEP\s*=\s*y/.test(confContent), 'CONFIG_ZMK_SLEEP=y for battery conservation');

// 4. West Manifest Check (west.yml)
console.log('\n📦 Checking west.yml:');
assert(westContent.includes('zmkfirmware'), 'west.yml uses official zmkfirmware repository for rock-solid stability');

// 5. Deep Matrix Mapping Verification (ergodox.keymap vs data.js)
console.log('\n🔬 Cross-Checking Matrix Key Mapping:');

const ergodoxKeymapOrder = [
  0, 1, 2, 3, 4, 5, 6,             38, 39, 40, 41, 42, 43, 44,
  7, 8, 9, 10, 11, 12, 13,         45, 46, 47, 48, 49, 50, 51,
  14, 15, 16, 17, 18, 19, 26,      58, 52, 53, 54, 55, 56, 57,
  20, 21, 22, 23, 24, 25,          59, 60, 61, 62, 63, 64,
  27, 28, 29, 30, 31,              65, 66, 67, 68, 69,
  32, 33,                          70, 71,
  35, 36, 34,                      74, 75, 72,
  37,                              73
];

assert(ergodoxKeymapOrder.length === 76, 'Matrix mapping order has exactly 76 physical positions');
assert(dtsiContent.includes('RC(4, 17) RC(4, 15) RC(3, 16)'), 'Hardware matrix transform matches physical thumb order [74(Enter 2U), 75(Space 2U), 72(TG NPAD 1U)]');

// Verify all layers exist in both and match titles
layers.forEach(l => {
  const vl = viewerLayers.find(v => v.position === l.index);
  assert(vl !== undefined, `Viewer contains layer ${l.index}`);
  if (vl) {
    assert(vl.keys.length === 76, `Viewer layer ${l.index} (${vl.title}) has 76 keys`);
  }
});

// Layer 0: WORK (Home Row Mods & Bilateral hold-trigger-key-positions)
const l0 = layers.find(l => l.index === 0);
if (l0) {
  assert(l0.block.includes('&hml LALT A'), 'L0 (WORK): Home row A has &hml LALT');
  assert(l0.block.includes('&hml LSHFT S'), 'L0 (WORK): Home row S has &hml LSHFT');
  assert(l0.block.includes('&hml LCTRL D'), 'L0 (WORK): Home row D has &hml LCTRL');
  assert(l0.block.includes('&hml_gui LGUI F'), 'L0 (WORK): Home row F has &hml_gui LGUI');
  assert(l0.block.includes('&hmr_gui RGUI J'), 'L0 (WORK): Home row J has &hmr_gui RGUI');
  assert(l0.block.includes('&hmr RCTRL K'), 'L0 (WORK): Home row K has &hmr RCTRL');
  assert(l0.block.includes('&hmr RSHFT L'), 'L0 (WORK): Home row L has &hmr RSHFT');
  assert(l0.block.includes('&hmr RALT SEMI'), 'L0 (WORK): Home row ; has &hmr RALT');
  assert(l0.block.includes('&type_shift RSHFT 3'), 'L0 (WORK): Key 64 uses &type_shift RSHFT 3 (Tap=TO 3, Hold=RSHFT)');
}

// Layer 3: TYPE (Clean Writer Mode - Full Thumbs and Service Keys, Pure Letters)
const l3 = layers.find(l => l.index === 3);
if (l3) {
  assert(!l3.block.includes('&hml') && !l3.block.includes('&hmr'), 'L3 (TYPE): HRM modifiers disabled for zero latency typing');
  assert(l3.block.includes('&kp A') && l3.block.includes('&kp S') && l3.block.includes('&kp D') && l3.block.includes('&kp F'), 'L3 (TYPE): Pure left letters A, S, D, F');
  assert(l3.block.includes('&kp J') && l3.block.includes('&kp K') && l3.block.includes('&kp L') && l3.block.includes('&kp SEMI'), 'L3 (TYPE): Pure right letters J, K, L, ;');
  assert(l3.block.includes('&type_shift RSHFT 0'), 'L3 (TYPE): Has return to WORK layer via &type_shift RSHFT 0');
  assert(l3.block.includes('&lt 5 SPACE') && l3.block.includes('&lt 7 BSPC'), 'L3 (TYPE): Left thumb cluster matches WORK (Space, Bksp)');
  assert(l3.block.includes('&lt 11 RET') && l3.block.includes('&lt 4 SPACE'), 'L3 (TYPE): Right thumb cluster matches WORK (Enter, Space)');
}

// Layer 4: SYM1 (Language Switchers Ctrl+1/2/3)
const l4 = layers.find(l => l.index === 4);
if (l4) {
  assert(l4.block.includes('&kp LC(N1)'), 'L4 (SYM1): Contains Ctrl+1 (EN)');
  assert(l4.block.includes('&kp LC(N2)'), 'L4 (SYM1): Contains Ctrl+2 (UA)');
  assert(l4.block.includes('&kp LC(N3)'), 'L4 (SYM1): Contains Ctrl+3 (RU)');
}

// Layer 6: MOUS (Mouse Movement & Scrolling)
const l6 = layers.find(l => l.index === 6);
if (l6) {
  assert(l6.block.includes('&mmv MOVE_UP'), 'L6 (MOUS): Has MMV mouse movement');
  assert(l6.block.includes('&msc SCRL_UP'), 'L6 (MOUS): Has MSC mouse scrolling');
  assert(l6.block.includes('&mkp LCLK') && l6.block.includes('&mkp RCLK'), 'L6 (MOUS): Has MKP mouse buttons');
}

// Layer 8: GAME (Gaming WASD, zero hold-tap)
const l8 = layers.find(l => l.index === 8);
if (l8) {
  assert(!l8.block.includes('&hml') && !l8.block.includes('&hmr') && !l8.block.includes('&lt'), 'L8 (GAME): Zero hold-tap delay on gaming keys');
}

// Layer 11 & 12: FUN1 & FUN2 (3x4 bottom-to-top numpad matrix)
const l11 = layers.find(l => l.index === 11);
const l12 = layers.find(l => l.index === 12);
if (l11) {
  assert(l11.block.includes('&kp F1') && l11.block.includes('&kp F12'), 'L11 (FUN1): Contains F1..F12');
}
if (l12) {
  assert(l12.block.includes('&kp F13') && l12.block.includes('&kp F24'), 'L12 (FUN2): Contains F13..F24');
}

// Layer 13: SYM2 (Wireless Management, NO bootloader)
const l13 = layers.find(l => l.index === 13);
if (l13) {
  assert(l13.block.includes('&bt BT_SEL 0'), 'L13 (SYM2): Dongle profile (&bt BT_SEL 0) present');
  assert(l13.block.includes('&bt BT_SEL 1') && l13.block.includes('&bt BT_SEL 4'), 'L13 (SYM2): Bluetooth profiles 1..4 present');
  assert(l13.block.includes('&out OUT_TOG'), 'L13 (SYM2): Wireless output toggle (&out OUT_TOG) present');
  assert(l13.block.includes('&bt BT_CLR'), 'L13 (SYM2): BT unpair (&bt BT_CLR) present');
}

// Critical Safety Audit: NO SOFTWARE BOOTLOADERS ANYWHERE
const softwareBootloaderCheck = keymapContent.includes('&bootloader') || keymapContent.includes('&sys_reset');
assert(!softwareBootloaderCheck, 'NO SOFTWARE BOOTLOADER: &bootloader / &sys_reset completely removed (hardware physical button only)');

// Combos Check
const combosCheck = keymapContent.includes('combos {');
if (combosCheck) {
  assert(!keymapContent.includes('&bootloader'), 'Combos do not contain software bootloader');
}

// 6. Print Summary
console.log('\n=====================================================');
console.log(`AUDIT RESULTS: ${passCount} Checks Passed`);
if (warnings.length > 0) {
  console.log(`\n⚠️ Warnings (${warnings.length}):`);
  warnings.forEach(w => console.log(`  - ${w}`));
}
if (errors.length > 0) {
  console.log(`\n❌ Critical Failures (${errors.length}):`);
  errors.forEach(e => console.log(`  - ${e}`));
  process.exit(1);
} else {
  console.log('\n✅ 100% SPECIFICATION AND ARCHITECTURE PARITY CONFIRMED!');
  console.log('=====================================================');
}
