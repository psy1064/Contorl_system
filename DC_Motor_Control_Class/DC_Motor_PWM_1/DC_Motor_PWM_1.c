#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 
	 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 


void msec_delay(int n) ;
 
void DC_Motor_Run_Fwd( short duty );    // DC 모터 정회전(PWM구동) 함수 
void DC_Motor_Run_Rev( short duty );    // DC 모터 역회전(PWM구동) 함수  
void DC_Motor_Stop( void );             // DC 모터 정지 함수  
 
void DC_Motor_PWM( short Vref );        // DC 모터 PWM 신호 발생 함수  
                                        // 정토크(Vref>0), 역토크(Vref<0), 영토크(Vref=0) 모두 포함 

static volatile short  Vmax = 3200 ;    //  Vmax = 3200  :  최대 전압( PWM duty = 100 % )


int main() 
{   
	short duty = 0;       


	DDRB |= 0x20;   // 모터구동신호 + 단자:  PWM 포트( pin: OC1A(PB5) )   --> 출력 설정 
	DDRA |= 0x01;   // 모터구동신호 - 단자 : 범용 입/출력포트(pin : PA0 ) --> 출력 설정 

	LcdInit();
 

// 모터구동신호 ( pin: OC1A(PB5) ),   Timer1, PWM signal (period= 200 usec )

	TCCR1A = 0x82;    // OC1A(PB5)) :  PWM 포트 설정,   Fast PWM ( mode 14 )
	TCCR1B = 0x19;    // 1 분주( No Prescale ) 타이머 1 시작 (내부클럭 주기 =  1/(16*10^6) = 1/16 usec ),  Fast PWM ( mode 14 ) 
	ICR1 = Vmax;      // PWM 주기 = 3200 * 1/16 usec = 200 usec


	OCR1A = duty;      //  OC1A(PB5) PWM duty = 0 설정 : 모터 정지
//////////////////////////////////////////////////////////////////

    duty = 3200;      // 모터 속도 설정, 최대 = Vmax = 50,  최소 = 0 

	LcdMove(0,0); 
	LcdPuts("DC Motor Control");

	LcdMove(1,0); 
	LcdPuts("Duty = ");

	 
	 
	while (1) 
	{ 

	    LcdMove(1,7); 
        Display_Number_LCD(duty, 4); 

 
		DC_Motor_Run_Fwd( duty );     // DC Motor 정회전 5초
        msec_delay( 5000 ); 
 
		DC_Motor_Stop();              // DC Motor 정지 2초
        msec_delay( 2000 ); 


		DC_Motor_Run_Rev( duty );     // DC Motor 역회전 5초
        msec_delay( 5000 ); 
 
		DC_Motor_Stop();              // DC Motor 정지 2초
        msec_delay( 2000 ); 


     }


} 



void DC_Motor_Run_Fwd( short duty )   // DC 모터 정회전 함수 
{

    if( duty > Vmax )     duty = Vmax ;

    PORTA &= ~0x01;     //  모터구동신호 - 단자 : 0 V 인가( PA0 = 0 );  
	OCR1A = duty;       //  모터구동신호 + 단자 : OC1A(PB5) PWM duty 설정 


}

void DC_Motor_Run_Rev( short duty )   // DC 모터 역회전 함수 
{

    if( duty > Vmax )     duty = Vmax ;

    PORTA |= 0x01;            //  모터구동신호 - 단자 : 5 V 인가( PA0 = 1 );  
	OCR1A = Vmax - duty;      //  모터구동신호 + 단자 : OC1A(PB5) PWM duty 설정 


}


void DC_Motor_Stop( void )   // DC 모터 정지 함수 
{

    PORTA &= ~0x01;     //  모터구동신호 - 단자 : 0 V 인가( PA0 = 0 );  
	OCR1A = 0;          //  모터구동신호 + 단자 : OC1A(PB5) PWM duty = 0 설정 


}



void DC_Motor_PWM( short Vref )   // DC 모터 PWM 신호 발생 함수  
{

   if ( Vref > Vmax )       Vref = Vmax ;
   else if( Vref < -Vmax )  Vref = -Vmax ;

   if( Vref > 0 )  
   {
      DC_Motor_Run_Fwd( Vref ) ;
   }
   else if( Vref == 0 )  
   {
      DC_Motor_Stop() ;
   }
   else if( Vref < 0 )  
   {
      DC_Motor_Run_Rev( -Vref ) ;
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





