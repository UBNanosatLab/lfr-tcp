/* Little Free Radio - An Open Source Radio for CubeSats
 * Copyright (C) 2018 Grant Iraci, Brian Bezanson
 * A project of the University at Buffalo Nanosatellite Laboratory
 * See LICENSE for details
 */

#ifndef LFR_TCP_H
#define LFR_TCP_H

#define MAX_PKT_SIZE 255

#define KISS_BUF_SIZE 512

#define KISS_FEND 0xC0
#define KISS_FESC 0xDB
#define KISS_TFEND 0xDC
#define KISS_TFESC 0xDD

#define HEXDUMP_WIDTH 16

extern uint8_t sys_stat;
extern uint16_t tx_gate_bias;

void set_cmd_flag(uint8_t flag);

void uart_putc(char c);
void uart_puts(char *s);

int kiss_send_async(int len, uint8_t *buf);

void log_err(const char *fmt, ...);
void log_info(const char *fmt, ...);


#endif
