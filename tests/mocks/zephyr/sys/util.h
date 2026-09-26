#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

#ifndef WRITE_BIT
#define WRITE_BIT(var, bit, val) ((var) = ((var) & ~BIT(bit)) | ((val) ? BIT(bit) : 0))
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, field) \
    ((type *)((char *)(ptr) - offsetof(type, field)))
#endif

#if !defined(__packed)
#if defined(_MSC_VER)
#define __packed
#else
#define __packed __attribute__((__packed__))
#endif
#endif
