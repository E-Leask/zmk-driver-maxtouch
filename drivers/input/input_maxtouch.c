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

    LOG_INF("maxtouch msg: rpt_id=%d [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
            msg->report_id, msg->data[0], msg->data[1], msg->data[2], msg->data[3], msg->data[4], msg->data[5]);

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

    if (data->t25_self_test_report_id && msg->report_id == data->t25_self_test_report_id) {
        uint8_t status = msg->data[0];
        data->t25_status = status;
        data->t25_report_received = true;

        if (status == MXT_T25_STATUS_PASS) {
            LOG_INF("T25 Self Test Report: ALL TESTS PASSED (0xFE)");
        } else if (status == MXT_T25_STATUS_POWER_FAULT) {
            LOG_ERR("T25 Self Test Report: AVdd power NOT present (0x01)");
        } else if (status == MXT_T25_STATUS_PIN_FAULT) {
            uint8_t seq = msg->data[1];
            uint8_t x_pin = msg->data[2];
            uint8_t y_pin = msg->data[3];
            const char *seq_str = "Unknown";
            switch (seq) {
            case MXT_T25_SEQ_DRIVEN_GND:
                seq_str = "Driven Ground (shorts to power)";
                break;
            case MXT_T25_SEQ_DRIVEN_HIGH:
                seq_str = "Driven High (shorts to GND)";
                break;
            case MXT_T25_SEQ_WALKING_1:
                seq_str = "Walking 1 (resistive pull-up short)";
                break;
            case MXT_T25_SEQ_WALKING_0:
                seq_str = "Walking 0 (resistive pull-up short)";
                break;
            case MXT_T25_SEQ_HIGH_VOLTAGE:
                seq_str = "Initial High Voltage short";
                break;
            }

            if (x_pin == 0 && y_pin == 0) {
                LOG_ERR("T25 Self Test: PIN FAULT (seq=0x%02x [%s]): Driven shield failed",
                        seq, seq_str);
            } else if (x_pin != 0 && y_pin != 0) {
                LOG_ERR("T25 Self Test: PIN FAULT (seq=0x%02x [%s]): X%d (pin %d) and Y%d (pin %d) short",
                        seq, seq_str, x_pin - 1, x_pin, y_pin - 1, y_pin);
            } else if (x_pin != 0) {
                LOG_ERR("T25 Self Test: PIN FAULT (seq=0x%02x [%s]): X%d sense pin failed (pin %d)",
                        seq, seq_str, x_pin - 1, x_pin);
            } else {
                LOG_ERR("T25 Self Test: PIN FAULT (seq=0x%02x [%s]): Y%d sense pin failed (pin %d)",
                        seq, seq_str, y_pin - 1, y_pin);
            }
        } else if (status == MXT_T25_STATUS_SIGNAL_LIMIT) {
            uint8_t type_num = msg->data[1];
            uint8_t instance = msg->data[2];
            LOG_ERR("T25 Self Test: SIGNAL LIMIT FAULT (0x17) on Object T%d instance %d",
                    type_num, instance);
        } else if (status == MXT_T25_STATUS_INVALID) {
            LOG_ERR("T25 Self Test: Invalid command code (0xFD)");
        } else {
            LOG_WRN("T25 Self Test: Unknown status 0x%02x [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
                    status, msg->data[1], msg->data[2], msg->data[3], msg->data[4], msg->data[5]);
        }
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

        LOG_INF("T100 touch report: finger=%d, detect=%d, type=%d, X=%d, Y=%d (status=0x%02x)",
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
        LOG_DBG("Other report: rpt_id=%d [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
                msg->report_id, msg->data[0], msg->data[1], msg->data[2], msg->data[3], msg->data[4], msg->data[5]);
    }
}

