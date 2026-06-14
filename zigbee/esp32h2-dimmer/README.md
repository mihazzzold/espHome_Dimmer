# ESP32-H2 Диммер Zigbee — Color Dimmable Light

[![Status](https://img.shields.io/badge/status-ready-green.svg)](../README.md) [![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.2.2-blue.svg)](https://docs.espressif.com/projects/esp-idf/) [![Zigbee](https://img.shields.io/badge/Zigbee-esp--zigbee--lib%20%5E1.6.3-orange.svg)](https://github.com/espressif/esp-zigbee-sdk)

Zigbee-прошивка для **адресной LED-ленты WS2812B** на **ESP32-H2 SuperMini**.

> ⚠️ **ESP32-H2 не имеет Wi-Fi 802.11.** Управление только через Zigbee.  
> Для Wi-Fi + расписание — используйте C3/C6/ESP32 с ESPHome Диммер.

## Подключение

| Сигнал | GPIO | Описание |
|--------|------|----------|
| WS2812B DATA | **GPIO12** | DIN ленты через резистор 300–500 Ом |
| Кнопка паринга | **GPIO9** | Кнопка → GPIO9 + GND (удержание > 5 с = сброс сети) |
| 5V | внешний БП | Питание ленты |
| GND | GND | Общий минус |

> **Количество LED:** по умолчанию 30 — измените `LED_COUNT` в `main.c`.

## Требования

- ESP-IDF v5.2.x с поддержкой esp32h2
- ESP32-H2 SuperMini

## Установка ESP-IDF (Windows)

```powershell
winget install --id Espressif.EIM-CLI --exact --accept-package-agreements --accept-source-agreements --disable-interactivity
eim install --non-interactive true --idf-versions v5.2.2 --target esp32h2 --path "C:\Espressif" --install-all-prerequisites true --create-bat-activation-script true --cleanup true
```

## Сборка и прошивка

```powershell
cd firmwares/zigbee/esp32h2-dimmer
eim run "idf.py set-target esp32h2" v5.2.2
eim run "idf.py build" v5.2.2
eim run "idf.py -p COM5 flash monitor" v5.2.2
```

## Отличия от C6-версии

| | ESP32-C6 | ESP32-H2 |
|--|--|--|
| Wi-Fi | ✅ 802.11ax | ❌ нет |
| Zigbee/BLE | ✅ | ✅ |
| LED DATA pin | GPIO10 | **GPIO12** |
| Zigbee роль | Router | Router |
| `xTaskCreate` | `PinnedToCore(1)` | обычный |
| Расписание | через HA или ESPHome Wi-Fi версию | только через HA |

## Связанные проекты

- Wi-Fi версия (ESPHome, расписание): [`firmwares/ci-overrides/mihazzzold.espHome_Dimmer/`](../../ci-overrides/mihazzzold.espHome_Dimmer/)
- C6 Zigbee версия: [`firmwares/zigbee/esp32c6-dimmer/`](../esp32c6-dimmer/)
