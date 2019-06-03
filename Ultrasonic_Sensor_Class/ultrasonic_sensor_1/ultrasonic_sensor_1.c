#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 
char NumToAsc( unsigned char Num ); 
static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 
	 
void Display_Number_LCD( unsigned int num )  ;     // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 

void msec_delay(int n)  ;   // msec 단위 시간지연
void usec_delay(int n)  ;   // usec 단위 시간지연

static volatile unsigned short    distance_1 = 0  ;
static volatile unsigned short    distance_1_old = 0 ;

int main() 
{   

	unsigned short dist_1 = 0  ;


	LcdInit();      //  LCd 초기화 함수 

	LcdMove(0,0); 
	LcdPuts("UltrasonicSensor"); 
	LcdMove(1,0); 
	LcdPuts("Dist_1 =    cm");


//// 초음파센서( Ultrasonic Sensor) ////////////

//// 출력포트 설정 
	
	DDRB |= 0x01;     	// Data Direct Register B(0이면 입력, 1이면 출력)
						// 초음파센서 Trigger signal( PB0 : 출력포트 설정  )
					  	// Trigger signal을 보내주어야 하기 때문에 출력 포트로 설정
						// 원하는 비트만 1로 설정해 주는 법
					  	// DDRB = XXXX XXXX
						// 0x01 = 0000 0001
						//|------------------
						//        XXXX XXX1
	DDRB |= 0x10;		// DDRB = XXX1 XXXX 버저 출력
	DDRB |= 0x20;		// DDRB = XX1X XXXX LED 출력 

	PORTB &= ~0x01;   	// PB0  : Low  ( Trigger signal OFF )  
						// 원하는 비트만 0로 설정해 주는 법
						// PORTB = XXXX XXXX
						// ~0x01 = 1111 1110
						//&--------------------
						//         XXXX XXX0  
						// PORTB = XXXX XXX0
	PORTB &= ~0x10;		// 버저 OFF(LOW)
	PORTB |= 0x20;		// LED OFF(HIGH)


////////////  Timer 0 설정  ( 10 msec 주기 타이머 0 인터럽트 )  ///////////////
// 50msec마다 Trigger 신호를 초음파 모듈에 보냄
// p. 133
        
    TCCR0 = 0x00;            // 타이머 0 정지(분주비 = 1024 ) , Normal mode(타이머모드)
						
    TCNT0 = 256 - 156;       // 초깃값 설정 	
						     // 내부클럭주기 = 1024(=분주비) / 16x10^6(=내부 클럭) = 64 usec,  
                             // 오버플로인터럽트 주기 = 10msec
							 // 내브클럭주기 x n = 오버플로인터럽트 주기
                             // 64usec x n = 10mesc
							 // n = 10msec / 64usec
							 // therefore n = 156
							 // 64usec 클럭이 156번 반복됐을때가 10msec임
							 // 오버플로우가 발생했을때 인터럽트가 걸리기 때문에 8bit가 오버플로우가 걸리는 256값에서 n값을 빼주면
							 // n번 반복했을때 오버플로우가 걸린다

    TIMSK |= 0x01;            // 타이머 0 오버플로인터럽트 허용

///////////////////////////////////////////////////////////    


// Echo Signal Pulse Width measurment,  Timer3 
// 에코 신호의 폭 시간을 측정하기 위한 타이머 인터럽트
// p.238

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // 타이머 3 시작(분주비 8) ,  0.5usec 단위로 측정 

/////////////////////////////////////////////////////////

/* 
// 초음파센서 Echo Signals : external interrupt 4( pin: INT4 (PE4)),  
// p.109 

	DDRE &= ~0x10;		// PE4(INT4)를 입력으로 설정

	EICRB |= 0x01;     	// INT4 Both falling edge and rising edge interrupt
						// EICRB = XXXX XXX1
	EICRB &= ~0x02;		// EICRB = XXXX XX0X
	EIMSK |= 0x10;     	// INT4 Enable 
	sei(); 				// 인터럽트 허용  <-> 금지 = cli();

///////////////////////////////////////
*/

// 초음파센서 Echo Signals : external interrupt 0( pin: INT0 (PD0)),  
// p.109 

	DDRD &= ~0x01;		// PD0(INT0)를 입력으로 설정

	EICRA |= 0x03;     	// INT0 rising edge interrupt
						// EICRA = XXXX XX11
	EIMSK |= 0x01;     	// INT0 Enable 
	sei(); 				// 인터럽트 허용  <-> 금지 = cli();

///////////////////////////////////////

// 최초 초음파센서 1 트리거 신호 발생(초음파 1 발사)  


	PORTB |= 0x01;    	// PB0 : High
	usec_delay(20);  	// 20usec 동안 High 유지 
	PORTB &= ~0x01;    	// PB0 : Low
						// = PORTB &= 0xFE;
          
//////////////////////////////////////

    TCCR0 |= 0x07;    // 타이머 0 시작(분주비 = 1024)

////////////////////////////////////// 
 
	while (1) 
	{ 
		cli();			// 데이터를 읽을땐 인터럽트 금지
 	    dist_1 = distance_1 ;
 		sei();			// 인터럽트 허용 

	    LcdMove(1, 9); 
        Display_Number_LCD(dist_1); 
    }
} // 프로그램 동작

