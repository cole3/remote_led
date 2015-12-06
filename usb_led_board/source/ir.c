/*
 * ir
 * using timer1 counter to invode interrupt because STC15F104E's ex int error.
 * timer0 is used to calculate high/low level's time.
*/

#include "ir.h"
#include "timer.h"
#include "delay.h"
#include "soft_uart.h"


//#define CONFIG_RC3_CODE

#pragma asm
IR_SEND      BIT     P3.4
#pragma endasm

static bit ir_rcv_int = 0;


void ir_rcv_isr(void)
{
    Timer1_InterruptDisable();
    ir_rcv_int = 1;
}

void ir_rcv_t0_timer_cfg(void)
{
    TIM_InitTypeDef tim_type;

    tim_type.TIM_Mode = TIM_16BitAutoReload;
    tim_type.TIM_Polity = 1;
    tim_type.TIM_Interrupt = DISABLE;
    tim_type.TIM_ClkSource = TIM_CLOCK_12T;
    tim_type.TIM_ClkOut = DISABLE;
    tim_type.TIM_Value = 0;
    tim_type.TIM_Run = DISABLE;
    tim_type.TIM_Isr = 0;
    Timer_Inilize(Timer0, &tim_type);
}

void ir_rcv_t1_timer_cfg(bit enable)
{
    TIM_InitTypeDef tim_type;

    tim_type.TIM_Mode = TIM_16BitAutoReload;
    tim_type.TIM_Polity = PolityLow;
    tim_type.TIM_Interrupt = enable;
    tim_type.TIM_ClkSource = TIM_CLOCK_Ext;
    tim_type.TIM_ClkOut = DISABLE;
    tim_type.TIM_Value = 0xFFFF;
    tim_type.TIM_Run = enable;
    tim_type.TIM_Isr = ir_rcv_isr;
    Timer_Inilize(Timer1, &tim_type);
}

static unsigned int Ir_Get_Low()
{
    unsigned int t = 0;

    TL0 = 0;
    TH0 = 0;
    TR0 = 1;
    while (!IR_REV && !(TH0 & 0x80));

    TR0 = 0;
    t = TH0;
    t <<= 8;
    t |= TL0;

    return t;
}

static unsigned int Ir_Get_High()
{
    unsigned int t = 0;

    TL0 = 0;
    TH0 = 0;
    TR0 = 1;
    while (IR_REV && !(TH0 & 0x80));

    TR0 = 0;
    t = TH0;
    t <<= 8;
    t |= TL0;

    return t;
}

static bit nec_decode(unsigned char *key)
{
    unsigned int temp;
    char i,j;

    temp = Ir_Get_High();
    if ((temp < 3686) || (temp > 4608)) {
        return 0;
    }

    for (i=0; i<4; i++) {
        for (j=0; j<8; j++) {
            temp = Ir_Get_Low();
            if ((temp < 184) || (temp > 737)) //200~800us
                return 0;

            temp = Ir_Get_High();
            if ((temp < 184) || (temp > 1843)) //200~2000us
                return 0;

            key[i] >>= 1;
            if (temp > 1032) //1120us
                key[i] |= 0x80;
        }
    }

    return 1;
}

#ifdef CONFIG_RC3_CODE
static bit rc5_decode(unsigned char *key)
{
    unsigned int temp, c = 0;
    bit state = 1;
    char i = 0, j;

    while (1) { //13 bit
        temp = Ir_Get_High();
        test[2] = temp;
        if ((temp < 300) || (temp > 900)) //200~800us
            goto err;

        if (temp > 600) {
            state = !state;
            i++;
        }
        c |= state;
        c <<= 1;
        i++;

        temp = Ir_Get_Low();
        test[3] = temp;
        if ((temp < 300) || (temp > 900)) //200~800us
            goto err;

        if (temp > 600) {
            state = !state;
            i++;
        }
        c |= state;
        c <<= 1;
        i++;
    }

err:
    return 1;
}
#endif

bit ir_rcv(unsigned char *key)
{
    unsigned int temp;
    bit ret = 0;
    char i,j;

    if (!ir_rcv_int) {
        return 0;
    }

    ir_rcv_t0_timer_cfg();

    temp = Ir_Get_Low();
    if (temp > 3000 && temp < 8755) {
        ret = nec_decode(key);
    }
#ifdef CONFIG_RC3_CODE
    else if (temp <= 3000) {
        ret = rc5_decode(key);
    }
#endif

    ir_rcv_int = 0;
    Timer1_InterruptEnable();

    return ret;
}

void ir_rcv_init(void)
{
    ir_rcv_t1_timer_cfg(ENABLE);
    ir_rcv_int = 0;
}

void ir_rcv_deinit(void)
{
    ir_rcv_t1_timer_cfg(DISABLE);
    ir_rcv_int = 0;
}



static bit ir_out;
static unsigned char ir_cnt;

void ir_send_isr(void)
{
    if (ir_out) {
        IR_SEND = !IR_SEND;
    }

    if (ir_cnt) {
        ir_cnt--;
    }
}

