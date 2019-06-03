#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


// 온습도 센서
#define DHT11_PIN PD4
void Request();
void Response();
uint8_t Receive_data();
void getDHT();
uint8_t c=0,I_RH,D_RH,I_Temp,D_Temp,CheckSum;
char i_rh[5], d_rh[5], i_temp[5], d_temp[5];

// LCD
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 

// Blue tooth communication
void init_serial(void) ;  //  Serial 토신포트 초기화
void SerialPutChar(char ch);
void SerialPutString(char str[]);
void sendDHT();

// 범용 
void HexToDec( unsigned short num, unsigned short radix); 
char NumToAsc( unsigned char Num ); 
void msec_delay(unsigned int n);
void usec_delay(unsigned int n);
void pin_init();		// 핀 설정 초기화
void init();			// 초기 설정

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 



int main() 
{   
	pin_init();		  // Pin 초기화
	init();			  // Interrupt , Timer, Register 초기화 
	init_serial() ;   // Serial Port (USART1) 초기화
		 
	while (1) 
	{ 
		
	}
} 


ISR(TIMER0_OVF_vect)   // Timer0 overflow interrupt( 10 msec)  service routine
{

	static unsigned short  time_index = 0, send_time_index = 0;

    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec

    time_index++ ; 
	
    if( time_index == 500 )    // 샘플링주기 10msec
    {
       time_index = 0; 
	   getDHT();
	   sendDHT();
	   
   }
}


void pin_init()
{
	DDRB |= 0x10;     // LED (PB4 : 출력설정 )
	PORTB &= ~0x10;   // PB4  : High ( LED OFF) 
}
void init()
{
	LcdInit();

	LcdCommand(ALLCLR);
	LcdMove(0,0);  
	LcdPuts("HUM=");
	LcdMove(1,0); 
	LcdPuts("TMP= ");


/**** Timer0 Overflow Interrupt  ******/

	TCCR0 = 0x00; 
    TCNT0 = 256 - 100;       //  내부클럭주기 = 8/ (16x10^6) = 0.5 usec,  
                             //  오버플로인터럽트 주기 = 50usec
                             //  156 = 50usec/ 0.5use

	TIMSK = 0x01;  // Timer0 overflow interrupt enable 
	sei();         // Global Interrupt Enable 


	TCCR0 |= 0x07; // Clock Prescaler N=1024 (Timer 0 Start)
}
void init_serial(void)
{
    UCSR0A = 0x00;                    //초기화
    UCSR0B = 0x18  ;                  //송수신허용,  송수신 인터럽트 금지
    UCSR0C = 0x06;                    //데이터 전송비트 수 8비트로 설정.
    
    UBRR0H = 0x00;
    UBRR0L = 103;                     //Baud Rate 9600 
}
void sendDHT()
{
	SerialPutString(i_temp);
	SerialPutChar('.');
	SerialPutString(d_temp);
	SerialPutChar(',');
	SerialPutString(i_rh);
	SerialPutChar('.');
	SerialPutString(d_rh);
}
void getDHT()
{

	Request();		/* send start pulse */
	Response();		/* receive response */
	I_RH=Receive_data();	/* store first eight bit in I_RH */
	D_RH=Receive_data();	/* store next eight bit in D_RH */
	I_Temp=Receive_data();	/* store next eight bit in I_Temp */
	D_Temp=Receive_data();	/* store next eight bit in D_Temp */
	CheckSum=Receive_data();/* store next eight bit in CheckSum */
	
	if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum)
	{
		LcdMove(0,0);
		LcdPuts("Error");
	}
		
	else
	{	
		itoa(I_RH,i_rh,10);
		LcdMove(0,4);
		LcdPuts(i_rh);
		LcdMove(0,6);
		LcdPuts(".");
			
		itoa(D_RH,d_rh,10);
		LcdMove(0,7);
		LcdPuts(d_rh);
		LcdMove(0,8);
		LcdPuts("%");
		itoa(I_Temp,i_temp,10);
		LcdMove(1,4);
		LcdPuts(i_temp);
		LcdMove(1,6);
		LcdPuts(".");
			
		itoa(D_Temp,d_temp,10);
		LcdMove(1,7);
		LcdPuts(d_temp);
		LcdMove(1,8);
		LcdPuts("C");
		
	}
				
	_delay_ms(10);
}

void Request()				/* Microcontroller send start pulse/request */
{
	DDRD |= (1<<DHT11_PIN);
	PORTD &= ~(1<<DHT11_PIN);	/* set to low pin */
	_delay_ms(20);			/* wait for 20ms */
	PORTD |= (1<<DHT11_PIN);	/* set to high pin */
	_delay_us(40);
}

void Response()				/* receive response from DHT11 */
{
	DDRD &= ~(1<<DHT11_PIN);
	while(PIND & (1<<DHT11_PIN));
	while((PIND & (1<<DHT11_PIN))==0);
	while(PIND & (1<<DHT11_PIN));
}

uint8_t Receive_data()			/* receive data */
{	
	for (int q=0; q<8; q++)
	{
		while((PIND & (1<<DHT11_PIN)) == 0);  /* check received bit 0 or 1 */
		_delay_us(30);
		if(PIND & (1<<DHT11_PIN)) /* if high pulse is greater than 30ms */
		{
			c = (c<<1)|(0x01);	/* then its logic HIGH */
		}
		else			/* otherwise its logic LOW */
		{
			c = (c<<1);
		}

		while(PIND & (1<<DHT11_PIN));
		
	}
	return c;
}

void SerialPutChar(char ch)
{
	while(!(UCSR0A & (1<<UDRE)));			// 버퍼가 빌 때를 기다림
  	UDR0 = ch;								// 버퍼에 문자를 쓴다
} // 한 문자를 송신한다.

void SerialPutString(char *str)
 {

    while(*str != '\0')          // 수신된 문자가 Null 문자( 0x00 )가 아니면 
    {
        SerialPutChar(*str++);
    }
} // 문자열을 송신한다.
  // 입력   : str - 송신한 문자열을 저장할 버퍼의 주소


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



void msec_delay(unsigned int n)
{	
	for(; n>0; n--)		// 1msec 시간 지연을 n회 반복
		_delay_ms(1);		// 1msec 시간 지연
}

void usec_delay(unsigned int n)
{	
	for(; n>0; n--)		// 1usec 시간 지연을 n회 반복
		_delay_us(1);		// 1usec 시간 지연
}



