#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 


void msec_delay(int n)  ;   // msec 단위 시간지연
void usec_delay(int n)  ;   // usec 단위 시간지연

unsigned char Time_Delay_Polling( unsigned short d_time ) ;   // 시간지연 체크함수(폴링방식)

////////////////////////////////////////

void DC_Motor_Run_Fwd_L( short duty );    // 왼쪽 DC 모터 정회전(PWM구동) 함수 
void DC_Motor_Run_Rev_L( short duty );    // 왼쪽 DC 모터 역회전(PWM구동) 함수  
void DC_Motor_Stop_L( void );             // 왼쪽 DC 모터 정지 함수  
 
void DC_Motor_Run_Fwd_R( short duty );    // 오른쪽 DC 모터 정회전(PWM구동) 함수 
void DC_Motor_Run_Rev_R( short duty );    // 오른쪽 DC 모터 역회전(PWM구동) 함수  
void DC_Motor_Stop_R( void );             // 오른쪽 DC 모터 정지 함수  

void DC_Motor_PWM_L( short Vref );        // 왼쪽 DC 모터 PWM 신호 발생 함수  
                                          // 정토크(Vref>0), 역토크(Vref<0), 영토크(Vref=0) 모두 포함 
void DC_Motor_PWM_R( short Vref );        // 오른쪽 DC 모터 PWM 신호 발생 함수  
                                          // 정토크(Vref>0), 역토크(Vref<0), 영토크(Vref=0) 모두 포함 

static volatile short  Vmax = 0, Vmax_L = 0, Vmax_R = 0  ; 

//////////////////////////////////////


static volatile unsigned short    distance_1 = 0, distance_2 = 0,  distance_3 = 0, sensor_count = 0, active_sensor_flag = 0  ;

static volatile unsigned short    distance_1_old = 0, distance_2_old = 0,  distance_3_old = 0 ;

static volatile  unsigned char    Warning_Flag = 0 ;
static volatile  unsigned short   Delay_Time = 0;


