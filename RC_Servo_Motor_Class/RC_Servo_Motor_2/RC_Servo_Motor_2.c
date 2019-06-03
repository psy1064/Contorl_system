#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit )  ;     // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 


void msec_delay(int n)  ; 


static volatile unsigned char    Pos_CMD = 0 ;    // 서보 위치 명령 ( 범위 : 0 - 180,  단위:  도 )

int main() 
{   

	DDRB |= 0x80;    //  PWM 포트: OC2( PB7 ) 출력설정 
 

	LcdInit();    //  LCD 초기화 

	LcdMove(0,0); 
	LcdPuts("RC Servo Motor");
	LcdMove(1,0); 
	LcdPuts("Servo_Pos = ");

    msec_delay(2000); 


// PWM 신호  pin: OC2(PB7), Timer2, PWM signal (period= 16.384msec )

	TCCR2 |= 0x68;   //  Trigger signal (OC2)   발생 :  WGM20(bit6)=1,  WGM21(bit3)=1,  COM21(bit5)=1, COM20(bit4)=0 ,  
	TCCR2 |= 0x05;   //  1024분주,  내부클럭주기 = 64usec  : CS22(bit2) = 1, CS21(bit1) = 0,  CS20(bit0) = 1 

                    
//  OCR2 = 10;     //  펄스폭 = 0.64msec = 64usec * 10,   왼쪽 끝(0 도)  (펄스폭 = 0.66msec )
//  OCR2 = 23;     //  펄스폭 = 1.47msec = 64usec * 23 ,  가운데(90 도) (펄스폭 = 1.5msec )
//  OCR2 = 37;     //  펄스폭 = 2.37msec = 64usec * 37 ,  오른쪽 끝(180 도) (펄스폭 = 2.45msec ) 

    Pos_CMD = 90 ;
    OCR2 = ( 135 * Pos_CMD )/900  + 10   ; 
	LcdMove(1, 12); 
    Display_Number_LCD( Pos_CMD , 3); 

	while (1) 
	{ 
        Pos_CMD = 0 ;   		                 // 서보 위치 명령 =  0 도 (왼쪽 끝)  
        OCR2 = ( 135 * Pos_CMD )/900  + 10  ;   

	    LcdMove(1, 12); 
        Display_Number_LCD( Pos_CMD , 3 ); 

        msec_delay(5000);

                                
        Pos_CMD = 90 ;                           // 서보 위치 명령 =  90 도 (가운데)  
        OCR2 = ( 135 * Pos_CMD )/900  + 10   ; 

	    LcdMove(1, 12); 
        Display_Number_LCD( Pos_CMD , 3  ); 

        msec_delay(5000);


        Pos_CMD = 180 ;                          // 서보 위치 명령 =  180 도 (오른쪽 끝)   
        OCR2 = ( 135 * Pos_CMD )/900  + 10   ; 

	    LcdMove(1, 12); 
        Display_Number_LCD( Pos_CMD , 3 ); 

        msec_delay(5000); 

  
     }


} 



void Display_Number_LCD( unsigned int num, unsigned char digit )       // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 
{

	HexToDec( num, 10); //10진수로 변환 

	if( digit == 0 )     digit = 1 ;
	if( digit > 5 )      digit = 5 ;
 
    if( digit >= 5 )     LcdPutchar( NumToAsc(cnumber[4]) );  // 10000자리 디스플레이
	
	if( digit >= 4 )     LcdPutchar(NumToAsc(cnumber[3]));    // 1000자리 디스플레이 

	if( digit >= 3 )     LcdPutchar(NumToAsc(cnumber[2]));    // 100자리 디스플레이 

	if( digit >= 2 )     LcdPutchar(NumToAsc(cnumber[1]));    // 10자리 디스플레이

	if( digit >= 1 )     LcdPutchar(NumToAsc(cnumber[0]));    //  1자리 디스플레이

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
	for(; n>0; n--)		// 1msec 시간 지연을 n회 반복
		_delay_ms(1);		// 1msec 시간 지연
}



