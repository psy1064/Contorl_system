#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num )  ;       // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 
void Display_Distance_LCD( unsigned int num )  ;     // 초음파가 측정한 거리를 10진수 형태로 LCD 에 디스플레이 

void msec_delay(int n)  ;   // msec 단위 시간지연
void usec_delay(int n)  ;   // usec 단위 시간지연

unsigned char Time_Delay_Polling( unsigned short d_time ) ;   // 시간지연 체크함수(폴링방식)


static volatile unsigned short    distance_1 = 0  ;

static volatile unsigned short    distance_1_old = 0 ;

static volatile  unsigned char    Warning_Flag = 0 ;
static volatile  unsigned short   Delay_Time = 0;



int main() 
{   


	LcdInit();      //  LCd 초기화 함수 

	LcdMove(0,0); 
	LcdPuts("UltrasonicSensor"); 
	LcdMove(1,0); 
	LcdPuts("Dist_1 =      cm");


////   초음파센서( Ultrasonic Sensor) ////////////

// 출력포트 설정 
	
	DDRB |= 0x01;     // 초음파센서 Trigger signal( PB0 : 출력포트 설정  )
	PORTB &= ~0x01;   // PB0  : Low  ( Trigger signal OFF )  

	DDRB |= 0x10;     // 버저(Buzzer) ( PB4 : 출력포트 설정)
	PORTB &= ~0x10;   // PB4  : Low  ( 버저 OFF )  

	DDRB |= 0x20;     // LED ( PB5 : 출력포트 설정)
	PORTB |= 0x20;   // PB5  : High ( LED OFF)    


 ////////////  Timer 0 설정  ( 10 msec 주기 타이머 0 인터럽트 )  ///////////////
        
    TCCR0 = 0x00;            // 타이머 0 정지(분주비 = 1024 ) , Normal mode(타이머모드)

    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec

    TIMSK = 0x01;            // 타이머0 오버플로인터럽트 허용
///////////////////////////////////////////////////////////    


// Echo Signal Pulse Width measurment,  Timer3 

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // 타이머 3 시작(분주비 8) ,  0.5usec 단위로 측정 

/////////////////////////////////////////////////////////

 
// 초음파센서 Echo Signals : external interrupt 4( pin: INT4 (PE4)),  

	EICRB = 0x01;      // INT4 Both falling edge and rising edge interrupt
	EIMSK |= 0x10;     // INT4 Enable 
	sei(); 

///////////////////////////////////////

   //  최초 초음파센서 1 트리거 신호 발생(초음파 1 발사)  
	PORTB |= 0x01;    // PB0 : High
	usec_delay(20) ;  // 20usec 동안 High 유지 
	PORTB &= 0xFE;    // PB0 : Low 
          

  /////////////////////////////////////////////


    TCCR0 |= 0x07;    // 타이머 0 시작(분주비 = 1024 )  

	 
	while (1) 
	{ 

	    LcdMove(1, 9); 
        Display_Distance_LCD( distance_1 ); 


    }


} 



