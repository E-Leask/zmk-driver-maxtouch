#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifndef EIO
#define EIO 5
#endif
#ifndef EINVAL
#define EINVAL 22
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct device {
    const char *name;
    const void *config;
    void *data;
};

struct k_work {
    void (*handler)(struct k_work *work);
};

static inline void k_work_init(struct k_work *work, void (*handler)(struct k_work *work)) {
    if (work) work->handler = handler;
}

static inline int k_work_submit(struct k_work *work) {
    return 0;
}

static inline void k_msleep(int ms) {
    (void)ms;
}

#ifndef DT_INST_FOREACH_STATUS_OKAY
#define DT_INST_FOREACH_STATUS_OKAY(inst_fn)
#endif

#ifdef __cplusplus
}
#endif
