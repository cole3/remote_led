
/*************	����˵��	**************

���ļ�Ϊģ�⴮�ڷ��ͳ���, һ��Ϊ���Լ����.

���ڲ���:9600,8,n,1.

���Ը�����ʱ���Զ���Ӧ.

******************************************/

#include	"soft_uart.h"
#include 	"timer.h"

/***************************************************************************/


#define BaudRate		9600		//ģ�⴮�ڲ�����
#define Timer0_Reload	(65536 - MAIN_Fosc / BaudRate / 3)
#define D_RxBitLenth	9		//9: 8 + 1 stop
#define D_TxBitLenth	9		//9: 1 stop bit

sbit RXB = P3^0;                //define UART TX/RX port
sbit TXB = P3^1;

static unsigned char TBUF,RBUF;
static unsigned char TCNT,RCNT;	//���ͺͽ��ռ�� ������(3�����ʼ��)
static unsigned char TBIT,RBIT;	//���ͺͽ��յ����ݼ�����
static unsigned char rcv_cnt, get_cnt;
static unsigned char buf[16];

static bit TING,RING;	//���ڷ��ͻ����һ���ֽ�
static bit init;

#define	RxBitLenth	9	//8������λ+1��ֹͣλ
#define	TxBitLenth	9	//8������λ+1��ֹͣλ


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
		if (--RCNT == 0)				  //���������Զ�ʱ����1/3������
		{
			RCNT = 3;                   //���ý��ռ�����  ���������Զ�ʱ����1/3������	reset send baudrate counter
			if (--RBIT == 0)			  //������һ֡����
			{
				RING = 0;               //ֹͣ����			stop receive
				buf[rcv_cnt++ & 0x0f] = RBUF;
			}
			else
			{
				RBUF >>= 1;			  //�ѽ��յĵ�b���� �ݴ浽 RDAT(���ջ���)
				if (RXB) RBUF |= 0x80;  //shift RX data to RX buffer
			}
		}
	}
	else if (!RXB)		//�ж��ǲ��ǿ�ʼλ RXB=0;
	{
		RING = 1;       //����������ÿ�ʼ���ձ�־λ 	set start receive flag
		RCNT = 4;       //��ʼ�����ղ����ʼ�����       	initial receive baudrate counter
		RBIT = RxBitLenth;       //��ʼ�����յ�����λ��(8������λ+1��ֹͣλ)    initial receive bit number (8 data bits + 1 stop bit)
	}

	if (TING)			//���Ϳ�ʼ��־λ   judge whether sending
	{
		if (--TCNT == 0)			//���������Զ�ʱ����1/3������
		{
			TCNT = 3;				//���÷��ͼ�����   reset send baudrate counter
			if (TBIT == 0)			//���ͼ�����Ϊ0 �������ֽڷ��ͻ�û��ʼ
			{
				TXB = 0;			//���Ϳ�ʼλ     					send start bit
				TBIT = TxBitLenth;	//��������λ�� (8����λ+1ֹͣλ)	initial send bit number (8 data bits + 1 stop bit)
			}
			else					//���ͼ�����Ϊ��0 ���ڷ�������
			{
				if (--TBIT == 0)	//���ͼ�������Ϊ0 �������ֽڷ��ͽ���
				{
					TXB = 1;		//��ֹͣλ����
					TING = 0;		//����ֹͣλ    			stop send
				}
				else
				{
					TBUF >>= 1;		//�����λ�͵� CY(�洦��־λ) shift data to CY
					TXB = CY;		//���͵�b����				write CY to TX port
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