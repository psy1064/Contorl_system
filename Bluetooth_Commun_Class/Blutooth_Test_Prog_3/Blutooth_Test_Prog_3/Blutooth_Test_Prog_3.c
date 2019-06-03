
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "lcd.h"

void init_serial(void) ;  //  Serial 토신포트 초기화

void SerialPutChar(char ch);
void SerialPutString(char str[]);


void HexToDec(unsigned short num, unsigned short radix);
char NumToAsc(char Num);

void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이

void msec_delay( int n );

static volatile unsigned char cnumber[5] = { 0, 0, 0, 0, 0};


static volatile char Send_Message_1[] = { "Received Data Count = " } ;  

static volatile char Cmd_Message_1[] = { "led on" } ;     //  Blutooth Command
static volatile char Cmd_Message_2[] = { "led off" } ;  
static volatile char Cmd_Message_3[] = { "led toggle" } ;  
static volatile char Cmd_Message_4[] = { "send data" } ; 
static volatile char Cmd_Message_5[] = { "buzzer on" } ; 
static volatile char Cmd_Message_6[] = { "buzzer off" };

static volatile  char  rdata = 0,  recv_cnt = 0, new_recv_flag = 0  ;                
static volatile  char  recv_data[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  


static volatile unsigned char   Command_Error_Flag = 0 ; 


/********************************************************************************************************************
                                      					main
********************************************************************************************************************/
int main(void)
{ 
	char    eq_count1=0, eq_count2=0, cmd_data = 0xFF  ;  	  
    unsigned char   i=0 ;
	unsigned short duty = 0;

    init_serial() ;    // Serial Port (USART1) 초기화

    LcdInit();         // LCD 초기화 

    UCSR0B |=  0x80  ;      // 송신(RX) 완료 인터럽트 허용
	sei() ;                 // 전역인터럽트허용

	// 모터구동신호 ( pin: OC1A(PB5) ),   Timer1, PWM signal (period= 200 usec )

	TCCR1A = 0x82;    // OC1A(PB5)) :  PWM 포트 설정,   Fast PWM ( mode 14 )
	TCCR1B = 0x1b;    // 64 분주 타이머 1 시작 (내부클럭 주기 =  64/(16*10^6) = 4 usec ),  Fast PWM ( mode 14 ) 
	ICR1 = 50;        // PWM 주기 = 50 * 4 usec = 200 usec (  PWM 주파수 = 1/200usec = 5 kHz )

    Vmax = ICR1; 

	OCR1A = duty;      //  OC1A(PB5) PWM duty = 0 설정 : 모터 정지

	////////////////////////////////////////////////////////////////////////////////////////////////////////

    LcdCommand( ALLCLR ) ;    // LCD Clear
    LcdMove(0,0);    
	LcdPuts("Bluetooth Prog"); 
 
  	LcdMove(1,0);    
	LcdPuts("Send Command."); 

  
	 while(1)
	 {

            if( new_recv_flag == 1 )      // 문자열 수신완료 시 
			{ 

		        if( Command_Error_Flag == 1 )    // 이전 명령에 오류가 있었으면
			    {  
			        Command_Error_Flag = 0 ;     // 이전 Command_Error_Flag 리셋 
                   
					LcdCommand( ALLCLR ) ;       // LCD 화면 지움 

	                LcdMove(0,0); 
	                LcdPuts("Bluetooth Prog"); 

		            LcdMove(1,0);    
		            LcdPuts("Recv cnt = "); 

               }
                 
               for( i=0; i < recv_cnt ; i++) 
			   {
			      if( recv_data[i] == Cmd_Message_1[i] ) eq_count1++ ;
			      if( recv_data[i] == Cmd_Message_2[i] ) eq_count2++ ;

               }

               if     ( eq_count1 == 15 )  cmd_data = 1 ;     // 명령 1
               else if( eq_count2 == 13 )  cmd_data = 2 ;     // 명령 2   
			   else                       cmd_data = 0xFE ;  // 명령 오류

               eq_count1 = 0;  eq_count2 = 0;

               new_recv_flag = 0 ; 

			}
     
        
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

		        HexToDec(recv_cnt,10);   // 수신된 바이트수 recv_cnt 십진수로 변환

                SerialPutString( "Received Data Count = " );     // 휴대폰으로 메시지 전송
//              SerialPutString( Send_Message_1 );               // 휴대폰으로 메시지 전송

                SerialPutChar( NumToAsc(cnumber[2]));  //  변수 recv_cnt 값을 휴대폰으로 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 

                SerialPutChar('\n');                  // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함

			}
			else if( cmd_data == 5 )      // 명령 5 이면
			{
                PORTB |= 0x40;            // Buzzer on
			}

            else if( cmd_data == 6 )      // 명령 6 이면
			{
                PORTB &= ~0x40;            // LED Toggle
			}


            else if( cmd_data == 0xFE )      //  명령 오류 이면 
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  명령 오류 메시지 전송

			    LcdCommand( 0x01) ;                                 // LCD Claear

			    LcdMove(0, 0 );                                     // LCD에 오류메시지 디스플레이
		        LcdPuts("Cmd Error!!"); 
			    LcdMove(1, 0 );
		        LcdPuts("Try Again."); 

				Command_Error_Flag = 1;                              // 명령 오류 플래그 셋

			}


     ///////////////////////////////////////////////////////////////


		   if( Command_Error_Flag == 0  &&  cmd_data != 0xFF  )   // 명령에 오류가 없고 명령초기상태(0xFF)가 아니면   
		   {  

	           LcdMove(1, 11); 
               Display_Number_LCD( recv_cnt, 3 ) ;    // 수신된 바이트수 recv_cnt를 십진수로 변환하여 LCD에 디스플레이

           }

     ////////////////////////////////////////////////////////////////


		   cmd_data = 0xFF;                             //  명령을 초기값으로 리셋


     ////////////////////////////////////////////////////////////


	}   // end of while(1)

}     //  End of main()

////

// UART0 수신 인터럽트 서비스 프로그렘 

ISR(  USART0_RX_vect )
{

    static unsigned char r_cnt = 0 ;

    rdata = UDR0; 

    if( rdata != '.' )                      // 수신된 데이터가 마지막 문자를 나타내는 데이터(마침표)가 아니면
    {
        recv_data[r_cnt] = rdata;        //  수신된 문자 저장 
	    r_cnt++;                         //  수신 문자 갯수 증가 

		new_recv_flag = 0;
    }
    else if(  rdata == '.' )                // 수신된데이터가 마지막 문자를 나타내는 데이터(마침표) 이면
    {
        recv_cnt = r_cnt ;                  // 수신된 데이터 바이트수 저장
        r_cnt = 0;  
        
		new_recv_flag = 1;

    }
}





void init_serial(void)
{
    UCSR0A=0x00;                    //초기화
    UCSR0B = 0x18  ;                //송수신허용,  송수신 인터럽트 금지
    UCSR0C=0x06;                    //데이터 전송비트 수 8비트로 설정.
    
    UBRR0H=0x00;
    UBRR0L=103;                     //Baud Rate 9600 
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

    while(*str != '\0')
    {

        SerialPutChar(*str++);
    }
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


void msec_delay( int n )
{
	for(; n> 0 ; n-- )   _delay_ms(1); 
}

