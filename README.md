# 💡 Диммер LED Strip — `mihazzzold.espHome_Dimmer`

Универсальная прошивка для LED-лент: адресная WS2812B, одноцветная PWM и RGB без адресации.
В Home Assistant — `light` с яркостью, цветом, эффектами и расписанием прямо на устройстве.

## Матрица плат

| Плата | Wi-Fi | Zigbee |
|-------|:-----:|:------:|
| ESP32-C3 SuperMini | ✅ | ❌ |
| ESP32-C6 SuperMini | ✅ | 🐝 [`esp32c6-dimmer`](../../zigbee/esp32c6-dimmer/) |
| ESP32-H2 SuperMini | ❌ | 🐝 [`esp32h2-dimmer`](../../zigbee/esp32h2-dimmer/) |
| ESP8266 NodeMCU | ✅ | ❌ |

## Типы лент (`led_type`)

| Значение | Лента | Пины C6 | Пины C3 | Пины ESP8266 |
|----------|-------|---------|---------|-------------|
| `addressable` *(по умолч.)* | WS2812B (neopixelbus/RMT) | GPIO18 | GPIO5 | GPIO4 (D2) |
| `pwm_mono` | Одноцветная через MOSFET | GPIO18 | GPIO5 | GPIO4 (D2) |
| `pwm_rgb` | RGB без адресации (3 MOSFET) | R=GPIO18, G=GPIO19, B=GPIO20 | R=GPIO5, G=GPIO6, B=GPIO7 | R=GPIO4, G=GPIO12, B=GPIO14 |

> **C6 SuperMini:** GPIO10/GPIO11 не разведены физически. Для ленты используется GPIO18.

## Структура шаблона

```
led-strip.yaml.j2          ← точка входа: переменные + 4 include
_includes/
  runtime.yaml.j2          ← платформа + Wi-Fi + OTA (esp32 + esp8266)
  logic.yaml.j2            ← light (addressable/pwm_mono/pwm_rgb), schedule_check
  maintenance.yaml.j2      ← расписание (4×number + switch), OTA
  diagnostics.yaml.j2      ← uptime, RSSI, heap
```

## Параметры сборки

| Параметр | По умолчанию | Описание |
|----------|-------------|----------|
| `led_type` | `addressable` | Тип ленты: `addressable` / `pwm_mono` / `pwm_rgb` |
| `led_count` | `30` | Количество LED (только `addressable`) |
| `led_strip_pin` | из board_profile | Пин данных |
| `led_r_pin` / `led_g_pin` / `led_b_pin` | из board_profile | Пины RGB (только `pwm_rgb`) |
| `use_ha_time` | `true` | `false` = SNTP без Home Assistant |

## Сборка

```powershell
# Адресная лента на C6
py -3.13 scripts/flasher.py --local `
  -f firmwares-external/mihazzzold.espHome_Dimmer/led-strip.yaml.j2 `
  --board-profile esp32c6-supermini -a run --port COM5 -y

# Одноцветная PWM на C3
py -3.13 scripts/flasher.py --local `
  -f firmwares-external/mihazzzold.espHome_Dimmer/led-strip.yaml.j2 `
  --board-profile esp32c3-supermini -a run --port COM5 -y `
  --var led_type=pwm_mono

# RGB PWM на ESP8266
py -3.13 scripts/flasher.py --local `
  -f firmwares-external/mihazzzold.espHome_Dimmer/led-strip.yaml.j2 `
  --board-profile esp8266-nodemcu -a run --port COM5 -y `
  --var led_type=pwm_rgb
```

## OTA-каналы

`mihazzzold.espHome_Dimmer.{esp32c3,esp32c6,esp8266}`

## Zigbee-вариант

Полная реализация (не scaffold) в [`firmwares/zigbee/`](../../zigbee/):
- [`esp32c6-dimmer/`](../../zigbee/esp32c6-dimmer/) — C6 Router, Color Dimmable Light, HSV→RGB, кнопка паринга GPIO9
- [`esp32h2-dimmer/`](../../zigbee/esp32h2-dimmer/) — H2 End Device (Zigbee-only, нет Wi-Fi)