//////////////////////////////////////

ISR(TIMER0_OVF_vect)    //  10 msec 주기 타이머1 오버플로 인터럽트 서비스프로그램
{
    static unsigned short  time_index = 0 ; 

    TCNT0 = 256 - 156;   // 초기값을 또 설정해주어야 함 

    time_index++ ; 

    if( time_index == 5 )   // 50 msec 주기 
    {

       time_index = 0; 

       //  초음파센서 1 트리거 신호 발생(초음파 1 발사) 

	   PORTB |= 0x01;    // PB0 : High
	   usec_delay(20) ;  // 20usec 동안 High 유지 
	   PORTB &= ~0x01;    // PB0 : Low 
	   if( distance_1 < 40)
	   {
   	   		PORTB |= 0x10;		// 버저 ON(LOW)
			PORTB &= ~0x20;		// LED ON(HIGH)
       }
	   else
	   {
   	   		PORTB &= ~0x10;		// 버저 OFF(LOW)
			PORTB |= 0x20;		// LED OFF(HIGH)
	   }

   }
} // 메인 제어 문
/*ISR(INT4_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;


	if(flag == 0) 
	{
		count1 = TCNT3; 
	  	flag = 1;

	} // 상승 인터럽트 
	else if(flag == 1)
	{ 
	  	count2 = TCNT3; 
		del_T = count2 - count1;
    	distance_1 = del_T/(2*58);		  	// 거리 계산 식

        if( distance_1 > 380 )              // 반사되는 초음파가 검출되지 않을때 
		{
			distance_1 = distance_1_old ;   // 직전 측정값 사용 
		} 

        distance_1_old = distance_1 ;       // 직전 측정값 저장 변수 업데이트  

		flag = 0; 
	} // 하강 인터럽트 
}*/

ISR(INT0_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;


	if(flag == 0) 
	{
		count1 = TCNT3; 
	  	flag = 1;
		EICRA |= 0x02;						
		EICRA &= ~0x01;						// 하강 엣지에서 인터럽트가 걸리게 설정 변경
	} // 상승 인터럽트 
	else if(flag == 1)
	{ 
	  	count2 = TCNT3; 
		del_T = count2 - count1;
    	distance_1 = del_T/(2*58);		  	// 거리 계산 식

        if( distance_1 > 380 )              // 반사되는 초음파가 검출되지 않을때 
		{
			distance_1 = distance_1_old ;   // 직전 측정값 사용 
		} 

        distance_1_old = distance_1 ;       // 직전 측정값 저장 변수 업데이트  

		flag = 0; 
		EICRA |= 0x03;						// 상승 엣지에서 인터럽트가 걸리게 설정 변경
	} // 하강 인터럽트 
}
void Display_Number_LCD( unsigned int num )       // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 
{

	HexToDec( num, 10); //10진수로 변환


	LcdPutchar(NumToAsc(cnumber[2]));    // 100자리 디스플레이 

	LcdPutchar(NumToAsc(cnumber[1]));    // 10자리 디스플레이

	LcdPutchar(NumToAsc(cnumber[0]));    //  1자리 디스플레이

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
		_delay_ms(1);	// 1msec 시간 지연
}
void usec_delay(int n)
{	
	for(; n>0; n--)		// 1usec 시간 지연을 n회 반복
		_delay_us(1);	// 1usec 시간 지연
}
