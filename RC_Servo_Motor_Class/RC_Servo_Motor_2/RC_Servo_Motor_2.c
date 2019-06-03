#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit )  ;     // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 


void msec_delay(int n)  ; 


static volatile unsigned char    Pos_CMD = 0 ;    // ���� ��ġ ��� ( ���� : 0 - 180,  ����:  �� )

int main() 
{   

	DDRB |= 0x80;    //  PWM ��Ʈ: OC2( PB7 ) ��¼��� 
 

	LcdInit();    //  LCD �ʱ�ȭ 

	LcdMove(0,0); 
	LcdPuts("RC Servo Motor");
	LcdMove(1,0); 
	LcdPuts("Servo_Pos = ");

    msec_delay(2000); 


// PWM ��ȣ  pin: OC2(PB7), Timer2, PWM signal (period= 16.384msec )

	TCCR2 |= 0x68;   //  Trigger signal (OC2)   �߻� :  WGM20(bit6)=1,  WGM21(bit3)=1,  COM21(bit5)=1, COM20(bit4)=0 ,  
	TCCR2 |= 0x05;   //  1024����,  ����Ŭ���ֱ� = 64usec  : CS22(bit2) = 1, CS21(bit1) = 0,  CS20(bit0) = 1 

                    
//  OCR2 = 10;     //  �޽��� = 0.64msec = 64usec * 10,   ���� ��(0 ��)  (�޽��� = 0.66msec )
//  OCR2 = 23;     //  �޽��� = 1.47msec = 64usec * 23 ,  ���(90 ��) (�޽��� = 1.5msec )
//  OCR2 = 37;     //  �޽��� = 2.37msec = 64usec * 37 ,  ������ ��(180 ��) (�޽��� = 2.45msec ) 

    Pos_CMD = 90 ;
    OCR2 = ( 135 * Pos_CMD )/900  + 10   ; 
	LcdMove(1, 12); 
    Display_Number_LCD( Pos_CMD , 3); 

	while (1) 
	{ 
        Pos_CMD = 0 ;   		                 // ���� ��ġ ��� =  0 �� (���� ��)  
        OCR2 = ( 135 * Pos_CMD )/900  + 10  ;   

	    LcdMove(1, 12); 
        Display_Number_LCD( Pos_CMD , 3 ); 

        msec_delay(5000);

                                
        Pos_CMD = 90 ;                           // ���� ��ġ ��� =  90 �� (���)  
        OCR2 = ( 135 * Pos_CMD )/900  + 10   ; 

	    LcdMove(1, 12); 
        Display_Number_LCD( Pos_CMD , 3  ); 

        msec_delay(5000);


        Pos_CMD = 180 ;                          // ���� ��ġ ��� =  180 �� (������ ��)   
        OCR2 = ( 135 * Pos_CMD )/900  + 10   ; 

	    LcdMove(1, 12); 
        Display_Number_LCD( Pos_CMD , 3 ); 

        msec_delay(5000); 

  
     }


} 



void Display_Number_LCD( unsigned int num, unsigned char digit )       // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
{

	HexToDec( num, 10); //10������ ��ȯ 

	if( digit == 0 )     digit = 1 ;
	if( digit > 5 )      digit = 5 ;
 
    if( digit >= 5 )     LcdPutchar( NumToAsc(cnumber[4]) );  // 10000�ڸ� ���÷���
	
	if( digit >= 4 )     LcdPutchar(NumToAsc(cnumber[3]));    // 1000�ڸ� ���÷��� 

	if( digit >= 3 )     LcdPutchar(NumToAsc(cnumber[2]));    // 100�ڸ� ���÷��� 

	if( digit >= 2 )     LcdPutchar(NumToAsc(cnumber[1]));    // 10�ڸ� ���÷���

	if( digit >= 1 )     LcdPutchar(NumToAsc(cnumber[0]));    //  1�ڸ� ���÷���

}


void HexToDec( unsigned short num, unsigned short radix) 
{
	int j ;

	for(j=0; j<5 ; j++) cnumber[j] = 0 ;

	j=0;
	do
	{
		cnumber[j++] = num % radix ; 
		num /= radix; 

	} while(num);

} 

char NumToAsc( unsigned char Num )
{
	if( Num <10 ) Num += 0x30; 
	else          Num += 0x37; 

	return Num ;
}



void msec_delay(int n)
{	
	for(; n>0; n--)		// 1msec �ð� ������ nȸ �ݺ�
		_delay_ms(1);		// 1msec �ð� ����
}



