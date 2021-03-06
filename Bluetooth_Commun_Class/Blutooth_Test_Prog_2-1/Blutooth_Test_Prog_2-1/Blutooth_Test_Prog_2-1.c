
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "lcd.h"


#define  Avg_Num     4         //  이동 평균 갯수 
#define  Amp_Gain   11         //  증폭기 이득  

#define  TMP_Mode   0          //  온도센서 디스플레이 모드   
#define  CDS_Mode   1          //  CDS 센서 디스플레이 모드  



void init_serial(void) ;  //  Serial 토신포트 초기화

void SerialPutChar(char ch);
void SerialPutString(char str[]);

void HexToDec(unsigned short num, unsigned short radix);
char NumToAsc(char Num);

void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 
void Display_TMP_LCD( unsigned int tp  )  ;                           // 온도를 10진수 형태로 LCD 에 디스플레이 

void msec_delay( int n );

static volatile unsigned char cnumber[5] = { 0, 0, 0, 0, 0};


static volatile  char  recv_cnt = 0, rdata=0, new_recv_flag = 0  ;  
               

static volatile unsigned short TMP_sensor_ouput= 0, TMP_sensor_ouput_C = 0,  TMP_sensor_ouput_avg = 0, TMP_sensor_ouput_avg_C = 0 ; 
static volatile unsigned short CDS_sensor_ouput= 1000,  CDS_sensor_ouput_avg = 1000 ; 

static volatile unsigned char int_num = 0,  Sensor_Flag = TMP_Mode ;



/********************************************************************************************************************
                                      					main
********************************************************************************************************************/
int main(void)
{ 	  
 

	 DDRB |= 0x10 ; 	// LED (PB4 ) :출력설정	

	 PORTB |= 0x10;     // LED OFF


   // Push Switch : 외부인터럽트 0 (INT0 : PD0 )에 연결   
	DDRD &= ~0x01;     // PD0 (외부인터럽트 INT0 ) : 입력설정   
    PORTD |= 0x01;     // PD0 : 내부풀업사용   


   ////////  외부 인터럽트(INT0 ) 설정  ///////////


    EICRA &= ~0x01;  // INT0 하강모서리에서 인터럽트 걸림
    EICRA |=  0x02;  // INT0 하강모서리에서 인터럽트 걸림

    EIMSK |=  0x01;  // INT0 인터럽트  허용

  ///////////////////////////////////////////////


    LcdInit();         // LCD 초기화 

	LcdCommand(ALLCLR);
	LcdMove(0,0);  
	LcdPuts("TMP avg =     C");
//	LcdPuts("TMP value =    ");
	LcdMove(1,0); 
	LcdPuts("CDS avg =     ");


  /////////////////////////////////////////////////

     init_serial() ;    // Serial Port (USART1) 초기화
     UCSR1B |=  0x80  ;      // UART1 송신(RX) 완료 인터럽트 허용


/*****   AD Converter **********/

	ADMUX &= ~0xE0;    //  ADC 기준전압 = AREF ,   ADC 결과 오른쪽정렬 
	ADCSRA |= 0x87;     // ADC enable, Prescaler = 128

/**** Timer0 Overflow Interrupt  ******/
/**************************************/
	TCCR0 = 0x00; 
    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64use

	TIMSK = 0x01;  // Timer0 overflow interrupt enable 
	sei();         // Global Interrupt Enable 


	TCCR0 |= 0x07; // Clock Prescaler N=1024 (Timer 0 Start)



	  while(1)
	  {

         if( new_recv_flag == 1 )      // 1 문자 수신완료 시 
		 { 


		  //////////////  명령어 처리   //////////////

			if( rdata == '0' )          // 문자 0 이 수신되면 
			{
                PORTB |= 0x10;          // LED OFF 
			}
			else if( rdata == '1' )     // 문자 1 이 수신되면
			{
                PORTB &= ~0x10;         // LED ON
			}
			else if( rdata == '2')      // 문자 2 가 수신되면 
			{

		        HexToDec(TMP_sensor_ouput_C,10);   // 현재 온도 값 TMP_sensor_ouput_C 십진수로 변환

                SerialPutString( "Temperature = " );   //  메시지 전송
                SerialPutChar( NumToAsc(cnumber[2]));  //  현재 온도값, 변수 TMP_sensor_ouput_C 값 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( '.'); 
                SerialPutChar( NumToAsc(cnumber[0]));  // 소숫점 이하 자리 전송 
                SerialPutChar( 'C'); 
                SerialPutChar('\n');                   // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함

			} 

			else if( rdata == '3')      // 문자 3 이 수신되면 
			{

		        HexToDec( CDS_sensor_ouput, 10);   // 현재 광량(CDS)센서 값 CDS_sensor_ouput 십진수로 변환

                SerialPutString( "CDS Value = " );     //  메시지 전송
                SerialPutChar( NumToAsc(cnumber[3]));  //  현재 광량(CDS)센서 값, 변수 CDS_sensor_ouput 값 전송
                SerialPutChar( NumToAsc(cnumber[2])); 
                SerialPutChar( NumToAsc(cnumber[1]));   
                SerialPutChar( NumToAsc(cnumber[0]));   
                SerialPutChar('\n');                   // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함

			} 


			else if( rdata != 0xFF)    //  명령 오류 이면
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  명령 오류 메시지 전송


			}


		    rdata = 0xFF;
            new_recv_flag = 0;      // 새 문자(명령) 수신 플래그 Reset
  

        }

     ///////////////////////////////////////////////////

       if( Sensor_Flag == TMP_Mode )
 	   { 
	       LcdMove(0,10); 
           Display_TMP_LCD( TMP_sensor_ouput_C  );  
	   }
       else if( Sensor_Flag == CDS_Mode )
 	   { 
 	       LcdMove(1,10); 
           Display_Number_LCD( CDS_sensor_ouput, 4 ); 
       }




    //////////////////////////////////////////////////

 
	}   // end of while(1)

}     //  End of main()



