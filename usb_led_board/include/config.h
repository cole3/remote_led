
#ifndef		__CONFIG_H
#define		__CONFIG_H


/*********************************************************/

//#define MAIN_Fosc		22118400L	//������ʱ��
//#define MAIN_Fosc		12000000L	//������ʱ��
#define MAIN_Fosc		11059200L	//������ʱ��
//#define MAIN_Fosc		 5529600L	//������ʱ��
//#define MAIN_Fosc		24000000L	//������ʱ��


/*********************************************************/

#include	"STC15Fxxxx.H"


/**************************************************************************/

#define Main_Fosc_KHZ	(MAIN_Fosc / 1000)

/***********************************************************/

#define WHITE_LED	P32
#define COLOR_LED	P33
#define IR_SEND		P34
#define IR_REV		P35

/***********************************************************/

#endif