#include "config.h"
#include "GPIO.h"
#include "delay.h"
#include "soft_uart.h"
#include "ir.h"


void gpio_init(void)
{
	GPIO_InitTypeDef t_gpio_type;

	t_gpio_type.Mode = GPIO_HighZ;
	t_gpio_type.Pin = GPIO_Pin_0;	// UART RXD
	GPIO_Inilize(GPIO_P3, &t_gpio_type);

	t_gpio_type.Mode = GPIO_PullUp;
	t_gpio_type.Pin = GPIO_Pin_1;	// UART TXD
	GPIO_Inilize(GPIO_P3, &t_gpio_type);

	t_gpio_type.Mode = GPIO_PullUp;
	t_gpio_type.Pin = GPIO_Pin_2;	// white LED
	GPIO_Inilize(GPIO_P3, &t_gpio_type);
	WHITE_LED = 1;

	t_gpio_type.Mode = GPIO_PullUp;
	t_gpio_type.Pin = GPIO_Pin_3;	// Color LED
	GPIO_Inilize(GPIO_P3, &t_gpio_type);
	COLOR_LED = 1;

	t_gpio_type.Mode = GPIO_PullUp;
	t_gpio_type.Pin = GPIO_Pin_4;	// IR send
	GPIO_Inilize(GPIO_P3, &t_gpio_type);
	IR_SEND = 1;

	t_gpio_type.Mode = GPIO_HighZ;
	t_gpio_type.Pin = GPIO_Pin_5;	// IR rev
	GPIO_Inilize(GPIO_P3, &t_gpio_type);
}

void slow_led(bit on, unsigned int sec)
{
	unsigned int i, cnt, m;
#define INTER_MS	600
	m = sec * 600;

	WHITE_LED = on;
	for (i=0; i<m; i++)
	{
		cnt = i;
		WHITE_LED = !WHITE_LED;
		while (cnt--);
		
		cnt = m - i;
		WHITE_LED = !WHITE_LED;
		while (cnt--);
	}
	WHITE_LED = !on;
}

void breath_led(void)
{
	while (1)
	{
		slow_led(1, 2);
		slow_led(0, 2);
	}
}

unsigned char Ir_Buf[4]; //用于保存解码结果
char c=0;
char i=0;
unsigned int ir_loop_cnt = 0;


main()
{
	EA = 1;	
	gpio_init();
	ir_rcv_init();

	//breath_led();
	while (1)
	{
#if 1
		if (ir_loop_cnt++ > 5000)
		{
			ir_loop_cnt = 0;
			if (check_ir_loop())
			{
				slow_led(1, 4);
				delay_ms(30000);
				slow_led(0, 4);
			}
		}
#endif
		//MCU_IDLE();
		if (ir_rcv(Ir_Buf))
		{
			uart_init();

			uart_send(Ir_Buf[0]);
			uart_send(Ir_Buf[1]);
			uart_send(Ir_Buf[2]);	
			uart_send(Ir_Buf[3]);
			uart_send(0xFF);

			uart_deinit();

			if (Ir_Buf[0] == 0x2D &&
				Ir_Buf[1] == 0x2D &&
				Ir_Buf[2] == 0x30 &&
				Ir_Buf[3] == 0xCF)
			{
				WHITE_LED = !WHITE_LED;
			}
			if (Ir_Buf[0] == 0x2D &&
				Ir_Buf[1] == 0x2D &&
				Ir_Buf[2] == 0x58 &&
				Ir_Buf[3] == 0xA7)
			{
				COLOR_LED = !COLOR_LED;
			}
			if (Ir_Buf[0] == 0x18 &&
				Ir_Buf[1] == 0xE7 &&
				Ir_Buf[2] == 0x08 &&
				Ir_Buf[3] == 0xF7)
			{
				WHITE_LED = !WHITE_LED;
			}
			if (Ir_Buf[0] == 0x18 &&
				Ir_Buf[1] == 0xE7 &&
				Ir_Buf[2] == 0x04 &&
				Ir_Buf[3] == 0xFB)
			{
				COLOR_LED = !COLOR_LED;
			}
		}
	}
}