// UART1 수신 인터럽트 서비스 프로그램

ISR(  USART1_RX_vect )
{

    rdata = UDR1; 
 
    SerialPutChar( rdata);           // Echo  수신된 데이터를 바로 송신하여 수신된 데이터가 정확한지 확인 
    SerialPutChar('\n');             // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함

    recv_cnt++ ;                     // 수신된 데이터 바이트수 저장

    new_recv_flag = 1;               // 새 문자(명령) 수신 플래그 Set

}



///////////////////////////////////////////////////////////////


ISR(TIMER0_OVF_vect)   // Timer0 overflow interrupt( 10 msec)  service routine
{

    static unsigned short  time_index = 0,  count1 = 0, TMP_Sum = 0, CDS_Sum = 0 ; 
    static unsigned short  TMP_sensor_ouput_buf[Avg_Num ], CDS_sensor_ouput_buf[Avg_Num ]   ; 


    unsigned char i = 0 ;

    sei();            // 전역인터럽트 허용( 다른인터럽트(타이머인터럽트) 허용하고 싶을때) 

    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec

    time_index++ ; 


    if( time_index == 25 )    // 샘플링주기 =  250 msec = 10msec x 25 
    {

       time_index = 0; 


      /**************   CDS Sensor signal detection(AD 변환) ************/

	   ADMUX &= ~0x1F;    //  ADC Chanel 0 : ADC0 선택

	   ADCSRA |= 0x40;   // ADC start 

	   while( ( ADCSRA & 0x10 ) == 0x00  ) ;  // Check if ADC Conversion is completed 

	   CDS_sensor_ouput = ADC;   
 
     /******************************************************/ 


      /**************   Temperature Sensor signal detection(AD 변환) ************/

	   ADMUX &= ~0x1F;    //  ADC Chanel 선택 Clear
	   ADMUX |= 0x01;     //  ADC Chanel 1 : ADC 1 선택

	   ADCSRA |= 0x40;    // ADC start 

	   while( ( ADCSRA & 0x10 ) == 0x00  ) ;  // Check if ADC Conversion is completed 

	   TMP_sensor_ouput = ADC;                // ADC Conversion 이 완료되었으면 ADC 결과 저장 
 
     /******************************************************/ 


   ////////////////////////////////////////////////////////////////////
   //////////                                               /////////// 
   //////////  Avg_Num(4개) 개씩 이동 평균(Moving Average)  ///////////
   //////////                                               ///////////
   ////////////////////////////////////////////////////////////////////

	   if( count1 <= ( Avg_Num -1 ) )
	   {

             CDS_sensor_ouput_buf[ count1 ] = CDS_sensor_ouput ;
			 CDS_Sum +=  CDS_sensor_ouput_buf[ count1 ] ; 

          ////////////////////////////////////////////////////

             TMP_sensor_ouput_buf[ count1 ] = TMP_sensor_ouput ;
			 TMP_Sum +=  TMP_sensor_ouput_buf[ count1 ] ; 

	         count1++ ; 

	   } 
	   else
	   {

             CDS_Sum +=  CDS_sensor_ouput  ;	       // 가장 최근 값 더하고  
             CDS_Sum -=  CDS_sensor_ouput_buf[ 0 ] ;   // 가장 오랜된 값 빼고 

             CDS_sensor_ouput_avg = CDS_Sum / Avg_Num ;     // 4개 이동 평균 

             for( i = 0; i <= (Avg_Num - 2) ; i++ )
			 {
                 CDS_sensor_ouput_buf[ i ]  = CDS_sensor_ouput_buf[ i+1 ] ;
			 } 

             CDS_sensor_ouput_buf[ Avg_Num - 1 ]  = CDS_sensor_ouput ;  

            ////////////////////////////////////////////////////////////////


             TMP_Sum +=  TMP_sensor_ouput  ;	       // 가장 최근 값 더하고  
             TMP_Sum -=  TMP_sensor_ouput_buf[ 0 ] ;   // 가장 오랜된 값 빼고 

             TMP_sensor_ouput_avg = TMP_Sum / Avg_Num ;     // 4개 이동 평균 

             //  섭씨온도 계산 : 증폭기(증폭기 이득 = Amp_Gain ) 사용했을때 
               TMP_sensor_ouput_avg_C =   ( unsigned short) ( (unsigned long) 1250 * TMP_sensor_ouput_avg  / (256 * Amp_Gain)  )  ;    // 온도 계산 [C] 단위
               TMP_sensor_ouput_C =   ( unsigned short) ( (unsigned long) 1250 * TMP_sensor_ouput  / (256 * Amp_Gain)  )  ;    // 온도 계산 [C] 단위

             // 섭씨온도 계산 : 증폭기 사용하지 않았을때  
             //  TMP_sensor_ouput_avg_C =   ( unsigned short) ( (unsigned long) 1250 * TMP_sensor_ouput_avg  / 256  )  ;           // 온도 계산 [C] 단위
 

             for( i = 0; i <= (Avg_Num - 2) ; i++ )
			 {
                 TMP_sensor_ouput_buf[ i ]  = TMP_sensor_ouput_buf[ i+1 ] ;
			 } 

             TMP_sensor_ouput_buf[ Avg_Num - 1 ]  = TMP_sensor_ouput ;  

	   }

       //////////////////////////////////////////////////////////////////

/**
	   if( CDS_sensor_ouput_avg <= 500 )
	   {	
	       PORTB &= ~0x10;   // PB4  : Low ( LED ON )  
       }

	   else if( CDS_sensor_ouput_avg > 500 )
	   {	
	       PORTB |= 0x10;   // PB4  : High ( LED OFF )  
       }
**/


   }


}



