#pragma once

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

#define K_NO_WAIT 0
#define K_FOREVER -1

#ifdef __cplusplus
extern "C" {
#endif

static inline int input_report_rel(const struct device *dev, uint16_t code, int32_t value, bool sync, int timeout) {
    (void)dev; (void)code; (void)value; (void)sync; (void)timeout;
    return 0;
}

static inline int input_report_abs(const struct device *dev, uint16_t code, int32_t value, bool sync, int timeout) {
    (void)dev; (void)code; (void)value; (void)sync; (void)timeout;
    return 0;
}

static inline int input_report_key(const struct device *dev, uint16_t code, int32_t value, bool sync, int timeout) {
    (void)dev; (void)code; (void)value; (void)sync; (void)timeout;
    return 0;
}

#ifdef __cplusplus
}
#endif
