#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint16_t sys_cpu_to_le16(uint16_t val) {
    return val;
}

static inline uint16_t sys_le16_to_cpu(uint16_t val) {
    return val;
}

#ifdef __cplusplus
}
#endif
