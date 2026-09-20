# План реализации: Интеграция тачпада Synaptics TM-P3125 в ZMK ErgoDox Wireless

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Реализовать поддержку и прошивку для автономного беспроводного модуля тачпада Synaptics TM-P3125 на контроллере nice!nano v2, работающего в единой BLE сплит-сети с центральным донглом и половинками ErgoDox.

**Architecture:** Написание драйвера Zephyr Input Subsystem под шину I2C и прерывания GPIOTE, создание сплит-щита `ergodox_touchpad` с трансляцией событий мыши через `zmk,input-split`, и обновление конфигурации центрального донгла до 3 периферий с активацией `CONFIG_ZMK_POINTING`.

**Tech Stack:** C, Zephyr RTOS, ZMK Firmware, Devicetree (DTS), Kconfig, CMake.

**Spec:** [docs/superpowers/specs/2026-09-20-touchpad-integration-design.md](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/docs/superpowers/specs/2026-09-20-touchpad-integration-design.md)

---

## Задачи

### Задача 1: Модернизация конфигурации Центрального Донгла
- [ ] Обновить `zmk-config/boards/shields/ergodox/ergodox_dongle.conf` (`CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=3`, `CONFIG_ZMK_POINTING=y`).
- [ ] Обновить `zmk-config/boards/shields/ergodox/ergodox_dongle_display.conf` (`CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=3`, `CONFIG_ZMK_POINTING=y`).

### Задача 2: Разработка драйвера ввода Zephyr для Synaptics TM-P3125
- [ ] Создать Devicetree binding `zmk-config/dts/bindings/input/synaptics,tm-p3125.yaml`.
- [ ] Реализовать C-драйвер `zmk-config/drivers/input/input_synaptics_tm_p3125.c`.
- [ ] Настроить билд-систему Zephyr (`CMakeLists.txt`, `Kconfig`, `zephyr/module.yml`).

### Задача 3: Создание ZMK Shield `ergodox_touchpad`
- [ ] Зарегистрировать щит в `zmk-config/boards/shields/ergodox/Kconfig.shield` и `Kconfig.defconfig`.
- [ ] Создать Devicetree оверлей `zmk-config/boards/shields/ergodox/ergodox_touchpad.overlay` (шина I2C, пины P0.17, P0.20, P0.22, узел `zmk,input-split`).
- [ ] Создать конфигурационный файл `zmk-config/boards/shields/ergodox/ergodox_touchpad.conf`.

### Задача 4: Интеграция в матрицу сборки CI и валидация
- [ ] Добавить таргет `ergodox_touchpad` в `zmk-config/build.yaml`.
- [ ] Запустить локальный аудит целостности прошивки и синтаксиса.
