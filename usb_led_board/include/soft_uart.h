
#ifndef	__SOFT_UART_H
#define	__SOFT_UART_H

#include	"config.h"

void uart_init(void);
void uart_deinit(void);
bit uart_rev(unsigned char *dat);
bit uart_send(unsigned char dat);
void print_str(unsigned char code *str);
void print_hex(unsigned char hex);
void print_dec(unsigned char dec);

#endif