void ir_send_t0_timer_cfg(bit enable)
{
    TIM_InitTypeDef tim_type;

    tim_type.TIM_Mode = TIM_16BitAutoReload;
    tim_type.TIM_Polity = 1;
    tim_type.TIM_Interrupt = enable;
    tim_type.TIM_ClkSource = TIM_CLOCK_12T;
    tim_type.TIM_ClkOut = DISABLE;
    tim_type.TIM_Value = 65536 - MAIN_Fosc / (38000 * 2);
    tim_type.TIM_Run = enable;
    tim_type.TIM_Isr = ir_send_isr;
    Timer_Inilize(Timer0, &tim_type);
}


#pragma asm
        // 1 cycle = 1/38K = 291 clock = 145 + 146
CYCLE:  SETB    IR_SEND         // 4
        NOP                     // 1
        MOV     R0,     #33     // 2
        DJNZ    R0,     $       // 4 * 33 = 132

        CLR     IR_SEND         // 4
        MOV     R0,     #33     // 2
        DJNZ    R0,     $       // 4 * 33 = 132
        RET                     // 4

        // 0 bit = cycle(560us) + 0(560us) = cycle(21 cycle) + 0(6193.152 = 4*12*129 clock)
IRS0:   MOV     R1,     #21     // 2
CC1:    LCALL   CYCLE           // 6
        DJNZ    R1,     CC1     // 4

        MOV     R2,     #12    // 2
CC2:    MOV     R1,     #129    // 2
        DJNZ    R1,     $       // 4
        DJNZ    R2,     CC2     // 4
        RET

        // 1 bit = cycle(560us) + 0(1690us) = cycle(21 cycle) + 0(18690.048 = 4*32*146 clock)
IRS1:   MOV     R1,     #21     // 2
CC3:    LCALL   CYCLE           // 6
        DJNZ    R1,     CC3     // 4

        MOV     R2,     #32     // 2
CC4:    MOV     R1,     #146    // 2
        DJNZ    R1,     $       // 4
        DJNZ    R2,     CC4     // 4
        RET
#pragma endasm

bit ir_send(unsigned char *key)
{
#pragma asm
        PUSH    ACC
        MOV     ACC,    R0
        PUSH    ACC
        MOV     ACC,    R1
        PUSH    ACC
        MOV     ACC,    R2
        PUSH    ACC
        MOV     ACC,    R3
        PUSH    ACC
        MOV     ACC,    R4
        PUSH    ACC
        MOV     ACC,    R5
        PUSH    ACC

        MOV     A,      R1
        MOV     R5,     A

        // send start code: 0(9000us = 342 cycle = 171 + 171) + 1(4500us = 49767 clock = 4 * 49 * 255)
        MOV     R1,     #170    // 2
        MOV     R2,     #170    // 2
C1:     LCALL   CYCLE           // 6
        DJNZ    R1,     C1      // 4
C2:     LCALL   CYCLE           // 6
        DJNZ    R2,     C2      // 4

        MOV     R2,     #49     // 2
C3:     MOV     R1,     #255    // 2
        DJNZ    R1,     $       // 4
        DJNZ    R2,     C3      // 4

        // send byte
        MOV     R4,     #4      //
C7:     MOV     A,      R5
        MOV     R0,     A
        MOV     A,      @R0     //
        INC     R5
        MOV     R3,     #8      //
C6:     RRC     A               // 1
        JC      C4              // 3
        LCALL   IRS0            // 6
        LJMP    C5
C4:     LCALL   IRS1            // 6
C5:     DJNZ    R3,     C6      // 4
        DJNZ    R4,     C7      // 4

        MOV     R0,     #0FFH
        DJNZ    R0,     $

        POP     ACC
        MOV     R5,     ACC
        POP     ACC
        MOV     R4,     ACC
        POP     ACC
        MOV     R3,     ACC
        POP     ACC
        MOV     R2,     ACC
        POP     ACC
        MOV     R1,     ACC
        POP     ACC
        MOV     R0,     ACC
        POP     ACC
#pragma endasm

    return 0;
}

void ir_send_init(void)
{
    IR_SEND = 1;
    ir_out = 0;
    ir_cnt = 0;
    ir_send_t0_timer_cfg(ENABLE);
}

void ir_send_deinit(void)
{
    ir_send_t0_timer_cfg(DISABLE);
    IR_SEND = 1;
    ir_out = 0;
    ir_cnt = 0;
}


bit check_ir_loop(void)
{
    unsigned char n = 2;

    ir_send_init();

    do {
        if (ir_rcv_int) {
            break;
        }
        ir_cnt = 10;
        ir_out = 1;
        while (ir_cnt);
        ir_out = 0;

        if (!ir_rcv_int) {
            break;
        }
        ir_rcv_int = 0;
        Timer1_InterruptEnable();

        delay_ms(1);
    } while (--n);

    ir_send_deinit();

    return (n == 0);
}
