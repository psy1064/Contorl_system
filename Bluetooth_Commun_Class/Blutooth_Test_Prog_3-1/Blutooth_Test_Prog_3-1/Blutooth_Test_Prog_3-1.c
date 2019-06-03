#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"

/////////////////
void init_serial(void) ;  //  Serial 토신포트 초기화

void SerialPutChar(char ch);
void SerialPutString(char str[]);

//////////////////////


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이


void msec_delay(int n)  ;   // msec 단위 시간지연
void usec_delay(int n)  ;   // usec 단위 시간지연

unsigned char Time_Delay_Polling( unsigned short d_time ) ;   // 시간지연 체크함수(폴링방식)


static volatile unsigned short    distance_1 = 0, distance_2 = 0,  distance_3 = 0, sensor_count = 0, active_sensor_flag = 0  ;

static volatile unsigned short    distance_1_old = 0, distance_2_old = 0,  distance_3_old = 0 ;

static volatile  unsigned char    Warning_Flag = 0 ;
static volatile  unsigned short   Delay_Time = 0;


////////////////////////////////

static volatile char Cmd_Message_1[] = { "led on" } ;     //  Blutooth Command
static volatile char Cmd_Message_2[] = { "led off" } ;  
static volatile char Cmd_Message_3[] = { "led toggle" } ;  
static volatile char Cmd_Message_4[] = { "send distance 1 data" } ;  
static volatile char Cmd_Message_5[] = { "send distance 2 data" } ;  

