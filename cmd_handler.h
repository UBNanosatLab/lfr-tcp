/* Little Free Radio - An Open Source Radio for CubeSats
 * Copyright (C) 2018 Grant Iraci, Brian Bezanson
 * A project of the University at Buffalo Nanosatellite Laboratory
 * See LICENSE for details
 */

#ifndef CMD_HANDLER_H
#define CMD_HANDLER_H

#include <stdint.h>

/**
 * Send reply character
 * @param c the character to send
 */
int reply_putc(uint8_t c);

/**
 * No-OPeration
 * Replies with success always
 */
void cmd_nop();

/**
 * Reset
 * Forces a reset of the MCU
 */
void cmd_reset();

/**
 * Get transmit power
 * Returns the transmit power level (in arbitrary units)
 * TODO: Use dBm?
 */
void cmd_get_txpwr();

/**
 * Set transmit power
 * Sets the transmitter power level (in arbitrary units)
 * TODO: Use dBm?
 * @param pwr the transmitter power level (gate bias)
 */
void cmd_set_txpwr(uint16_t pwr);

/**
 * Transmit the provided data
 * @param len the length of the data in bytes
 * @param data pointer to the data
 */
void cmd_tx_data(int len, uint8_t *data);

/**
 * Set configuration
 * @param len the length of the cfg data in bytes
 * @param data pointer to the cfg data
 */
void cmd_set_cfg(int len, uint8_t *data);

/**
 * Get configuration
 * Returns the configuration data
 */
void cmd_get_cfg();

/**
 * Save configuration
 * Writes the current configuration data to non-volatile storage
 */
void cmd_save_cfg();

/**
 * Load default configuration
 * Loads the default (factory) configuration into the volatile settings
 */
void cmd_cfg_default();


/**
 * Set frequency
 * Changes the frequency
 * @param freq desired frequency in Hz
 */
void cmd_set_freq(uint32_t freq);

/**
 * Abort TX
 * Immediately kills the transmitter and returns to normal receive mode
 */
void cmd_abort_tx();

/**
 * Transmit psuedo-random sequence
 * Transmits a psuedo-random sequence
 */
void cmd_tx_psr();

/**
 * Get received packet
 * Returns the next packet in the receive queue
 */
void cmd_rx_data();

/**
 * Get the depth of the transmit queue
 * Returns the (16-bit) depth of the queue
 */
void cmd_get_queue_depth();

/**
 * Send an error back
 * @param err the (positive) error code
 */
void cmd_err(int err);

#endif
