# ErgoDox Wireless AI Agents & Skills

Этот репозиторий содержит специализированные конфигурации и агентов для разработки прошивки и раскладки клавиатуры ErgoDox Wireless (ZMK Firmware).

## Доступные навыки и агенты

### 1. `firmware-critic` (`.agents/skills/firmware-critic/SKILL.md`)
- **Назначение**: Строгий критик и аудитор прошивки ZMK. Сверяет реализацию `zmk-config/config/ergodox.keymap` и `ergodox.conf` с документацией (`docs/layout/`) и интерактивным просмотрщиком (`layout-viewer/data.js`).
- **Автоматический скрипт валидации**: 
  ```bash
  node .agents/skills/firmware-critic/scripts/audit.js
  ```
- **Проверяемые инварианты**:
  - 14 слоев, строго по 76 клавиш на слой (1064 биндинга).
  - Home Row Mods на WORK (0) и перенесенные HRM на сопутствующих слоях.
  - Писательский режим TYPE (3) с прозрачными `&trans` и без HRM.
  - Языковые хоткеи SYM1 (4): `Ctrl+1` (EN), `Ctrl+2` (UA), `Ctrl+3` (RU).
  - Геймерский WASD GAME (8): 0 ms задержки, отсутствие hold-tap.
  - Функциональные блоки FUN1 (11) и FUN2 (12): сетка 3x4 снизу вверх (как нампад).
  - Беспроводное управление SYM2 (13): Dongle (`BT_SEL 0`), Bluetooth (`BT_SEL 1..4`), тумблер `OUT_TOG`, сброс `BT_CLR`.
  - **Безопасность**: Полное отсутствие программных бутлоадеров (`&bootloader` / `&sys_reset` удалены, сброс только физической кнопкой).

## Правила репозитория (`.agents/rules/`)
- `ergodox_sync.md`: Обязательная 100% взаимная синхронизация между `layout-viewer/data.js`, `ergodox.keymap` и `docs/layout/02_LAYERS.md` при любых изменениях раскладки.
