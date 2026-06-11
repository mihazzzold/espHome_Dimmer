// SPDX-License-Identifier: MIT
// Гиммер — Zigbee Color Dimmable Light на ESP32-H2 + WS2812B.
//
// ESP32-H2: только Zigbee/BLE (НЕТ Wi-Fi 802.11).
// Профиль: HA Color Dimmable Light (device_id = 0x0102)
// Кластеры: Basic, Identify, On/Off, Level Control, Color Control (HS)
// Лента: WS2812B через RMT, GPIO12, 30 LED по умолчанию.
//
// Отличие от C6-версии: только GPIO пин ленты (GPIO12 вместо GPIO10).
// Код идентичен — разница только в LED_STRIP_GPIO.

#include <math.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "esp_zigbee_core.h"
#include "zcl/esp_zigbee_zcl_common.h"

// ---------------------------------------------------------------------------
// Конфигурация железа
// ---------------------------------------------------------------------------
#define LED_STRIP_GPIO       12          // GPIO12 на ESP32-H2 SuperMini
#define LED_COUNT            30
#define PAIR_BUTTON_GPIO     9           // Кнопка сброса (zigbee_action_button из board profile)
#define PAIR_HOLD_MS         5000

// ---------------------------------------------------------------------------
// Конфигурация Zigbee
// ---------------------------------------------------------------------------
#define ZIGBEE_ENDPOINT      10
#define ZIGBEE_CHANNEL_MASK  ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK

// ---------------------------------------------------------------------------
static const char *TAG = "gimmer-h2";

// ---------------------------------------------------------------------------
// Состояние ленты
// ---------------------------------------------------------------------------
static led_strip_handle_t s_strip;

static bool    s_on  = false;
static uint8_t s_lvl = 254;
static uint8_t s_hue = 0;
static uint8_t s_sat = 0;

// ---------------------------------------------------------------------------
// HSV → RGB
// ---------------------------------------------------------------------------
static void hsv_to_rgb(uint8_t h8, uint8_t s8, uint8_t v8,
                        uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s8 == 0) { *r = *g = *b = v8; return; }
    uint16_t h = (uint16_t)h8 * 360 / 254;
    uint8_t  s = s8, v = v8;
    uint8_t  region  = h / 60;
    uint8_t  rem     = (h % 60) * 255 / 60;
    uint8_t  p = (uint16_t)v * (255 - s) / 255;
    uint8_t  q = (uint16_t)v * (255 - ((uint16_t)s * rem / 255)) / 255;
    uint8_t  t = (uint16_t)v * (255 - ((uint16_t)s * (255 - rem) / 255)) / 255;
    switch (region) {
        case 0: *r=v; *g=t; *b=p; break;
        case 1: *r=q; *g=v; *b=p; break;
        case 2: *r=p; *g=v; *b=t; break;
        case 3: *r=p; *g=q; *b=v; break;
        case 4: *r=t; *g=p; *b=v; break;
        default:*r=v; *g=p; *b=q; break;
    }
}

// ---------------------------------------------------------------------------
// Применить состояние к ленте
// ---------------------------------------------------------------------------
static void leds_apply(void)
{
    if (!s_on) {
        led_strip_clear(s_strip);
        led_strip_refresh(s_strip);
        return;
    }
    uint8_t r, g, b;
    hsv_to_rgb(s_hue, s_sat, s_lvl, &r, &g, &b);
    for (int i = 0; i < LED_COUNT; i++) {
        led_strip_set_pixel(s_strip, i, r, g, b);
    }
    led_strip_refresh(s_strip);
    ESP_LOGI(TAG, "LEDs → on=%d h=%d s=%d v=%d  rgb=(%d,%d,%d)",
             s_on, s_hue, s_sat, s_lvl, r, g, b);
}

// ---------------------------------------------------------------------------
// Инициализация LED-ленты
// ---------------------------------------------------------------------------
static void led_strip_init_hw(void)
{
    led_strip_config_t cfg = {
        .strip_gpio_num           = LED_STRIP_GPIO,
        .max_leds                 = LED_COUNT,
        .color_component_format   = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .led_model                = LED_MODEL_WS2812,
        .flags.invert_out         = false,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags.with_dma    = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&cfg, &rmt_cfg, &s_strip));
    led_strip_clear(s_strip);
    led_strip_refresh(s_strip);
    ESP_LOGI(TAG, "LED strip OK: GPIO%d, %d LEDs", LED_STRIP_GPIO, LED_COUNT);
}

