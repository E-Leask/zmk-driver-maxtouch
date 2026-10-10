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
int mxt_load_config(const struct device *dev, const struct mxt_information_block *information);
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
        // Wire up the active mock
        g_i2c_mock = &mock_i2c;

        // Initialize mutable structures before each test
        memset(&data, 0, sizeof(data));
        memset(&dev, 0, sizeof(dev));

        // Link device data and config pointers
        dev.data = &data;
        dev.config = &config;
        data.dev = &dev;
    }

    void TearDown() override {
        // Unhook mock
        g_i2c_mock = nullptr;
    }
};

/**
 * @brief Test fixture setup & initial state validation
 */
TEST_F(MaxTouchTest, FixtureInitialization) {
    ASSERT_NE(dev.data, nullptr);
    ASSERT_NE(dev.config, nullptr);
    EXPECT_EQ(data.dev, &dev);
}

/**
 * @brief Step 3.1: Test bus failure during initialization
 */
TEST_F(MaxTouchTest, BusNotReady) {
    // Expect i2c_is_ready_dt to be called and return false
    EXPECT_CALL(mock_i2c, is_ready_dt(&config.bus))
        .WillOnce(::testing::Return(false));

    // When bus is not ready, mxt_init should return -EIO
    int ret = mxt_init(&dev);
    EXPECT_EQ(ret, -EIO);
}

/**
 * @brief Step 3.2: Test info block read failure during initialization
 */
TEST_F(MaxTouchTest, LoadObjectTableReadFailure) {
    // Bus is ready
    EXPECT_CALL(mock_i2c, is_ready_dt(&config.bus))
        .WillOnce(::testing::Return(true));

    // Reading info block returns I/O error
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_information_block)))
        .WillOnce(::testing::Return(-EIO));

    int ret = mxt_init(&dev);
    EXPECT_EQ(ret, -EIO);
}

/**
 * @brief Step 3.3: Test info block read success using Invoke
 */
TEST_F(MaxTouchTest, ReadInfoBlockSuccess) {
    struct mxt_information_block mock_info = {};
    mock_info.family_id = 0xA0;
    mock_info.variant_id = 0x01;
    mock_info.version = 0x10;
    mock_info.build = 0x01;
    mock_info.matrix_x_size = 16;
    mock_info.matrix_y_size = 10;
    mock_info.num_objects = 0;

    // Catch-all default expectations first
    EXPECT_CALL(mock_i2c, write_dt(&config.bus, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(0));

    // Specific expectations (GMock evaluates in reverse order of definition)
    EXPECT_CALL(mock_i2c, is_ready_dt(&config.bus))
        .WillOnce(::testing::Return(true));

    // When reading info block, copy mock_info to output buffer and return 0
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_information_block)))
        .WillOnce(::testing::Invoke([&mock_info](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t num_read) {
            memcpy(read_buf, &mock_info, sizeof(mock_info));
            return 0;
        }));

    int ret = mxt_init(&dev);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief Test that object table loading correctly discovers and stores T18 COMMSCONFIG address
 */
TEST_F(MaxTouchTest, LoadObjectTableIdentifiesT18) {
    struct mxt_information_block mock_info = {};
    mock_info.num_objects = 1;

    struct mxt_object_table_element mock_obj = {};
    mock_obj.type = 18;
    mock_obj.position = 0x04AE;
    mock_obj.size_minus_one = 1; // 2 bytes
    mock_obj.instances_minus_one = 0;
    mock_obj.report_ids_per_instance = 0;

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
    EXPECT_EQ(data.t18_comms_config_address, 0x04AE);
}

/**
 * @brief Test that mxt_load_config preserves factory T7 and T8 and configures T18 to 0x01 (Mode 1 / level)
 */
