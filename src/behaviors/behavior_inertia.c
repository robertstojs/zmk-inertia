#define DT_DRV_COMPAT zmk_behavior_inertia

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define MOVE_MAX 127

struct behavior_inertia_config {
    int8_t x_direction;
    int8_t y_direction;
    uint16_t delay_ms;
    uint16_t interval_ms;
    int16_t max_speed;
    int16_t time_to_max;
    uint8_t friction;
    int8_t move_delta;
};

// Shared across instances so velocity persists for glide effect
static struct {
    struct k_work_delayable tick_work;
    uint8_t frame;
    int8_t x_dir;
    int8_t y_dir;
    int8_t x_velocity;
    int8_t y_velocity;
    int16_t time_to_max;
    int16_t max_speed;
    uint16_t interval_ms;
    uint16_t delay_ms;
    uint8_t friction;
    int8_t move_delta;
} state = {
    .frame = 0,
    .x_dir = 0,
    .y_dir = 0,
    .x_velocity = 0,
    .y_velocity = 0,
    .time_to_max = 32,
    .max_speed = 16,
    .interval_ms = 16,
    .delay_ms = 150,
    .friction = 24,
    .move_delta = 1,
};

#endif
