
#ifndef	__IR_H
#define	__IR_H

#include	"config.h"

void ir_rcv_init(void);
void ir_rcv_deinit(void);
bit ir_rcv(unsigned char *key);

void ir_send_init(void);
void ir_send_deinit(void);
bit ir_send(unsigned char *key);

bit check_ir_loop(void);

#endif