static int mxt_report_data(const struct device *dev) {
    struct mxt_data *data = dev->data;
    int ret;

    if (!data->t5_message_processor_address) {
        LOG_WRN("No T5 message processor object found!");
        return 0;
    }

    uint16_t pending_fingers = 0;
    bool last_touch_status = false;

    int messages_processed = 0;
    uint8_t read_len = (data->t5_max_message_size > 0 && data->t5_max_message_size <= sizeof(struct mxt_message))
                           ? data->t5_max_message_size
                           : 11;
    LOG_INF("MXT: Reading T5 from 0x%04x, len=%d", data->t5_message_processor_address, read_len);
    if (data->t44_message_count_address) {
        uint8_t buf[1 + sizeof(struct mxt_message)] = {0};
        //TODO: Reveiw if this is valid, it technically is but this component is failing we might be messing somehting up here.
        ret = mxt_seq_read(dev, data->t44_message_count_address, buf, 1 + read_len);
        if (ret < 0) {
            LOG_ERR("Failed to read T44: %d", ret);
            return 0;
        }

        uint8_t count = buf[0];
        struct mxt_message msg = {0};
        memcpy(&msg, &buf[1], read_len);

        if (count == 0 || msg.report_id == 0xFF || msg.report_id == 0x00) {
            LOG_DBG("FIFO empty (T44 count=%d, rpt_id=%d)", count, msg.report_id);
            return 0;
        }

        LOG_INF("T44 read (12B from 0x%04x): count=%d, rpt_id=%d [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]",
                data->t44_message_count_address, count, msg.report_id,
                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
                buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);

        mxt_proc_message(dev, &msg, &pending_fingers, &last_touch_status);
        messages_processed++;

        // Read exactly the remaining messages reported by T44 (count - 1)
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
            LOG_INF("T5 msg %d/%d: rpt_id=%d [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
                    m + 2, count, rem_msg.report_id, rem_msg.data[0], rem_msg.data[1], rem_msg.data[2],
                    rem_msg.data[3], rem_msg.data[4], rem_msg.data[5]);
            mxt_proc_message(dev, &rem_msg, &pending_fingers, &last_touch_status);
            messages_processed++;
        }
    } else {
        for (int i = 0; i < 20; i++) {
            struct mxt_message msg = {0};
            ret = mxt_seq_read(dev, data->t5_message_processor_address, &msg, read_len);
            if (ret < 0) {
                LOG_ERR("Failed to read message from T5: %d", ret);
                break;
            }

            LOG_INF("T5 fallback read %d: rpt_id=%d [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
                    i, msg.report_id, msg.data[0], msg.data[1], msg.data[2],
                    msg.data[3], msg.data[4], msg.data[5]);

            if (msg.report_id == 0xFF || msg.report_id == 0x00) {
                if (i == 0 && data->t6_command_processor_address) {
                    uint8_t t6_status = 0;
                    if (mxt_seq_read(dev, data->t6_command_processor_address, &t6_status, 1) == 0) {
                        LOG_INF("T5 empty (0xFF). T6 Status: 0x%02x%s%s%s%s%s%s",
                                t6_status,
                                t6_status == 0 ? " OK" : "",
                                (t6_status & MXT_T6_STATUS_RESET) ? " RESET" : "",
                                (t6_status & MXT_T6_STATUS_OFL) ? " OFL" : "",
                                (t6_status & MXT_T6_STATUS_SIGERR) ? " SIGERR" : "",
                                (t6_status & MXT_T6_STATUS_CAL) ? " CAL" : "",
                                (t6_status & MXT_T6_STATUS_CFGERR) ? " CFGERR" : "");
                    }
                }
                break;
            }

            messages_processed++;
            mxt_proc_message(dev, &msg, &pending_fingers, &last_touch_status);
        }
    }

    if (pending_fingers != 0) {
        input_report_key(dev, INPUT_BTN_TOUCH, last_touch_status, true, K_FOREVER);
    }

    return messages_processed;
}

static void mxt_work_cb(struct k_work *work) {
    struct mxt_data *data = CONTAINER_OF(work, struct mxt_data, work);
    const struct mxt_config *config = data->dev->config;

    LOG_INF("mxt_work_cb triggered, CHG pin level=%d", gpio_pin_get_dt(&config->chg));

    int total_processed = 0;
    int retries = 50;
    while (--retries > 0) {
        int processed = mxt_report_data(data->dev);
        total_processed += processed;
        if (processed == 0) {
            break;
        }
        if (gpio_pin_get_dt(&config->chg) == 0) {
            break;
        }
    }

    if (gpio_pin_get_dt(&config->chg) == 1) {
        LOG_DBG("CHG line still asserted after work, re-queuing work");
        k_work_submit(&data->work);
    } else {
        /* Re-enable level interrupt once CHG line is de-asserted (idle) */
        gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_LEVEL_ACTIVE);
    }
}

