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

static int mxt_seq_write(const struct device *dev, const uint16_t addr, const void *buf,
                         const uint8_t len) {
    const struct mxt_config *config = dev->config;
    uint8_t tx_buf[128];

    if (len + 2 > sizeof(tx_buf)) {
        LOG_ERR("Write length %d exceeds buffer size", len);
        return -EINVAL;
    }

    const uint16_t addr_lsb = sys_cpu_to_le16(addr);
    memcpy(&tx_buf[0], &addr_lsb, 2);
    memcpy(&tx_buf[2], buf, len);

    return i2c_write_dt(&config->bus, tx_buf, len + 2);
}

static inline bool is_t100_report(const struct device *dev, int report_id) {
    const struct mxt_config *config = dev->config;
    struct mxt_data *data = dev->data;

    return (report_id >= data->t100_first_report_id + 2 &&
            report_id < data->t100_first_report_id + 2 + config->max_touch_points);
}

static void mxt_proc_message(const struct device *dev, const struct mxt_message *msg,
                             uint16_t *pending_fingers, bool *last_touch_status) {
    struct mxt_data *data = dev->data;

    if (msg->report_id == data->t6_command_processor_report_id) {
        uint8_t status = msg->data[0];
        LOG_INF("T6 Status Report: 0x%02x%s%s%s%s%s%s%s",
                status,
                status == 0 ? " OK" : "",
                (status & MXT_T6_STATUS_RESET) ? " RESET" : "",
                (status & MXT_T6_STATUS_OFL) ? " OFL" : "",
                (status & MXT_T6_STATUS_SIGERR) ? " SIGERR" : "",
                (status & MXT_T6_STATUS_CAL) ? " CAL" : "",
                (status & MXT_T6_STATUS_CFGERR) ? " CFGERR" : "",
                (status & MXT_T6_STATUS_COMSERR) ? " COMSERR" : "");
        return;
    }

    if (is_t100_report(dev, msg->report_id)) {
        uint8_t finger_idx = msg->report_id - data->t100_first_report_id - 2;
        bool pending_for_finger = (*pending_fingers & BIT(finger_idx)) != 0;

        uint8_t status = msg->data[0];
        bool detect = (status & MXT_T100_DETECT) != 0;
        uint8_t type = (status & MXT_T100_TYPE_MASK) >> 4;
        uint16_t x_pos = msg->data[1] | (msg->data[2] << 8);
        uint16_t y_pos = msg->data[3] | (msg->data[4] << 8);

        LOG_INF("T100 touch: finger=%d, detect=%d, type=%d, X=%d, Y=%d (status=0x%02x)",
                finger_idx, detect, type, x_pos, y_pos, status);

        if (finger_idx < 5) {
            if (detect) {
                if (!data->finger_active[finger_idx]) {
                    data->finger_active[finger_idx] = true;
                    data->prev_x[finger_idx] = x_pos;
                    data->prev_y[finger_idx] = y_pos;
                    LOG_INF("Finger %d DOWN at (%d, %d)", finger_idx, x_pos, y_pos);
                } else {
                    int16_t dx = (int16_t)x_pos - data->prev_x[finger_idx];
                    int16_t dy = (int16_t)y_pos - data->prev_y[finger_idx];
                    data->prev_x[finger_idx] = x_pos;
                    data->prev_y[finger_idx] = y_pos;
                    if (dx != 0 || dy != 0) {
                        input_report_rel(dev, INPUT_REL_X, dx, false, K_NO_WAIT);
                        input_report_rel(dev, INPUT_REL_Y, dy, true, K_NO_WAIT);
                    }
                }
            } else {
                if (data->finger_active[finger_idx]) {
                    data->finger_active[finger_idx] = false;
                    LOG_INF("Finger %d UP at (%d, %d)", finger_idx, x_pos, y_pos);
                }
            }
        }

        if (pending_for_finger) {
            input_report_key(dev, INPUT_BTN_TOUCH, *last_touch_status, true, K_FOREVER);
            *pending_fingers = 0;
        }
        WRITE_BIT(*pending_fingers, finger_idx, 1);
        *last_touch_status = detect;
        input_report_abs(dev, INPUT_ABS_MT_SLOT, finger_idx, false, K_FOREVER);
        input_report_abs(dev, INPUT_ABS_X, x_pos, false, K_FOREVER);
        input_report_abs(dev, INPUT_ABS_Y, y_pos, false, K_FOREVER);
        input_report_key(dev, INPUT_BTN_TOUCH, *last_touch_status, false, K_FOREVER);
    } else {
        LOG_DBG("Other report: rpt_id=%d [0x%02x 0x%02x]",
                msg->report_id, msg->data[0], msg->data[1]);
    }
}

