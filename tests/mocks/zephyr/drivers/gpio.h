#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>

#define GPIO_INPUT 0
#define GPIO_PULL_UP (1U << 4)
#define GPIO_INT_EDGE_TO_ACTIVE 0

#ifdef __cplusplus
extern "C" {
#endif

struct gpio_dt_spec {
    const struct device *port;
    uint32_t pin;
    uint32_t dt_flags;
#ifdef __cplusplus
    constexpr gpio_dt_spec() : port(nullptr), pin(0), dt_flags(0) {}
    constexpr gpio_dt_spec(const struct device *p, uint32_t pin, uint32_t flags) : port(p), pin(pin), dt_flags(flags) {}
#endif
};

struct gpio_callback;
typedef void (*gpio_callback_handler_t)(const struct device *port, struct gpio_callback *cb, uint32_t pins);

struct gpio_callback {
    gpio_callback_handler_t handler;
    uint32_t pin_mask;
};

static inline int gpio_pin_configure_dt(const struct gpio_dt_spec *spec, int flags) {
    (void)spec; (void)flags;
    return 0;
}

static inline int gpio_init_callback(struct gpio_callback *callback, gpio_callback_handler_t handler, uint32_t pin_mask) {
    if (callback) {
        callback->handler = handler;
        callback->pin_mask = pin_mask;
    }
    return 0;
}

static inline int gpio_add_callback(const struct device *port, struct gpio_callback *callback) {
    (void)port; (void)callback;
    return 0;
}

static inline int gpio_pin_interrupt_configure_dt(const struct gpio_dt_spec *spec, int flags) {
    (void)spec; (void)flags;
    return 0;
}

static inline int gpio_pin_get_dt(const struct gpio_dt_spec *spec) {
    (void)spec;
    return 0;
}

#ifdef __cplusplus
}
#endif