int main() 
{   
	short duty_L = 0, duty_R = 0; 


////  3 개의 초음파센서( Ultrasonic Sensor) ////////////

// 출력포트 설정 
	
	DDRB |= 0x07;     // 3 초음파센서 Trigger signals( PB0, PB1, PB2 : 출력포트 설정  )
	PORTB &= ~0x07;   // PB0, PB1, PB2  : Low  ( 3 Trigger signals OFF )  

	DDRB |= 0x08;     // 버저(Buzzer) ( PB3 : 출력포트 설정    )
	PORTB &= ~0x08;   // PB3  : Low  ( 버저 OFF )  

	DDRB |= 0x10;     // LED ( PB4 : 출력포트 설정    )
	PORTB |= 0x10;    // PB4  : High ( LED OFF)    



	DDRB |= 0x20;   // 왼쪽 모터구동신호 + 단자:  PWM 포트( pin: OC1A(PB5) )   --> 출력 설정 
	DDRA |= 0x01;   // 왼쪽 모터구동신호 - 단자 : 범용 입/출력포트(pin : PA0 ) --> 출력 설정 

	DDRB |= 0x40;   // 오른쪽 모터구동신호 + 단자:  PWM 포트( pin: OC1A(PB6) )   --> 출력 설정 
	DDRA |= 0x02;   // 오른쪽 모터구동신호 - 단자 : 범용 입/출력포트(pin : PA1 ) --> 출력 설정 
	 

///////////////////////

	LcdInit();

	LcdMove(0,0); 
	LcdPuts("DC Motor Control");

	LcdMove(1,0); 
	LcdPuts("UltrasonicSensor");

    msec_delay(2000) ;   // 2초간 디스플레이 

    LcdCommand( ALLCLR ) ;   // LCD 화면 지움 
	LcdMove(0,0); 
	LcdPuts("Speed = ");

	LcdMove(1,0); 
	LcdPuts("Dist_1 =    cm");

////////////////////////////////
	

/////////////    Timer1 설정 :  Fast PWM Mode    ////////////////////////

// 왼쪽, 오른쪽 모터 구동 PWM 신호 ( pin: OC1A(PB5)), OC1B(PB6) ),   Timer1,  PWM signal (period= 200 usec )

	TCCR1A = 0xA2;    // OC1A(PB5)), OC1B(PB6) :  PWM 포트 설정,   Fast PWM ( mode 14 )
	TCCR1B = 0x1B;    // 64 분주 타이머 1 시작 (내부클럭 주기 =  64/(16*10^6) = 4 usec ),  Fast PWM ( mode 14 ) 
	ICR1 = 50;        // PWM 주기 = 50 * 4 usec = 200 usec

    Vmax_L = ICR1; 
    Vmax_R = ICR1; 

    Vmax =  ICR1 ;

//////////////////////////////////////////////////////////////////
    duty_L = 0;
	OCR1A = duty_L;      //  왼쪽 모터 정지: : OC1A(PB5) PWM duty = 0 설정 
	
    duty_R = 0;
	OCR1B = duty_R;      //  오른쪽 모터 정지: : OC1B(PB6) PWM duty = 0 설정 	
	 
//////////////////////////////////////////////////////////////////

    duty_L = 50;         // 왼쪽 모터 속도 설정,   최대 = Vmax = 50,  최소 = 0 

    duty_R = 50;         // 오른쪽 모터 속도 설정, 최대 = Vmax = 50,  최소 = 0 
///////////////////////////////////////////////////////////////////////////

 ////////////  Timer 0 설정  ( 10 msec 주기 타이머 0 인터럽트 )  ///////////////
        
    TCCR0 = 0x00;            // 타이머 0 정지(분주비 = 1024 ) , Normal mode(타이머모드)

    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec

    TIMSK = 0x01;            // 타이머0 오버플로인터럽트 허용
///////////////////////////////////////////////////////////    


////////////   Timer3  설정 ( 3 Echo Signals Pulse Width measurment )  /////////////

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // 타이머 3 시작(분주비 8) ,  0.5usec 단위로 측정 

///////////////////////////////////////////////////////////////////////////////////

 
// 3 초음파센서 Echo Signals : external interrupt 4( pin: INT4 (PE4)),  external interrupt 5( pin: INT5 (PE5)) 
//                           : external interrupt 6( pin: INT4 (PE6)) 

	EICRB |= 0x15;    // Both falling edge and rising edge interrupt
	EICRB &= ~0x2A;   // Both falling edge and rising edge interrupt

	EIMSK |= 0x70;    // INT4 Enable, INT5 Enable, INT6 Enable
	sei(); 

///////////////////////////////////////

   //  최초 초음파센서 1 트리거 신호 발생(초음파 1 발사)  
	PORTB |= 0x01;    // PB0 : High
	usec_delay(20) ;  // 20usec 동안 High 유지 
	PORTB &= ~0x01;   // PB0 : Low 
          
	active_sensor_flag = 1; 
    sensor_count = 1;

  /////////////////////////////////////////////


    TCCR0 |= 0x07;    // 타이머 0 시작(분주비 = 1024 ) 

	 
	while (1) 
	{ 

	    LcdMove(0, 8); 
        Display_Number_LCD( duty_L, 2 );          // 왼쪽 모터 속도 디스플레이 

	    LcdMove(1,9); 
        Display_Number_LCD( distance_1, 3 );      // 초음파센서 1 측정 거리 디스플레이 

		DC_Motor_PWM_L( duty_L );      // 왼쪽 DC Motor 정회전  
		DC_Motor_PWM_R( -duty_R );     // 오른쪽 DC Motor 역회전  
        msec_delay( 5000 );          // 5초간 회전
 
		DC_Motor_PWM_L( 0 );         // 왼쪽 DC Motor 정지  
		DC_Motor_PWM_R( 0 );         // 오른쪽 DC Motor 정지  
        msec_delay( 2000 );          // 2초간 정지 

		DC_Motor_PWM_L( -duty_L );     // 왼쪽 DC Motor 역회전  
		DC_Motor_PWM_R( duty_R );      // 오른쪽 DC Motor 정회전  
        msec_delay( 5000 );          // 5초간 회전
 
		DC_Motor_PWM_L( 0 );         // 왼쪽 DC Motor 정지  
		DC_Motor_PWM_R( 0 );         // 오른쪽 DC Motor 정지  
        msec_delay( 2000 );          // 2초간 정지 


        msec_delay( 100 ); 

     }


} 

///////////////////////////////////////////////////////////////////////



void DC_Motor_Run_Fwd_L( short duty )   // DC 모터 정회전 함수 
{

    if( duty > Vmax_L )     duty = Vmax_L ;

    PORTA &= ~0x01;     //  왼쪽 모터 구동신호 - 단자 : 0 V 인가( PA0 = 0 );  
	OCR1A = duty;       //  왼쪽 모터 구동신호 + 단자 : OC1A(PB5) PWM duty 설정 

}

void DC_Motor_Run_Rev_L( short duty )   // DC 모터 역회전 함수 
{

    if( duty > Vmax_L )     duty = Vmax_L ;

    PORTA |= 0x01;              //  왼쪽 모터 구동신호 - 단자 : 5 V 인가( PA0 = 1 );  
	OCR1A = Vmax_L - duty;      //  왼쪽 모터 구동신호 + 단자 : OC1A(PB5) PWM duty 설정 

}


void DC_Motor_Stop_L( void )   // 왼쪽 DC 모터 정지 함수 
{

    PORTA &= ~0x01;     //  왼쪽 모터 구동신호 - 단자 : 0 V 인가( PA0 = 0 );  
	OCR1A = 0;          //  왼쪽 모터 구동신호 + 단자 : OC1A(PB5) PWM duty = 0 설정 

}



