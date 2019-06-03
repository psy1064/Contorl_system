#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void init_serial(void) ;  //  Serial 토신포트 초기화

void SerialPutChar(char ch);
void SerialPutString(char str[]);


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num); 
short AscToNum( unsigned char Num);

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 
	 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 


void msec_delay(int n) ;
 
void DC_Motor_Run_Fwd( short duty );    // DC 모터 정회전(PWM구동) 함수 
void DC_Motor_Run_Rev( short duty );    // DC 모터 역회전(PWM구동) 함수  
void DC_Motor_Stop( void );             // DC 모터 정지 함수  
void DC_Motor_PWM( short Vref );        // DC 모터 PWM 신호 발생 함수  
                                        // 정토크(Vref>0), 역토크(Vref<0), 영토크(Vref=0) 모두 포함 
static volatile short  Vmax = 0 ; 


static volatile  char  recv_cnt = 0, rdata = 0, new_recv_flag = 0, rdata_old = 0 ;  
static volatile  char  recv_data[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
static volatile unsigned char   Command_Error_Flag = 0 ; 

static volatile char Cmd_Message_1[] = {"dc motor speed=" } ;     //  Blutooth Command
static volatile char Cmd_Message_2[] = {"dc motor stop"} ;  

int main() 
{   
	char    eq_count1=0, eq_count2=0, cmd_data = 0xFF  ;  	  
    unsigned char   i=0 ;
	short duty = 0;	
	
	DDRB |= 0x20;   // 모터구동신호 + 단자:  PWM 포트( pin: OC1A(PB5) )   --> 출력 설정 
	DDRA |= 0x01;   // 모터구동신호 - 단자 : 범용 입/출력포트(pin : PA0 ) --> 출력 설정 

    init_serial() ;    // Serial Port (USART0) 초기화

    UCSR0B |=  0x80  ;      // UART0 송신(RX) 완료 인터럽트 허용
	sei() ;                 // 전역인터럽트허용


// 모터구동신호 ( pin: OC1A(PB5) ),   Timer1, PWM signal (period= 200 usec )

	TCCR1A = 0x82;    // OC1A(PB5)) :  PWM 포트 설정,   Fast PWM ( mode 14 )
	TCCR1B = 0x1b;    // 64 분주 타이머 1 시작 (내부클럭 주기 =  64/(16*10^6) = 4 usec ),  Fast PWM ( mode 14 ) 
	ICR1 = 50;        // PWM 주기 = 50 * 4 usec = 200 usec (  PWM 주파수 = 1/200usec = 5 kHz )

    Vmax = ICR1; 

	OCR1A = duty;      //  OC1A(PB5) PWM duty = 0 설정 : 모터 정지
//////////////////////////////////////////////////////////////////

	LcdInit();

	LcdMove(0,0); 
	LcdPuts("DC Motor Control");

	LcdMove(1,0); 
	LcdPuts("Duty = ");
	while(1)
	{
        if( new_recv_flag == 1 )      // 문자열 수신완료 시 
		{ 
	        if( Command_Error_Flag == 1 )    // 이전 명령에 오류가 있었으면
		    {  
		        Command_Error_Flag = 0 ;     // 이전 Command_Error_Flag 리셋 
                  
				LcdCommand( ALLCLR ) ;       // LCD 화면 지움 
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
			if(recv_data[15] == '-')	
			{
				duty = AscToNum(recv_data[16]) * 10 + AscToNum(recv_data[17]);
				LcdCommand(ALLCLR);
				LcdMove(0,0); 
				LcdPuts("DC Motor Control");

				LcdMove(1,0); 
				LcdPuts("Duty = ");

				LcdMove(1,7); 
        		LcdPuts("-");

				LcdMove(1,8);
				Display_Number_LCD(duty,2);

				DC_Motor_Run_Rev( duty );
			}
			else
			{
				duty = AscToNum(recv_data[15]) * 10 + AscToNum(recv_data[16]);
				LcdCommand(ALLCLR);
				LcdMove(0,0); 
				LcdPuts("DC Motor Control");

				LcdMove(1,0); 
				LcdPuts("Duty = ");

				LcdMove(1,7);
				Display_Number_LCD(duty,2);
				DC_Motor_Run_Fwd( duty );        // DC 모터 PWM 신호 발생 함수 사용
			}  
		}
		else if( cmd_data == 2 )      // 명령 2 이면
		{
			LcdCommand(ALLCLR);
			LcdMove(0,0); 
			LcdPuts("DC Motor Control");

			LcdMove(1,0); 
			LcdPuts("Duty = ");
        	DC_Motor_Stop();               // DC Motor 정지
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

			/*LcdMove(1, 11); 
            Display_Number_LCD( recv_cnt, 3 ) ;    // 수신된 바이트수 recv_cnt를 십진수로 변환하여 LCD에 디스플레이*/

        }
		////////////////////////////////////////////////////////////////

		cmd_data = 0xFF;                             //  명령을 초기값으로 리셋

////////////////////////////////////////////////////////////


	}   // end of while(1)

}     //  End of main()

//////////////////////////////////////////////////////////////////////

void DC_Motor_Run_Fwd( short duty )   // DC 모터 정회전 함수 
{

    if( duty > Vmax )     duty = Vmax ;

    PORTA &= ~0x01;     //  모터구동신호 - 단자 : 0 V 인가( PA0 = 0 );  
	OCR1A = duty;       //  모터구동신호 + 단자 : OC1A(PB5) PWM duty 설정 


}

void DC_Motor_Run_Rev( short duty )   // DC 모터 역회전 함수 
{

    if( duty > Vmax )     duty = Vmax ;

    PORTA |= 0x01;            //  모터구동신호 - 단자 : 5 V 인가( PA0 = 1 );  
	OCR1A = Vmax - duty;      //  모터구동신호 + 단자 : OC1A(PB5) PWM duty 설정 


}


void DC_Motor_Stop( void )   // DC 모터 정지 함수 
{

    PORTA &= ~0x01;     //  모터구동신호 - 단자 : 0 V 인가( PA0 = 0 );  
	OCR1A = 0;          //  모터구동신호 + 단자 : OC1A(PB5) PWM duty = 0 설정 
}


void DC_Motor_PWM( short Vref )   // DC 모터 PWM 신호 발생 함수  
{

   if ( Vref > Vmax )       Vref = Vmax ;
   else if( Vref < -Vmax )  Vref = -Vmax ;

   if( Vref > 0 )  
   {
      DC_Motor_Run_Fwd( Vref ) ;
   }
   else if( Vref == 0 )  
   {
      DC_Motor_Stop() ;
   }
   else if( Vref < 0 )  
   {
      DC_Motor_Run_Rev( -Vref ) ;
   }


}

///////////////////////////////////////////////////////////////////////////////



// UART0 수신 인터럽트 서비스 프로그램

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

		SerialPutString(recv_data);
		SerialPutString("\n");

    }	
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
short AscToNum( unsigned char Num)
{
	Num -= 0x30;
	return Num;
}
void msec_delay(int n)
{	
	for(; n>0; n--)		// 1msec 시간 지연을 n회 반복
		_delay_ms(1);		// 1msec 시간 지연
}





