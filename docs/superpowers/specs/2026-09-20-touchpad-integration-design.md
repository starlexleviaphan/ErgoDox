# Архитектурный дизайн: Интеграция тачпада Synaptics TM-P3125 в систему ErgoDox Wireless

- **Дата:** 2026-09-20
- **Статус:** Реализовано (Оптимизировано в [`2026-10-16-touchpad-optimization-design.md`](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/docs/superpowers/specs/2026-10-16-touchpad-optimization-design.md))
- **Целевая платформа:** ZMK Firmware / Zephyr RTOS (nice!nano v2, nRF52840)

---

## 1. Введение и цели

Целью проекта является добавление автономного беспроводного модуля тачпада к существующей сплит-клавиатуре ErgoDox Wireless. 
Тачпад работает в связке с общим центральным донглом (Central Dongle) клавиатуры, не требуя отдельного USB-приемника или подключения к ПК по отдельному Bluetooth-профилю.

---

## 2. Аппаратная часть (Hardware Architecture)

- **Сенсор:** Synaptics TM-P3125 (I2C-HID, разрешение $X_{max}=1219, Y_{max}=919$, размер $97 \times 54$ мм).
- **Микроконтроллер:** `nice!nano v2` (или Pro Micro/SuperMini nRF52840).
- **Схема подключения:**
  - `VCC (3.3V)` $\to$ `3V3` тачпада.
  - `GND` $\to$ `GND` тачпада.
  - `P0.17` (Pro Micro Pin `D2`) $\to$ `SDA` тачпада (шина `&i2c0`, 400 kHz).
  - `P0.20` (Pro Micro Pin `D3`) $\to$ `SCL` тачпада (шина `&i2c0`, 400 kHz).
  - `P0.22` (Pro Micro Pin `D4`) $\to$ `INT` тачпада (Active-Low, Open-Drain, прерывание GPIOTE).
  - `~RESET` не подключен (подтянут на плате тачпада к 3.3V).
- **Питание:** Li-Po аккумулятор 3.7V, подключенный к `B+` через ползунковый тумблер и к `B-` (GND).

---

## 3. Архитектура прошивки ZMK / Zephyr

### 3.1. Драйвер сенсора (`drivers/input/input_synaptics_tm_p3125.c`)
- Реализуется как Zephyr Input Device Driver (`DEVICE_DT_INST_DEFINE`). Подробная спецификация конвейера и жестов приведена в [`2026-10-16-touchpad-optimization-design.md`](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/docs/superpowers/specs/2026-10-16-touchpad-optimization-design.md).
- **Инициализация:**
  1. Настройка GPIO прерывания `P0.22` с флагами `GPIO_INT_EDGE_FALLING | GPIO_PULL_UP`.
  2. Программный I2C сброс сенсора: запись команды Power ON (`0x0005` $\to$ `0x0800`) и Soft Reset (`0x0005` $\to$ `0x0100`).
  3. Вычитывание 32-байтных пакетов для очистки буфера готовности.
  4. Перевод сенсора в режим Windows Precision Touchpad (PTP): запись Feature Report `0x04` (`SET_REPORT` в `0x0005` с данными `0x06, 0x00, 0x04, 0x03`).
  5. Фоновый watchdog (`heartbeat_work` каждые 15 с) для поддержания режима PTP при кратковременных просадках питания.
- **Конвейер чтения I2C и 8 мс BLE-коалесцер (`SYNAPTICS_BLE_FRAME_MS = 8`):**
  1. Прерывание `INT` немедленно пробуждает `synaptics_work_handler` через `k_work_reschedule(&data->work, K_NO_WAIT)` (0 Гц опроса в простое).
  2. За один проход вычитывается до `SYNAPTICS_MAX_PACKETS_PER_PASS = 6` пакетов точного размера **32 байта** (`SYNAPTICS_PTP_REPORT_LEN = 32`, адрес `0x2C`), что сокращает время блокировки I2C-шины в 2 раза.
  3. Относительные смещения курсора (`pend_dx`, `pend_dy`) и скролла (`pend_wheel`, `pend_hwheel`) агрегируются и отправляются в `zmk,input-split` единым фреймом раз в **8 мс** (или принудительно перед кликом/отрывом пальцев), предотвращая переполнение очередей BLE GATT (`CONFIG_BT_L2CAP_TX_BUF_COUNT=16`).
