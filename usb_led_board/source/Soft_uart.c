
/*************	功能说明	**************

本文件为模拟串口发送程序, 一般为测试监控用.

串口参数:9600,8,n,1.

可以根据主时钟自动适应.

******************************************/

#include	"soft_uart.h"
#include 	"timer.h"

/***************************************************************************/


#define BaudRate		9600		//模拟串口波特率
#define Timer0_Reload	(65536 - MAIN_Fosc / BaudRate / 3)
#define D_RxBitLenth	9		//9: 8 + 1 stop
#define D_TxBitLenth	9		//9: 1 stop bit

sbit RXB = P3^0;                //define UART TX/RX port
sbit TXB = P3^1;

static unsigned char TBUF,RBUF;
static unsigned char TCNT,RCNT;	//发送和接收检测 计数器(3倍速率检测)
static unsigned char TBIT,RBIT;	//发送和接收的数据计数器
static unsigned char rcv_cnt, get_cnt;
static unsigned char buf[16];

static bit TING,RING;	//正在发送或接收一个字节
static bit init;

#define	RxBitLenth	9	//8个数据位+1个停止位
#define	TxBitLenth	9	//8个数据位+1个停止位


bit uart_rev(unsigned char *dat)
{
	if (!init)
	{
		return 0;
	}
	if (get_cnt != rcv_cnt)
	{
		*dat = buf[get_cnt++ & 0x0f];
		return 1;
	}
	return 0;
}

bit uart_send(unsigned char dat)
{
	if (!init)
	{
		return 0;
	}
	while (TING);
	TBUF = dat;
	TING = 1;
	return 1;
}

void print(unsigned char code *str)
{
	while (*str)
	{
		uart_send(*str++);
	}
}

void uart_isr(void)
{
	if (RING)
	{
		if (--RCNT == 0)				  //接收数据以定时器的1/3来接收
		{
			RCNT = 3;                   //重置接收计数器  接收数据以定时器的1/3来接收	reset send baudrate counter
			if (--RBIT == 0)			  //接收完一帧数据
			{
				RING = 0;               //停止接收			stop receive
				buf[rcv_cnt++ & 0x0f] = RBUF;
			}
			else
			{
				RBUF >>= 1;			  //把接收的单b数据 暂存到 RDAT(接收缓冲)
				if (RXB) RBUF |= 0x80;  //shift RX data to RX buffer
			}
		}
	}
	else if (!RXB)		//判断是不是开始位 RXB=0;
	{
		RING = 1;       //如果是则设置开始接收标志位 	set start receive flag
		RCNT = 4;       //初始化接收波特率计数器       	initial receive baudrate counter
		RBIT = RxBitLenth;       //初始化接收的数据位数(8个数据位+1个停止位)    initial receive bit number (8 data bits + 1 stop bit)
	}

	if (TING)			//发送开始标志位   judge whether sending
	{
		if (--TCNT == 0)			//发送数据以定时器的1/3来发送
		{
			TCNT = 3;				//重置发送计数器   reset send baudrate counter
			if (TBIT == 0)			//发送计数器为0 表明单字节发送还没开始
			{
				TXB = 0;			//发送开始位     					send start bit
				TBIT = TxBitLenth;	//发送数据位数 (8数据位+1停止位)	initial send bit number (8 data bits + 1 stop bit)
			}
			else					//发送计数器为非0 正在发送数据
			{
				if (--TBIT == 0)	//发送计数器减为0 表明单字节发送结束
				{
					TXB = 1;		//送停止位数据
					TING = 0;		//发送停止位    			stop send
				}
				else
				{
					TBUF >>= 1;		//把最低位送到 CY(益处标志位) shift data to CY
					TXB = CY;		//发送单b数据				write CY to TX port
				}
			}
		}
	}
}

void uart_init(void)
{
	TIM_InitTypeDef tim_type;

	tim_type.TIM_Mode = TIM_16BitAutoReload;
	tim_type.TIM_Polity = 1;
	tim_type.TIM_Interrupt = ENABLE;
	tim_type.TIM_ClkSource = TIM_CLOCK_1T;
	tim_type.TIM_ClkOut = DISABLE;
	tim_type.TIM_Value = Timer0_Reload;
	tim_type.TIM_Run = ENABLE;
	tim_type.TIM_Isr = uart_isr;
	Timer_Inilize(Timer0, &tim_type);

	TING = 0;
	RING = 0;
	TCNT = 3;
	RCNT = 0;
	init = 1;
}

void uart_deinit(void)
{
	init = 0;
	while (TING);
	Timer0_Stop();
	Timer0_InterruptDisable();
}
