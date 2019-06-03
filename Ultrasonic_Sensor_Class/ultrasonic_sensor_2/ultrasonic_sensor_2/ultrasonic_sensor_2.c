#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"

void init_serial(void) ;  //  Serial 토신포트 초기화

void SerialPutChar(char ch);
void SerialPutString(char str[]);

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

static volatile  char  recv_cnt = 0, rdata=0, new_recv_flag = 0  ;  
static volatile unsigned char   Command_Error_Flag = 0 ; 

int sensor_flag = 0;


int main() 
{   
    
	init_serial() ;    // Serial Port (USART1) 초기화

	LcdInit();      //  LCd 초기화 함수 


////  3 개의 초음파센서( Ultrasonic Sensor) ////////////

// 출력포트 설정 
	
	DDRB |= 0x07;     // 3 초음파센서 Trigger signals( PB0, PB1, PB2 : 출력포트 설정  )
	PORTB &= ~0x07;   // PB0, PB1, PB2  : Low  ( 3 Trigger signals OFF )  

////////////  Timer 0 설정  ( 10 msec 주기 타이머 0 인터럽트 )  ///////////////
        
    TCCR0 = 0x00;            // 타이머 0 정지(분주비 = 1024 ) , Normal mode(타이머모드)

    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec

    TIMSK &= ~0x01;            // 타이머0 오버플로인터럽트 금지 

///////////////////////////////////////////////////////////    


// 3 Echo Signal Pulse Width measurment,  Timer3 

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // 타이머 3 시작(분주비 8) ,  0.5usec 단위로 측정 


// 3 초음파센서 Echo Signals : external interrupt 4( pin: INT4 (PE4)),  external interrupt 5( pin: INT5 (PE5)) 
//                           : external interrupt 6( pin: INT4 (PE6)) 
	
	DDRE &= ~0x70;	  // PE4(INT4), PE5(INT5), PE6(INT6) set input

	EICRB |= 0x15;    // Both falling edge and rising edge interrupt
	EICRB &= ~0x2A;   // Both falling edge and rising edge interrupt

	EIMSK |= 0x70;    // INT4 Enable, INT5 Enable, INT6 Enable

	UCSR0B |= 0x80  ;      // UART0 송신(RX) 완료 인터럽트 허용

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
	    if( new_recv_flag == 1 )      // 1 문자 수신완료 시 
		 { 


		    if( Command_Error_Flag == 1 )    // 이전 명령에 오류가 있었으면
			{  
			      Command_Error_Flag = 0 ;     // 이전 Command_Error_Flag 리셋 
            }

		  //////////////  명령어 처리   //////////////

			if( rdata == '0' )          // 문자 0 이 수신되면 
			{
				sensor_flag = 0;

                TIMSK |= 0x01;            // 타이머0 오버플로인터럽트 허용
				
				HexToDec(distance_1,10);   // 수신된 바이트수 distance_1 십진수로 변환
				
                SerialPutString( "Measured Distance 1 = " );     //  메시지 전송 

                SerialPutChar( NumToAsc(cnumber[2]));            //  변수 recv_cnt 값 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 
				SerialPutChar('\n');                    // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함
				
				HexToDec(distance_2,10);   // 수신된 바이트수 distance_2 십진수로 변환
				
                SerialPutString( "Measured Distance 2 = " );     //  메시지 전송 

                SerialPutChar( NumToAsc(cnumber[2]));           
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 
				SerialPutChar('\n');                    // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함
				
				HexToDec(distance_3,10);   // 수신된 바이트수 distance_3 십진수로 변환
				
                SerialPutString( "Measured Distance 3 = " );     //  메시지 전송 

                SerialPutChar( NumToAsc(cnumber[2]));            //  변수 recv_cnt 값 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 

				SerialPutChar('\n');                    // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함
			}
			else if( rdata == '1' )     // 문자 1 이 수신되면
			{
				sensor_flag = 1;

				TIMSK |= 0x01;            // 타이머0 오버플로인터럽트 허용

                HexToDec(distance_1,10);   // 수신된 바이트수 distance_1 십진수로 변환
				
                SerialPutString( "Measured Distance 1 = " );     //  메시지 전송 

                SerialPutChar( NumToAsc(cnumber[2]));            //  변수 recv_cnt 값 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 
				SerialPutChar('\n');                    // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함
			}
			else if( rdata == '2')      // 문자 2 가 수신되면
			{
				sensor_flag = 2;

				TIMSK |= 0x01;            // 타이머0 오버플로인터럽트 허용

				HexToDec(distance_2,10);   // 수신된 바이				트수 distance_1 십진수로 변환
				
                SerialPutString( "Measured Distance 2 = " );     //  메시지 전송 

                SerialPutChar( NumToAsc(cnumber[2]));            //  변수 recv_cnt 값 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 
				SerialPutChar('\n');                    // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함
			}
			else if( rdata == '3')
			{
				sensor_flag = 3;

				TIMSK |= 0x01;            // 타이머0 오버플로인터럽트 허용

				HexToDec(distance_3,10);   // 수신된 바이트수 distance_1 십진수로 변환
				
                SerialPutString( "Measured Distance 3 = " );     //  메시지 전송 

                SerialPutChar( NumToAsc(cnumber[2]));            //  변수 recv_cnt 값 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 

				SerialPutChar('\n');                    // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함
			}
			else if( rdata == '4')
			{
				sensor_flag = 4;

				TIMSK &= ~0x01;            // 타이머0 오버플로인터럽트 금지 
				
				SerialPutString( "Ultrasonic Sensor OFF" );     //  메시지 전송 

				SerialPutChar('\n');                    // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함
			}

			else if( rdata != 0xFF)    //  명령 오류 이면
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  명령 오류 메시지 전송

			    LcdCommand( 0x01) ;       // LCD Claear

			    LcdMove(0, 0 );           // LCD에 오류메시지 디스플레이
		        LcdPuts("Cmd Error!!"); 
			    LcdMove(1, 0 );
		        LcdPuts("Try Again."); 


				Command_Error_Flag = 1;                             // 명령 오류 플래그 셋
			}

     ///////////////////////////////////////////////////////////////

		   if( Command_Error_Flag == 0  )          // 명령에 오류가 없으면  
		   {  

	           LcdMove(0, 11); 
//               Display_Number_LCD( recv_cnt, 3 ) ;  // 수신된 바이트수 recv_cnt를 십진수로 변환하여 LCD에 디스플레이

		       LcdMove(1,12);    
		       LcdPutchar( rdata );                 // 수신된 문자 rdata를 LCD에 디스플레이

           }

      ////////////////////////////////////////////////////////////////


		   rdata = 0xFF;                           // 수신된 명령을 초기값으로 리셋
           new_recv_flag = 0;                      // 새 문자(명령) 수신 플래그 Reset
  

        }

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

	   switch(sensor_flag) 
	   {
	   		case 0: 
	   		{
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
				else if ( sensor_count == 3 )   //  초음파센서 2 트리거 신호 발생(초음파 2 발사)
	   			{
	      			PORTB |= 0x04;    // PB2 : High
	 	  			usec_delay(20) ;  // 20usec 동안 High 유지 
	      			PORTB &= 0xFB;    // PB2 : Low 
	
			  		active_sensor_flag = 3;
	   			}
				break;
	   		}
	   		case 1:
	   		{
				PORTB |= 0x01;    // PB0 : High
				usec_delay(20) ;  // 20usec 동안 High 유지 
				PORTB &= 0xFE;    // PB0 : Low 	  
				
				active_sensor_flag = 1; 
				break;
	   		}
	   		case 2:
	   		{
				PORTB |= 0x02;    // PB1 : High
	 	  		usec_delay(20) ;  // 20usec 동안 High 유지 
	      		PORTB &= 0xFD;    // PB1 : Low 

				active_sensor_flag = 2;
				break;
		
	   		}
	   		case 3:
	   		{
				PORTB |= 0x04;    // PB2 : High
	 	  		usec_delay(20) ;  // 20usec 동안 High 유지 
	      		PORTB &= 0xFB;    // PB2 : Low 

				active_sensor_flag = 3;
				break;
	   		}
	   }
		

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