- **Фильтрация и полный набор жестов (1F / 2F / 3F):**
  - **Аппаратная фильтрация и Palm Rejection:** валидация слотов по битам `Tip Switch | Confidence` (`(status & 0x03) == 0x03`), подавление касаний ладонью по краям ($X < 70$ или $X > 1150$), перепривязка координат после паузы $> 45$ мс и защитное окно `20 мс` при переходе `1F -> 2F`.
  - **1 палец (Слот 0):** 2-tap EMA сглаживание микродрожания на малых скоростях ($v \le 6$), 4-ступенчатая кусочно-линейная баллистика скорости ($0.75\times, 1.0\times, 1.5\times, 2.0\times$) с Q4 субпиксельным аккумулятором остатка, одиночный тап (`INPUT_BTN_LEFT`), Double-Tap & Drag и `180 мс` Drag-Lock continuation при перехвате пальца.
  - **Физический кликпад (`buf[31] & 0x01`):** зональный клик — нижний правый угол ($X > 650, Y > 650$) или клик 2 пальцами эмулируют `INPUT_BTN_RIGHT`, клик 3 пальцами — `INPUT_BTN_MIDDLE`, остальная область — `INPUT_BTN_LEFT`.
  - **2 пальца (Слоты 0 и 1):** тап двумя пальцами с анти-джиттер мертвой зоной $\to$ `INPUT_BTN_RIGHT`; натуральный вертикальный и горизонтальный скролл (`INPUT_REL_WHEEL` / `INPUT_REL_HWHEEL` с доминантной блокировкой оси); жест Pinch-to-Zoom (`INPUT_BTN_5` $\to$ `Ctrl+Plus`, `INPUT_BTN_6` $\to$ `Ctrl+Minus`).
  - **3 пальца (Слоты 0, 1 и 2):** тап тремя пальцами $\to$ `INPUT_BTN_MIDDLE`; вертикальный свайп вверх $\to$ `INPUT_BTN_3` (`Win+Tab` Task View); вертикальный свайп вниз $\to$ `INPUT_BTN_4` (`Win+D` Show Desktop).

### 3.2. Shield тачпада (`ergodox_touchpad`)
- Включает Devicetree overlay с активацией `&i2c0`, подключением драйвера `synaptics_tm_p3125` и связыванием его с `zmk,input-split`.
- Конфигурация:
  - `CONFIG_ZMK_SPLIT=y`
  - `CONFIG_ZMK_SPLIT_BLE=y`
  - `CONFIG_ZMK_POINTING=y`
  - `CONFIG_ZMK_SPLIT_POINTING_INPUT=y`
  - `CONFIG_BT_L2CAP_TX_BUF_COUNT=16`

### 3.3. Модификация Центрального Донгла
- В файлах `ergodox_dongle.conf` и `ergodox_dongle_display.conf`:
  - `CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=3` (Левая половина, Правая половина, Тачпад).
  - `CONFIG_ZMK_POINTING=y` (Включение поддержки указателя мыши и дескрипторов мыши в USB HID).
- В `ergodox_dongle.overlay` и `ergodox_dongle_display.overlay`:
  - Маппинг `&zip_button_behaviors` для `INPUT_BTN_0..6` и `INPUT_BTN_LEFT/RIGHT/MIDDLE` (включая `LG(TAB)`, `LG(D)`, `LC(EQUAL)`, `LC(MINUS)` и `MCLK`).
- `build.yaml`:
  - Добавление таргета сборки:
    ```yaml
    - board: nice_nano_v2
      shield: ergodox_touchpad
    ```
