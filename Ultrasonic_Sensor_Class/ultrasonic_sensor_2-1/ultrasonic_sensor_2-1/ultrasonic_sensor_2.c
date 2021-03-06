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

unsigned char Time_Delay_Polling(unsigned char tim_num, unsigned short d_time) ;   // 시간지연 체크함수(폴링방식)

volatile unsigned short    distance_1 = 0, distance_2 = 0,  distance_3 = 0, sensor_count = 0, active_sensor_flag = 0  ;
volatile unsigned short    distance_1_old = 0, distance_2_old = 0,  distance_3_old = 0 ;

volatile  unsigned char    Warning_Flag_1 = 0 , Warning_Flag_2 = 0;
volatile  unsigned short   Delay_Time[10] = {0,0,0,0,0,0,0,0,0,0};

int main() 
{   
	LcdInit();      //  LCd 초기화 함수 

	LcdMove(0,0); 
	LcdPuts("Dist_1 =    cm");
	LcdMove(1,0); 
	LcdPuts("Dist_2 =    cm");
//	LcdMove(1,0); 
//	LcdPuts("Dist_3 =    cm"); 
	
////  3 개의 초음파센서( Ultrasonic Sensor) ////////////

// 출력포트 설정 
	
	DDRB |= 0x07;     // 3 초음파센서 Trigger signals( PB0, PB1, PB2 : 출력포트 설정  )
					  // Data Direct Register B(0이면 입력, 1이면 출력)
					  // 초음파센서 Trigger signal( PB0,1,2 : 출력포트 설정)
					  // Trigger signal을 보내주어야 하기 때문에 출력 포트로 설정
					  // 원하는 비트만 1로 설정해 주는 법
					  // DDRB = XXXX XXXX
					  // 0x07 = 0000 0111		
					  //|------------------
					  //        XXXX X111	
	PORTB &= ~0x07;   // PB0, PB1, PB2  : Low  ( 3 Trigger signals OFF )  

	DDRB |= 0x10;     // 버저(Buzzer) ( PB4 : 출력포트 설정)
	PORTB &= ~0x10;   // PB4  : Low  ( 버저 OFF )  

	DDRB |= 0x20;     // LED(PB5 : 출력포트 설정)
	PORTB |= 0x20;    // PB5 : High(LED OFF)    

	DDRB |= 0x40;     // LED(PB6 : 출력포트 설정)
	PORTB |= 0x40;    // PB6 : High(LED OFF)

////////////  Timer 0 설정  ( 10 msec 주기 타이머 0 인터럽트 )  ///////////////
        
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

    TIMSK = 0x01;            // 타이머0 오버플로인터럽트 허용

///////////////////////////////////////////////////////////    
	
// 3 Echo Signal Pulse Width measurment,  Timer3 

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // 타이머 3 시작(분주비 8) ,  0.5usec 단위로 측정 

/////////////////////////////////////////////////////////

// 3 초음파센서 Echo Signals : external interrupt 4( pin: INT4 (PE4)),  external interrupt 5( pin: INT5 (PE5)) 
//                           : external interrupt 6( pin: INT4 (PE6)) 
	
	DDRE &= ~0x70;	  // PE4(INT4), PE5(INT5), PE6(INT6) set input

	EICRB |= 0x15;    // Both falling edge and rising edge interrupt
	EICRB &= ~0x2A;   // Both falling edge and rising edge interrupt

	EIMSK |= 0x70;    // INT4 Enable, INT5 Enable, INT6 Enable

	sei(); 

///////////////////////////////////////
/*
// 초음파센서 Echo Signals : external interrupt 0( pin: INT0 (PD0)),  
// p.109 

	DDRD &= ~0x01;		// PD0(INT0)를 입력으로 설정

	EICRA |= 0x03;     	// INT0 rising edge interrupt
						// EICRA = XXXX XX11
	EIMSK |= 0x01;     	// INT0 Enable 
	sei(); 				// 인터럽트 허용  <-> 금지 = cli();
*/
///////////////////////////////////////

// 최초 초음파센서 1 트리거 신호 발생(초음파 1 발사)  
	PORTB |= 0x01;    // PB0 : High
	usec_delay(20) ;  // 20usec 동안 High 유지 
	PORTB &= 0xFE;    // PB0 : Low 
          
	active_sensor_flag = 1; 
    sensor_count = 1;

/////////////////////////////////////////////
	
    TCCR0 |= 0x07;    // 타이머 0 시작(분주비 = 1024 ) 
		 
	while (1) 
	{ 
	    LcdMove(0, 9); 
        Display_Number_LCD(distance_1); 
	
 	    LcdMove(1, 9); 
        Display_Number_LCD(distance_2); 

//      LcdMove(1, 9); 
//      Display_Number_LCD(distance_3); 
     }
} 

