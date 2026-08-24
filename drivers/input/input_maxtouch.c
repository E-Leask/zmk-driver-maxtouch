#define DT_DRV_COMPAT microchip_maxtouch

#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>

#include "input_maxtouch.h"

LOG_MODULE_REGISTER(maxtouch, CONFIG_INPUT_LOG_LEVEL);

static int mxt_seq_read(const struct device *dev, const uint16_t addr, void *buf,
                        const uint8_t len) {
    const struct mxt_config *config = dev->config;

    const uint16_t addr_lsb = sys_cpu_to_le16(addr);

    return i2c_write_read_dt(&config->bus, &addr_lsb, sizeof(addr_lsb), buf, len);
}

static int mxt_seq_write(const struct device *dev, const uint16_t addr, void *buf,
                         const uint8_t len) {
    const struct mxt_config *config = dev->config;
    struct i2c_msg msg[2];

    const uint16_t addr_lsb = sys_cpu_to_le16(addr);
    msg[0].buf = (uint8_t *)&addr_lsb;
    msg[0].len = 2U;
    msg[0].flags = I2C_MSG_WRITE;

    msg[1].buf = (uint8_t *)buf;
    msg[1].len = len;
    msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

    return i2c_transfer_dt(&config->bus, msg, 2);
}

static inline bool is_t100_report(const struct device *dev, int report_id) {
    const struct mxt_config *config = dev->config;
    struct mxt_data *data = dev->data;

    return (report_id >= data->t100_first_report_id + 2 &&
            report_id < data->t100_first_report_id + 2 + config->max_touch_points);
}

static void mxt_report_data(const struct device *dev) {
    struct mxt_data *data = dev->data;
    int ret;

    if (!data->t5_message_processor_address) {
        LOG_WRN("No T5 message processor object found!");
        return;
    }

    uint16_t pending_fingers = 0;
    bool last_touch_status = false;

    uint8_t t44_count = 0;
    if (data->t44_message_count_address) {
        ret = mxt_seq_read(dev, data->t44_message_count_address, &t44_count, 1);
        if (ret == 0) {
            LOG_INF("T44 count: %d", t44_count);
        }
    }

    int read_iterations = (t44_count > 0) ? t44_count : 10;

    // Read messages from T5
    for (int i = 0; i < read_iterations; i++) {
        struct mxt_message msg = {0};
        uint8_t read_len = (data->t5_max_message_size > 0 && data->t5_max_message_size <= sizeof(msg))
                               ? data->t5_max_message_size
                               : 11;

        ret = mxt_seq_read(dev, data->t5_message_processor_address, &msg, read_len);
        if (ret < 0) {
            LOG_ERR("Failed to read message from T5: %d", ret);
            break;
        }

        LOG_INF("T5 read (i=%d, len=%d): rpt_id=%d [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
                i, read_len, msg.report_id, msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5]);

        if (msg.report_id == 0xFF || msg.report_id == 0x00) {
            if (t44_count == 0) {
                break;
            }
        }

        if (is_t100_report(dev, msg.report_id)) {
            uint8_t finger_idx = msg.report_id - data->t100_first_report_id - 2;
            bool pending_for_finger = (pending_fingers & BIT(finger_idx)) != 0;

            enum t100_touch_event ev = msg.data[0] & 0xF;
            uint16_t x_pos = msg.data[1] + (msg.data[2] << 8);
            uint16_t y_pos = msg.data[3] + (msg.data[4] << 8);

            LOG_INF("Touch event: ev=%d, finger=%d, X=%d, Y=%d", ev, finger_idx, x_pos, y_pos);

            if (finger_idx < 5) {
                if (ev == DOWN) {
                    data->finger_active[finger_idx] = true;
                    data->prev_x[finger_idx] = x_pos;
                    data->prev_y[finger_idx] = y_pos;
                } else if (ev == MOVE && data->finger_active[finger_idx]) {
                    int16_t dx = (int16_t)x_pos - data->prev_x[finger_idx];
                    int16_t dy = (int16_t)y_pos - data->prev_y[finger_idx];
                    data->prev_x[finger_idx] = x_pos;
                    data->prev_y[finger_idx] = y_pos;
                    if (dx != 0 || dy != 0) {
                        input_report_rel(dev, INPUT_REL_X, dx, false, K_NO_WAIT);
                        input_report_rel(dev, INPUT_REL_Y, dy, true, K_NO_WAIT);
                    }
                } else if (ev == UP) {
                    data->finger_active[finger_idx] = false;
                }
            }

            switch (ev) {
            case DOWN:
            case MOVE:
            case UP:
            case NO_EVENT:
                if (pending_for_finger) {
                    input_report_key(dev, INPUT_BTN_TOUCH, last_touch_status, true, K_FOREVER);
                    pending_fingers = 0;
                }
                WRITE_BIT(pending_fingers, finger_idx, 1);
                last_touch_status = (ev != UP);
                input_report_abs(dev, INPUT_ABS_MT_SLOT, finger_idx, false, K_FOREVER);
                input_report_abs(dev, INPUT_ABS_X, x_pos, false, K_FOREVER);
                input_report_abs(dev, INPUT_ABS_Y, y_pos, false, K_FOREVER);
                input_report_key(dev, INPUT_BTN_TOUCH, last_touch_status, false, K_FOREVER);
                break;
            default:
                break;
            }
        }
    }

    if (pending_fingers != 0) {
        input_report_key(dev, INPUT_BTN_TOUCH, last_touch_status, true, K_FOREVER);
    }

    return;
}

