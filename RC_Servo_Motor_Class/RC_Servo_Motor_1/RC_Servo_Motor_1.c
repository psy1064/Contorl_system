#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이


void msec_delay(int n)  ; 



int main() 
{   
	DDRE |= 0x08;    //  PWM 포트: OC3A( PE3 ) 출력설정 
 

	LcdInit();    //  LCD 초기화 

	LcdMove(0,0); 
	LcdPuts("RC Servo Motor");
	LcdMove(1,0); 
	LcdPuts("duty =     ");

    msec_delay(2000); 


// PWM 신호  pin: OC3A(PE3), Timer3, PWM signal (period= 20msec )

	TCCR3A = 0x82;   // Trigger signal 3 (OC3A) 먼저 발생 
	TCCR3B = 0x1B;   // 64분주,  내부클럭주기 = 4usec
 	ICR3 = 5000;     //  PWM signal (period= 20msec )
    OCR3A = 375;    //  펄스폭 = 1.5msec = 4usec * 375, 가운데

	LcdMove(1, 9); 
    Display_Number_LCD( OCR3A , 3 ); 

// 	ICR3 = 834;     //  PWM signal (period= 3.33msec )
// 	ICR3 = 750;     //  PWM signal (period= 3.0msec )
	int i ;
	while (1) 
	{ 
		for(i = 165 ; i < 613 ; i++)
		{

		    OCR3A = i;        //  165 :  펄스폭 = 0.66msec = 165 * 4usec,  왼쪽 끝 

	    	LcdMove(1, 9); 
        	Display_Number_LCD( OCR3A , 3 ); 

	        msec_delay(10);
		}
		for(i = 613 ; i > 165 ; i--)
		{

		    OCR3A = i;        //  165 :  펄스폭 = 0.66msec = 165 * 4usec,  왼쪽 끝 

	    	LcdMove(1, 9); 
        	Display_Number_LCD( OCR3A , 3 ); 

	        msec_delay(10);
		}
	    /*OCR3A = 570;        //  375 ;  펄스폭 = 1.5msec  = 4usec * 375,  가운데

	    LcdMove(1, 9); 
        Display_Number_LCD( OCR3A , 3 ); 

        msec_delay(1000);

	    OCR3A = 620;        //  613 :  펄스폭 = 2.45msec = 4usec * 613,  오른쪽 

	    LcdMove(1, 9); 
        Display_Number_LCD( OCR3A , 3); 

        msec_delay(1000); 

 		OCR3A = 570;        //  375 ;  펄스폭 = 1.5msec  = 4usec * 375,  가운데

	    LcdMove(1, 9); 
        Display_Number_LCD( OCR3A , 3 ); 

  		msec_delay(1000); */
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



