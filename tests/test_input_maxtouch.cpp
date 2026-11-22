/**
 * @file test_input_maxtouch.cpp
 * @brief Google Test suite for Microchip maXTouch driver (input_maxtouch.c)
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <zephyr/drivers/i2c.h>

// Step 1: Definition of the active mock instance pointer
I2cMock *g_i2c_mock = nullptr;

#ifdef __cplusplus
extern "C" {
#endif

#include "input_maxtouch.h"

// Driver internal functions exposed for testing via input_maxtouch_test_adapter.c
int mxt_init(const struct device *dev);
int mxt_load_object_table(const struct device *dev, struct mxt_information_block *info);
int mxt_load_config(const struct device *dev);
int mxt_report_data(const struct device *dev);

// Step 1: C API Trampolines connecting C driver calls to GMock C++ methods
bool i2c_is_ready_dt(const struct i2c_dt_spec *spec) {
    return g_i2c_mock ? g_i2c_mock->is_ready_dt(spec) : true;
}

int i2c_write_read_dt(const struct i2c_dt_spec *spec,
                      const void *write_buf, size_t num_write,
                      void *read_buf, size_t num_read) {
    return g_i2c_mock ? g_i2c_mock->write_read_dt(spec, write_buf, num_write, read_buf, num_read) : 0;
}

int i2c_write_dt(const struct i2c_dt_spec *spec,
                 const void *buf, size_t num_bytes) {
    return g_i2c_mock ? g_i2c_mock->write_dt(spec, buf, num_bytes) : 0;
}

#ifdef __cplusplus
}
#endif

/**
 * @brief Step 2: Test fixture with MockI2c wired to the C trampolines
 */
class MaxTouchTest : public ::testing::Test {
protected:
    MockI2c mock_i2c;
    struct mxt_data data{};
    struct mxt_config config{};
    struct device dev{};

    MaxTouchTest() : mock_i2c{}, data{}, config{}, dev{} {}

    void SetUp() override {
        g_i2c_mock = &mock_i2c;

        memset(&data, 0, sizeof(data));
        memset(&config, 0, sizeof(config));
        memset(&dev, 0, sizeof(dev));

        config.repeat_each_cycle = -1;
        config.swap_xy = -1;
        config.invert_x = -1;
        config.invert_y = -1;

        dev.data = &data;
        dev.config = &config;
        data.dev = &dev;
    }

    void TearDown() override {
        g_i2c_mock = nullptr;
    }
};

TEST_F(MaxTouchTest, FixtureInitialization) {
    ASSERT_NE(dev.data, nullptr);
    ASSERT_NE(dev.config, nullptr);
    EXPECT_EQ(data.dev, &dev);
}

TEST_F(MaxTouchTest, BusNotReady) {
    EXPECT_CALL(mock_i2c, is_ready_dt(&config.bus))
        .WillOnce(::testing::Return(false));

    int ret = mxt_init(&dev);
    EXPECT_EQ(ret, -EIO);
}

TEST_F(MaxTouchTest, LoadObjectTableReadFailure) {
    EXPECT_CALL(mock_i2c, is_ready_dt(&config.bus))
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_information_block)))
        .WillOnce(::testing::Return(-EIO));

    int ret = mxt_init(&dev);
    EXPECT_EQ(ret, -EIO);
}

TEST_F(MaxTouchTest, ReadInfoBlockSuccess) {
    struct mxt_information_block mock_info = {};
    mock_info.family_id = 0xA6;
    mock_info.variant_id = 0x0A;
    mock_info.version = 0x11;
    mock_info.build = 0xAA;
    mock_info.matrix_x_size = 12;
    mock_info.matrix_y_size = 12;
    mock_info.num_objects = 0;

    EXPECT_CALL(mock_i2c, write_dt(&config.bus, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(mock_i2c, is_ready_dt(&config.bus))
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_information_block)))
        .WillOnce(::testing::Invoke([&mock_info](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &mock_info, sizeof(mock_info));
            return 0;
        }));

    int ret = mxt_init(&dev);
    EXPECT_EQ(ret, 0);
}

TEST_F(MaxTouchTest, LoadObjectTableIdentifiesT51AndUnlocksT5) {
    struct mxt_information_block mock_info = {};
    mock_info.num_objects = 1;

    struct mxt_object_table_element mock_obj = {};
    mock_obj.type = 51;
    mock_obj.position = 0x04F9;
    mock_obj.size_minus_one = 12; // 13 bytes
    mock_obj.instances_minus_one = 0;
    mock_obj.report_ids_per_instance = 1;

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_information_block)))
        .WillOnce(::testing::Invoke([&mock_info](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &mock_info, sizeof(mock_info));
            return 0;
        }));

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_object_table_element)))
        .WillOnce(::testing::Invoke([&mock_obj](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &mock_obj, sizeof(mock_obj));
            return 0;
        }));

    int ret = mxt_load_object_table(&dev, &mock_info);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(data.t51_custom_address, 0x04F9);

    // Expect write to T51+8 (0x0501) = 0x80
    EXPECT_CALL(mock_i2c, write_dt(&config.bus, ::testing::_, 3))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void *buf, size_t) {
            const uint8_t *bytes = static_cast<const uint8_t*>(buf);
            EXPECT_EQ(bytes[0], 0x01); // 0x0501 LSB
            EXPECT_EQ(bytes[1], 0x05); // 0x0501 MSB
            EXPECT_EQ(bytes[2], 0x80);
            return 0;
        }));

    ret = mxt_load_config(&dev);
    EXPECT_EQ(ret, 0);
}