static void mxt_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    struct mxt_data *data = CONTAINER_OF(cb, struct mxt_data, gpio_cb);
    const struct mxt_config *config = data->dev->config;

    LOG_DBG("CHG level interrupt triggered!");
    /* Disable level interrupt to prevent an ISR storm while worker thread drains T5 messages */
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
        LOG_DBG("Obj %d: Type T%d at 0x%04x (size %d, instances %d reports %d)", i, obj_table.type, addr,
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
        case 15:
            //TODO: implement T15 processing
            break;
        case 18:
            data->t18_comms_config_address = addr;
            break;
        case 25:
            data->t25_self_test_address = addr;
            data->t25_self_test_report_id = report_id;
            data->t25_size = obj_table.size_minus_one + 1;
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
};

static int mxt_load_config(const struct device *dev) {
    struct mxt_data *data = dev->data;
    const struct mxt_config *config = dev->config;
    int ret;

    if (data->t7_powerconfig_address) {
        struct mxt_gen_powerconfig_t7 t7_conf = {0};
        ret = mxt_seq_read(dev, data->t7_powerconfig_address, &t7_conf, sizeof(t7_conf));
        if (ret == 0) {
            LOG_INF("Factory T7 Power: idleacqint=%d, actacqint=%d, actv2idleto=%d, cfg=0x%02x, cfg2=0x%02x",
                    t7_conf.idleacqint, t7_conf.actacqint, t7_conf.actv2idleto, t7_conf.cfg, t7_conf.cfg2);

            if (config->has_t7_config) {
                if (config->idle_acq_time > 0) {
                    t7_conf.idleacqint = config->idle_acq_time;
                }
                if (config->active_acq_time > 0) {
                    t7_conf.actacqint = config->active_acq_time;
                }
                if (config->active_to_idle_timeout > 0) {
                    t7_conf.actv2idleto = config->active_to_idle_timeout;
                }
                t7_conf.cfg |= (MXT_T7_CFG_ACTVPIPEEN | MXT_T7_CFG_IDLEPIPEEN | MXT_T7_CFG_INITACTV);

                ret = mxt_seq_write(dev, data->t7_powerconfig_address, &t7_conf, sizeof(t7_conf));
                if (ret < 0) {
                    LOG_ERR("Failed to set T7 config: %d", ret);
                    return ret;
                }
                LOG_INF("Updated T7 Power config from DTS");
            } else {
                LOG_INF("Preserving factory T7 Power configuration");
            }
        }
    }

    if (data->t8_acquisitionconfig_address) {
        struct mxt_gen_acquisitionconfig_t8 t8_conf = {0};
        ret = mxt_seq_read(dev, data->t8_acquisitionconfig_address, &t8_conf, sizeof(t8_conf));
        if (ret == 0) {
            LOG_INF("Factory T8 Acquisition: chrgtime=%d, tchdrift=%d, driftst=%d, tchautocal=%d",
                    t8_conf.chrgtime, t8_conf.tchdrift, t8_conf.driftst, t8_conf.tchautocal);

            if (config->has_charge_time) {
                t8_conf.chrgtime = config->charge_time;
                ret = mxt_seq_write(dev, data->t8_acquisitionconfig_address, &t8_conf, sizeof(t8_conf));
                if (ret < 0) {
                    LOG_ERR("Failed to set T8 config: %d", ret);
                    return ret;
                }
                LOG_INF("Updated T8 charge time to %d from DTS", config->charge_time);
            } else {
                LOG_INF("Preserving factory T8 Acquisition configuration");
            }
        }
    }

    // Configure Communications Configuration (T18) for Mode 1 (level-triggered)
    if (data->t18_comms_config_address) {
        struct mxt_spt_commsconfig_t18 t18_conf = {0};
        ret = mxt_seq_read(dev, data->t18_comms_config_address, &t18_conf, sizeof(t18_conf));
        if (ret == 0) {
            uint8_t mode = t18_conf.ctrl & MXT_T18_CTRL_MODE_MASK;
            uint8_t retrigen = (t18_conf.ctrl & MXT_T18_CTRL_RETRIGEN) ? 1 : 0;
            LOG_INF("Factory T18 COMMSCONFIG (0x%04x): ctrl=0x%02x, cmd=0x%02x (CHG mode=%d [%s], retrigen=%d)",
                    data->t18_comms_config_address, t18_conf.ctrl, t18_conf.cmd,
                    mode, mode ? "Mode 1 (level)" : "Mode 0 (edge)", retrigen);

            // Per ATMXT datasheet: "The CHG line remains low as long as there are messages to be read.
            // The host should be configured so that the CHG line is connected to an interrupt line
            // that is level-triggered. The host should not use an edge-triggered interrupt as this
            // means adding extra software precautions."
            // Mode 1 (ctrl=0x01, level-triggered): CHG remains asserted (low) as long as unread
            // messages are in the T5 message queue, returning high only when all messages are drained.
            if (t18_conf.ctrl != 0x01) {
                t18_conf.ctrl = 0x01;
                ret = mxt_seq_write(dev, data->t18_comms_config_address, &t18_conf, sizeof(t18_conf));
                if (ret < 0) {
                    LOG_ERR("Failed to set T18 COMMSCONFIG: %d", ret);
                    return ret;
                }
                LOG_INF("Configured T18 COMMSCONFIG to 0x01 (Mode 1 / level-triggered)");
            } else {
                LOG_INF("Preserving factory T18 COMMSCONFIG Mode 1 (0x01)");
            }

            struct mxt_spt_commsconfig_t18 t18_verify = {0};
            ret = mxt_seq_read(dev, data->t18_comms_config_address, &t18_verify, sizeof(t18_verify));
            if (ret == 0) {
                LOG_INF("Verified T18 in chip SRAM: ctrl=0x%02x, cmd=0x%02x", t18_verify.ctrl, t18_verify.cmd);
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

    // Preserve factory Mutual Capacitive Touch Engine (CTE) configuration (drive voltages, syncs, timings)
    if (data->t46_cte_config_address) {
        struct mxt_spt_cteconfig_t46 t46_conf = {0};
        ret = mxt_seq_read(dev, data->t46_cte_config_address, &t46_conf, sizeof(t46_conf));
        if (ret == 0) {
            LOG_INF("Factory T46 CTE: xvoltage=%d, syncdelay=%d, activesyncsperx=%d",
                    t46_conf.xvoltage, sys_le16_to_cpu(t46_conf.syncdelay), t46_conf.activesyncsperx);
        }
    }

    if (data->t80_proci_retransmissioncompensation_address) {
        LOG_INF("Preserving factory T80 Retransmission Compensation configuration");
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

        LOG_HEXDUMP_INF(&t100_conf, t100_len, "Raw Factory T100");
        LOG_INF("Factory T100 (size %d): ctrl=0x%02x, cfg1=0x%02x, scraux=0x%02x, tchaux=0x%02x, "
                "numtch=%d, xycfg=0x%02x, gain=%d, tchthr=%d, xrange=%d, yrange=%d",
                t100_len, t100_conf.ctrl, t100_conf.cfg1, t100_conf.scraux, t100_conf.tchaux,
                t100_conf.numtch, t100_conf.xycfg, t100_conf.gain, t100_conf.tchthr,
                sys_le16_to_cpu(t100_conf.xrange), sys_le16_to_cpu(t100_conf.yrange));

        bool t100_modified = false;

        // Apply axis configuration from device tree if configured
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

            struct mxt_touch_multiscreen_t100 t100_verify = {0};
            ret = mxt_seq_read(dev, data->t100_multiple_touch_touchscreen_address, &t100_verify, t100_len);
            if (ret == 0) {
                LOG_INF("Verified T100 in chip SRAM: ctrl=0x%02x, cfg1=0x%02x, numtch=%d, tchthr=%d",
                        t100_verify.ctrl, t100_verify.cfg1, t100_verify.numtch, t100_verify.tchthr);
            }
        } else {
            LOG_INF("Preserving factory T100 touch configuration");
        }
    }

    if (data->t6_command_processor_address) {
        // Read initial T6 status
        uint8_t t6_status = 0;
        ret = mxt_seq_read(dev, data->t6_command_processor_address, &t6_status, 1);
        if (ret == 0) {
            LOG_INF("T6 Status before calibrate (0x%04x): 0x%02x%s%s%s%s%s%s%s",
                    data->t6_command_processor_address, t6_status,
                    t6_status == 0 ? " OK" : "",
                    (t6_status & MXT_T6_STATUS_RESET) ? " RESET" : "",
                    (t6_status & MXT_T6_STATUS_OFL) ? " OFL" : "",
                    (t6_status & MXT_T6_STATUS_SIGERR) ? " SIGERR" : "",
                    (t6_status & MXT_T6_STATUS_CAL) ? " CAL" : "",
                    (t6_status & MXT_T6_STATUS_CFGERR) ? " CFGERR" : "",
                    (t6_status & MXT_T6_STATUS_COMSERR) ? " COMSERR" : "");
        }

        uint8_t cal = 1;
        ret = mxt_seq_write(dev, data->t6_command_processor_address + 2, &cal, 1);
        if (ret < 0) {
            LOG_ERR("Failed to send T6 calibrate command: %d", ret);
        } else {
            LOG_INF("Sent T6 calibrate command successfully");
        }

        // Send REPORTALL command so all objects post their initial status reports into T5
        uint8_t reportall = 1;
        ret = mxt_seq_write(dev, data->t6_command_processor_address + 3, &reportall, 1);
        if (ret == 0) {
            LOG_INF("Sent T6 REPORTALL command successfully");
        }
    }

    return 0;
}

static int mxt_run_self_test(const struct device *dev) {
    struct mxt_data *data = dev->data;
    int ret;

    if (!data->t25_self_test_address) {
        LOG_INF("T25 Self Test object not found on device, skipping self test");
        return 0;
    }

    LOG_INF("Starting T25 Self Test at 0x%04x (size=%d, report_id=%d)...",
            data->t25_self_test_address, data->t25_size, data->t25_self_test_report_id);

    // 1. Read and log existing T25 object configuration
    uint8_t t25_raw[16] = {0};
    uint8_t read_sz = (data->t25_size > 0 && data->t25_size <= sizeof(t25_raw)) ? data->t25_size : sizeof(t25_raw);
    ret = mxt_seq_read(dev, data->t25_self_test_address, t25_raw, read_sz);
    if (ret == 0) {
        LOG_INF("T25 raw config (%d bytes): [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]",
                read_sz,
                t25_raw[0], t25_raw[1], t25_raw[2], t25_raw[3],
                t25_raw[4], t25_raw[5], t25_raw[6], t25_raw[7],
                t25_raw[8], t25_raw[9], t25_raw[10], t25_raw[11],
                t25_raw[12], t25_raw[13], t25_raw[14], t25_raw[15]);
        uint16_t upsig = t25_raw[2] | (t25_raw[3] << 8);
        uint16_t losig = t25_raw[4] | (t25_raw[5] << 8);
        uint16_t sigrange = t25_raw[7] | (t25_raw[8] << 8);
        LOG_INF("T25 parsed: ctrl=0x%02x, cmd=0x%02x, upsiglim=%u, losiglim=%u, pindwell=%u, sigrange=%u, pinthr=%u",
                t25_raw[0], t25_raw[1], upsig, losig, t25_raw[6], sigrange, t25_raw[9]);
    } else {
        LOG_ERR("Failed to read T25 configuration: %d", ret);
        return ret;
    }

    data->t25_report_received = false;
    data->t25_status = 0;

    // 2. Ensure T25 is enabled and reporting enabled, and trigger CMD 0xFE (all tests)
    // Write both CTRL (byte 0) and CMD (byte 1) starting at the T25 base address
    uint8_t ctrl = t25_raw[0] | MXT_T25_CTRL_ENABLE | MXT_T25_CTRL_RPTEN;
    uint8_t trigger_buf[2] = { ctrl, MXT_T25_TEST_ALL };

    ret = mxt_seq_write(dev, data->t25_self_test_address, trigger_buf, sizeof(trigger_buf));
    if (ret < 0) {
        LOG_ERR("Failed to write T25 trigger command: %d", ret);
        return ret;
    }

    // Immediate readback to check if command was accepted
    uint8_t readback[2] = {0};
    mxt_seq_read(dev, data->t25_self_test_address, readback, sizeof(readback));
    LOG_INF("T25 trigger written (ctrl=0x%02x, cmd=0x%02x) -> immediate readback: ctrl=0x%02x, cmd=0x%02x",
            trigger_buf[0], trigger_buf[1], readback[0], readback[1]);

    // 3. Poll for test completion and drain T5 messages
    int timeout_ms = 800;
    int cmd_cleared_wait_count = 0;
    bool completed = false;

    while (timeout_ms > 0) {
        k_msleep(20);
        timeout_ms -= 20;

        // Drain any messages from T5 (this will parse T25 report if ready)
        mxt_report_data(dev);

        if (data->t25_report_received) {
            completed = true;
            break;
        }

        // Check if CMD field has been cleared back to 0x00
        uint8_t cur_cmd = 0xFF;
        ret = mxt_seq_read(dev, data->t25_self_test_address + 1, &cur_cmd, 1);
        if (ret == 0 && cur_cmd == 0x00) {
            cmd_cleared_wait_count++;
            if (cmd_cleared_wait_count == 1) {
                LOG_INF("T25 CMD cleared to 0x00, waiting for T5 report message...");
            }
            // Allow up to 260ms (13 checks x 20ms) after CMD cleared for T5 message delivery
            if (cmd_cleared_wait_count >= 13) {
                completed = true;
                break;
            }
        }
    }

    if (!completed && !data->t25_report_received) {
        LOG_WRN("T25 Self Test timed out waiting for completion");
        return -ETIMEDOUT;
    }

    if (data->t25_report_received) {
        if (data->t25_status == MXT_T25_STATUS_PASS) {
            LOG_INF("T25 Self Test: Board reports NO issues! All tests passed (0xFE).");
            return 0;
        } else {
            LOG_ERR("T25 Self Test: Board reports issues (status=0x%02x)!", data->t25_status);
            return -EIO;
        }
    }

    LOG_INF("T25 Self Test completed: PASSED! (CMD cleared to 0x00, no faults detected, T6 status OK)");
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

    LOG_INF("READ OBJECT TABLE--------------------------------------------");
    struct mxt_information_block info = {0};
    ret = mxt_load_object_table(dev, &info);
    if (ret < 0) {
        LOG_ERR("Failed to load the ojbect table: %d", ret);
        return -EIO;
    }
    LOG_INF("CONFIGURE GPIO-------------------------------------------");

    gpio_pin_configure_dt(&config->chg, GPIO_INPUT);
    gpio_init_callback(&data->gpio_cb, mxt_gpio_cb, BIT(config->chg.pin));
    ret = gpio_add_callback(config->chg.port, &data->gpio_cb);
    if (ret < 0) {
        LOG_ERR("Failed to set DR callback: %d", ret);
        return -EIO;
    }

    LOG_INF("INIT WORK QUEUE-----------------------------------------");
    k_work_init(&data->work, mxt_work_cb);

    LOG_INF("INIT INTERRUPT (HELD DISABLED UNTIL CONFIG COMPLETE)-----");
    ret = gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_DISABLE);
    if (ret < 0) {
        LOG_ERR("Failed to initialize interrupt for CHG pin %d", ret);
        return -EIO;
    }

    LOG_INF("CHG pin logical level at init: %d (port=%s, pin=%d)",
            gpio_pin_get_dt(&config->chg), config->chg.port->name, config->chg.pin);

    // Drain any initial power-on / reset status messages from T5
    int init_drain = 10;
    while (--init_drain > 0) {
        int processed = mxt_report_data(dev);
        if (processed == 0 || gpio_pin_get_dt(&config->chg) == 0) {
            break;
        }
    }

    LOG_INF("LOAD CONFIG-----------------------------------------------");
    ret = mxt_load_config(dev);
    if (ret < 0) {
        LOG_ERR("Failed to load default config: %d", ret);
        return -EIO;
    }

    LOG_INF("RUN CALIBRATION---------------------------------------------");
    if (data->t6_command_processor_address) {
        // Calibration typically takes ~160-250ms.
        // Wait for calibration to finish and poll-drain until the chip completes calibration
        // and releases the CHG line (logic 0 = physically HIGH / idle).
        int cal_wait_ms = 400;
        while (cal_wait_ms > 0) {
            k_msleep(20);
            cal_wait_ms -= 20;

            int processed = mxt_report_data(dev);

            // If calibration completed (we processed message(s) or waited at least 200ms)
            // AND the CHG line has returned to idle (0):
            if (gpio_pin_get_dt(&config->chg) == 0 && (processed > 0 || cal_wait_ms <= 200)) {
                break;
            }
        }
    }

    LOG_INF("CHG pin logical level after calibration & drain: %d  ------------------------", gpio_pin_get_dt(&config->chg));

    ret = mxt_run_self_test(dev);
    if (ret < 0) {
        LOG_WRN("T25 self test did not pass or reported error: %d", ret);
    }

    LOG_INF("ENABLE LEVEL INTERRUPT-----------------------------------");
    ret = gpio_pin_interrupt_configure_dt(&config->chg, GPIO_INT_LEVEL_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to enable level interrupt for CHG pin %d", ret);
        return -EIO;
    }

    if (gpio_pin_get_dt(&config->chg) != 0) {
        LOG_WRN("CHG pin still asserted after init (%d), queuing work", gpio_pin_get_dt(&config->chg));
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
                         DT_INST_NODE_HAS_PROP(n, active_acq_time_ms) ||                                 \
                         DT_INST_NODE_HAS_PROP(n, active_to_idle_timeout_ms),                            \
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
