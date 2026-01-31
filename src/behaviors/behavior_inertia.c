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

static void send_mouse_report(int8_t x, int8_t y) {
    if (x == 0 && y == 0) {
        return;
    }

    LOG_DBG("Inertia move: x=%d y=%d", x, y);

    zmk_hid_mouse_movement_set(x, y);
    zmk_endpoints_send_mouse_report();
    zmk_hid_mouse_movement_set(0, 0);
}

static void tick_work_handler(struct k_work *work) {
    state.x_velocity = calc_velocity(state.x_dir, state.x_velocity);
    state.y_velocity = calc_velocity(state.y_dir, state.y_velocity);

    int8_t move_x = calc_movement(state.x_dir, state.x_velocity);
    int8_t move_y = calc_movement(state.y_dir, state.y_velocity);

    send_mouse_report(move_x, move_y);

    state.frame++;

    if (state.x_dir == 0 && state.y_dir == 0 &&
        state.x_velocity == 0 && state.y_velocity == 0) {
        state.frame = 0;
        LOG_DBG("Inertia stopped");
        return;
    }

    k_work_schedule(&state.tick_work, K_MSEC(state.interval_ms));
}

static int behavior_inertia_init_global(void) {
    k_work_init_delayable(&state.tick_work, tick_work_handler);
    return 0;
}

SYS_INIT(behavior_inertia_init_global, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int on_inertia_binding_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (!dev) {
        LOG_ERR("Unable to find inertia device");
        return -ENODEV;
    }

    const struct behavior_inertia_config *cfg = dev->config;

    state.delay_ms = cfg->delay_ms;
    state.interval_ms = cfg->interval_ms;
    state.max_speed = cfg->max_speed;
    state.time_to_max = cfg->time_to_max;
    state.friction = cfg->friction;
    state.move_delta = cfg->move_delta;

    if (cfg->y_direction != 0) {
        state.y_dir = cfg->y_direction;
    }
    if (cfg->x_direction != 0) {
        state.x_dir = cfg->x_direction;
    }

    LOG_DBG("Inertia pressed: x_dir=%d y_dir=%d", state.x_dir, state.y_dir);

    // Send immediate movement on first press
    if (state.frame == 0) {
        int8_t move_x = calc_movement(state.x_dir, state.x_velocity);
        int8_t move_y = calc_movement(state.y_dir, state.y_velocity);
        send_mouse_report(move_x, move_y);
    }

    if (!k_work_delayable_is_pending(&state.tick_work)) {
        uint32_t delay = (state.frame > 0) ? state.interval_ms : state.delay_ms;
        k_work_schedule(&state.tick_work, K_MSEC(delay));
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

#endif