static int mxt_report_data(const struct device *dev) {
    struct mxt_data *data = dev->data;
    int ret;

    if (!data->t44_message_count_address || !data->t5_message_processor_address) {
        return 0;
    }

    uint8_t read_len = (data->t5_max_message_size > 0 &&
                        data->t5_max_message_size <= sizeof(struct mxt_message))
                           ? data->t5_max_message_size
                           : 11;

    uint8_t buf[1 + sizeof(struct mxt_message)] = {0};
    ret = mxt_seq_read(dev, data->t44_message_count_address, buf, 1 + read_len);
    if (ret < 0) {
        LOG_ERR("Failed to read T44: %d", ret);
        return 0;
    }

    uint8_t count = buf[0];
    struct mxt_message msg = {0};
    memcpy(&msg, &buf[1], read_len);

    if (count == 0 || msg.report_id == 0xFF || msg.report_id == 0x00) {
        return 0;
    }

    uint16_t pending_fingers = 0;
    bool last_touch_status = false;
    int messages_processed = 0;

    mxt_proc_message(dev, &msg, &pending_fingers, &last_touch_status);
    messages_processed++;

    uint8_t num_left = (count > 1) ? (count - 1) : 0;
    if (num_left > 15) {
        num_left = 15;
    }

    for (uint8_t m = 0; m < num_left; m++) {
        struct mxt_message rem_msg = {0};
        ret = mxt_seq_read(dev, data->t5_message_processor_address, &rem_msg, read_len);
        if (ret < 0 || rem_msg.report_id == 0xFF || rem_msg.report_id == 0x00) {
            break;
        }
        mxt_proc_message(dev, &rem_msg, &pending_fingers, &last_touch_status);
        messages_processed++;
    }

    if (pending_fingers != 0) {
        input_report_key(dev, INPUT_BTN_TOUCH, last_touch_status, true, K_FOREVER);
    }

    return messages_processed;
}

static void mxt_work_cb(struct k_work *work) {
    struct mxt_data *data = CONTAINER_OF(work, struct mxt_data, work);
    const struct mxt_config *config = data->dev->config;

    LOG_INF("mxt_work_cb triggered, CHG=%d", gpio_pin_get_dt(&config->chg));

    int retries = 20;
    while (retries-- > 0) {
        if (mxt_report_data(data->dev) == 0) {
            break;
        }
        if (gpio_pin_get_dt(&config->chg) == 0) {
            break;
        }
    }

    if (gpio_pin_get_dt(&config->chg) == 1) {
        k_work_submit(&data->work);
    } else {
        /* Re-enable level interrupt once CHG line is de-asserted (idle) */
        gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_LEVEL_ACTIVE);
    }
}

static void mxt_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    struct mxt_data *data = CONTAINER_OF(cb, struct mxt_data, gpio_cb);
    const struct mxt_config *config = data->dev->config;

    /* Disable level interrupt while worker thread drains T5 messages */
    gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_DISABLE);
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
        LOG_DBG("Obj %d: Type T%d at 0x%04x (size %d, instances %d, reports %d)", i, obj_table.type, addr,
                obj_table.size_minus_one + 1, obj_table.instances_minus_one + 1, obj_table.report_ids_per_instance);

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
        case 51:
            data->t51_custom_address = addr;
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
            data->t100_size = obj_table.size_minus_one + 1;
            break;
        default:
            LOG_DBG("Unimplemented object type: %d", obj_table.type);
            break;
        }

        object_addr += sizeof(obj_table);
        report_id += obj_table.report_ids_per_instance * (obj_table.instances_minus_one + 1);
    }

    return 0;
}

