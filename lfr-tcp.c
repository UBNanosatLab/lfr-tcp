/* Little Free Radio - An Open Source Radio for CubeSats
 * Copyright (C) 2018 Grant Iraci, Brian Bezanson
 * A project of the University at Buffalo Nanosatellite Laboratory
 * See LICENSE for details
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lfr-tcp.h"
#include "cmd_parser.h"

int kissfd = -1;
int uartfd = -1;

uint8_t sys_stat = 0;
uint16_t tx_gate_bias;

void log_err(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
}

void log_info(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
}

void log_data(char *s, uint8_t *data, int len) 
{
    int addr = 0;
    char chr[HEXDUMP_WIDTH];

    fprintf(stderr, "%s\n", s);

    for (addr = 0; addr < len; addr++)
    {
        if (addr % HEXDUMP_WIDTH == 0) {
            fprintf(stderr, "%04x    ", addr);
        }

        fprintf(stderr, "%02x ", data[addr]);

        // Is it a printable character?
        if (data[addr] >= 0x20 && data[addr] < 0x7F) {
            chr[addr % HEXDUMP_WIDTH] = data[addr];
        } else {
            chr[addr % HEXDUMP_WIDTH] = '.';
        }

        if (addr % HEXDUMP_WIDTH == (HEXDUMP_WIDTH - 1)) {
            int j;
            fprintf(stderr, "    ");
            for (j = 0; j < HEXDUMP_WIDTH; j++) {
                fprintf(stderr, "%c", chr[j]);
            }
            fprintf(stderr, "\n");
        }
    }

    if (addr % HEXDUMP_WIDTH != 0) {
        fprintf(stderr, "\n");
    }
}

void uart_putc(char c)
{
    int n; 
    
    if (uartfd < 0)
    {
        log_err("ERROR UART socket not connected!\n");
        return;
    }
    
    n = write(uartfd, &c, 1);

    if (n < 0)
    {
        log_err("ERROR writing to socket: %s\n", strerror(errno));
    }
}

void uart_puts(char *s)
{
    int n; 

    if (uartfd < 0)
    {
        log_err("ERROR UART socket not connected!\n");
        return;
    }

    n = write(uartfd, s, strlen(s));

    if (n < 0)
    {
        log_err("ERROR writing to socket: %s\n", strerror(errno));
    }
}

int kiss_putc(char c)
{
    int n; 
    
    if (kissfd < 0)
    {
        log_err("ERROR KISS socket not connected!\n");
        return -1;
    }
    
    n = write(kissfd, &c, 1);

    if (n < 0)
    {
        log_err("ERROR writing to socket: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int kiss_send_async(int len, uint8_t *buf)
{
    int err;
    int i;

    log_data("TX" , buf, len);
 
    // Start frame
    err = kiss_putc(KISS_FEND);
    if (err < 0) {
        return -3; // -EINVAL from si446x
    }

    // KISS TX command
    err = kiss_putc(0x00);
    if (err < 0) {
        return -3; // -EINVAL from si446x
    }

    for (i = 0; i < len; i++) {
        if (buf[i] == KISS_FEND) {
            err = kiss_putc(KISS_FESC);
            if (err < 0) {
                return -3; // -EINVAL from si446x
            }
            
            err = kiss_putc(KISS_TFEND);
            if (err < 0) {
                return -3; // -EINVAL from si446x
            }
        } else if (buf[i] == KISS_FESC) {
            err = kiss_putc(KISS_FESC);
            if (err < 0) {
                return -3; // -EINVAL from si446x
            }
            
            err = kiss_putc(KISS_TFESC);
            if (err < 0) {
                return -3; // -EINVAL from si446x
            }
        } else {
            err = kiss_putc(buf[i]);
            if (err < 0) {
                return -3; // -EINVAL from si446x
            }
        }
    }

    // End frame
    err = kiss_putc(KISS_FEND);
    if (err < 0) {
        return -3; // -EINVAL from si446x
    }

    return 0;
}

int open_server(char *host, int port)
{

    int sockfd;
    struct sockaddr_in serv_addr;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) {
        log_err("Error opening socket: %s\n", strerror(errno));
        return -1;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (host) {
        // Convert IPv4 and IPv6 addresses from text to binary form 
        if(inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0)  
        {
            log_err("ERROR Invalid address: %s\n", host);
            return -1; 
        }
    } else {
       serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        log_err("ERROR binding: %s\n", strerror(errno));
        return -1;
    }

    return sockfd;
}

int open_socket(char *host, int port)
{

    int sockfd;
    int flags;
    struct sockaddr_in serv_addr;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);    
    if (sockfd < 0) {
        log_err("Error opening socket: %s\n", strerror(errno));
        return -1;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (host) {
        // Convert IPv4 and IPv6 addresses from text to binary form 
        if(inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0)  
        {
            log_err("ERROR Invalid address: %s\n", host);
            return -1; 
        }
    } else {
       serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } 

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        log_err("ERROR connecting: %s\n", strerror(errno));
        return -1;
    }

    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        log_err("ERROR getting socket flags: %s\n", strerror(errno));
        return -1;
    }
    flags |= O_NONBLOCK;
    flags = fcntl(sockfd, F_SETFL, flags);
    if (flags == -1) {
        log_err("ERROR setting socket flags: %s\n", strerror(errno));
        return -1;
    }

    return sockfd;
}

int process_kiss(uint8_t *buf, int len)
{
    uint8_t pkt[MAX_PKT_SIZE];
    uint8_t cmd;    
    int i, j;

    // Empty frame is allowed, ignore
    if (len == 0)
        return 0;

    cmd = buf[0];

    j = 0;
    for (i = 1; i < len; i++) {
        
        if (j == MAX_PKT_SIZE) {
            log_err("ERROR receiving KISS: Packet too long\n");
            return -1;
        }
        
        if (buf[i] == KISS_FESC) {
            i++;
            if (buf[i] == KISS_TFEND) {
                pkt[j++] = KISS_FEND;
            } else if (buf[i] == KISS_TFESC) {
                pkt[j++] = KISS_FESC;
            } else {
                log_err("ERROR receiving KISS: invalid transpose\n");
                return -1; 
            }
        } else {
            pkt[j++] = buf[i];
        }
    }
    
    if (cmd == 0) {
        log_data("RX", pkt, j);
        reply(CMD_RXDATA, j, pkt);
    } else {
        log_err("ERROR processing KISS: unknown command %d\n", cmd);
        return -1;
    }

    return 0;
}

int kiss_char(uint8_t c)
{
    static uint8_t kiss_buf[KISS_BUF_SIZE];
    static int kiss_buf_len = 0;
    
    if (c == KISS_FEND) {
        int ret;
        ret = process_kiss(kiss_buf, kiss_buf_len);
        kiss_buf_len = 0;
        return ret;
    } else {
        if (kiss_buf_len == KISS_BUF_SIZE)
            return -1;

        kiss_buf[kiss_buf_len++] = c;
        return 0;
    }
}

int main(int argc, char **argv)
{
    int kiss_port;
    int uart_port;

    int serverfd;

    fd_set fds;
    int maxfd = 0;

    if (argc != 4) {
        fprintf(stderr, "usage %s hostname port uart_port\n", argv[0]);
        return -1;
    }
    
    kiss_port = atoi(argv[2]);
    uart_port = atoi(argv[3]);

    serverfd = open_server(NULL, uart_port);
    if (serverfd < 0) return -1;

    kissfd = open_socket(argv[1], kiss_port);
    if (kissfd < 0) return -1;

    if (listen(serverfd, 1)) { 
        log_err("ERROR listening: %s\n", strerror(errno));
        return -1;
    }

    FD_ZERO(&fds);

    FD_SET(serverfd, &fds);
    if (serverfd > maxfd)
        maxfd = serverfd;

    FD_SET(kissfd, &fds);
    if (kissfd > maxfd)
        maxfd = kissfd;


    while (1) {
        fd_set read_fds;
        
        read_fds = fds;
        if (select (FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            log_err("ERROR in select: %s\n", strerror(errno));    
            return -1;
        }

        if (uartfd > 0 && FD_ISSET(uartfd, &read_fds)) {
            int n;
            char c;
                
            while (1) {
                n = read(uartfd, &c, 1);
            
                if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    } else {
                        log_err("ERROR reading from UART fd: %s\n", 
                                strerror(errno));
                        return -1;
                    }
                }

                if (n == 0) {
                    log_info("UART socket closed\n");
                    close(uartfd);
                    FD_CLR(uartfd, &fds);
                    uartfd = -1;
                    break;
                }
                
                parse_char(c);
            }
        }

        if (serverfd > 0 && FD_ISSET(serverfd, &read_fds)) {
            struct sockaddr_in clientaddr;
            socklen_t clientaddr_len = sizeof(clientaddr);
            int newfd;

            newfd = accept(serverfd, (struct sockaddr *) &clientaddr, 
                           &clientaddr_len);

            if (newfd < 0) {
                log_err("ERROR accepting socket: %s\n", strerror(errno));
            } else {
                log_info("New UART connection from %s\n", 
                         inet_ntoa(clientaddr.sin_addr));
                
                if (uartfd > 0) {
                    log_info("Closing existing UART connection\n");
                    close(uartfd);
                    FD_CLR(uartfd, &fds);
                    uartfd = -1;
                }

                uartfd = newfd;
                            
                FD_SET(uartfd, &fds);
                if (uartfd > maxfd)
                    maxfd = uartfd;
            }
        }

        if (kissfd > 0 && FD_ISSET(kissfd, &read_fds)) {
            int n;
            uint8_t buf[KISS_BUF_SIZE];

            n = read(kissfd, buf, KISS_BUF_SIZE);

            if (n < 0) {
                log_err("ERROR reading from KISS fd: %s\n", strerror(errno));
                return -1;
            }

            if (n == 0) {
                log_err("KISS socket closed\n");
                close(kissfd);
                FD_CLR(kissfd, &fds);
                kissfd = -1;
                
                // Can't recover!
                return -1;
            }

            for (int i = 0; i < n; i ++) {
                kiss_char(buf[i]);
            }
            
        }
    }

    // ???

    return 0;
}   
