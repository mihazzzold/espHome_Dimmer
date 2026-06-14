# 💡 espHome_Dimmer (Диммер) — адресная LED-лента WS2812B

> 📌 **Коротко:** умная LED-лента на **ESPHome** — **ESP32-C6** / **ESP32-C3** / классический **ESP32** / **ESP8266 NodeMCU**, лента **WS2812B**, управление из **Home Assistant** с расписанием включения/выключения, яркостью и цветом.

Zigbee-версии (ESP32-C6 / ESP32-H2) — в папке [`zigbee/`](./zigbee/).

---

## 📑 Оглавление

| Раздел | О чём |
|--------|--------|
| [1. Что это](#overview) | Назначение и связь с HA |
| [2. Как работает](#how) | Лента, расписание, эффекты |
| [3. Железо](#hw) | Состав, питание, GPIO по платам |
| [4. Прошивка](#flash) | `sync`, `flasher`, Wi-Fi |
| [5. Home Assistant](#ha) | Добавление устройства |
| [6. Использование](#daily) | Расписание, цвет, эффекты |
| [7. OTA и логи](#ota) | Обновления и отладка |
| [8. Проблемы](#trouble) | Таблица симптомов |
| [9. Клон репо](#clone) | Без полного esphomeFlasher |
| [Ссылки](#links) | Внешние ресурсы |

---

<a id="overview"></a>
## 1. 📌 Что это за устройство

Контроллер по **Wi-Fi** подключается к **Home Assistant** (нативный ESPHome) и управляет адресной лентой **WS2812B**. Каждый светодиод адресуется независимо — поддерживаются цветовые эффекты, переходы и расписание.

> ⚠️ Если нужен **Zigbee** (без Wi-Fi) — см. [`zigbee/`](./zigbee/): варианты для **ESP32-C6** и **ESP32-H2**.

---

<a id="how"></a>
## 2. 🧠 Как это работает

1. **Лента** — платформа ESPHome `neopixelbus` (WS2812X, GRB): данные передаются через RMT (ESP32) или GPIO (ESP8266).
2. **Расписание** — четыре `number`-сущности в HA задают час и минуту включения/выключения; каждые 30 с прошивка проверяет текущее время и переключает ленту. Работает через полночь.
3. **Ручное управление** — `light.led_лента` в HA: вкл/выкл, яркость, цвет (Hue/Saturation), 6 встроенных эффектов.
4. **Время** — по умолчанию берётся из Home Assistant (`homeassistant` time platform); при `use_ha_time: false` — SNTP (автономно).

---

<a id="hw"></a>
## 3. 🔌 Железо и подключение

### 3.1. 📦 Состав

| Компонент | Назначение |
|-----------|------------|
| **ESP32-C6 SuperMini** (рекомендуется) / **C3** / **ESP32** / **ESP8266 NodeMCU** | Wi-Fi, логика, GPIO |
| **Лента WS2812B** | Адресные RGB-светодиоды (GRB) |
| **Резистор 300–500 Ом** | Защита линии данных от отражений |
| **Внешний БП 5V** | Питание ленты (не от платы при > 5–10 LED) |

### 3.2. ⚡ Питание

| Что | Как |
|-----|-----|
| Логика ESP | **5 V USB** или **3,3 V** с платы |
| Лента (до ~10 LED) | Можно от 5 V платы |
| Лента (> 10 LED) | **Отдельный БП 5 V**, общий GND с ESP |

> ⛔ Не питайте длинные ленты от USB — пиковый ток WS2812B ~60 мА/LED при белом цвете.

### 3.3. 📍 GPIO по платам

**ESP32-C6 SuperMini** (`--board-profile esp32c6-supermini`):

| Сигнал | GPIO | Примечание |
|--------|------|------------|
| DATA (WS2812B DIN) | **GPIO10** | через резистор 300–500 Ом |
| Статус-LED | **GPIO8** | встроенный |

**ESP32-C3 SuperMini** (`--board-profile esp32c3-supermini`):

| Сигнал | GPIO | Примечание |
|--------|------|------------|
| DATA | **GPIO5** | через резистор 300–500 Ом |
| Статус-LED | **GPIO8** | встроенный |

**ESP32 WROOM-32** (`--board-profile esp32-supermini`):

| Сигнал | GPIO | Примечание |
|--------|------|------------|
| DATA | **GPIO13** | через резистор 300–500 Ом |
| Статус-LED | **GPIO2** | встроенный |

**ESP8266 NodeMCU** (`--board-profile esp8266-nodemcu`):

| Сигнал | GPIO | На плате |
|--------|------|----------|
| DATA | **GPIO4** | D2 — не конфликтует с UART0 логгера |
| Статус-LED | **GPIO2** | D4 |

> Пины берутся из профилей `firmwares/boards/*.yaml` — при необходимости переопределите параметром `led_strip_pin`.

---

<a id="flash"></a>
## 4. 🛠️ Сборка и прошивка

### 4.1. 🔐 Секреты

Скопируйте пример:
```bash
cp secrets.yaml.example secrets.yaml
# Отредактируйте wifi_ssid, wifi_password, ota_password, api_encryption_key
```

### 4.2. 📥 Прошивка по USB

Замените `COM5` на ваш порт.

**ESP32-C6:**
```bash
python scripts/flasher.py -f firmwares-external/espHome_Dimmer/led-strip.yaml.j2 -a run --port COM5 --board-profile esp32c6-supermini --local -y
```

**ESP32-C3:**
```bash
python scripts/flasher.py -f firmwares-external/espHome_Dimmer/led-strip.yaml.j2 -a run --port COM5 --board-profile esp32c3-supermini --local -y
```

**ESP32 WROOM-32:**
```bash
python scripts/flasher.py -f firmwares-external/espHome_Dimmer/led-strip.yaml.j2 -a run --port COM5 --board-profile esp32-supermini --local -y
```

**ESP8266 NodeMCU:**
```bash
python scripts/flasher.py -f firmwares-external/espHome_Dimmer/led-strip.yaml.j2 -a run --port COM5 --board-profile esp8266-nodemcu --local -y
```

### 4.3. 📶 Первичный Wi-Fi (без SSID в прошивке)

```bash
python scripts/flasher.py -f firmwares-external/espHome_Dimmer/led-strip.yaml.j2 -a run --port COM5 --board-profile esp32c6-supermini --wifi-provisioning --local -y
```
Подключитесь к AP `dimmer-…-setup` и задайте сеть через captive portal.

### 4.4. Параметры шаблона

| Параметр | По умолчанию | Описание |
|----------|-------------|----------|
| `led_count` | 30 | Количество светодиодов |
| `led_strip_pin` | из board_profile | Переопределить пин |
| `use_ha_time` | true | false = SNTP (автономно) |
| `serial_number` | — | Серийный номер |
| `friendly_name_override` | — | Имя в Home Assistant |

---

<a id="ha"></a>
## 5. 🏠 Home Assistant

После выхода в сеть HA найдёт устройство через ESPHome. Появятся сущности:

| Сущность | Тип | Описание |
|----------|-----|----------|
| `light.led_лента` | light | Управление лентой |
| `switch.расписание_активно` | switch | Вкл/выкл расписание |
| `number.расписание_включение_ч` | number | Час включения (0–23) |
| `number.расписание_включение_мин` | number | Минута включения (0–59) |
| `number.расписание_выключение_ч` | number | Час выключения (0–23) |
| `number.расписание_выключение_мин` | number | Минута выключения (0–59) |

---

<a id="daily"></a>
## 6. ✨ Использование

### Расписание
1. Установите час и минуту включения/выключения через `number`-сущности.
2. Включите `switch.расписание_активно`.
3. Лента будет автоматически включаться и выключаться. Диапазон через полночь поддерживается (например, 22:00 → 07:00).

### Эффекты
Выберите в карточке `light` → выпадающий список эффектов:

| Эффект | Описание |
|--------|----------|
| Радуга | Плавная смена спектра по всей ленте |
| Бегущий огонь | Последовательная заливка цветом |
| Мерцание | Случайное мерцание отдельных LED |
| Бегающий огонёк | Бегающий «сканер» (Larson scanner) |
| Фейерверк | Случайные вспышки |
| Пульсация | Плавное нарастание/угасание |

---

<a id="ota"></a>
## 7. 🚀 OTA и логи

| Задача | Как |
|--------|-----|
| OTA-обновление | `flasher.py -a ota` или ESPHome Dashboard |
| Логи | `python scripts/flasher.py -f led-strip.yaml.j2 -a logs --port COM5` |

---

<a id="trouble"></a>
## 8. 🐛 Частые проблемы

| Симптом | Что проверить |
|---------|---------------|
| Лента не светится | GND общий, 5 V на ленте, пин DATA, резистор |
| Цвета неправильные | Порядок цветов (GRB vs RGB) — поменяйте `type` в шаблоне |
| Расписание не работает | Проверьте `switch.расписание_активно`, синхронизацию времени с HA |
| Мерцание / артефакты | Длинный провод DATA без резистора, EMI; добавьте конденсатор 100 мкФ на питание ленты |
| ESP8266 не компилируется | Нужен `framework_type: arduino`; проверьте версию ESPHome ≥ 2024 |

---

<a id="clone"></a>
## 9. 📂 Только этот репозиторий

```bash
git clone git@github.com:mihazzzold/espHome_Dimmer.git
cd espHome_Dimmer
cp secrets.yaml.example secrets.yaml
# Отредактируйте secrets.yaml
esphome run led-strip.yaml  # после рендера шаблона
```

Сборку и прошивку удобнее вести из **монорепо** (раздел [4](#flash)). Файл `secrets.yaml` **не в git** (`.gitignore`).

---

<a id="links"></a>
## 🔗 Ссылки

| Ресурс | URL |
|--------|-----|
| 🛠️ esphomeFlasher | https://github.com/mihazzzold/esphomeFlasher |
| 💡 Zigbee-версии (C6/H2) | [zigbee/](./zigbee/) |
| 📚 ESPHome NeopixelBus | https://esphome.io/components/light/neopixelbus.html |
| 📚 ESPHome Light effects | https://esphome.io/components/light/index.html#light-effects |

---

<p align="center"><b>Сделано с ESPHome · Home Assistant · MiHAzzz Lab</b> 💡</p>