TEST_F(MaxTouchTest, ReportDataReturnsZeroOnT44CountZero) {
    data.t44_message_count_address = 0x0158;
    data.t5_message_processor_address = 0x0159;
    data.t5_max_message_size = 11;

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 11))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            uint8_t *buf = static_cast<uint8_t*>(read_buf);
            memset(buf, 0, 11);
            buf[0] = 0;
            return 0;
        }));

    int processed = mxt_report_data(&dev);
    EXPECT_EQ(processed, 0);
}

TEST_F(MaxTouchTest, ReportDataProcessesMessagesThroughT44AndT5) {
    data.t44_message_count_address = 0x0158;
    data.t5_message_processor_address = 0x0159;
    data.t5_max_message_size = 11;
    data.t6_command_processor_report_id = 1;

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 11))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            uint8_t *buf = static_cast<uint8_t*>(read_buf);
            memset(buf, 0, 11);
            buf[0] = 2; // count = 2
            buf[1] = 1; // report_id = 1
            return 0;
        }));

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 10))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            struct mxt_message *msg = static_cast<struct mxt_message*>(read_buf);
            memset(msg, 0, sizeof(*msg));
            msg->report_id = 1;
            return 0;
        }));

    int processed = mxt_report_data(&dev);
    EXPECT_EQ(processed, 2);
}

TEST_F(MaxTouchTest, T100ConfigTriStatePreservesEnablesAndDisablesBits) {
    data.t100_multiple_touch_touchscreen_address = 0x06B4;
    data.t100_size = 62;
    config.max_touch_points = 5;

    // Case 1: All -1 (omitted) -> preserves existing CFG1 bits from NVRAM
    config.repeat_each_cycle = -1;
    config.swap_xy = -1;
    config.invert_x = -1;
    config.invert_y = -1;

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 62))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            struct mxt_touch_multiscreen_t100 *t100 = static_cast<struct mxt_touch_multiscreen_t100*>(read_buf);
            memset(t100, 0, sizeof(*t100));
            t100->ctrl = MXT_T100_CTRL_ENABLE | MXT_T100_CTRL_RPTEN | MXT_T100_CTRL_SCANEN;
            t100->numtch = 5;
            t100->tchaux = 0x01;
            t100->tcheventcfg = 0x00;
            t100->cfg1 = MXT_T100_CFG_DISLOCK | MXT_T100_CFG_SWITCHXY | MXT_T100_CFG_INVERTX;
            return 0;
        }));

    // Expect no write to T100 since nothing changed
    EXPECT_EQ(mxt_load_config(&dev), 0);

    // Case 2: Explicitly disable swap_xy (0) and enable invert_y (1), keep invert_x (-1)
    config.swap_xy = 0;
    config.invert_y = 1;

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 62))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            struct mxt_touch_multiscreen_t100 *t100 = static_cast<struct mxt_touch_multiscreen_t100*>(read_buf);
            memset(t100, 0, sizeof(*t100));
            t100->ctrl = MXT_T100_CTRL_ENABLE | MXT_T100_CTRL_RPTEN | MXT_T100_CTRL_SCANEN;
            t100->numtch = 5;
            t100->tchaux = 0x01;
            t100->tcheventcfg = 0x07;
            t100->cfg1 = MXT_T100_CFG_SWITCHXY | MXT_T100_CFG_INVERTX;
            return 0;
        }));

    EXPECT_CALL(mock_i2c, write_dt(&config.bus, ::testing::_, 64))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void *buf, size_t) {
            const uint8_t *bytes = static_cast<const uint8_t*>(buf);
            const struct mxt_touch_multiscreen_t100 *written =
                reinterpret_cast<const struct mxt_touch_multiscreen_t100*>(&bytes[2]);
            // SWITCHXY cleared, INVERTX preserved, INVERTY set, DISLOCK set, TCHEVENTCFG cleared
            EXPECT_EQ(written->cfg1, MXT_T100_CFG_DISLOCK | MXT_T100_CFG_INVERTX | MXT_T100_CFG_INVERTY);
            EXPECT_EQ(written->tcheventcfg, 0x00);
            return 0;
        }));

    EXPECT_EQ(mxt_load_config(&dev), 0);
}

#if !defined(__ZEPHYR__)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
