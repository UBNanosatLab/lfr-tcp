#!/usr/bin/env python3

SAT_PORT = 52001
GND_PORT = 52002

import socket, select
import random
from time import sleep

FEND = 0xC0
FESC = 0xDB
TFEND = 0xDC
TFESC = 0xDD

def percent(p):
    return random.random() < (p / 100.)

def hexdump(src, length=16, sep='.'):
    FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or sep for x in range(256)])
    lines = []
    for c in range(0, len(src), length):
        chars = src[c:c+length]
        hexstr = ' '.join(["%02x" % ord(x) for x in chars]) if type(chars) is str else ' '.join(['{:02x}'.format(x) for x in chars])
        if len(hexstr) > 24:
            hexstr = "%s %s" % (hexstr[:24], hexstr[24:])
        printable = ''.join(["%s" % ((ord(x) <= 127 and FILTER[ord(x)]) or sep) for x in chars]) if type(chars) is str else ''.join(['{}'.format((x <= 127 and FILTER[x]) or sep) for x in chars])
        lines.append("%04x:  %-*s  |%s|" % (c, length*3, hexstr, printable))
    return '\n'.join(lines)

def uplink_filter(msg):
    sleep((len(msg) + 9) * 8 / 10000 + 0.004)
    if percent(99):
        print('GND>\n{}'.format(hexdump(msg)))
        return msg
    else:
        print('\033[1;31mGND>\n{}\033[0;0m'.format(hexdump(msg)))
        return None

def downlink_filter(msg):
    sleep((len(msg) + 9) * 8 / 10000 + 0.004)
    if percent(99):
        print('SAT>\n{}'.format(hexdump(msg)))
        return msg
    else:
        print('\033[1;31mSAT>\n{}\033[0;0m'.format(hexdump(msg)))
        return None

def encode_kiss(data):
    if data is None:
        return None

    packet = b'\x00'

    for b in data:
        if (b == FESC):
            packet += bytes([FESC, TFESC])
        elif (b == FEND):
            packet += bytes([FESC, TFEND])
        else:
            packet += bytes([b])

    packet += bytes([FEND])

    return packet

def decode_kiss(kiss):
    if not kiss or kiss[0] != 0x00:
        return None
    
    packet = b''
    transpose = False

    for b in kiss[1:]:
        if transpose:
            if (b == TFEND):
                packet += bytes([FEND])
            elif (b == TFESC):
                packet += bytes([FESC])
            else:
                # Invalid escape sequence, right now we ignore it
                pass
            transpose = False
        else:
            if (b == FEND):
                return packet
            elif (b == FESC):
                transpose = True
            else:
                packet += bytes([b])

    # Assume a trimmed FEND
    return packet

def broadcast(pkt, socks, pkt_filter=None):
    if pkt_filter:
        data = decode_kiss(pkt)
        filtered = pkt_filter(data)
        pkt = encode_kiss(filtered) 

    if pkt:
        for sock in socks:
            try:
                sock.send(pkt)
            except :
                sock.close()
                if sock in sat_socks:
                    sat_socks.remove(sock)
                if sock in gnd_socks:
                    gnd_socks.remove(sock)

sat_socks = []
gnd_socks = []

sat_server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sat_server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sat_server_socket.bind(("0.0.0.0", SAT_PORT))
sat_server_socket.listen(1)

print('Listening for satellite on port', SAT_PORT)

gnd_server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
gnd_server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
gnd_server_socket.bind(("0.0.0.0", GND_PORT))
gnd_server_socket.listen(1)

print('Listening for ground on port', GND_PORT)

bufs = {}

while True:
    rlist = [sat_server_socket, gnd_server_socket] + sat_socks + gnd_socks
    read_sockets, _, _ = select.select(rlist, [], sat_socks + gnd_socks)

    for sock in read_sockets:
        # New satellite connection
        if sock == sat_server_socket:
            sockfd, addr = sock.accept()
            sat_socks.append(sockfd)
            bufs[sockfd] = b''
            print("Satellite ({}) connected".format(addr[0]))

        # New ground connection
        elif sock == gnd_server_socket:
            sockfd, addr = sock.accept()
            gnd_socks.append(sockfd)
            bufs[sockfd] = b''
            print("Ground ({}) connected".format(addr[0]))

        # Some incoming message from a client
        else:
            try:
                data = sock.recv(1)
                if data:
                    if data[0] == FEND:
                        if bufs[sock]:
                            if sock in sat_socks:
                                broadcast(bufs[sock], gnd_socks, downlink_filter)
                            elif sock in gnd_socks:
                                broadcast(bufs[sock], sat_socks, uplink_filter)
                            else:
                                # huh?
                                raise Exception("What?")
                        bufs[sock] = b''
                    else:
                        bufs[sock] += data
            
            except Exception as e:
                print("Client went offline:", e)
                sock.close()
                if sock in sat_socks:
                    sat_socks.remove(sock)
                if sock in gnd_socks:
                    gnd_socks.remove(sock)




def broadcast_data (sock, message):
    #Do not send the message to master socket and the client who has send us the message
    for socket in CONNECTION_LIST:
        if socket != server_socket and socket != sock :
            try :
                socket.send(message)
            except :
                # broken socket connection may be, chat client pressed ctrl+c for example
                socket.close()
                CONNECTION_LIST.remove(socket)


# List to keep track of socket descriptors
CONNECTION_LIST = []
RECV_BUFFER = 4096 # Advisable to keep it as an exponent of 2
PORT = 52001

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind(("0.0.0.0", PORT))
server_socket.listen(10)

# Add server socket to the list of readable connections
CONNECTION_LIST.append(server_socket)

print("Server started on port " + str(PORT))

while 1:
    # Get the list sockets which are ready to be read through select
    read_sockets,write_sockets,error_sockets = select.select(CONNECTION_LIST,[],[])

    for sock in read_sockets:
        #New connection
        if sock == server_socket:
            # Handle the case in which there is a new connection received through server_socket
            sockfd, addr = server_socket.accept()
            CONNECTION_LIST.append(sockfd)
            print("Client (%s, %s) connected" % addr)
        
        #Some incoming message from a client
        else:
            # Data recieved from client, process it
            try:
                #In Windows, sometimes when a TCP program closes abruptly,
                # a "Connection reset by peer" exception will be thrown
                data = sock.recv(RECV_BUFFER)
                if data:
                    print('DATA:', data)
                    broadcast_data(sock, data)
            
            except:
                print("Client (%s, %s) is offline" % addr)
                sock.close()
                CONNECTION_LIST.remove(sock)
                continue

server_socket.close()