static void mxt_work_cb(struct k_work *work) {
    struct mxt_data *data = CONTAINER_OF(work, struct mxt_data, work);
    const struct mxt_config *config = data->dev->config;

    LOG_INF("mxt_work_cb triggered, CHG pin level=%d", gpio_pin_get_dt(&config->chg));

    int retries = 50;
    do {
        mxt_report_data(data->dev);
    } while (gpio_pin_get_dt(&config->chg) == 1 && --retries > 0);
}

static void mxt_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    struct mxt_data *data = CONTAINER_OF(cb, struct mxt_data, gpio_cb);
    LOG_DBG("CHG interrupt triggered!");
    k_work_submit(&data->work);
}

static int mxt_load_object_table(const struct device *dev, struct mxt_information_block *info) {
    struct mxt_data *data = dev->data;
    int ret = 0;

    ret = mxt_seq_read(dev, MXT_REG_INFORMATION_BLOCK, info, sizeof(struct mxt_information_block));

    if (ret < 0) {
        LOG_ERR("Failed to load the info block: %d", ret);
        return ret;
    }

    LOG_HEXDUMP_DBG(info, sizeof(struct mxt_information_block), "info block");
    LOG_DBG("Found a maXTouch: family %d, variant %d, version %d. Matrix size: "
            "%d/%d and num of objects %d",
            info->family_id, info->variant_id, info->version, info->matrix_x_size,
            info->matrix_y_size, info->num_objects);

    uint8_t report_id = 1;
    uint16_t object_addr =
        sizeof(struct mxt_information_block); // Object table starts after the info block
    for (int i = 0; i < info->num_objects; i++) {
        struct mxt_object_table_element obj_table;

        ret = mxt_seq_read(dev, object_addr, &obj_table, sizeof(obj_table));
        if (ret < 0) {
            LOG_ERR("Failed to load object table %d: %d", i, ret);
            return ret;
        }

        uint16_t addr = sys_le16_to_cpu(obj_table.position);
        LOG_DBG("Obj %d: Type T%d at 0x%04x (size %d, reports %d)", i, obj_table.type, addr,
                obj_table.size_minus_one + 1, obj_table.report_ids_per_instance);

        switch (obj_table.type) {
        case 2:
            data->t2_encryption_status_address = addr;
            break;
        case 5:
            data->t5_message_processor_address = addr;
            data->t5_max_message_size = obj_table.size_minus_one + 1;
            break;
        case 6:
            data->t6_command_processor_address = addr;
            data->t6_command_processor_report_id = report_id;
            break;
        case 7:
            data->t7_powerconfig_address = addr;
            break;
        case 8:
            data->t8_acquisitionconfig_address = addr;
            break;
        case 25:
            data->t25_self_test_address = addr;
            data->t25_self_test_report_id = report_id;
            break;
        case 37:
            data->t37_diagnostic_debug_address = addr;
            break;
        case 42:
            data->t42_proci_touchsupression_address = addr;
            break;
        case 44:
            data->t44_message_count_address = addr;
            break;
        case 46:
            data->t46_cte_config_address = addr;
            break;
        case 47:
            data->t47_proci_stylus_address = addr;
            break;
        case 56:
            data->t56_proci_shieldless_address = addr;
            break;
        case 65:
            data->t65_proci_lensbending_address = addr;
            break;
        case 80:
            data->t80_proci_retransmissioncompensation_address = addr;
            break;
        case 100:
            data->t100_multiple_touch_touchscreen_address = addr;
            data->t100_first_report_id = report_id;
            break;
        }

        object_addr += sizeof(obj_table);
        report_id += obj_table.report_ids_per_instance * (obj_table.instances_minus_one + 1);
    }

    return 0;
};

