/* Little Free Radio - An Open Source Radio for CubeSats
 * Copyright (C) 2018 Grant Iraci, Brian Bezanson
 * A project of the University at Buffalo Nanosatellite Laboratory
 * See LICENSE for details
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lfr-tcp.h"
#include "cmd_handler.h"
#include "cmd_parser.h"

void cmd_nop() {
    log_info("NOP\n");
    reply(CMD_NOP, 0, NULL);
}

void cmd_reset() {
    log_info("RESET!\n");
}

void cmd_set_txpwr(uint16_t pwr) {
    log_info("SET_TXPWR: 0x%04X\n", pwr);
    tx_gate_bias = pwr;
    reply(CMD_SET_TXPWR, 0, NULL);
}

void cmd_get_txpwr() {
    uint8_t resp[] = {(uint8_t)(tx_gate_bias >> 8), 
                      (uint8_t)(tx_gate_bias & 0xFF)};
    reply(CMD_READ_TXPWR, sizeof(resp), resp);
}

void cmd_tx_data(int len, uint8_t *data) {
    int err = kiss_send_async(len, data);

    if (err) {
        // Halt and catch fire
        reply_error((uint8_t) -err);
    } else {
            reply(CMD_TXDATA, 0, NULL);
    }
}

void cmd_set_freq(uint32_t freq) {

    log_info("SET_FREQ: %d Hz\n", freq);
    int err = 0;

    if (err) {
        // Halt and catch fire
        reply_error((uint8_t) -err);
    } else {
            reply(CMD_SET_FREQ, 0, NULL);
    }
}

void cmd_get_cfg() {
    int err = -127; // ENOTIMPL

    log_info("GET_CFG: Not implemented!\n");

    if (err) {
        reply_error((uint8_t) -err);
    } else {
        reply(CMD_GET_CFG, 0, NULL);
    }
}

void cmd_set_cfg(int len, uint8_t *data) {
    int err = 0;

    if (len == 0) {
        err = -ECMDINVAL;
    } else {
        log_info("CFG_SET: ver %d\n", data[0]);
    }
    

    if (err) {
        reply_error((uint8_t) -err);
    } else {
        reply(CMD_SET_CFG, 0, NULL);
    }
}

void cmd_cfg_default() {
    int err = 0;

    log_info("CFG_DEFAULT\n");

    if (err) {
        reply_error((uint8_t) -err);
    } else {
        reply(CMD_CFG_DEFAULT, 0, NULL);
    }
}

void cmd_save_cfg() {
    int err = 0;

    log_info("SAVE_CFG\n");

    if (err) {
        reply_error((uint8_t) -err);
    } else {
        reply(CMD_CFG_DEFAULT, 0, NULL);
    }
}

void cmd_get_queue_depth() {
    int err = 0;
    uint16_t depth = 0;
    uint8_t data[] = {depth >> 8, depth & 0xFF};

    log_info("GET_QUEUE_DEPTH\n");

    if (err) {
        reply_error((uint8_t) -err);
    } else {
        reply(CMD_GET_QUEUE_DEPTH, sizeof(data), data);
    }
}

void cmd_abort_tx() {
    int err = 0;

    log_info("ABORT_TX\n");

    if (err) {
        reply_error((uint8_t) -err);
    } else {
        reply(CMD_TX_ABORT, 0, NULL);
    }
}

void cmd_tx_psr() {
    int err = 0;

    log_info("TX_PSR\n");

    if (err) {
        reply_error((uint8_t) -err);
    } else {
        reply(CMD_TX_PSR, 0, NULL);
    }
}

void cmd_err(int err) {
    reply_error((uint8_t) err);
}

int reply_putc(uint8_t c) {
    uart_putc(c);
    return 0;
}