TEST_F(MaxTouchTest, PreserveFactoryT7T8AndConfigureT18) {
    data.t7_powerconfig_address = 0x048F;
    data.t8_acquisitionconfig_address = 0x0494;
    data.t18_comms_config_address = 0x04AE;
    config.has_t7_config = false;
    config.has_charge_time = false;

    struct mxt_gen_powerconfig_t7 mock_t7 = {};
    mock_t7.idleacqint = 20;
    mock_t7.actacqint = 10;
    mock_t7.actv2idleto = 50;
    mock_t7.cfg = 0x83;

    struct mxt_gen_acquisitionconfig_t8 mock_t8 = {};
    mock_t8.chrgtime = 15;

    struct mxt_spt_commsconfig_t18 mock_t18 = {};
    mock_t18.ctrl = 0x00; // Mode 0 (edge), so driver configures to 0x01 (Mode 1 / level)
    mock_t18.cmd = 0x00;

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_gen_powerconfig_t7)))
        .WillOnce(::testing::Invoke([&mock_t7](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &mock_t7, sizeof(mock_t7));
            return 0;
        }));

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_gen_acquisitionconfig_t8)))
        .WillOnce(::testing::Invoke([&mock_t8](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &mock_t8, sizeof(mock_t8));
            return 0;
        }));

    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, sizeof(struct mxt_spt_commsconfig_t18)))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([&mock_t18](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &mock_t18, sizeof(mock_t18));
            return 0;
        }));

    // Expect write to T18 configuring 0x01 (size is 2 bytes data + 2 bytes addr = 4 bytes)
    EXPECT_CALL(mock_i2c, write_dt(&config.bus, ::testing::_, sizeof(struct mxt_spt_commsconfig_t18) + 2))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void *buf, size_t num_bytes) {
            const uint8_t *bytes = static_cast<const uint8_t*>(buf);
            EXPECT_EQ(bytes[0], 0xAE);
            EXPECT_EQ(bytes[1], 0x04);
            EXPECT_EQ(bytes[2], 0x01); // ctrl = 0x01 (Mode 1 / level)
            EXPECT_EQ(bytes[3], 0x00); // cmd = 0x00
            return 0;
        }));

    struct mxt_information_block info = {};
    int ret = mxt_load_config(&dev, &info);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief Test that mxt_load_config enables T100 touch events and vector coordinates when disabled in factory NVRAM
 */
TEST_F(MaxTouchTest, ConfiguresT100EventAndVectorReporting) {
    testing::InSequence seq;
    const uint8_t t100_len = 62;
    data.t100_multiple_touch_touchscreen_address = 0x06B4;
    data.t100_size = t100_len;

    struct mxt_touch_multiscreen_t100 factory_t100 = {};
    factory_t100.ctrl = 0x8F;
    factory_t100.cfg1 = 0xF4;
    factory_t100.scraux = 0x00;
    factory_t100.tchaux = 0x00;      // vectors disabled in factory
    factory_t100.tcheventcfg = 0x00; // events disabled in factory
    factory_t100.numtch = 2;
    factory_t100.xycfg = 0x88;
    factory_t100.gain = 20;
    factory_t100.tchthr = 29;

    // 1. Initial read of factory T100 (62 bytes)
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, t100_len))
        .WillOnce(::testing::Invoke([&factory_t100](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &factory_t100, sizeof(factory_t100));
            return 0;
        }));

    // 2. Expect write of updated T100 with tcheventcfg=0x07 and tchaux=0x01 (62 bytes + 2 addr bytes = 64)
    EXPECT_CALL(mock_i2c, write_dt(&config.bus, ::testing::_, t100_len + 2))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void *buf, size_t num_bytes) {
            const uint8_t *bytes = static_cast<const uint8_t*>(buf);
            EXPECT_EQ(bytes[0], 0xB4); // addr LSB (0x06B4)
            EXPECT_EQ(bytes[1], 0x06); // addr MSB
            const struct mxt_touch_multiscreen_t100 *written_t100 =
                reinterpret_cast<const struct mxt_touch_multiscreen_t100*>(&bytes[2]);
            EXPECT_EQ(written_t100->tcheventcfg, 0x07);
            EXPECT_EQ(written_t100->tchaux & 0x01, 0x01);
            return 0;
        }));

    // 3. Verification read of updated T100 from chip SRAM (62 bytes)
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, t100_len))
        .WillOnce(::testing::Invoke([&factory_t100](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            struct mxt_touch_multiscreen_t100 verified_t100 = factory_t100;
            verified_t100.tcheventcfg = 0x07;
            verified_t100.tchaux |= 0x01;
            memcpy(read_buf, &verified_t100, sizeof(verified_t100));
            return 0;
        }));

    struct mxt_information_block info = {};
    int ret = mxt_load_config(&dev, &info);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief Test that mxt_report_data returns 0 immediately when FIFO is empty
 */
