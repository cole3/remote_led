/*
 * ir 
 * using timer1 counter to invode interrupt because STC15F104E's ex int error.
 * timer0 is used to calculate high/low level's time.
*/

#include "ir.h"
#include "timer.h"
#include "delay.h"
#include "soft_uart.h"

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

unsigned int test[4];

static bit nec_decode(unsigned char *key)
{
	unsigned int temp;
	char i,j;

	temp = Ir_Get_High();
	test[1] = temp;
	if ((temp < 3686) || (temp > 4608))  //引导脉冲高电平4000~5000us
	{
		return 0;
	}

	for (i=0; i<4; i++) //4个字节
	{
		for (j=0; j<8; j++) //每个字节8位
		{
			temp = Ir_Get_Low();
			test[2] = temp;
			if ((temp < 184) || (temp > 737)) //200~800us
				return 0;

			temp = Ir_Get_High();
			test[3] = temp;
			if ((temp < 184) || (temp > 1843)) //200~2000us
				return 0;

			key[i] >>= 1;
			if (temp > 1032) //1120us
				key[i] |= 0x80;
		}
	}

	return 1;
}

static bit rc5_decode(unsigned char *key)
{
	unsigned int temp, c = 0;
	bit state = 1;
	char i = 0, j;

#if 0
	temp = Ir_Get_High();
	test[1] = temp;
	if ((temp < 600) || (temp > 900))  //引导脉冲高电平4000~5000us
	{
		goto err;
	}
#endif
	while (1) //13 bit
	{
		temp = Ir_Get_High();
		test[2] = temp;
		if ((temp < 300) || (temp > 900)) //200~800us
			goto err;

		if (temp > 600)
		{
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

		if (temp > 600)
		{
			state = !state;
			i++;
		}
		c |= state;
		c <<= 1;
		i++;
	}

err:
	uart_init();

	uart_send(i);

	uart_send(temp >> 8);
	uart_send(temp & 0xFF);

	uart_send(c >> 8);
	uart_send(c & 0xFF);
	uart_send(0xAA);

	uart_deinit();

	return 1;
}

bit ir_rcv(unsigned char *key)
{
	unsigned int temp;
	bit ret = 0;
	char i,j;

	if (!ir_rcv_int)
	{
		return 0;
	}

	ir_rcv_t0_timer_cfg();

	temp = Ir_Get_Low();
	test[0] = temp;
#if 0
	if ((temp < 7833) || (temp > 8755))  //引导脉冲低电平8500~9500us
		return 0;
#endif
	if (temp > 3000 && temp < 8755)
	{
		ret = nec_decode(key);
	}
	else if (temp <= 3000)
	{
		ret = rc5_decode(key);
	}

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
	if (ir_out)
	{
		IR_SEND = !IR_SEND;
	}

	if (ir_cnt)
	{
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

bit ir_send(unsigned char *key)
{
	// TODO:
	key = key;
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
		if (ir_rcv_int)
		{
			break;
		}
		ir_cnt = 10;
		ir_out = 1;
		while (ir_cnt);
		ir_out = 0;

		if (!ir_rcv_int)
		{
			break;
		}
		ir_rcv_int = 0;
		Timer1_InterruptEnable();

		delay_ms(1);
	} while (--n);

	ir_send_deinit();

	return (n == 0);
}