ISR( TIMER0_OVF_vect )    //  10 msec 주기 타이머1 오버플로 인터럽트 서비스프로그램
{
	static unsigned short  time_index = 0 ; 
	
    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec
    time_index++ ; 
	
    if( time_index == 5 )   // 50 msec 주기 
    {
       time_index = 0; 
       sensor_count++;          // 초음파 센서 카운터 값 증가 
	       
	   if( sensor_count == 3 )  sensor_count = 1; 
	   
       if ( sensor_count == 1 )        //  초음파센서 1 트리거 신호 발생(초음파 1 발사) 
	   {
	      PORTB |= 0x01;    // PB0 : High
		  usec_delay(20) ;  // 20usec 동안 High 유지 
	      PORTB &= 0xFE;    // PB0 : Low 
          
		  active_sensor_flag = 1;
	   }
       else if ( sensor_count == 2 )   //  초음파센서 2 트리거 신호 발생(초음파 2 발사)
	   {
	      PORTB |= 0x02;    // PB1 : High
	 	  usec_delay(20) ;  // 20usec 동안 High 유지 
	      PORTB &= 0xFD;    // PB1 : Low 

		  active_sensor_flag = 2;
	   }
       /*else if ( sensor_count == 3 )   //  초음파센서 3 트리거 신호 발생(초음파 3 발사)
	   {
	      PORTB |= 0x04;    // PB2 : High
		  usec_delay(20) ;  // 20usec 동안 High 유지 
	      PORTB &= 0xFB;    // PB2 : Low 

		  active_sensor_flag = 3;
	   }*/

       ////////  경고음 발생   /////////////

       if( distance_1 <=  40)    Warning_Flag_1 = 1 ;     // 측정된 거리가 40 cm 이하이면 경고음 발생 플래그 set
       else 					 Warning_Flag_1 = 0 ;  
	   if( distance_2 <=  40)    Warning_Flag_2 = 1 ;     // 측정된 거리가 40 cm 이하이면 경고음 발생 플래그 set
       else 					 Warning_Flag_2 = 0 ;  
	      	
       Delay_Time[1] =  distance_1 / 10 + 1;            // 거리에 비레하는 주기(= Delay_Time * 50 msec )를 갖는 경고음 발생
	   Delay_Time[2] =  distance_2 / 10 + 1;            
       	    
	   if( Delay_Time[1] >= Delay_Time[2])   Delay_Time[0] = Delay_Time[2] ; 
	   else								 	 Delay_Time[0] = Delay_Time[1] ;

	   if( Delay_Time[0] <= 1) Delay_Time[0] = 1;
	   if( Delay_Time[0] >= 4) Delay_Time[0] = 4;
	   
	   if( Delay_Time[1] <= 1) Delay_Time[1] = 1;
	   if( Delay_Time[1] >= 4) Delay_Time[1] = 4;
	   
	   if( Delay_Time[2] <= 1) Delay_Time[2] = 1;
	   if( Delay_Time[2] >= 4) Delay_Time[2] = 4;
 
 	   if( Warning_Flag_1 == 1 || Warning_Flag_2 == 1)
	   {
           if( Time_Delay_Polling(0, Delay_Time[0] ) == 1 )     // 50msec * Delay_Time 경과 후 
	       {
           	   PORTB ^= 0x10  ;    // PB4(버저) toggle : 버저 단속음
	       }
           
	   }
       else if(Warning_Flag_1 == 0 && Warning_Flag_2 == 0)
	   {
           PORTB &= ~0x10  ;    // PB4(버저) OFF : 버저 OFF 
	   }
		/// 버저 컨트롤

       if( Warning_Flag_1 == 1 )
	   {
           if( Time_Delay_Polling(1, Delay_Time[1] ) == 1 )     // 50msec * Delay_Time 경과 후 
	       {
           	   PORTB ^= 0x20  ;    // PB5(LED) toggle :  LED ON, OFF 반복 
	       }        
	   }
       else if( Warning_Flag_1 == 0 )
	   {
           PORTB |= 0x20  ;     // PB5(LED)  0FF :  LED  OFF 
	   }
		/// LED4 컨트롤

	   if( Warning_Flag_2 == 1 )
	   {
           if( Time_Delay_Polling(2, Delay_Time[2] ) == 1 )     // 50msec * Delay_Time 경과 후 
	       {
           	   PORTB ^= 0x40  ;    // PB6(LED) toggle :  LED ON, OFF 반복 
	       }
	   }
       else if( Warning_Flag_2 == 0 )
	   {
       	   PORTB |= 0x40  ;     // PB6(LED)  0FF :  LED  OFF 
	   }
		/// LED5 컨트롤
	   	         
       /////////////////////////////////////  
   }
}

