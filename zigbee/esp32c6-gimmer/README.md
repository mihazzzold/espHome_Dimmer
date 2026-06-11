# ESP32-C6 Гиммер Zigbee — Color Dimmable Light

[![Status](https://img.shields.io/badge/status-ready-green.svg)](../README.md) [![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.2.2-blue.svg)](https://docs.espressif.com/projects/esp-idf/) [![Zigbee](https://img.shields.io/badge/Zigbee-esp--zigbee--lib%20%5E1.6.3-orange.svg)](https://github.com/espressif/esp-zigbee-sdk)

Zigbee-прошивка для **адресной LED-ленты WS2812B** на **ESP32-C6 SuperMini**.

Устройство регистрируется в ZHA / Zigbee2MQTT как **Color Dimmable Light**:
- Включение / выключение
- Яркость (0–100%)
- Цвет (Hue / Saturation)

> Wi-Fi версия с расписанием: [`firmwares/ci-overrides/mihazzzold.espHome_Gimmer/`](../../ci-overrides/mihazzzold.espHome_Gimmer/)  
> H2 вариант (Zigbee-only): [`firmwares/zigbee/esp32h2-gimmer/`](../esp32h2-gimmer/)

## Подключение

| Сигнал | GPIO | Описание |
|--------|------|----------|
| WS2812B DATA | **GPIO10** | DIN ленты через резистор 300–500 Ом |
| Кнопка паринга | **GPIO9** | Кнопка → GPIO9 + GND (удержание > 5 с = сброс сети) |
| 5V | внешний БП | Питание ленты (не от платы при > 5–10 LED) |
| GND | GND | Общий минус |

> **Количество LED:** по умолчанию 30 — измените `LED_COUNT` в `main.c`.

## Требования

- ESP-IDF v5.2.x
- ESP32-C6 SuperMini (или любая плата с ESP32-C6)
- WS2812B лента

## Установка ESP-IDF (Windows)

```powershell
winget install --id Espressif.EIM-CLI --exact --accept-package-agreements --accept-source-agreements --disable-interactivity
eim install --non-interactive true --idf-versions v5.2.2 --target esp32c6 --path "C:\Espressif" --install-all-prerequisites true --create-bat-activation-script true --cleanup true
```

## Сборка и прошивка

```powershell
cd firmwares/zigbee/esp32c6-gimmer
eim run "idf.py set-target esp32c6" v5.2.2
eim run "idf.py build" v5.2.2
eim run "idf.py -p COM5 flash monitor" v5.2.2
```

## Паринг с Home Assistant (ZHA)

1. Прошейте устройство.
2. В HA → **Настройки → Устройства и службы → ZHA → Добавить устройство**.
3. Нажмите кнопку GPIO9 > 5 с (или первый старт после прошивки — устройство само ищет сеть).
4. Устройство появится как **Color Light** с сущностями:
   - `light.<имя>` — управление (вкл/выкл, яркость, цвет, эффекты через HA)

## Расписание

В Zigbee-версии расписание настраивается **через Home Assistant Automations**, а не на устройстве.  
Для расписания прямо на МК — используйте Wi-Fi версию (ESPHome Гиммер).

## Архитектура кода

```
app_main()
  ├── nvs_flash_init()          — NVS для хранения Zigbee-конфига сети
  ├── led_strip_init_hw()       — RMT + WS2812B инициализация
  ├── pair_button_task()        — GPIO9: удержание 5 с = factory reset
  └── zigbee_task()
        ├── esp_zb_platform_config()   — Radio + Host
        ├── esp_zb_init()              — Router role
        ├── Color Dimmable Light EP    — HA profile, device_id=0x0102
        │     clusters: Basic, Identify, On/Off, Level Control, Color Control
        ├── esp_zb_core_action_handler_register(zb_attr_handler)
        └── esp_zb_main_loop_iteration()
              └── zb_attr_handler() → leds_apply() → RMT → WS2812B
```
