# espHome Гиммер — адресная LED-лента WS2812B

ESPHome-прошивка для управления адресной LED-лентой **WS2812B** с расписанием.

## Поддерживаемые платы

| Плата | DATA pin | Фреймворк |
|-------|----------|-----------|
| ESP32 WROOM-32 | GPIO13 | arduino |
| ESP32-C3 SuperMini | GPIO5 | arduino |
| ESP32-C6 SuperMini | GPIO10 | arduino |
| ESP8266 NodeMCU | GPIO4 (D2) | arduino |

> Zigbee-версии (ESP32-C6 / ESP32-H2) — в репозитории [esphomeFlasher](https://github.com/mihazzzold/esphomeFlasher/tree/main/firmwares/zigbee).

## Подключение

```
WS2812B DIN → пин из таблицы выше (через резистор 300–500 Ом)
GND         → GND платы
5V          → внешний БП (не питать ленту > 5 LED от платы)
```

## Параметры сборки (led-strip.yaml.j2)

| Параметр | По умолчанию | Описание |
|----------|-------------|----------|
| `led_count` | 30 | Количество светодиодов |
| `led_strip_pin` | из board_profile | Переопределить пин |
| `use_ha_time` | true | false = SNTP (автономно без HA) |
| `serial_number` | — | Серийный номер устройства |
| `friendly_name_override` | — | Имя в Home Assistant |

## Сущности в Home Assistant

| Сущность | Тип | Описание |
|----------|-----|----------|
| `light.led_лента` | light | Управление (вкл/выкл, яркость, цвет, эффекты) |
| `switch.расписание_активно` | switch | Включить/выключить расписание |
| `number.расписание_включение_ч` | number | Час включения (0–23) |
| `number.расписание_включение_мин` | number | Минута включения (0–59) |
| `number.расписание_выключение_ч` | number | Час выключения (0–23) |
| `number.расписание_выключение_мин` | number | Минута выключения (0–59) |

Расписание работает через полночь: например, 22:00 → 07:00.

## Эффекты

- Радуга (Rainbow)
- Бегущий огонь (Color Wipe)
- Мерцание (Twinkle)
- Бегающий огонёк (Scanner)
- Фейерверк (Fireworks)
- Пульсация (Pulse)

## Быстрый старт

```bash
# Установить ESPHome
pip install esphome

# Скопировать и заполнить secrets.yaml
cp secrets.yaml.example secrets.yaml  # или создайте вручную

# Собрать и прошить
esphome run led-strip.yaml
```

## Файлы

```
led-strip.yaml.j2     — точка входа (Jinja2-шаблон)
_includes/
  runtime.yaml.j2     — платформа: Wi-Fi, API, OTA
  logic.yaml.j2       — лента, расписание
  maintenance.yaml.j2 — настройки расписания, globals, OTA
  diagnostics.yaml.j2 — uptime, RSSI, heap
_shared/              — общие фрагменты (OTA, labels, diagnostics)
hardware.yaml         — схема подключения по платам
secrets.yaml          — секреты (git-ignored)
```
