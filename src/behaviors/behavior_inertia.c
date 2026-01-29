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

static int8_t calc_velocity(int8_t direction, int8_t velocity) {
    // Friction
    if ((direction > -1) && (velocity < 0)) {
        velocity = (int16_t)(velocity + 1) * (256 - state.friction) / 256;
    } else if ((direction < 1) && (velocity > 0)) {
        velocity = (int16_t)velocity * (256 - state.friction) / 256;
    }

    // Acceleration
    if ((direction > 0) && (velocity < state.time_to_max)) {
        velocity++;
    } else if ((direction < 0) && (velocity > -state.time_to_max)) {
        velocity--;
    }

    return velocity;
}

static int8_t calc_movement(int8_t direction, int8_t velocity) {
    int16_t unit;

    if (state.frame == 0) {
        unit = direction * state.move_delta;
    } else {
        // Quadratic acceleration curve
        int32_t percent = ((int32_t)velocity << 8) / state.time_to_max;
        percent = (percent * percent) >> 8;
        if (velocity < 0) {
            percent = -percent;
        }

        if (velocity > 0) {
            unit = 1;
        } else if (velocity < 0) {
            unit = -1;
        } else {
            unit = 0;
        }

        unit = unit + ((state.max_speed * percent) >> 8);
    }

    if (unit > MOVE_MAX) {
        unit = MOVE_MAX;
    } else if (unit < -MOVE_MAX) {
        unit = -MOVE_MAX;
    }

    return (int8_t)unit;
}

#endif
