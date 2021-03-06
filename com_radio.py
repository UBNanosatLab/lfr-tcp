#!/usr/bin/env python3

import sys
import socket
from time import sleep
import threading

import socket

from enum import Enum, auto

SYNCWORD_H = 0xBE
SYNCWORD_L = 0xEF

class ParseState(Enum):
    SYNC_H = 0
    SYNC_L = 1
    FLAGS = 2
    CMD = 3
    PAYLOAD_LEN = 4
    PAYLOAD = 5
    CHKSUM_H = 6
    CHKSUM_L = 7

class Command(Enum):
    NOP = 0x00
    RESET = 0x01
    TXDATA = 0x02
    GET_TXPWR = 0x03
    SET_TXPWR = 0x04

    RXDATA = 0x10

    ERROR = 0x7F

    REPLY = 0x80

def fletcher(chksum, byte):
    lsb = chksum & 0xFF
    msb = chksum >> 8
    msb += byte
    msb &= 0xFF
    lsb += msb
    lsb &= 0xFF
    return (msb << 8) | lsb

def compute_chksum(data):
    chksum = 0
    for x in data:
        chksum = fletcher(chksum, ord(x))
    return chksum

def create_tx_pkt(data):
    pkt = '\x02'
    pkt += chr(len(data))
    pkt += data
    chksum = compute_chksum(pkt)
    pkt += chr(chksum >> 8)
    pkt += chr(chksum & 0xFF)
    pkt = '\xBE\xEF' + pkt
    return pkt

class RadioException(Exception):

    def __init__(self, code):
        self.code = code

        if code == 0:
            self.error = 'ESUCCESS'
            self.msg = 'command succeeded'
        elif code == 1:
            self.error = 'ETIMEOUT'
            self.msg = 'timeout waiting for CTS'
        elif code == 2:
            self.error = 'EWRONGPART'
            self.msg = 'unsupported part number'
        elif code == 3:
            self.error = 'EINVAL'
            self.msg = 'invalid parameter'
        elif code == 4:
            self.error = 'EINVALSTATE'
            self.msg = 'invalid internal state'
        elif code == 5:
            self.error = 'ETOOLONG'
            self.msg = 'packet too long'
        elif code == 6:
            self.error = 'ECHKSUM'
            self.msg = 'invalid checksum'
        elif code == 7:
            self.error = 'EBUSY'
            self.msg = 'pending operation'
        elif code == 8:
            self.error = 'ERXTIMEOUT'
            self.msg = 'Si446x RX timed out (zero len bug?)'
        else:
            self.error = 'UNKNOWN'
            self.msg = 'An unknown error occurred'

    def __str__(self):
        return 'RadioException(' + self.error + '): ' + self.msg

    pass

class Radio:

    def __init__(self, host, port=2600):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))
        self.state = ParseState.SYNC_H

    def tx(self, data):
        pkt = chr(Command.TXDATA.value)
        pkt += chr(len(data))
        pkt += data
        chksum = compute_chksum(pkt)
        pkt += chr(chksum >> 8)
        pkt += chr(chksum & 0xFF)
        pkt = '\xBE\xEF' + pkt

        self.sock.sendall(bytes([ord(x) for x in pkt]))

        (flags, cmd, pay) = self.recv()

        if cmd  == Command.ERROR.value | Command.REPLY.value:
            err = RadioException(ord(pay[0]))
            if err.error == 'EBUSY':
                self.tx(data)
            else:
                raise err

        elif Command.TXDATA.value | Command.REPLY.value:
            return
        else:
            raise Exception('Unexpected response: ' + str((flags, cmd, pay)))

    def rx(self):
        (flags, cmd, pay) = self.recv()

        if cmd  == Command.ERROR.value | Command.REPLY.value:
            raise RadioException(pay[0])
        elif Command.RXDATA.value | Command.REPLY.value:
            return pay
        else:
            raise Exception('Unexpected response: ' + str((flags, cmd, pay)))

    def recv(self):

        payload = ''

        while True:
            c = self.sock.recv(1)[0]

            if self.state is ParseState.SYNC_H:
                if c == SYNCWORD_H:
                    self.state = ParseState.SYNC_L
            elif self.state is ParseState.SYNC_L:
                if c == SYNCWORD_L:
                    self.state = ParseState.FLAGS
                elif c == SYNCWORD_H:
                    self.state = ParseState.SYNC_L
                else:
                    self.state = ParseState.SYNC_H
            elif self.state is ParseState.FLAGS:
                flags = c
                self.state = ParseState.CMD
            elif self.state is ParseState.CMD:
                cmd = c
                self.state = ParseState.PAYLOAD_LEN
            elif self.state is ParseState.PAYLOAD_LEN:
                length = c
                # TODO: Validate len for cmd
                if (length):
                    self.state = ParseState.PAYLOAD
                else:
                    chksum = compute_chksum(''.join([chr(flags), chr(cmd), chr(0)]))
                    self.state = ParseState.CHKSUM_H

            elif self.state is ParseState.PAYLOAD:
                payload += chr(c)
                length -= 1
                self.state = ParseState.PAYLOAD
                if (length == 0):
                    chksum = compute_chksum(''.join([chr(flags), chr(cmd), chr(len(payload))]) + payload)
                    self.state = ParseState.CHKSUM_H
            elif self.state is ParseState.CHKSUM_H:
                if (c == chksum >> 8):
                    self.state = ParseState.CHKSUM_L
                else:
                    # TODO: Handle error
                    pass
                    self.state = ParseState.SYNC_H
                    break
            elif self.state is ParseState.CHKSUM_L:
                if (c != chksum & 0xFF):
                    # TODO: Handle error
                    pass
                self.state = ParseState.SYNC_H
                break


        return (flags, cmd, payload)


def main():

    if (len(sys.argv) == 3 or (len(sys.argv) == 5 and sys.argv[3] == 'rx')):
        radio = Radio(sys.argv[1], int(sys.argv[2]))
        numPackets = 0
        while True:
            pkt = radio.rx()
            print('RX>', pkt)
            numPackets += 1
            print('Received ' + str(numPackets) + ' packet(s)')

    elif (len(sys.argv) == 5 and sys.argv[3] == 'tx'):
        radio = Radio(sys.argv[1], int(sys.argv[2]))
        sleep(1)
        for i in range(int(sys.argv[4])):
            data = ('KC2QOL ' + str(i + 1) + ' ').ljust(104, 'x')
            print('TX>', data)
            radio.tx(data)
            print('Sent ' + str(len(data)) + ' byte(s)')
            # Look ma, no sleep!

    else:
        print('Usage: python3', sys.argv[0], 'hostname port [rx | tx n]')


if __name__ == '__main__':
    main()