static int mxt_load_config(const struct device *dev,
                           const struct mxt_information_block *information) {
    struct mxt_data *data = dev->data;
    const struct mxt_config *config = dev->config;
    int ret;

    if (data->t7_powerconfig_address) {
        struct mxt_gen_powerconfig_t7 t7_conf = {0};
        ret = mxt_seq_read(dev, data->t7_powerconfig_address, &t7_conf, sizeof(t7_conf));
        if (ret == 0) {
            t7_conf.idleacqint = config->idle_acq_time;
            t7_conf.actacqint = config->active_acq_time;
            t7_conf.actv2idleto = config->active_to_idle_timeout;
            t7_conf.cfg |= (MXT_T7_CFG_ACTVPIPEEN | MXT_T7_CFG_IDLEPIPEEN | MXT_T7_CFG_INITACTV);

            ret = mxt_seq_write(dev, data->t7_powerconfig_address, &t7_conf, sizeof(t7_conf));
            if (ret < 0) {
                LOG_ERR("Failed to set T7 config: %d", ret);
                return ret;
            }
        }
    }

    if (data->t8_acquisitionconfig_address) {
        struct mxt_gen_acquisitionconfig_t8 t8_conf = {0};
        ret = mxt_seq_read(dev, data->t8_acquisitionconfig_address, &t8_conf, sizeof(t8_conf));
        if (ret == 0) {
            t8_conf.chrgtime = config->charge_time;
            ret = mxt_seq_write(dev, data->t8_acquisitionconfig_address, &t8_conf, sizeof(t8_conf));
            if (ret < 0) {
                LOG_ERR("Failed to set T8 config: %d", ret);
                return ret;
            }
        }
    }

#ifdef MXT_ENABLE_STYLUS
    if (data->t42_proci_touchsupression_address) {
        struct mxt_proci_touchsupression_t42 t42_conf = {};

        t42_conf.ctrl = MXT_T42_CTRL_ENABLE | MXT_T42_CTRL_SHAPEEN;
        t42_conf.maxapprarea = 0;   // Default (0): suppress any touch that approaches >40 channels.
        t42_conf.maxtcharea = 0;    // Default (0): suppress any touch that covers >35 channels.
        t42_conf.maxnumtchs = 6;    // Suppress all touches if >6 are detected.
        t42_conf.supdist = 0;       // Default (0): Suppress all touches within 5 nodes of a suppressed large object detection.
        t42_conf.disthyst = 0;
        t42_conf.supstrength = 0;   // Default (0): suppression strength of 128.
        t42_conf.supextto = 0;      // Timeout to save power; set to 0 to disable.
        t42_conf.shapestrength = 0; // Default (0): shape suppression strength of 10, range [0, 31].
        t42_conf.maxscrnarea = 0;
        t42_conf.edgesupstrength = 0;
        t42_conf.cfg = 1;

        ret = mxt_seq_write(dev, data->t42_proci_touchsupression_address, &t42_conf, sizeof(t42_conf));
        if (ret < 0) {
            LOG_ERR("Failed to set T42 config: %d", ret);
            return ret;
        }
    }
#endif

    // Preserve factory Mutual Capacitive Touch Engine (CTE) configuration (drive voltages, syncs, timings)
    if (data->t46_cte_config_address) {
        struct mxt_spt_cteconfig_t46 t46_conf = {};
        ret = mxt_seq_read(dev, data->t46_cte_config_address, &t46_conf, sizeof(t46_conf));
        if (ret == 0) {
            LOG_INF("Factory T46 CTE: xvoltage=%d, syncdelay=%d, activesyncsperx=%d",
                    t46_conf.xvoltage, sys_le16_to_cpu(t46_conf.syncdelay), t46_conf.activesyncsperx);
        }
    }

    if (data->t80_proci_retransmissioncompensation_address) {
        struct mxt_proci_retransmissioncompensation_t80 t80_conf = {};
        ret = mxt_seq_read(dev, data->t80_proci_retransmissioncompensation_address, &t80_conf, sizeof(t80_conf));
        if (ret == 0) {
            t80_conf.ctrl = (config->retransmission_compensation_disable == false);
            ret = mxt_seq_write(dev, data->t80_proci_retransmissioncompensation_address, &t80_conf, sizeof(t80_conf));
        }
    }

    if (data->t100_multiple_touch_touchscreen_address) {
        struct mxt_touch_multiscreen_t100 t100_conf = {0};

        ret = mxt_seq_read(dev, data->t100_multiple_touch_touchscreen_address, &t100_conf,
                           sizeof(t100_conf));
        if (ret < 0) {
            LOG_ERR("Failed to load the initial T100 config: %d", ret);
            return ret;
        }

        LOG_INF("Factory T100: ctrl=0x%02x, gain=%d, tchthr=%d, xrange=%d, yrange=%d",
                t100_conf.ctrl, t100_conf.gain, t100_conf.tchthr,
                sys_le16_to_cpu(t100_conf.xrange), sys_le16_to_cpu(t100_conf.yrange));

        t100_conf.ctrl =
            MXT_T100_CTRL_RPTEN | MXT_T100_CTRL_ENABLE | MXT_T100_CTRL_SCANEN;

        uint8_t cfg1 = 0;
        if (config->repeat_each_cycle) {
            cfg1 |= MXT_T100_CFG_RPTEACHCYCLE;
        }
        if (config->swap_xy) {
            cfg1 |= MXT_T100_CFG_SWITCHXY;
        }
        if (config->invert_x) {
            cfg1 |= MXT_T100_CFG_INVERTX;
        }
        if (config->invert_y) {
            cfg1 |= MXT_T100_CFG_INVERTY;
        }
        t100_conf.cfg1 = cfg1;
        t100_conf.scraux = 0x7;
        t100_conf.numtch = config->max_touch_points;
        t100_conf.xsize = information->matrix_x_size;
        t100_conf.ysize = information->matrix_y_size;

        if (config->sensor_width > 0 && information->matrix_x_size > 0) {
            t100_conf.xpitch = (config->sensor_width * 10 / information->matrix_x_size);
        }
        if (config->sensor_height > 0 && information->matrix_y_size > 0) {
            t100_conf.ypitch = (config->sensor_height * 10 / information->matrix_y_size);
        }

        uint16_t logical_x = config->sensor_width * 10;
        uint16_t logical_y = config->sensor_height * 10;

        if (config->swap_xy) {
            t100_conf.xrange = sys_cpu_to_le16(logical_y - 1);
            t100_conf.yrange = sys_cpu_to_le16(logical_x - 1);
        } else {
            t100_conf.xrange = sys_cpu_to_le16(logical_x - 1);
            t100_conf.yrange = sys_cpu_to_le16(logical_y - 1);
        }

        ret = mxt_seq_write(dev, data->t100_multiple_touch_touchscreen_address, &t100_conf,
                            sizeof(t100_conf));
        if (ret < 0) {
            LOG_ERR("Failed to set T100 config: %d", ret);
            return ret;
        }
    }

    if (data->t6_command_processor_address) {
        uint8_t cal = 1;
        ret = mxt_seq_write(dev, data->t6_command_processor_address + 2, &cal, 1);
        if (ret < 0) {
            LOG_ERR("Failed to send T6 calibrate command: %d", ret);
        } else {
            LOG_INF("Sent T6 calibrate command successfully");
        }
    }

    return 0;
}