static int mxt_load_config(const struct device *dev) {
    struct mxt_data *data = dev->data;
    const struct mxt_config *config = dev->config;
    int ret;

    if (data->t7_powerconfig_address && config->has_t7_config) {
        struct mxt_gen_powerconfig_t7 t7_conf = {0};
        ret = mxt_seq_read(dev, data->t7_powerconfig_address, &t7_conf, sizeof(t7_conf));
        if (ret == 0) {
            if (config->idle_acq_time > 0) {
                t7_conf.idleacqint = config->idle_acq_time;
            }
            if (config->active_acq_time > 0) {
                t7_conf.actacqint = config->active_acq_time;
            }
            if (config->active_to_idle_timeout > 0) {
                t7_conf.actv2idleto = config->active_to_idle_timeout;
            }
            t7_conf.cfg |= (MXT_T7_CFG_ACTVPIPEEN | MXT_T7_CFG_IDLEPIPEEN);

            ret = mxt_seq_write(dev, data->t7_powerconfig_address, &t7_conf, sizeof(t7_conf));
            if (ret < 0) {
                LOG_ERR("Failed to set T7 config: %d", ret);
                return ret;
            }
        }
    }

    if (data->t8_acquisitionconfig_address && config->has_charge_time) {
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
        struct mxt_proci_touchsupression_t42 t42_conf = {0};

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

    // Unlock T44/T5 FIFO readout if T51 is present (e.g., mXT144U at 0x04F9 + offset 8 = 0x0501)
    if (data->t51_custom_address) {
        uint16_t t51_unlock_addr = data->t51_custom_address + 8;
        uint8_t unlock_val = 0xFF;
        ret = mxt_seq_write(dev, t51_unlock_addr, &unlock_val, 1);
        if (ret < 0) {
            LOG_ERR("Failed to write T51+8 (0x%04x): %d", t51_unlock_addr, ret);
            return ret;
        }
        LOG_INF("Unlocked T44/T5 message reporting via T51+8 (0x%04x = 0x%02x)", t51_unlock_addr, unlock_val);
    }

    if (data->t100_multiple_touch_touchscreen_address) {
        struct mxt_touch_multiscreen_t100 t100_conf = {0};
        uint8_t t100_len = (data->t100_size > 0 && data->t100_size <= sizeof(t100_conf))
                               ? data->t100_size
                               : 62;

        ret = mxt_seq_read(dev, data->t100_multiple_touch_touchscreen_address, &t100_conf, t100_len);
        if (ret < 0) {
            LOG_ERR("Failed to load initial T100 config: %d", ret);
            return ret;
        }

        bool t100_modified = false;

        // Ensure touch scanning, report generation, and object enable are active
        uint8_t req_ctrl = MXT_T100_CTRL_ENABLE | MXT_T100_CTRL_RPTEN | MXT_T100_CTRL_SCANEN;
        if ((t100_conf.ctrl & req_ctrl) != req_ctrl) {
            t100_conf.ctrl |= req_ctrl;
            t100_modified = true;
        }

        uint8_t desired_numtch = (config->max_touch_points > 0) ? config->max_touch_points : 5;
        if (t100_conf.numtch != desired_numtch) {
            t100_conf.numtch = desired_numtch;
            t100_modified = true;
        }

        if ((t100_conf.tchaux & 0x01) == 0) {
            t100_conf.tchaux |= 0x01;
            t100_modified = true;
        }

        if (config->repeat_each_cycle && !(t100_conf.cfg1 & MXT_T100_CFG_RPTEACHCYCLE)) {
            t100_conf.cfg1 |= MXT_T100_CFG_RPTEACHCYCLE;
            t100_modified = true;
        }
        if (config->swap_xy && !(t100_conf.cfg1 & MXT_T100_CFG_SWITCHXY)) {
            t100_conf.cfg1 |= MXT_T100_CFG_SWITCHXY;
            t100_modified = true;
        }
        if (config->invert_x && !(t100_conf.cfg1 & MXT_T100_CFG_INVERTX)) {
            t100_conf.cfg1 |= MXT_T100_CFG_INVERTX;
            t100_modified = true;
        }
        if (config->invert_y && !(t100_conf.cfg1 & MXT_T100_CFG_INVERTY)) {
            t100_conf.cfg1 |= MXT_T100_CFG_INVERTY;
            t100_modified = true;
        }

        if (t100_modified) {
            ret = mxt_seq_write(dev, data->t100_multiple_touch_touchscreen_address, &t100_conf, t100_len);
            if (ret < 0) {
                LOG_ERR("Failed to set T100 config: %d", ret);
                return ret;
            }
        }
    }

    if (data->t6_command_processor_address) {
        uint8_t cal = 1;
        ret = mxt_seq_write(dev, data->t6_command_processor_address + 2, &cal, 1);
        if (ret < 0) {
            LOG_ERR("Failed to send T6 calibrate command: %d", ret);
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
    }

    struct mxt_information_block info = {0};
    ret = mxt_load_object_table(dev, &info);
    if (ret < 0) {
        LOG_ERR("Failed to load the object table: %d", ret);
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

    /* Hold CHG interrupt disabled until T51 unlock and calibration messages are drained */
    ret = gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_DISABLE);
    if (ret < 0) {
        LOG_ERR("Failed to configure interrupt for CHG pin %d", ret);
        return -EIO;
    }

    ret = mxt_load_config(dev);
    if (ret < 0) {
        LOG_ERR("Failed to load default config: %d", ret);
        return -EIO;
    }

    /* Wait for T6 calibration to complete and drain initial T44/T5 messages so CHG returns to idle */
    if (data->t6_command_processor_address) {
        int cal_wait_ms = 300;
        while (cal_wait_ms > 0) {
            k_msleep(20);
            cal_wait_ms -= 20;

            int processed = mxt_report_data(dev);
            if (gpio_pin_get_dt(&config->chg) == 0 && (processed > 0 || cal_wait_ms <= 160)) {
                break;
            }
        }
    }

    ret = gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_LEVEL_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to enable level interrupt for CHG pin %d", ret);
        return -EIO;
    }

    if (gpio_pin_get_dt(&config->chg) != 0) {
        gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_DISABLE);
        k_work_submit(&data->work);
    }

    return 0;
}

