
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

static volatile  char  recv_cnt = 0, rdata=0, new_recv_flag = 0  ;  
               

static volatile unsigned char   Command_Error_Flag = 0 ; 


/********************************************************************************************************************
                                      					main
********************************************************************************************************************/
int main(void)
{ 	  
 

	 DDRB |= 0x10 ; 	// LED (PB4 ) :출력설정	

	 PORTB |= 0x10 ;    // LED OFF

     init_serial() ;    // Serial Port (USART1) 초기화

     LcdInit();         // LCD 초기화 


     UCSR0B |=  0x80  ;      // UART1 송신(RX) 완료 인터럽트 허용
	 sei() ;                 // 전역인터럽트허용


     LcdCommand( ALLCLR ) ;    // LCD Clear
  	 LcdMove(0,0);    
	 LcdPuts("Bluetooth Prog"); 
 
  	 LcdMove(1,0);    
	 LcdPuts("Send Command."); 

	  while(1)
	  {

         if( new_recv_flag == 1 )      // 1 문자 수신완료 시 
		 { 


		    if( Command_Error_Flag == 1 )    // 이전 명령에 오류가 있었으면
			{  
			      Command_Error_Flag = 0 ;     // 이전 Command_Error_Flag 리셋 
            }

            /////  수신된 바이트수(변수 값) LCD 디스플레이  /////////////////

            LcdCommand( ALLCLR ) ;    // LCD Clear

		    LcdMove(0,0);    
		    LcdPuts("Recv cnt = "); 
            Display_Number_LCD( recv_cnt, 3 ) ;  // 수신된 바이트수 recv_cnt를 십진수로 변환하여 LCD에 디스플레이


		    LcdMove(1,0);    
		    LcdPuts("Recv data = "); 

		    LcdPutchar( rdata );   // 수신된 문자 LCD에 디스플레이

          ////////////////////////////////////////////////////////

		  //////////////  명령어 처리   //////////////

			if( rdata == '0' )          // 문자 0 이 수신되면 
			{
                PORTB |= 0x10 ;          // LED OFF 
			}
			else if( rdata == '1' )     // 문자 1 이 수신되면
			{
                PORTB &= ~0x10 ;         // LED ON
			}
			else if( rdata == '2')      // 문자 2 가 수신되면
			{

		        HexToDec(recv_cnt,10);   // 수신된 바이트수 recv_cnt 십진수로 변환

                SerialPutString( "Received Data Count = " );     //  메시지 전송 
//              SerialPutString( Send_Message_1 );               //  메시지 전송 

                SerialPutChar( NumToAsc(cnumber[2]));            //  변수 recv_cnt 값 전송
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 
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
               Display_Number_LCD( recv_cnt, 3 ) ;  // 수신된 바이트수 recv_cnt를 십진수로 변환하여 LCD에 디스플레이

		       LcdMove(1,12);    
		       LcdPutchar( rdata );                 // 수신된 문자 rdata를 LCD에 디스플레이

           }

      ////////////////////////////////////////////////////////////////


		   rdata = 0xFF;                           // 수신된 명령을 초기값으로 리셋
           new_recv_flag = 0;                      // 새 문자(명령) 수신 플래그 Reset
  

        }
    //////////////////////////////////////////////////

 
	}   // end of while(1)

}     //  End of main()



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