TEST_F(MaxTouchTest, ReportDataReturnsZeroOnEmptyFifo) {
    data.t5_message_processor_address = 0x0159;
    data.t5_max_message_size = 11;
    data.t6_command_processor_address = 0x02D7;

    struct mxt_message mock_t5 = {0};
    mock_t5.report_id = 0xFF; // rpt_id = 0xFF (no message)

    uint8_t mock_t6_status = 0x00;

    // Expect direct read from T5
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 11))
        .WillOnce(::testing::Invoke([&mock_t5](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &mock_t5, 11);
            return 0;
        }));

    // Expect T6 status inspection when T5 is empty
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 1))
        .WillOnce(::testing::Invoke([&mock_t6_status](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            memcpy(read_buf, &mock_t6_status, 1);
            return 0;
        }));

    int processed = mxt_report_data(&dev);
    EXPECT_EQ(processed, 0);
}

/**
 * @brief Test that mxt_report_data returns 0 when T44 reports count == 0
 */
TEST_F(MaxTouchTest, ReportDataReturnsZeroOnT44CountZero) {
    data.t44_message_count_address = 0x0158;
    data.t5_message_processor_address = 0x0159;
    data.t5_max_message_size = 11;

    // Expect read of 12 bytes (1 byte count + 11 bytes message) starting at T44
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 12))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            uint8_t *buf = static_cast<uint8_t*>(read_buf);
            memset(buf, 0, 12);
            buf[0] = 0; // count = 0 (empty FIFO)
            return 0;
        }));

    int processed = mxt_report_data(&dev);
    EXPECT_EQ(processed, 0);
}

/**
 * @brief Test that mxt_report_data reads and processes a single message through T44 without trailing drain read
 */
TEST_F(MaxTouchTest, ReportDataProcessesMessageThroughT44) {
    data.t44_message_count_address = 0x0158;
    data.t5_message_processor_address = 0x0159;
    data.t5_max_message_size = 11;
    data.t6_command_processor_report_id = 1;

    // Expect read of 12 bytes starting at T44: count=1, report_id=1 (T6 status OK)
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 12))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            uint8_t *buf = static_cast<uint8_t*>(read_buf);
            memset(buf, 0, 12);
            buf[0] = 1; // count = 1
            buf[1] = 1; // report_id = 1 (t6_command_processor_report_id)
            buf[2] = 0; // status = OK
            return 0;
        }));

    int processed = mxt_report_data(&dev);
    EXPECT_EQ(processed, 1);
}

/**
 * @brief Test that mxt_report_data reads exactly the remaining messages indicated by T44 count
 */
TEST_F(MaxTouchTest, ReportDataProcessesMultipleMessagesThroughT44) {
    data.t44_message_count_address = 0x0158;
    data.t5_message_processor_address = 0x0159;
    data.t5_max_message_size = 11;
    data.t6_command_processor_report_id = 1;

    // 1. First read of 12 bytes from T44: count=2, msg 1 report_id=1
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 12))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            uint8_t *buf = static_cast<uint8_t*>(read_buf);
            memset(buf, 0, 12);
            buf[0] = 2; // count = 2
            buf[1] = 1; // report_id = 1
            buf[2] = 0; // status = OK
            return 0;
        }));

    // 2. Second read of remaining message (count - 1 = 1) directly from T5
    EXPECT_CALL(mock_i2c, write_read_dt(&config.bus, ::testing::_, ::testing::_, ::testing::_, 11))
        .WillOnce(::testing::Invoke([](const struct i2c_dt_spec*, const void*, size_t, void *read_buf, size_t) {
            struct mxt_message *msg = static_cast<struct mxt_message*>(read_buf);
            memset(msg, 0, sizeof(*msg));
            msg->report_id = 1; // report_id = 1
            return 0;
        }));

    int processed = mxt_report_data(&dev);
    EXPECT_EQ(processed, 2);
}

#if !defined(__ZEPHYR__)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
