# Task 2 Brief: Keymap Menu Trigger Binding & 3-Way Sync

## Objectives
1. In `zmk-config/config/ergodox.keymap`:
   - On Layer 6 (`MOUS`), change position 1 (first row, next to `&kp ESC`) from `&trans` to `&kp F21`.
2. In `layout-viewer/data.js`:
   - On Layer 6 (`MOUS`, `"position": 6`), update key with `"idx": 1`:
     - `"tapCode": "KC_F21"`
     - `"tapText": "MENU"`
     - `"subText": "F21"`
     - `"customLabel": "MENU"`
     - `"keyType": "special"`
3. In `docs/layout/02_LAYERS.md`:
   - In Layer 6 (`MOUS`) description / table, document the Dongle Menu toggle key (`MENU` / `F21`) at position 1.
4. Run validation audit:
   - `node .agents/skills/firmware-critic/scripts/audit.js`
   - Must pass with 90/90 checks and 0 errors.
5. Commit changes:
   - In `zmk-config`: commit `feat(keymap): bind dongle menu toggle on MOUS layer`
   - In root repo: commit `docs: update layout viewer and 02_LAYERS.md for dongle menu key`

## Report File
Write full execution details to `.superpowers/sdd/2026-10-13-dongle-cyber-hud-menu/task-2-report.md`.
Return only: status (DONE), commit hashes, test summary.
