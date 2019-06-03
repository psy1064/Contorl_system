
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "lcd.h"


void init_serial(void) ;  //  Serial 통신포트 초기화

void SerialPutChar(char ch);
void SerialPutString(char str[]);


void HexToDec(unsigned short num, unsigned short radix);

char NumToAsc( unsigned char Num );       // 숫자를 ASCII 코드(문자)로 변환하는 함수 

unsigned char AscToNum( char Asc ) ;      // 문자(ASCII 코드)를 숫자로 변환하는 함수 

void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이

void msec_delay( int n );

static volatile unsigned char cnumber[5] = { 0, 0, 0, 0, 0};


static volatile char Cmd_Message_1[] = { "Distance 1 = " } ;     //  Blutooth Command  Slave --> Master
static volatile char Cmd_Message_2[] = { "Distance 2 = " } ;  
 

static volatile  char  rdata = 0,  recv_cnt = 0, new_recv_flag = 0  ;                
static volatile  char  recv_data[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  };  


static volatile unsigned char   Command_Error_Flag = 0 ; 


/********************************************************************************************************************
                                      					main
********************************************************************************************************************/
int main(void)
{ 
 
   	 char    eq_count1 = 0, eq_count2 = 0, cmd_data = 0xFF  ;  	  
     unsigned char    i = 0 ;
     unsigned short   k = 0 ;

     unsigned short    distance_1 = 0, distance_2 = 0,  result = 0 ;


	 DDRB |= 0x10 ; 	// LED (PB4 ) :출력설정	

	 PORTB |= 0x10 ;    // LED OFF

     init_serial() ;    // Serial Port (USART1) 초기화

     LcdInit();         // LCD 초기화 


     UCSR1B |=  0x80  ;      // 송신(RX) 완료 인터럽트 허용
	 sei() ;                 // 전역인터럽트허용

     LcdCommand( ALLCLR ) ;    // LCD Clear
  	 LcdMove(0,0);    
	 LcdPuts("Blutooth Master"); 
 

  
	 while(1)
	 {

            if( new_recv_flag == 1 )      // 문자열 수신완료 시 
			{ 

		        if( Command_Error_Flag == 1 )    // 이전 명령에 오류가 있었으면
			    {  
			        Command_Error_Flag = 0 ;     // 이전 Command_Error_Flag 리셋 
                   
					LcdCommand( ALLCLR ) ;       // LCD 화면 지움 

////    측정거리 디스플레이  //////////////

	                LcdMove(0,0); 
	                LcdPuts("Dist_1 = ");
                    Display_Number_LCD( distance_1, 3 );      // 초음파센서 1 측정 거리 디스플레이 
	                LcdPuts("cm");

	                LcdMove(1,0); 
	                LcdPuts("Dist_2 = ");
                    Display_Number_LCD( distance_2, 3 );      // 초음파센서 2 측정 거리 디스플레이 
	                LcdPuts("cm");

////////////////////////////////


               }

  
               LcdMove(0, 0); 
			                  
               for( i = 0; i < ( recv_cnt - 3 ) ; i++) 
			   {
			      if( recv_data[i] == Cmd_Message_1[i] ) eq_count1++ ;
			      if( recv_data[i] == Cmd_Message_2[i] ) eq_count2++ ; 

                  LcdPutchar( recv_data[i] ); 

               }

               if     ( eq_count1 == 13 )  cmd_data = 1 ;     // 명령 1
               else if( eq_count2 == 13 )  cmd_data = 2 ;     // 명령 2   
			   else                       cmd_data = 0xFE ;  // 명령 오류

               eq_count1 = 0;  eq_count2 = 0;  

               new_recv_flag = 0 ; 

			}


         /////////////////////////////////
        
         ////////  명령(Command) 처리 ( Slave --> Master ) : Slave가 Master에게 보내온 데이터(명령) 처리


			if( cmd_data == 1 )     // 명령 1 이면
			{

				 for( i = 13 ; i < recv_cnt ; i++)
			     {
					  result *= 10 ; 
			          result += AscToNum( recv_data[i] )  ; 
                 }

                 distance_1  = result  ;
                 result = 0 ; 

//               SerialPutString( "Received Data Count = " );     // 휴대폰으로 메시지 전송
//               SerialPutChar('\n');                  // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함

             }
           
			else if( cmd_data == 2 )     // 명령 2 이면
			{

				 for( i = 13 ; i < recv_cnt ; i++)
			     {
					  result *= 10 ; 
			          result += AscToNum( recv_data[i] )  ; 
                 }

                 distance_2  = result  ;
                 result = 0 ;

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

////    측정거리 디스플레이

	           LcdMove(0,0); 
	           LcdPuts("Dist_1 = ");
               Display_Number_LCD( distance_1, 3 );      // 초음파센서 1 측정 거리 디스플레이 
	           LcdPuts("cm");

	           LcdMove(1,0); 
	           LcdPuts("Dist_2 = ");
               Display_Number_LCD( distance_2, 3 );      // 초음파센서 2 측정 거리 디스플레이 
	           LcdPuts("cm");

////////////////////////////////

           }

     ////////////////////////////////////////////////////////////////


		   cmd_data = 0xFF;                             //  명령을 초기값으로 리셋


     ////////////////////////////////////////////////////////////


	       k++ ;

		   if( k == 1 )  /////  명령(Command) 보냄 ( Master --> Slave ) : Master가 Slave에게 명령 보냄
		   {

                SerialPutString( "Read USonic Sensor1." );     // Slave 로 메시지 전송
 
		   }

		   else if( k == 4 )   //     
		   {
                SerialPutString( "Read USonic Sensor2." );     // Slave 로 메시지 전송
 
		   }

		   else if( k == 7 )   //   
		   { 
              k = 0; 
 
		   }


		   msec_delay(10) ;


	}   // end of while(1)

}     //  End of main()

////

// UART1 수신 인터럽트 서비스 프로그렘 

ISR(  USART1_RX_vect )
{

    static unsigned char r_cnt = 0 ;

    rdata = UDR1; 

    if( rdata != '.' )                      // 수신된 데이터가 마지막 문자를 나타내는 데이터(마침표)가 아니면
    {
//        SerialPutChar( rdata);               // Echo  수신된 데이터를 바로 송신하여 수신된 데이터가 정확한지 확인 
   	    recv_data[r_cnt] = rdata;        //  수신된 문자 저장 
	    r_cnt++;                         //  수신 문자 갯수 증가 

		new_recv_flag = 0;

    }
    else if(  rdata == '.' )                // 수신된데이터가 마지막 문자를 나타내는 데이터(마침표) 이면
    {
//        SerialPutChar('\n');                // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함 
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


// 숫자를 ASCII 코드(문자)로 변환하는 함수

char NumToAsc( unsigned char Num )
{
	if( Num <10 ) Num += 0x30; 
	else          Num += 0x37; 

	return Num ;
}


 // 문자(ASCII 코드)를 숫자로 변환하는 함수 

unsigned char AscToNum( char Asc )
{
	if( Asc <= '9' )       Asc -= 0x30; 
	else if( Asc >= 'A' )  Asc -= 0x37; 

	return Asc ;
}


void msec_delay(int n)
{	
	for(; n>0; n--)		// 1msec 시간 지연을 n회 반복
		_delay_ms(1);		// 1msec 시간 지연
}



