/* Little Free Radio - An Open Source Radio for CubeSats
 * Copyright (C) 2018 Grant Iraci, Brian Bezanson
 * A project of the University at Buffalo Nanosatellite Laboratory
 * See LICENSE for details
 */

#include "cmd_parser.h"
#include "cmd_handler.h"
//#define USE_PRINTF
#ifdef USE_PRINTF
#include <stdio.h>
#endif

void command_handler(uint8_t cmd, uint8_t len, uint8_t* payload);

 bool validate_cmd(uint8_t cmd);
 bool validate_length(uint8_t cmd, uint8_t len);
 uint16_t fletcher(uint16_t old_checksum, uint8_t c);


/**
 * enum for states of the byte parser state machine
 * Each state is named for the byte which the state machine expects to receive.
 */
enum parser_state_e {S_SYNC0, S_SYNC1, S_CMD, S_PAYLOADLEN, S_PAYLOAD, S_CHECKSUM0, S_CHECKSUM1};

/**
 * enum for the result of parsing a byte
 * The results can be:
 * wait for another character (take no action)
 * execute the command (if the byte completed a valid command)
 * invalid (the command was not valid, or the payload length was not valid for the command)
 * bad checksum (checksum mismatch)
 */
enum parser_result_e {R_WAIT, R_ACT, R_INVALID, R_BADSUM};

/* \fn parse_char(uint8_t c)
 * \brief Character-based command parser
 * \details Takes one character at a time, checks for validity, reports error or executes a completed command
 * \param c The next byte
 */
void parse_char(uint8_t c) {
  static enum parser_state_e next_state = S_SYNC0;
  static enum parser_result_e result = R_WAIT;
  static uint8_t cmd;
  static uint8_t payload_len = 0, payload_counter = 0;
  static uint8_t payload[MAX_PAYLOAD_LEN];
  static uint16_t checksum;
  static uint16_t calc_checksum;

  /* step through the packet structure based on the latest character */
  switch (next_state) {
    case S_SYNC0:
      if (SYNCWORD_H == c) next_state = S_SYNC1;
      break;
    case S_SYNC1:
      if (SYNCWORD_L == c) next_state = S_CMD;
      /* for a sync word "Sy", "SSy" should be detected as valid */
      else if (SYNCWORD_H != c)next_state = S_SYNC0;
      break;
    case S_CMD:
      if (validate_cmd(c)) {
        cmd = c;
        //calc_checksum = fletcher(calc_checksum, c);
        calc_checksum = fletcher(0, c);
        next_state = S_PAYLOADLEN;
      } else {
        result = R_INVALID;
      }
      break;
    case S_PAYLOADLEN:
      if (validate_length(cmd, c)) {
        payload_len = c;
        payload_counter = 0;
        calc_checksum = fletcher(calc_checksum, c);
        next_state = S_PAYLOAD;
      } else result = R_INVALID;
      break;
    case S_PAYLOAD:
      payload[payload_counter] = c;
      calc_checksum = fletcher(calc_checksum, c);
      payload_counter++;
      if (payload_counter == payload_len) {
        next_state = S_CHECKSUM0;
      }
      break;
    case S_CHECKSUM0:
      checksum = ((uint16_t) c) << 8;
      next_state = S_CHECKSUM1;
      break;
    case S_CHECKSUM1:
      checksum = checksum | (uint16_t)c;
      if (calc_checksum == checksum) {
        result = R_ACT;
      } else result = R_BADSUM;
      break;
    default:
      //error();
      break;
  }//switch(c)

  /* if that character created an error or concluded a valid packet, do something */
  switch (result) {
    case R_INVALID:
      next_state = S_SYNC0;
      result = R_WAIT;
      cmd_err(ECMDINVAL);
      break;
    case R_BADSUM:
      next_state = S_SYNC0;
      result = R_WAIT;
      cmd_err(ECMDBADSUM);
      break;
    case R_ACT:
      next_state = S_SYNC0;
      result = R_WAIT;
      command_handler(cmd, payload_len, payload);
    case R_WAIT:
      break;
  }
}
/* returns true for valid command types, false for invalid */
bool validate_cmd(uint8_t cmd) {
  switch (cmd) {
    case CMD_NOP:
    case CMD_RESET:
    case CMD_READ_TXPWR:
    case CMD_SET_TXPWR:
    case CMD_TXDATA:
    case CMD_SET_FREQ:
      return true;
    default:
      return false;
  }
}

/* returns true if the payload length is valid for the command type, false if not */
bool validate_length(uint8_t cmd, uint8_t len) {
  if (len > MAX_PAYLOAD_LEN) {
    return false;
  }

  switch (cmd) {
    case CMD_SET_TXPWR:
      return len == 2;
    case CMD_TXDATA:
      return len > 0;
    case CMD_SET_FREQ:
      return len == 4;
    default:
      return len == 0;
  }
}

/* update the mod-256 Fletcher checksum with the byte c */
uint16_t fletcher(uint16_t old_checksum, uint8_t c) {
  uint8_t lsb, msb;
  lsb = old_checksum;
  msb = (old_checksum >> 8) + c;
  lsb += msb;
  return ((uint16_t) msb<<8) | (uint16_t)lsb;
}

void command_handler(uint8_t cmd, uint8_t len, uint8_t* payload) {

    switch (cmd) {
      case CMD_NOP:
        cmd_nop();
        break;
      case CMD_RESET:
        cmd_reset();
        break;
      case CMD_READ_TXPWR:
        cmd_get_txpwr();
        break;
      case CMD_SET_TXPWR:
        cmd_set_txpwr((uint16_t) payload[0] << 8 | payload[1]);
      case CMD_TXDATA:
        cmd_tx_data(len, payload);
        break;
      case CMD_SET_FREQ:
        cmd_set_freq((uint32_t) payload[0] << 24 | (uint32_t) payload[1] << 16 | (uint32_t) payload[2] << 8 |
                     payload[3]);
        break;
    }
}

void reply_error(uint8_t sys_stat, uint8_t code)
{
    reply_putc(SYNCWORD_H);
    reply_putc(SYNCWORD_L);
    reply_putc(sys_stat);
    uint16_t chksum = fletcher(0, sys_stat);
    reply_putc(CMD_ERR);
    chksum = fletcher(chksum, CMD_ERR);
    reply_putc(1);
    chksum = fletcher(chksum, 1);
    reply_putc(code);
    chksum = fletcher(chksum, code);
    reply_putc((char) (chksum >> 8));
    reply_putc((char) chksum);
}

void reply(uint8_t sys_stat, uint8_t cmd, int len, uint8_t *payload)
{
    cmd ^= 0x80; // Flip highest bit in reply
    reply_putc(SYNCWORD_H);
    reply_putc(SYNCWORD_L);
    reply_putc(sys_stat);
    uint16_t chksum = fletcher(0, sys_stat);
    reply_putc(cmd);
    chksum = fletcher(chksum, cmd);

    reply_putc(len);
    chksum = fletcher(chksum, len);

    for (; len > 0; len--) {
        reply_putc(*payload);
        chksum = fletcher(chksum, *payload);
        payload++;
    }

    reply_putc((char) (chksum >> 8));
    reply_putc((char) chksum);
}