void DC_Motor_Run_Fwd_R( short duty )   // 오른쪽 DC 모터 정회전 함수 
{

    if( duty > Vmax_R )     duty = Vmax_R ;

    PORTA &= ~0x02;     //  오른쪽 모터 구동신호 - 단자 : 0 V 인가( PA1 = 0 );  
	OCR1B = duty;       //  오른쪽 모터 구동신호 + 단자 : OC1B(PB6) PWM duty 설정 

}

void DC_Motor_Run_Rev_R( short duty )   // 오른쪽 DC 모터 역회전 함수 
{

    if( duty > Vmax_R )     duty = Vmax_R ;

    PORTA |= 0x02;                //  오른쪽 모터 구동신호 - 단자 : 5 V 인가( PA1 = 1 );  
	OCR1B = Vmax_R - duty ;       //  오른쪽 모터 구동신호 + 단자 : OC1B(PB6) PWM duty 설정 

}


void DC_Motor_Stop_R( void )   // 오른쪽 DC 모터 정지 함수 
{

    PORTA &= ~0x02;     //  오른쪽 모터 구동신호 - 단자 : 0 V 인가( PA1 = 0 );  
	OCR1B = 0;          //  오른쪽 모터 구동신호 + 단자 : OC1B(PB6) PWM duty = 0 설정 

}


void DC_Motor_PWM_L( short Vref )   // 왼쪽 DC 모터 PWM 신호 발생 함수  
{

   if ( Vref > Vmax_L )       Vref = Vmax_L ;
   else if( Vref < -Vmax_L )  Vref = -Vmax_L ;

   if( Vref > 0 )  
   {
      DC_Motor_Run_Fwd_L( Vref ) ;
   }
   else if( Vref == 0 )  
   {
      DC_Motor_Stop_L() ;
   }
   else if( Vref < 0 )  
   {
      DC_Motor_Run_Rev_L( -Vref ) ;
   }

}


void DC_Motor_PWM_R( short Vref )   // 오른쪽 DC 모터 PWM 신호 발생 함수  
{

   if ( Vref > Vmax_R )       Vref = Vmax_R ;
   else if( Vref < -Vmax_R )  Vref = -Vmax_R ;

   if( Vref > 0 )  
   {
      DC_Motor_Run_Fwd_R( Vref ) ;
   }
   else if( Vref == 0 )  
   {
      DC_Motor_Stop_R() ;
   }
   else if( Vref < 0 )  
   {
      DC_Motor_Run_Rev_R( -Vref ) ;
   }


}

//////////////////////////////////////////////////////////////////////////////


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
	       
	   if( sensor_count == 4 )  sensor_count = 1; 


       if ( sensor_count == 1 )        //  초음파센서 1 트리거 신호 발생(초음파 1 발사) 
	   {
	      PORTB |= 0x01;    // PB0 : High
		  usec_delay(20) ;  // 20usec 동안 High 유지 
	      PORTB &= ~0x01;   // PB0 : Low 
          
		  active_sensor_flag = 1;

	   }
       else if ( sensor_count == 2 )   //  초음파센서 2 트리거 신호 발생(초음파 2 발사)
	   {
	      PORTB |= 0x02;    // PB1 : High
	 	  usec_delay(20) ;  // 20usec 동안 High 유지 
	      PORTB &= ~0x02;   // PB1 : Low 

		  active_sensor_flag = 2;

	   }
       else if ( sensor_count == 3 )   //  초음파센서 3 트리거 신호 발생(초음파 3 발사)
	   {
	      PORTB |= 0x04;    // PB2 : High
		  usec_delay(20) ;  // 20usec 동안 High 유지 
	      PORTB &= ~0x04;   // PB2 : Low 

		  active_sensor_flag = 3;

	   }



       ////////  경고음 발생   /////////////

 
       if( distance_1 <=  40 )    Warning_Flag = 1 ;     // 측정된 거리가 40 cm 이하이면 경고음 발생 플래그 set
       else                       Warning_Flag = 0 ;    
		
       Delay_Time =  distance_1 / 10 + 1;            // 거리에 비레하는 주기(= Delay_Time * 50 msec )를 갖는 경고음 발생
        
	   if( Delay_Time <= 1)   Delay_Time = 1 ;   // 경고음주기 하한 : 0.1초
	   if( Delay_Time >= 4)   Delay_Time = 4 ;   // 경고음주기 상한 : 0.4초 
 

       if( Warning_Flag == 1 )
	   {
           if( Time_Delay_Polling( Delay_Time ) == 1 )     // 50msec * Delay_Time 경과 후 
	       {
               PORTB ^= 0x08  ;    // PB3(버저) toggle : 버저 단속음 
			   PORTB ^= 0x10  ;    // PB4(LED) toggle :  LED ON, OFF 반복 
	       }
	   }
       else if( Warning_Flag == 0 )
	   {
           PORTB &= ~0x08  ;    // PB3(버저) OFF : 버저 OFF 
		   PORTB |= 0x10  ;     // PB4(LED) OFF :  LED  OFF 
	   }
      
       /////////////////////////////////////  





   }


}



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



ISR(INT6_vect)
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