/////////////////////////////////////////////////

ISR(INT0_vect)    //  INT0 서비스 프로그램
{


      sei();            // 전역인터럽트 허용( 다른인터럽트(타이머인터럽트) 허용하고 싶을때) 
      EIMSK = 0x00;     // INT0 인터럽트 금지


     int_num++;           // 스위치가 한번 눌러질 때마다 눌러진 횟수 1 증가 

     if( int_num == 2 ) int_num = 0 ;

	 if( int_num == 0 )       Sensor_Flag = TMP_Mode ;
	 else if( int_num == 1 )  Sensor_Flag = CDS_Mode ;
     

//////////////////// 채터링 방지 ///////////////////

	  msec_delay( 20 );
	  while( ~PIND & 0x01 );
	  msec_delay( 20 );

	  EIFR = 0x01;   // 플래그비트 리셋	

///////////////////////////////////////////////////

      EIMSK = 0x01;     // INT0 인터럽트 허용

 

}


///////////////////////////////////////////////////////////



// UART1 통신 초기화 프로그램 

void init_serial(void)
{
    UCSR1A = 0x00;                    //초기화
    UCSR1B = 0x18  ;                  //송수신허용,  송수신 인터럽트 금지
    UCSR1C = 0x06;                    //데이터 전송비트 수 8비트로 설정.
    
    UBRR1H = 0x00;
    UBRR1L = 103;                     //Baud Rate 9600 
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

    while(*str != '\0')         // 수신된 문자가 Null 문자( 0x00 )가 아니면 
    {

        SerialPutChar(*str++);
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


void Display_TMP_LCD( unsigned int tp  )       // 온도를 10진수 형태로 LCD 에 디스플레이 
{

	HexToDec( tp, 10); //10진수로 변환 

 
    LcdPutchar(NumToAsc(cnumber[2]) );   // 10자리 디스플레이
	
    LcdPutchar(NumToAsc(cnumber[1]));    // 1자리 디스플레이 

    LcdPuts( ".");                       // 소숫점(.) 디스플레이 

    LcdPutchar(NumToAsc(cnumber[0]));    // 0.1 자리 디스플레이 

 

}



void HexToDec(unsigned short num, unsigned short radix)
{
	int j = 0;

	for(j=0; j<5; j++) cnumber[j] = 0;

	j=0;

	do
	{
		cnumber[j++] = num % radix;
		num /= radix;

	}while(num);
}



char NumToAsc(char Num)
{
	
	if( Num > 9 ) Num += 0x37;
	else	      Num += 0x30;

	return Num;
}



void msec_delay( int n )
{
	for(; n> 0 ; n-- )   _delay_ms(1); 
}