static volatile  char  rdata = 0,  recv_cnt = 0, new_recv_flag = 0  ;                
static volatile  char  recv_data[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
//////////////////////////////

static volatile unsigned char   Command_Error_Flag = 0 ; 


int main() 
{   


    char   eq_count1 = 0, eq_count2 = 0, eq_count3 = 0, eq_count4 = 0, eq_count5 = 0, cmd_data = 0xFF  ;  
    unsigned  char   i = 0 ;   		

	LcdInit();      //  LCd 초기화 함수 

	LcdMove(0,0); 
	LcdPuts("Dist_1 =    cm");
	LcdMove(1,0); 
	LcdPuts("Dist_2 =    cm");
//	LcdMove(1,0); 
//	LcdPuts("Dist_3 =    cm"); 


///////////////////////////////////////////////

     init_serial() ;    // Serial Port (USART1) 초기화

     UCSR1B |=  0x80  ;      // 송신(RX) 완료 인터럽트 허용
	 sei() ;                 // 전역인터럽트허용
/////////////////////////////////////////////////



////  3 개의 초음파센서( Ultrasonic Sensor) ////////////

// 출력포트 설정 
	
	DDRB |= 0x07;     // 3 초음파센서 Trigger signals( PB0, PB1, PB2 : 출력포트 설정  )
	PORTB &= ~0x07;   // PB0, PB1, PB2  : Low  ( 3 Trigger signals OFF )  

	DDRB |= 0x08;     // 버저(Buzzer) ( PB3 : 출력포트 설정    )
	PORTB &= ~0x08;   // PB3  : Low  ( 버저 OFF )  

	DDRB |= 0x10;     // LED ( PB4 : 출력포트 설정    )
	PORTB |= 0x10;    // PB4  : High ( LED OFF)    


 ////////////  Timer 0 설정  ( 10 msec 주기 타이머 0 인터럽트 )  ///////////////
        
    TCCR0 = 0x00;            // 타이머 0 정지(분주비 = 1024 ) , Normal mode(타이머모드)

    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec

    TIMSK = 0x01;            // 타이머0 오버플로인터럽트 허용
///////////////////////////////////////////////////////////    


// 3 Echo Signalㄴ Pulse Width measurment,  Timer3 

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // 타이머 3 시작(분주비 8) ,  0.5usec 단위로 측정 

/////////////////////////////////////////////////////////

 
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
	PORTB &= 0xFE;    // PB0 : Low 
          
	active_sensor_flag = 1; 
    sensor_count = 1;

  /////////////////////////////////////////////


    TCCR0 |= 0x07;    // 타이머 0 시작(분주비 = 1024 ) 

	 
	while (1) 
	{ 

//////////////////////////////////


            if( new_recv_flag == 1 )      // 문자열 수신완료 시 
			{ 
                 

		        if( Command_Error_Flag == 1 )    // 이전 명령에 오류가 있었으면
			    {  
			        Command_Error_Flag = 0 ;     // 이전 Command_Error_Flag 리셋 
                   
					LcdCommand( ALLCLR ) ;       // LCD 화면 지움 

	                LcdMove(0,0); 
	                LcdPuts("Dist_1 =    cm");
	                LcdMove(1,0); 
	                LcdPuts("Dist_2 =    cm");
//	                LcdMove(1,0); 
//	                LcdPuts("Dist_3 =    cm"); 

               }


               for( i=0; i < recv_cnt ; i++) 
			   {
			      if( recv_data[i] == Cmd_Message_1[i] ) eq_count1++ ;
			      if( recv_data[i] == Cmd_Message_2[i] ) eq_count2++ ; 
			      if( recv_data[i] == Cmd_Message_3[i] ) eq_count3++ ;
			      if( recv_data[i] == Cmd_Message_4[i] ) eq_count4++ ;  
			      if( recv_data[i] == Cmd_Message_5[i] ) eq_count5++ ;  
               }

               if     ( eq_count1 == 6 )  cmd_data = 1 ;     // 명령 1
               else if( eq_count2 == 7 )  cmd_data = 2 ;     // 명령 2   
               else if( eq_count3 == 10)  cmd_data = 3 ;     // 명령 3
               else if( eq_count4 == 20 ) cmd_data = 4 ;     // 명령 4 
               else if( eq_count5 == 20 ) cmd_data = 5 ;     // 명령 5
			   else                       cmd_data = 0xFE ;  // 명령 오류

               eq_count1 = 0;  eq_count2 = 0;  eq_count3 = 0;  eq_count4 = 0,  eq_count5 = 0; 

               new_recv_flag = 0 ; 
 
			}

          //////////////////////////////////////


         ////////  명령(Command) 처리 

			if( cmd_data ==  1 )          // 명령 1 이면
			{
                PORTB &= ~0x10;           // LED ON
			}
			else if( cmd_data == 2 )      // 명령 2 이면
			{
                PORTB |= 0x10;            // LED OFF
			}
			else if( cmd_data == 3 )      // 명령 3 이면
			{
                PORTB ^= 0x10;            // LED Toggle 

			}

			else if( cmd_data == 4 )     // 명령 4 이면
			{

		        HexToDec( distance_1,10);                        // 초음파센서 1에 의해 측정된 거리 distance_1 십진수로 변환

                SerialPutString( "measured distance 1 = " );     // 휴대폰으로 메시지 전송

                SerialPutChar( NumToAsc(cnumber[2]));    // 변수 distance_1 값을 10진수 변환후 각 자리수를 문자데이터(ASCII)로 변환후 휴대폰으로 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 

                SerialPutString( "cm" );                 // 휴대폰으로 메시지(거리 단위 cm) 전송

                SerialPutChar('\n');                     // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함 


			}

			else if( cmd_data == 5 )     // 명령 5 이면
			{

		        HexToDec( distance_2,10);                        // 초음파센서 2에 의해 측정된 거리 distance_2 십진수로 변환

                SerialPutString( "measured distance 2 = " );     // 휴대폰으로 메시지 전송

                SerialPutChar( NumToAsc(cnumber[2]));    // 변수 distance_2 값을 10진수 변환후 각 자리수를 문자데이터(ASCII)로 변환후 휴대폰으로 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 

                SerialPutString( "cm" );                 // 휴대폰으로 메시지(거리 단위 cm) 전송

                SerialPutChar('\n');                     // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함 


			}

            else if( cmd_data == 0xFE )      //  명령 오류 이면 
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  명령 오류 메시지 전송

			    LcdCommand( 0x01) ;       // LCD Claear

			    LcdMove(0, 0 );           // LCD에 오류메시지 디스플레이
		        LcdPuts("Cmd Error!!"); 
			    LcdMove(1, 0 );
		        LcdPuts("Try Again."); 

				Command_Error_Flag = 1;                             // 명령 오류 플래그 셋 

			}

      ////////////////////////////////////////////////////////////////

		    cmd_data = 0xFF;                             //  명령을 초기값으로 리셋

     ///////////////////////////////////////////////////////////////


		   if( Command_Error_Flag == 0  )                // 명령에 오류가 없으면 거리측정 데이터를 LCD에 디스플레이 
		   {  

	           LcdMove(0, 9); 
               Display_Number_LCD(distance_1, 3);        // 거리측정 데이터 LCD에 디스플레이

 	           LcdMove(1, 9); 
               Display_Number_LCD(distance_2, 3); 

//             LcdMove(1, 9); 
//             Display_Number_LCD(distance_3); 

           }







      //////////////////////////////////////////////


     }


} 


