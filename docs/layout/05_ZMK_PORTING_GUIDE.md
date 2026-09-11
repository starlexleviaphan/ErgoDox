# 05. Руководство по портированию концепции на ZMK

[← Перевод авторского тура](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/docs/layout/04_TOUR_TRANSLATION.md) | [Оглавление](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/docs/layout/00_INDEX.md)

---

## 🌉 Сравнение архитектур: QMK/Oryx против ZMK

Перенос раскладки из Oryx (QMK) в ZMK для ErgoDox требует понимания различий между движками прошивок. ZMK построен на базе RTOS Zephyr, использует DeviceTree (`.keymap`) и Kconfig (`.conf`), и обладает чрезвычайно мощной и гибкой декларативной системой поведений (**behaviors**).

### Таблица прямого соответствия механик

| Механика в Oryx / QMK | Реализация в ZMK | Примечания и особенности |
| :--- | :--- | :--- |
| `LT(layer, key)` | `&lt layer key` | Нативное поведение Layer-Tap в ZMK. |
| `MT(mod, key)` | `&mt mod key` | Нативное поведение Mod-Tap. Настраивается через кастомные `hold-tap`. |
| `OSL(layer)` | `&sl layer` | Sticky Layer (слой активен на одно нажатие). |
| `OSM(mod)` | `&sk mod` | Sticky Key (модификатор активен на один символ). |
| `TG(layer)` | `&tog layer` | Toggle Layer (включение/выключение слоя по тапу). |
| `TO(layer)` | `&to layer` | Прямое переключение базового слоя. |
| `QK_LLCK` (Layer Lock) | Кастомный макрос / `&tog` | В ZMK аналог — переход в слой по `&tog` или использование Sticky Layer. |
| `CHORDAL_HOLD` | `hold-trigger-key-positions` | **Позиционный Hold-Tap**. В ZMK это реализовано даже чище и надёжнее, чем в QMK! |
| `QUICK_TAP_TERM = 0` | `quick-tap-ms = <0>;` | Свойство кастомного `hold-tap` в ZMK. |
| `PERMISSIVE_HOLD` | `flavor = "balanced";` или `hold-preferred` | `balanced` в ZMK ведет себя точно как `PERMISSIVE_HOLD` + `CHORDAL_HOLD`. |
| `AutoShift` | Кастомный `hold-tap` `&as` | В ZMK AutoShift настраивается как генератор заглавных букв при удержании. |
| `Tap Dance` | `compatible = "zmk,behavior-tap-dance";` | Нативная поддержка в блоке `behaviors`. |
| `Mouse Keys` | `&mmv`, `&msc`, `&mkp` | Требует `CONFIG_ZMK_MOUSE=y` в `ergodox.conf`. |
| `QK_BOOT` (Bootloader) | `&bootloader` | Полный эквивалент. Уже настроен в ваших комбо! |

---

## 1. Позиционный Hold-Tap: Реализация Chordal Hold в ZMK

В QMK `CHORDAL_HOLD` требовал C-кода и отдельной матрицы букв `'L'` и `'R'`.  
В ZMK этот функционал реализуется элегантно через директиву `hold-trigger-key-positions`:

```dts
/ {
    behaviors {
        // Модификатор левой руки: срабатывает как hold ТОЛЬКО если нажимается вместе с клавишей правой руки!
        hml: home_row_mods_left {
            compatible = "zmk,behavior-hold-tap";
            #binding-cells = <2>;
            flavor = "balanced";
            tapping-term-ms = <190>;
            quick-tap-ms = <0>;
            require-prior-idle-ms = <150>;
            bindings = <&kp>, <&kp>;
            // Индексы всех клавиш правой половины клавиатуры:
            hold-trigger-key-positions = <
                7 8 9 10 11 12 13
                21 22 23 24 25 26 27
                34 35 36 37 38 39
                46 47 48 49 50 51
                56 57 58 59 60
                67 68 69 70 71 72 73 74 75
            >;
        };

        // Модификатор правой руки: срабатывает ТОЛЬКО с клавишами левой половины:
        hmr: home_row_mods_right {
            compatible = "zmk,behavior-hold-tap";
            #binding-cells = <2>;
            flavor = "balanced";
            tapping-term-ms = <190>;
            quick-tap-ms = <0>;
            require-prior-idle-ms = <150>;
            bindings = <&kp>, <&kp>;
            // Индексы всех клавиш левой половины:
            hold-trigger-key-positions = <
                0 1 2 3 4 5 6
                14 15 16 17 18 19 20
                28 29 30 31 32 33
                40 41 42 43 44 45
                52 53 54 55
                61 62 63 64 65 66
            >;
        };
    };
};
```