#define MXT_INST(n)                                                                                     \
    static struct mxt_data mxt_data_##n;                                                                \
    static const struct mxt_config mxt_config_##n = {                                                   \
        .bus = I2C_DT_SPEC_INST_GET(n),                                                                 \
        .chg = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(n), chg_gpios, {}),                                      \
        .max_touch_points = DT_INST_PROP_OR(n, max_touch_points, 5),                                    \
        .idle_acq_time = DT_INST_PROP_OR(n, idle_acq_time_ms, 0),                                       \
        .active_acq_time = DT_INST_PROP_OR(n, active_acq_time_ms, 0),                                   \
        .active_to_idle_timeout = DT_INST_PROP_OR(n, active_to_idle_timeout_ms, 0),                     \
        .has_t7_config = DT_INST_NODE_HAS_PROP(n, idle_acq_time_ms) ||                                  \
                         DT_INST_NODE_HAS_PROP(n, active_acq_time_ms) ||                                \
                         DT_INST_NODE_HAS_PROP(n, active_to_idle_timeout_ms),                           \
        .repeat_each_cycle = DT_INST_PROP(n, repeat_each_cycle),                                        \
        .swap_xy = DT_INST_PROP(n, swap_xy),                                                            \
        .invert_x = DT_INST_PROP(n, invert_x),                                                          \
        .invert_y = DT_INST_PROP(n, invert_y),                                                          \
        .sensor_width = DT_INST_PROP(n, sensor_width),                                                  \
        .sensor_height = DT_INST_PROP_OR(n, sensor_height, 0),                                          \
        .touch_threshold = DT_INST_PROP_OR(n, touch_threshold, 18),                                     \
        .touch_hysteresis = DT_INST_PROP_OR(n, touch_hysteresis, 8),                                    \
        .internal_touch_threshold = DT_INST_PROP_OR(n, internal_touch_threshold, 10),                   \
        .internal_touch_hysteresis = DT_INST_PROP_OR(n, internal_touch_hysteresis, 4),                  \
        .gain = DT_INST_PROP_OR(n, gain, 4),                                                            \
        .charge_time = DT_INST_PROP_OR(n, charge_time, 0),                                              \
        .has_charge_time = DT_INST_NODE_HAS_PROP(n, charge_time),                                       \
        .allowed_measurement_types = DT_INST_PROP_OR(n, allowed_measurement_types, 3),                  \
        .active_syncs_per_x = DT_INST_PROP_OR(n, active_syncs_per_x, 20),                               \
        .idle_syncs_per_x = DT_INST_PROP_OR(n, idle_syncs_per_x, 20),                                   \
        .retransmission_compensation_disable = DT_INST_PROP(n, retransmission_compensation_disable),    \
    };                                                                                                  \
    DEVICE_DT_INST_DEFINE(n, mxt_init, NULL, &mxt_data_##n, &mxt_config_##n, POST_KERNEL,               \
                          CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MXT_INST)