ISR( TIMER0_OVF_vect )    //  10 msec 주기 타이머1 오버플로 인터럽트 서비스프로그램
{

    static unsigned short  time_index = 0 ; 


    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec

    time_index++ ; 


    if( time_index == 5 )    // 50 msec 주기 
    {

       time_index = 0; 

       ////////  초음파센서 1 트리거 신호 발생(초음파 1 발사)  /////////

	   PORTB |= 0x01;    // PB0 : High
	   usec_delay(20) ;  // 20usec 동안 High 유지 
	   PORTB &= 0xFE;    // PB0 : Low 



       ////////  경고음 발생   /////////////

/***
        if( distance_1 >  400 ) 
		{
		   Warning_Flag = 0 ;     

		}
		else if(  distance_1 >= 300 )
		{
		   Warning_Flag = 1 ;
		   Delay_Time =  5;    // 0.5초 주기 단속음 
		}

		else if(  distance_1 >= 200 )
		{
		   Warning_Flag = 1 ;
		   Delay_Time =  3;     // 0.3초 주기 단속음 
		}

		else if(  distance_1 >= 100 )
		{
		   Warning_Flag = 1 ;
		   Delay_Time =  2;     // 0.2초 주기 단속음
		}

		else if(  distance_1 >= 0 )
		{
		   Warning_Flag = 1 ;
		   Delay_Time =  1;    // 0.1초 주기 단속음  
		}
 
***/
 
       if( distance_1 <=  400 )   Warning_Flag = 1 ;     // 측정된 거리가 40 cm 이하이면 경고음 발생 플래그 set
       else                       Warning_Flag = 0 ;    
		
       Delay_Time =  distance_1 / 100 + 1;            // 거리에 비레하는 주기(= Delay_Time * 50 msec )를 갖는 경고음 발생
        
	   if( Delay_Time <= 1)   Delay_Time = 1 ;   // 경고음주기 하한 : 0.1초
	   if( Delay_Time >= 4)   Delay_Time = 4 ;   // 경고음주기 상한 : 0.


       if( Warning_Flag == 1 )
	   {
           if( Time_Delay_Polling( Delay_Time ) == 1 )     // 50msec * Delay_Time 경과 후 
	       {
               PORTB ^= 0x10  ;    // PB1(버저) toggle : 버저 단속음 
			   PORTB ^= 0x20  ;    // PB2(LED) toggle :  LED ON, OFF 반복 
	       }
	   }
       else if( Warning_Flag == 0 )
	   {
           PORTB &= ~0x10  ;    // PB1(버저) OFF : 버저 OFF 
		   PORTB |= 0x20  ;     // PB2(LED) OFF :  LED  OFF 
	   }
      
       /////////////////////////////////////  

   }


}


ISR(INT4_vect)
{

    static unsigned short count1 = 0, count2 = 0, flag = 0 ;
    unsigned short   del_T = 0, del_T_usec  = 0 ;


	  if(flag == 0) 
	  {
		  count1 = TCNT3; 
		  flag = 1;
	  } 
	  else 
	  { 
		  count2 = TCNT3; 

		  del_T = count2 - count1;

 		  del_T_usec = del_T / 2 ;            // 초음파 왕복시간 [ usec ] 단위 
 
      	  distance_1 = (unsigned short)((unsigned long)17 * del_T_usec/100);		// S = 340[m/s[ * T/2[usec]
		  																			// 거리 계산 [ mm ] 단위
 
          if( distance_1 > 3800 )              // 반사되는 초음파가 검출되지 않을때 
		  {
		      distance_1 = distance_1_old ;   // 직전 측정값 사용 
		  } 

          distance_1_old = distance_1 ;       // 직전 측정값 저장 변수 업데이트  

		  flag = 0; 

	  } 


} 



void Display_Number_LCD( unsigned int num )       // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 
{

	HexToDec( num, 10); //10진수로 변환


	LcdPutchar(NumToAsc(cnumber[4]));    // 10000자리 디스필레이 

	LcdPutchar(NumToAsc(cnumber[3]));    // 1000자리 디스필레이

	LcdPutchar(NumToAsc(cnumber[2]));    //  100자리 디스플레이

	LcdPutchar(NumToAsc(cnumber[1]));    //  10자리 디스플레이 

	LcdPutchar(NumToAsc(cnumber[0]));    //  1자리 디스플레이
}


void Display_Distance_LCD( unsigned int num )       // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 
{

	HexToDec( num, 10); //10진수로 변환


	LcdPutchar(NumToAsc(cnumber[3]));    // 100자리 디스필레이 

	LcdPutchar(NumToAsc(cnumber[2]));    // 10자리 디스필레이

	LcdPutchar(NumToAsc(cnumber[1]));    //  1자리 디스플레이

	LcdPuts("." );                       //  소숫점 디스플레이

	LcdPutchar(NumToAsc(cnumber[0]));    //  소수 1자리 디스플레이
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



void usec_delay(int n)
{	
	for(; n>0; n--)		// 1usec 시간 지연을 n회 반복
		_delay_us(1);		// 1usec 시간 지연
}


//////////////////////////////////////////////////////////

unsigned char Time_Delay_Polling( unsigned short d_time )
{

    static unsigned short  curr_delay = 0; 
	unsigned char  ret_val = 0;


    curr_delay++ ;  

    if( curr_delay >= d_time )   // 50msec * d_time 경과 후 
	{
       ret_val = 1; 
       curr_delay = 0 ;
	} 


    return  ret_val ;


}