static int mxt_init(const struct device *dev) {
    struct mxt_data *data = dev->data;
    const struct mxt_config *config = dev->config;

    int ret;

    data->dev = dev;

    if (!i2c_is_ready_dt(&config->bus)) {
        LOG_ERR("i2c bus isn't ready!");
        return -EIO;
    };

    struct mxt_information_block info = {0};
    ret = mxt_load_object_table(dev, &info);
    if (ret < 0) {
        LOG_ERR("Failed to load the ojbect table: %d", ret);
        return -EIO;
    }

    gpio_pin_configure_dt(&config->chg, GPIO_INPUT);
    gpio_init_callback(&data->gpio_cb, mxt_gpio_cb, BIT(config->chg.pin));
    ret = gpio_add_callback(config->chg.port, &data->gpio_cb);
    if (ret < 0) {
        LOG_ERR("Failed to set DR callback: %d", ret);
        return -EIO;
    }

    k_work_init(&data->work, mxt_work_cb);

    ret = gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure interrupt for CHG pin %d", ret);
        return -EIO;
    }

    LOG_INF("CHG pin logical level at init: %d (port=%s, pin=%d)",
            gpio_pin_get_dt(&config->chg), config->chg.port->name, config->chg.pin);

    ret = mxt_load_config(dev, &info);
    if (ret < 0) {
        LOG_ERR("Failed to load default config: %d", ret);
        return -EIO;
    }

    // Give calibration 100ms to complete, then drain all messages until CHG is released
    k_msleep(100);
    int drain_retries = 50;
    while (gpio_pin_get_dt(&config->chg) == 1 && --drain_retries > 0) {
        mxt_report_data(dev);
    }

    LOG_INF("CHG pin logical level after calibration & drain: %d", gpio_pin_get_dt(&config->chg));

    return 0;
}