/*
ISR(INT0_vect)
{

	static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0;


	if (flag == 0)
	{
		count1 = TCNT3;
		flag = 1;
		EICRA |= 0x02;
		EICRA &= ~0x01;						// 하강 엣지에서 인터럽트가 걸리게 설정 변경
	} // 상승 인터럽트 
	else if (flag == 1)
	{
		count2 = TCNT3;
		del_T = count2 - count1;
		distance_1 = del_T / (2 * 58);		  	// 거리 계산 식

		if (distance_1 > 380)              // 반사되는 초음파가 검출되지 않을때 
		{
			distance_1 = distance_1_old;   // 직전 측정값 사용 
		}

		distance_1_old = distance_1;       // 직전 측정값 저장 변수 업데이트  

		flag = 0;
		EICRA |= 0x03;						// 상승 엣지에서 인터럽트가 걸리게 설정 변경
	} // 하강 인터럽트 
} // PD0(INT0 인터럽트 사용)
*/

ISR(INT4_vect)
{
    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;

    if ( active_sensor_flag == 1 )
 	{

	   if(flag == 0) 
	   {
		  count1 = TCNT3; 
		  flag = 1;
	  } 
	  else 
	  { 
		  count2 = TCNT3; 
		  del_T = count2 - count1;

    	  distance_1 = del_T/(2*58); 

          if( distance_1 > 380 )  // 반사되는 초음파가 검출되지 않을때 
		  {
		      distance_1 = distance_1_old ;   // 직전 측정값 사용 
		  } 

          distance_1_old = distance_1 ;    // 직전 측정값 저장 변수 업데이트  

		  flag = 0; 

	 	  active_sensor_flag = 0;
	  } 
    }
} 

ISR(INT5_vect)
{
    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;
 
	if ( active_sensor_flag == 2 )
	{
	   if(flag == 0) 
	   {
		  count1 = TCNT3; 
		  flag = 1;
	  } 
	  else 
	  { 
		  count2 = TCNT3; 
		  del_T = count2 - count1;
    	  distance_2 = del_T/(2*58); 

          if( distance_2 > 380 )  // 반사되는 초음파가 검출되지 않을때 
		  {
		      distance_2 = distance_2_old ;   // 직전 측정값 사용 
		  } 

          distance_2_old = distance_2 ;    // 직전 측정값 저장 변수 업데이트  

		  flag = 0; 

	 	  active_sensor_flag = 0;
	  } 
    }
} 

/*ISR(INT6_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;

    if ( active_sensor_flag == 3 )
	{

	   if(flag == 0) 
	   {
		  count1 = TCNT3; 
		  flag = 1;
	  } 
	  else 
	  { 
		  count2 = TCNT3; 
		  del_T = count2 - count1;
    	  distance_3 = del_T/(2*58); 

          if( distance_3 > 380 )  // 반사되는 초음파가 검출되지 않을때 
		  {
		      distance_3 = distance_3_old ;   // 직전 측정값 사용 
		  } 

          distance_3_old = distance_3 ;    // 직전 측정값 저장 변수 업데이트  

		  flag = 0; 

	 	  active_sensor_flag = 0;
	  } 

    }

} 
*/

void Display_Number_LCD( unsigned int num )       // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 
{

	HexToDec( num, 10); //10진수로 변환

	LcdPutchar(NumToAsc(cnumber[2]));    // 100자리 디스필레이 
	LcdPutchar(NumToAsc(cnumber[1]));    // 10자리 디스필레이
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
		_delay_ms(1);		// 1msec 시간 지연
}

void usec_delay(int n)
{	
	for(; n>0; n--)		// 1usec 시간 지연을 n회 반복
		_delay_us(1);		// 1usec 시간 지연
}

//////////////////////////////////////////////////////////

unsigned char Time_Delay_Polling(unsigned char tim_num, unsigned short d_time)
{

    static unsigned short  curr_delay[10] = {0,0,0,0,0,0,0,0,0,0}; 
	unsigned char  ret_val = 0;
	
    curr_delay[tim_num]++ ;  

    if( curr_delay[tim_num] >= d_time )   // 50msec * d_time 경과 후 
	{
       ret_val = 1; 
       curr_delay[tim_num] = 0 ;
	} 
    return  ret_val ;
}