// ---------------------------------------------------------------------------
// Кнопка паринга
// ---------------------------------------------------------------------------
static void pair_button_task(void *arg)
{
    gpio_config_t cfg = {
        .pin_bit_mask = BIT64(PAIR_BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    uint32_t hold_ms = 0;
    while (1) {
        if (gpio_get_level(PAIR_BUTTON_GPIO) == 0) {
            hold_ms += 100;
            if (hold_ms >= PAIR_HOLD_MS) {
                ESP_LOGW(TAG, "Сброс Zigbee-сети (%lu мс)", hold_ms);
                esp_zb_factory_reset();
                hold_ms = 0;
            }
        } else {
            hold_ms = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ---------------------------------------------------------------------------
// Zigbee: обработчик атрибутов
// ---------------------------------------------------------------------------
static esp_err_t zb_attr_handler(const esp_zb_zcl_set_attr_value_message_t *msg)
{
    if (!msg) return ESP_ERR_INVALID_ARG;
    if (msg->info.status != ESP_ZB_ZCL_STATUS_SUCCESS) return ESP_ERR_INVALID_ARG;

    uint16_t cluster = msg->info.cluster;
    uint16_t attr    = msg->attribute.id;

    if (cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF) {
        if (attr == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID) {
            s_on = *(bool *)msg->attribute.data.value;
            ESP_LOGI(TAG, "On/Off → %s", s_on ? "ON" : "OFF");
        }
    } else if (cluster == ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL) {
        if (attr == ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID) {
            s_lvl = *(uint8_t *)msg->attribute.data.value;
            ESP_LOGI(TAG, "Level → %d", s_lvl);
        }
    } else if (cluster == ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL) {
        if (attr == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_HUE_ID) {
            s_hue = *(uint8_t *)msg->attribute.data.value;
        } else if (attr == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID) {
            s_sat = *(uint8_t *)msg->attribute.data.value;
        }
        ESP_LOGI(TAG, "Color → hue=%d sat=%d", s_hue, s_sat);
    }

    leds_apply();
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Zigbee: обработчик сигналов стека
// ---------------------------------------------------------------------------
static void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_s)
{
    uint32_t *p = signal_s->p_app_signal;
    esp_err_t err = signal_s->esp_err_status;
    esp_zb_app_signal_type_t sig = *p;

    switch (sig) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Zigbee-стек инициализируется...");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Устройство в сети.");
        } else {
            ESP_LOGW(TAG, "Нет сети — запуск поиска...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        }
        break;

    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Подключён. PAN=0x%04hx, ch=%d",
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());
        } else {
            ESP_LOGW(TAG, "Не нашёл сеть — повтор...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        }
        break;

    default:
        ESP_LOGD(TAG, "Сигнал: 0x%x err=0x%x", sig, err);
        break;
    }
}

// ---------------------------------------------------------------------------
// Zigbee: задача стека (не pinned — H2 однопроцессорный)
// ---------------------------------------------------------------------------
static void zigbee_task(void *arg)
{
    esp_zb_platform_config_t platform_cfg = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config  = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&platform_cfg));

    esp_zb_cfg_t zb_cfg = {
        .esp_zb_role         = ESP_ZB_DEVICE_TYPE_ROUTER,
        .install_code_policy = false,
        .nwk_cfg.zczr_cfg    = { .max_children = 10 },
    };
    esp_zb_init(&zb_cfg);

    esp_zb_color_dimmable_light_cfg_t light_cfg = ESP_ZB_DEFAULT_COLOR_DIMMABLE_LIGHT_CONFIG();
    esp_zb_cluster_list_t *clusters = esp_zb_color_dimmable_light_clusters_create(&light_cfg);

    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint           = ZIGBEE_ENDPOINT,
        .app_profile_id     = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id      = ESP_ZB_HA_COLOR_DIMMABLE_LIGHT_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_add_ep(ep_list, clusters, ep_cfg);
    esp_zb_device_register(ep_list);

    esp_zb_core_action_handler_register(zb_attr_handler);

    esp_zb_set_primary_network_channel_set(ZIGBEE_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_main_loop_iteration();
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// app_main
// ---------------------------------------------------------------------------
void app_main(void)
{
    ESP_LOGI(TAG, "Гиммер Zigbee (ESP32-H2) boot");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    led_strip_init_hw();

    xTaskCreate(pair_button_task, "pair_btn", 2048, NULL, 5, NULL);

    // H2 — однопроцессорный, xTaskCreate без pinned
    xTaskCreate(zigbee_task, "zigbee", 4096, NULL, 5, NULL);
}
