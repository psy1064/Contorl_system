
#include <avr/io.h>

#include "lcd.h"


void init_serial(void) ;  //  Serial 토신포트 초기화

void SerialPutChar(char ch);
void SerialPutString(char str[]);


char Send_Message_1[] = { "Tx Success!" } ;                    

/********************************************************************************************************************
                                      					main
********************************************************************************************************************/
int main(void)
{ 
 
   	 unsigned char rdata=0 ;  	  
 

	 DDRB |= 0x10; 	   // LED (PB4 ) :출력설정	

	 PORTB |= 0x10;    // LED OFF

     init_serial() ;   // Serial Port (USART1) 초기화

     LcdInit();       // LCD 초기화 


     LcdCommand( ALLCLR ) ;    // LCD Clear
  	 LcdMove(0,0);    
	 LcdPuts("Blutooth Module"); 
 
  	 LcdMove(1,0);    
	 LcdPuts("HC-06 Test Prog"); 
  
	 while(1)
	 {
       		if( (UCSR0A & 0x80) != 0)       // 1000 0000    문자 수신이 완료되면( UCSR1A의 RXC가 비트1이 되면) 
        	{                               // : Polling 방식으로 체크(인터럽트 사용안함)

            	rdata = UDR0;               //  수신된 문자 저장 
        	}
        	else rdata = rdata;             //  수신된 문자가 없으면 이전 데이터 사용

 
			if( rdata == '0' )              // 문자 0 이 수신되면 
			{
                PORTB |= 0x10;              // LED OFF 

				rdata = 0xFF;
			}
			else if( rdata == '1' )         // 문자 1 이 수신되면 
			{
                PORTB &= ~0x10;             // LED ON

				rdata = 0xFF;
			}
			else if( rdata == '2')          // 문자 2 가 수신되면 
			{
				SerialPutString("Tx Success!");  // 휴대폰으로 메시지 전송
                SerialPutChar('\n');             // 휴대폰으로 데이터(문자) 전송시 '\n'를 항상 끝에 전송해야함 

				rdata = 0xFF;
			}

 
	}

} 


void init_serial(void)
{
    UCSR0A=0x00;                    //초기화
    UCSR0B=0x18;                    //송수신허용,버퍼인터럽트 금지
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

    while(*str != '\0')          // 수신된 문자가 Null 문자( 0x00 )가 아니면 
    {
        SerialPutChar(*str++);
    }
}



