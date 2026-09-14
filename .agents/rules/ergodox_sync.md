# ErgoDox Wireless Layout Synchronization Rule

Whenever the user requests any change, tweak, or addition to the keyboard layout:

## Mandatory Consistency Enforcement
The AI assistant must always ensure 100% mutual consistency between:
1. **Interactive Visualizer Data**: `layout-viewer/data.js`
2. **ZMK Firmware Keymap**: `zmk-config/config/ergodox.keymap`
3. **Architecture Documentation**: `docs/layout/02_LAYERS.md`

## Architecture Invariants
- **14 Layers**:
  - `0: WORK`, `1: SWAP`, `2: LNPD`, `3: TYPE`, `4: SYM1`, `5: NRPD`, `6: MOUS`, `7: ARRW`, `8: GAME`, `9: GSWP`, `10: NPAD`, `11: FUN1`, `12: FUN2`, `13: SYM2`.
- **76 Keys Per Layer**:
  - Matrix binding order follows `ergodox.dtsi` `default_transform` (76 bindings per layer).
- **Home Row Mods on WORK (0)**:
  - Left hand: `Alt (A)`, `Shift (S)`, `Ctrl (D)`, `Win (F)` using positional `&hml` / `&hml_gui` (triggered only by right-hand chords, 190ms / 150ms Win).
  - Right hand: `Win (J)`, `Ctrl (K)`, `Shift (L)`, `Alt (;)` using positional `&hmr` / `&hmr_gui` (triggered only by left-hand chords, 190ms / 150ms Win).
- **100% Wireless Dongle & BLE**:
  - Key 1 (`&bt BT_SEL 0`): `Dongle` (PC 2.4G).
  - Keys 2..5 (`&bt BT_SEL 1..4`): `BT 1 .. BT 4` (Mobile devices).
  - Keys 0 and 73 (`&out OUT_TOG`): `Dgl/BT` Toggle.
  - Keys 6 and 38 (`&bt BT_CLR`): `BT Clr` Unpair.
- **Language Switchers on SYM1 (4)**:
  - `Ctrl+1` (`LC(N1)`): `EN` (English)
  - `Ctrl+2` (`LC(N2)`): `UA` (Ukrainian)
  - `Ctrl+3` (`LC(N3)`): `RU` (Russian)
  - Available on both Left hand (1..3) and Right hand (47..49).
- **Dynamic Macros (PR #2678)**:
  - `&dm DM_REC 0/1` and `&dm DM_PLY 0/1` for slots 1 and 2.
- **Bootloader**:
  - No software bootloader buttons or combos; hardware reset only.

## Automated Verification on Layout Edit
Whenever modifying any layout file:
1. Verify each layer has exactly 76 bindings.
2. Confirm there are no unmapped tokens (`NONE`, `OSM`, `undefined`).
3. Bump the cache buster in `layout-viewer/index.html` (e.g. `?v=N+1`).
4. Re-verify the updated layer visually using the browser subagent if relevant.
5. **Firmware Delivery**: Upon finishing layout modifications, trigger the build pipeline (`node .agents/skills/firmware-critic/scripts/build.js`) to ensure ready-to-flash `.uf2` binaries are downloaded directly into the repository's `firmware/` folder (`firmware/ergodox_dongle-nice_nano_v2-zmk.uf2`, etc.).