---

## 2. Индивидуальный Tapping Term для GUI / Win клавиш

В QMK Trevor использовал `g_tapping_term - 40` (150 мс) для клавиш GUI (G и K).  
В ZMK мы просто объявляем специализированное поведение `hml_gui` и `hmr_gui` с `tapping-term-ms = <150>;`:

```dts
hml_gui: home_row_gui_left {
    compatible = "zmk,behavior-hold-tap";
    #binding-cells = <2>;
    flavor = "balanced";
    tapping-term-ms = <150>; // Укороченный таймаут для исключения случайных открытий меню Пуск
    quick-tap-ms = <0>;
    bindings = <&kp>, <&kp>;
    hold-trigger-key-positions = < /* позиции правой руки */ >;
};
```

---

## 3. Реализация AutoShift в ZMK

В ZMK функция AutoShift строится на базе behavior `hold-tap`:

```dts
as: auto_shift {
    compatible = "zmk,behavior-hold-tap";
    #binding-cells = <2>;
    flavor = "tap-preferred";
    tapping-term-ms = <120>; // Таймаут 120 мс в точности как у Trevor
    quick-tap-ms = <0>;
    bindings = <&as_shift>, <&kp>;
};
```
На слое `TYPE` буквенные клавиши вызываются через `&as LS(A) A`, `&as LS(B) B` и т.д.  
Короткое нажатие выдает строчную `a`, зажатие на 120 мс выдает заглавную `A`.

---

## 4. Мышиный слой (Mouse Keys) в ZMK

Для работы слоя `MOUS` в ZMK необходимо активировать модуль мыши в файле конфигурации [ergodox.conf](file:///c:/Users/Starlex/Documents/GitHub/ErgoDox/zmk-config/config/ergodox.conf):

```properties
CONFIG_ZMK_MOUSE=y
```

И подключить заголовки в `ergodox.keymap`:
```dts
#include <dt-bindings/zmk/mouse.h>
```

Соответствие кодов:
- Клик ЛКМ: `&mkp LCLK`
- Клик ПКМ: `&mkp RCLK`
- Средний клик (СКМ): `&mkp MCLK`
- Боковые кнопки: `&mkp MB4`, `&mkp MB5`
- Движение курсора: `&mmv MOVE_UP`, `&mmv MOVE_DOWN`, `&mmv MOVE_LEFT`, `&mmv MOVE_RIGHT`
- Скроллинг: `&msc SCRL_UP`, `&msc SCRL_DOWN`, `&msc SCRL_LEFT`, `&msc SCRL_RIGHT`

---

## 5. Динамические макросы против Статических макросов ZMK

В QMK динамические макросы (`DM_REC1`, `DM_PLY1`) записывают нажатия клавиш прямо в оперативную память микроконтроллера на лету.  
В стандартном апстриме ZMK память защищена, и динамическая запись макросов «с клавиатуры» без компиляции не поддерживается в ядре (существуют экспериментальные пользовательские форки, но они снижают стабильность Bluetooth-стека).

### Какая альтернатива в ZMK?
1. **Статические макросы ZMK**: любые частые последовательности (пароли, email, шаблонный код, заготовки) описываются в блоке `macros` и привязываются к клавишам.
2. **Комбинации (Combos)**: мгновенный вызов нужных макросов при одновременном нажатии двух клавиш.
3. **ZMK Studio**: в вашем `ergodox.conf` уже включен `CONFIG_ZMK_STUDIO=y`, что позволяет менять назначения клавиш прямо через веб-интерфейс в браузере по USB без перекомпиляции!

---

## 6. Клавиша аварийного сброса FUNC OFF

В ZMK поведение `&to 0` полностью деактивирует все вышестоящие слои и возвращает клавиатуру на Слой 0 (`WORK`).  
Размещение `&to 0` на физической позиции Key 32 на всех вторичных слоях в точности воспроизведет поведение `FUNC_OFF`.
