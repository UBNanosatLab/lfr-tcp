/* Little Free Radio - An Open Source Radio for CubeSats
 * Copyright (C) 2018 Grant Iraci, Brian Bezanson
 * A project of the University at Buffalo Nanosatellite Laboratory
 * See LICENSE for details
 */

#ifndef _CMD_PARSER_H
#define _CMD_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#define MAX_PAYLOAD_LEN 255

#define ECMDBADSUM 0x16
#define ECMDINVAL  0x13

/* sync word */
#define SYNCWORD_H 0xbe
#define SYNCWORD_L 0xef

/* commands */
/* System Group */
#define CMD_NOP                 0x00
#define CMD_RESET               0x01
//#define CMD_UPTIME              0x02

/* Data Group */
#define CMD_TXDATA              0x10
#define CMD_RXDATA              0x11
#define CMD_TX_ABORT            0x12
#define CMD_TX_PSR              0x13

/* Configuration Group */
#define CMD_GET_CFG             0x20
#define CMD_SET_CFG             0x21
#define CMD_SAVE_CFG            0x22
#define CMD_CFG_DEFAULT         0x23
#define CMD_SET_FREQ            0x24
#define CMD_READ_TXPWR          0x25
#define CMD_SET_TXPWR           0x26

/* Status Group */
//#define CMD_GET_STATUS          0x30
//#define CMD_CLEAR_STATUS        0x31
#define CMD_GET_QUEUE_DEPTH     0x32

/* Peripheral Group */
//#define CMD_GPIO_WRITE          0x40

/* Error / Internal Use Group */
#define CMD_INTERNALERR         0x7E
#define CMD_REPLYERR            0x7F

/* Reply bit */
#define CMD_REPLY               0x80


#ifdef __cplusplus
extern "C" {
#endif

void parse_char(uint8_t c);

void reply_error(uint8_t code);
void reply(uint8_t cmd, int len, uint8_t *payload);

#ifdef __cplusplus
}
#endif

#endif