// UART1 수신 인터럽트 서비스 프로그램

ISR(  USART0_RX_vect )
{

    rdata = UDR0; 
 
    SerialPutChar( rdata);           // Echo  수신된 데이터를 바로 송신하여 수신된 데이터가 정확한지 확인 
    SerialPutChar('\n');             // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함

    recv_cnt++ ;                     // 수신된 데이터 바이트수 저장

    new_recv_flag = 1;               // 새 문자(명령) 수신 플래그 Set

}



// UART1 통신 초기화 프로그램 

void init_serial(void)
{
    UCSR0A = 0x00;                    //초기화
    UCSR0B = 0x18  ;                  //송수신허용,  송수신 인터럽트 금지
    UCSR0C = 0x06;                    //데이터 전송비트 수 8비트로 설정.
    
    UBRR0H = 0x00;
    UBRR0L = 103;                     //Baud Rate 9600 
}




//======================================
// 한 문자를 송신한다.
//======================================

void SerialPutChar(char ch)
{
	while(!(UCSR0A & (1<<UDRE)));			// 버퍼가 빌 때를 기다림
  	UDR0 = ch;								// 버퍼에 문자를 쓴다
}


//=============================================
// 문자열을 송신한다.
// 입력   : str - 송신한 문자열을 저장할 버퍼의 주소
//=============================================

 void SerialPutString(char *str)
 {

    while(*str != '\0')         // 수신된 문자가 Null 문자( 0x00 )가 아니면 
    {

        SerialPutChar(*str++);
    }
}



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