//////////////////////////////////////////////////////////////////


// UART1 수신 인터럽트 서비스 프로그렘 

ISR(  USART1_RX_vect )
{

    static unsigned char r_cnt = 0 ; 


    rdata = UDR1; 

    if( rdata != '.' )                      // 수신된 데이터가 마지막 문자를 나타내는 데이터(마침표)가 아니면
    {
        SerialPutChar( rdata);               // Echo  수신된 데이터를 바로 송신하여 수신된 데이터가 정확한지 확인 
   	    recv_data[r_cnt] = rdata;        //  수신된 문자 저장 
	    r_cnt++;                         //  수신 문자 갯수 증가 

		new_recv_flag = 0;

    }
    else if(  rdata == '.' )                // 수신된데이터가 마지막 문자를 나타내는 데이터(마침표) 이면
    {
        SerialPutChar('\n');                // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함 
        recv_cnt = r_cnt ;                  // 수신된 데이터 바이트수 저장
        r_cnt = 0;  
        
		new_recv_flag = 1;

    }


}




void init_serial(void)
{
    UCSR1A=0x00;                    //초기화
    UCSR1B = 0x18  ;                //송수신허용,  송수신 인터럽트 금지
    UCSR1C=0x06;                    //데이터 전송비트 수 8비트로 설정.
    
    UBRR1H=0x00;
    UBRR1L=103;                     //Baud Rate 9600 
}




//======================================
// 한 문자를 송신한다.
//======================================

void SerialPutChar(char ch)
{
	while(!(UCSR1A & (1<<UDRE)));			// 버퍼가 빌 때를 기다림
  	UDR1 = ch;								// 버퍼에 문자를 쓴다
}



//=============================================
// 문자열을 송신한다.
// 입력   : str - 송신한 문자열을 저장할 버퍼의 주소
//=============================================

 void SerialPutString(char *str)
 {

    while(*str != '\0')
    {

        SerialPutChar(*str++);
    }
}



///////////////////////////////////////////////////////////////



ISR( TIMER0_OVF_vect )    //  10 msec 주기 타이머1 오버플로 인터럽트 서비스프로그램
{

    static unsigned short  time_index = 0 ; 


    sei();            // 전역인터럽트 허용( 다른인터럽트( USART1 인터럽트) 허용하고 싶을때) 


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
       else if ( sensor_count == 3 )   //  초음파센서 3 트리거 신호 발생(초음파 3 발사)
	   {
	      PORTB |= 0x04;    // PB2 : High
		  usec_delay(20) ;  // 20usec 동안 High 유지 
	      PORTB &= 0xFB;    // PB2 : Low 

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
//			   PORTB ^= 0x10  ;    // PB4(LED) toggle :  LED ON, OFF 반복 
	       }
	   }
       else if( Warning_Flag == 0 )
	   {
           PORTB &= ~0x08  ;    // PB3(버저) OFF : 버저 OFF 
//		   PORTB |= 0x10  ;     // PB4(LED) OFF :  LED  OFF 
	   }
      
       /////////////////////////////////////  


   }


}



ISR(INT4_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;


    sei();            // 전역인터럽트 허용( 다른인터럽트( USART1 인터럽트) 허용하고 싶을때) 

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


    sei();            // 전역인터럽트 허용( 다른인터럽트( USART1 인터럽트) 허용하고 싶을때) 

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


    sei();            // 전역인터럽트 허용( 다른인터럽트( USART1 인터럽트) 허용하고 싶을때) 

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




