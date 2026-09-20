# Архитектурный дизайн: Интеграция тачпада Synaptics TM-P3125 в систему ErgoDox Wireless

- **Дата:** 2026-09-20
- **Статус:** Одобрено (В разработке)
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
  - `P0.17` (Pro Micro Pin `D2`) $\to$ `SDA` тачпада (шина `&i2c0`, 100 kHz).
  - `P0.20` (Pro Micro Pin `D3`) $\to$ `SCL` тачпада (шина `&i2c0`, 100 kHz).
  - `P0.22` (Pro Micro Pin `D4`) $\to$ `INT` тачпада (Active-Low, Open-Drain, прерывание GPIOTE).
  - `~RESET` не подключен (подтянут на плате тачпада к 3.3V).
- **Питание:** Li-Po аккумулятор 3.7V, подключенный к `B+` через ползунковый тумблер и к `B-` (GND).

---

## 3. Архитектура прошивки ZMK / Zephyr

### 3.1. Драйвер сенсора (`drivers/input/input_synaptics_tm_p3125.c`)
- Реализуется как Zephyr Input Device Driver (`DEVICE_DT_INST_DEFINE`).
- **Инициализация:**
  1. Настройка GPIO прерывания `P0.22` с флагами `GPIO_INT_EDGE_FALLING | GPIO_PULL_UP`.
  2. Программный I2C сброс сенсора: запись в регистр `0x0022` команды `0x0001` (Soft Reset).
  3. Ожидание спада на линии INT и вычитывание 60 байт ответа для подтверждения готовности.
  4. Перевод сенсора в режим PTP: отправка последовательности команды `0x22, 0x00, 0x34, 0x03, 0x23, 0x00, 0x04, 0x00, 0x04, 0x03`.
  5. Сброс буфера повторным чтением 60 байт.
- **Потоковая обработка пакетов:**
  1. Прерывание переводит задачу в Zephyr Work Queue.
  2. Вычитывается 60-байтный пакет по I2C (адрес `0x2C`).
  3. Если `buf[2] == 0x03` (Multi-touch packet):
     - **1 палец (Слот 0):** если `Tip Switch` активен, вычисляется относительное смещение $\Delta X$ и $\Delta Y$. Вызывается `input_report_rel(dev, INPUT_REL_X, dx, false)` и `input_report_rel(dev, INPUT_REL_Y, dy, false)`.
     - **Кликпад:** `buf[31] & 0x01` вызывает `input_report_key(dev, INPUT_BTN_LEFT, pressed, false)`.
     - **2 пальца (Слот 0 и 1):** вертикальное перемещение транслируется в `INPUT_REL_WHEEL` (скролл колесом мыши).
     - **Синхронизация:** `input_sync(dev)`.

### 3.2. Shield тачпада (`ergodox_touchpad`)
- Включает Devicetree overlay с активацией `&i2c0`, подключением драйвера `synaptics_tm_p3125` и связыванием его с `zmk,input-split`.
- Конфигурация:
  - `CONFIG_ZMK_SPLIT=y`
  - `CONFIG_ZMK_SPLIT_BLE=y`
  - `CONFIG_ZMK_POINTING=y`
  - `CONFIG_ZMK_SPLIT_POINTING_INPUT=y`

### 3.3. Модификация Центрального Донгла
- В файлах `ergodox_dongle.conf` и `ergodox_dongle_display.conf`:
  - `CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=3` (Левая половина, Правая половина, Тачпад).
  - `CONFIG_ZMK_POINTING=y` (Включение поддержки указателя мыши и дескрипторов мыши в USB HID).
- `build.yaml`:
  - Добавление таргета сборки:
    ```yaml
    - board: nice_nano_v2
      shield: ergodox_touchpad
    ```
