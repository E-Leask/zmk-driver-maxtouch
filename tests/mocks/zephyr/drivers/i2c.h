#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
#include <gmock/gmock.h>

class I2cMock {
public:
    virtual ~I2cMock() = default;
    virtual bool is_ready_dt(const struct i2c_dt_spec *spec) = 0;
    virtual int write_read_dt(const struct i2c_dt_spec *spec,
                              const void *write_buf, size_t num_write,
                              void *read_buf, size_t num_read) = 0;
    virtual int write_dt(const struct i2c_dt_spec *spec,
                         const void *buf, size_t num_bytes) = 0;
};

class MockI2c : public I2cMock {
public:
    MOCK_METHOD(bool, is_ready_dt, (const struct i2c_dt_spec *spec), (override));
    MOCK_METHOD(int, write_read_dt, (const struct i2c_dt_spec *spec,
                                     const void *write_buf, size_t num_write,
                                     void *read_buf, size_t num_read), (override));
    MOCK_METHOD(int, write_dt, (const struct i2c_dt_spec *spec,
                                const void *buf, size_t num_bytes), (override));
};

extern I2cMock *g_i2c_mock;

extern "C" {
#endif

struct i2c_dt_spec {
    const struct device *bus;
    uint16_t addr;
#ifdef __cplusplus
    constexpr i2c_dt_spec() : bus(nullptr), addr(0) {}
    constexpr i2c_dt_spec(const struct device *b, uint16_t a) : bus(b), addr(a) {}
#endif
};

// C API declarations linked to C++ trampolines
bool i2c_is_ready_dt(const struct i2c_dt_spec *spec);
int i2c_write_read_dt(const struct i2c_dt_spec *spec,
                      const void *write_buf, size_t num_write,
                      void *read_buf, size_t num_read);
int i2c_write_dt(const struct i2c_dt_spec *spec,
                 const void *buf, size_t num_bytes);

#ifdef __cplusplus
}
#endif