#define MXT_INST(n)                                                                                     \
    static struct mxt_data mxt_data_##n;                                                                \
    static const struct mxt_config mxt_config_##n = {                                                   \
        .bus = I2C_DT_SPEC_INST_GET(n),                                                                 \
        .chg = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(n), chg_gpios, {}),                                      \
        .max_touch_points = DT_INST_PROP_OR(n, max_touch_points, 5),                                    \
        .idle_acq_time = DT_INST_PROP_OR(n, idle_acq_time_ms, 32),                                      \
        .active_acq_time = DT_INST_PROP_OR(n, active_acq_time_ms, 10),                                  \
        .active_to_idle_timeout = DT_INST_PROP_OR(n, active_to_idle_timeout_ms, 50),                    \
        .repeat_each_cycle = DT_INST_PROP(n, repeat_each_cycle),                                        \
        .swap_xy = DT_INST_PROP(n, swap_xy),                                                            \
        .invert_x = DT_INST_PROP(n, invert_x),                                                          \
        .invert_y = DT_INST_PROP(n, invert_y),                                                          \
        .sensor_width = DT_INST_PROP(n, sensor_width),                                                  \
        .sensor_height = DT_INST_PROP(n, sensor_height),                                                \
        .touch_threshold = DT_INST_PROP_OR(n, touch_threshold, 18),                                     \
        .touch_hysteresis = DT_INST_PROP_OR(n, touch_hysteresis, 8),                                    \
        .internal_touch_threshold = DT_INST_PROP_OR(n, internal_touch_threshold, 10),                   \
        .internal_touch_hysteresis = DT_INST_PROP_OR(n, internal_touch_hysteresis, 4),                  \
        .gain = DT_INST_PROP_OR(n, gain, 4),                                                            \
        .charge_time = DT_INST_PROP_OR(n, charge_time, 10),                                             \
        .allowed_measurement_types = DT_INST_PROP_OR(n, allowed_measurement_types, 3),                  \
        .active_syncs_per_x = DT_INST_PROP_OR(n, active_syncs_per_x, 20),                               \
        .idle_syncs_per_x = DT_INST_PROP_OR(n, idle_syncs_per_x, 20),                                   \
        .retransmission_compensation_disable = DT_INST_PROP(n, retransmission_compensation_disable),    \
    };                                                                                                  \
    DEVICE_DT_INST_DEFINE(n, mxt_init, NULL, &mxt_data_##n, &mxt_config_##n, POST_KERNEL,               \
                          CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MXT_INST)
