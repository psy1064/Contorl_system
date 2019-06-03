#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void init_serial(void) ;  //  Serial 토신포트 초기화

void SerialPutChar(char ch);
void SerialPutString(char str[]);

void Servo_Move( short sv_pos_cmd ) ;                     // Servo Move 함수 

void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit )  ;     // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 


void msec_delay(int n)  ; 


static volatile short     Servo_Pos_CMD = 0 ;    // 서보 위치 명령 ( 범위 : 0 - 180,  단위:  도 )
static volatile short     Pos_max = 180 ;        // 서보 최대 위치 명령 ( 180 도 )
static volatile short     Pos_min = 0 ;          // 서보 최소 위치 명령 ( 0   도 )
static volatile short     Pos_center = 90 ;      // 서보 중간 위치 명령 ( 90  도 )

static volatile unsigned char   Command_Error_Flag = 0 ; 

static volatile  char  recv_cnt = 0, rdata = 0, new_recv_flag = 0, rdata_old = 0 ; 


int main() 
{   


	DDRB |= 0x80;    //  PWM 포트: OC2( PB7 ) 출력설정 
 

	DDRB |= 0x08;     // 버저(Buzzer) ( PB3 : 출력포트 설정    )
	PORTB &= ~0x08;   // PB3  : Low  ( 버저 OFF )  

	DDRB |= 0x10;     // LED ( PB4 : 출력포트 설정    )
	PORTB |= 0x10;    // PB4  : High ( LED OFF)    


    init_serial() ;    // Serial Port (USART1) 초기화

    UCSR0B |=  0x80  ;      // UART1 송신(RX) 완료 인터럽트 허용
	sei() ;                 // 전역인터럽트허용


	LcdInit();    //  LCD 초기화 

	LcdMove(0,0); 
	LcdPuts("RC Servo Motor");
	LcdMove(1,0); 
	LcdPuts("Servo_Pos = ");

    msec_delay(2000); 


// PWM 신호  pin: OC2(PB7), Timer2, PWM signal (period= 16.384msec )

	TCCR2 |= 0x68;   //  Trigger signal (OC2)   발생 :  WGM20(bit6)=1,  WGM21(bit3)=1,  COM21(bit5)=1, COM20(bit4)=0 ,  
	TCCR2 |= 0x05;   //  1024분주,  내부클럭주기 = 64usec  : CS22(bit2) = 1, CS21(bit1) = 0,  CS20(bit0) = 1 


    Servo_Pos_CMD = Pos_center ;                        // 서보모터 중간 위치로 회전하는 명령 설정  
    Servo_Move( Servo_Pos_CMD );                        // 주어진 명령대로 서보 모터 회전

	LcdMove(1, 12); 
    Display_Number_LCD( Servo_Pos_CMD , 3); 

	 
	while (1) 
	{ 
 

         if( new_recv_flag == 1 )      // 한 문자 수신완료 시 
		 { 

		    if( Command_Error_Flag == 1 )    // 이전 명령에 오류가 있었으면
			{  
			    Command_Error_Flag = 0 ;     // 이전 Command_Error_Flag 리셋 

			    LcdCommand( ALLCLR ) ;       // LCD Claear
 	            LcdMove(0,0); 
	            LcdPuts("RC Servo Motor");
	            LcdMove(1,0); 
	            LcdPuts("Servo_Pos = ");

            }

		  //////////////  명령어 처리   //////////////

			if( rdata == '0' )          // 문자 0 이 수신되면 
			{
                PORTB |= 0x10 ;          // LED OFF 
			}
			else if( rdata == '1' )     // 문자 1 이 수신되면
			{
                PORTB &= ~0x10 ;         // LED ON
			}

			else if( rdata == '2' )            // 문자 2 가 수신되면
			{
		        Servo_Pos_CMD += 10 ;                                         // 서보모터 위치 10 도씩 증가  
                if( Servo_Pos_CMD >= Pos_max )   Servo_Pos_CMD = Pos_max ;    // 서보모터 최대 위치 Pos_max = 180 도
		        Servo_Move( Servo_Pos_CMD );                                  // 주어진 명령대로 서보 모터 회전  
 
				rdata_old = rdata ; 
			}

			else if( rdata == '3' )            // 문자 3 이 수신되면
			{
		        Servo_Pos_CMD -= 10 ;                                        // 서보모터 위치 10 도씩 증가  
                if( Servo_Pos_CMD < Pos_min )   Servo_Pos_CMD = Pos_min ;    // 서보모터 최소 위치 Pos_min = 0 도
		        Servo_Move( Servo_Pos_CMD );                                 // 주어진 명령대로 서보 모터 회전  
 
				rdata_old = rdata ; 
			}

			else if( rdata == '4' )            // 문자 4 가 수신되면
			{
                Servo_Pos_CMD = Pos_max ;
 		        Servo_Move( Servo_Pos_CMD );                // 서보모터 최대 위치 Pos_max = 180 도로 서보 모터 회전  
										 
			}

			else if( rdata == '5' )            // 문자 5 가 수신되면
			{

                Servo_Pos_CMD = Pos_center ;
 		        Servo_Move( Servo_Pos_CMD );               // 서보모터 가운데 위치 Pos_center = 90 도로 서보 모터 회전  
										 
			}

			else if( rdata == '6' )            // 문자 6 이 수신되면
			{
                Servo_Pos_CMD = Pos_min ;
 		        Servo_Move( Servo_Pos_CMD );               // 서보모터 최소 위치 Pos_min = 0 도로 서보 모터 회전  
										 
			}


			else if( rdata == '7')      // 문자 7 이 수신되면
			{

		        HexToDec( Servo_Pos_CMD,10);   // 서보 위치(명령) Servo_Pos_CMD 십진수로 변환

                SerialPutString( "Servo Moror Position = " );   //  메시지 전송 

                SerialPutChar( NumToAsc(cnumber[2]));           //  변수 Servo_Pos_CMD 값 전송
                SerialPutChar( NumToAsc(cnumber[1]));            
                SerialPutChar( NumToAsc(cnumber[0])); 

                SerialPutString( "deg" );                       //  메시지 전송
                SerialPutChar('\n');                            // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함

			} 

			else if( rdata != 0xFF)    //  명령 오류 이면
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  명령 오류 메시지 전송

			    LcdCommand( ALLCLR ) ;    // LCD Claear

			    LcdMove(0, 0 );           // LCD에 오류메시지 디스플레이
		        LcdPuts("Cmd Error!!"); 
			    LcdMove(1, 0 );
		        LcdPuts("Try Again."); 

				Command_Error_Flag = 1;  

			}


		    rdata = 0xFF;
            new_recv_flag = 0;      // 새 문자(명령) 수신 플래그 Reset
  

		   if( Command_Error_Flag == 0 )                // 명령에 오류가 없으면 
		   {  
	           LcdMove(1, 12); 
               Display_Number_LCD( Servo_Pos_CMD , 3 ); 
           }


        }              


     }           // end of while(1) loop


}                // end of main() loop



/////////////////////////////////////////////////////////////////////////////////////////



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


////////////////////////////////////////////////////////////////////////////////



void Servo_Move( short sv_pos_cmd )
{

      OCR2 = ( 135 * sv_pos_cmd )/900  + 10  ;  

      //  펄스폭 = 0.64msec = 64usec * 10,   왼쪽 끝(0 도)  (펄스폭 = 0.66msec )
      //  펄스폭 = 1.47msec = 64usec * 23 ,  가운데(90 도) (펄스폭 = 1.5msec )
      //  펄스폭 = 2.37msec = 64usec * 37 ,  오른쪽 끝(180 도) (펄스폭 = 2.45msec ) 

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



