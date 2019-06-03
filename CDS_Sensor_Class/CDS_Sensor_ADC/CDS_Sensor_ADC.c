#include <avr/io.h>
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"

#define  Avg_Num   4         //  이동 평균 갯수 

void HexToDec( unsigned short num, unsigned short radix); 
char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

void Display_Number_LCD( unsigned int num, unsigned char digit ) ;      // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 

void msec_delay(unsigned int n);
void usec_delay(unsigned int n);

unsigned short ADC_Read(unsigned char chan_num);

static volatile unsigned short CDS_sensor_ouput= 0,  CDS_sensor_ouput_avg = 0, LED_count = 0 ; 

int main() 
{   
	DDRB |= 0x70;     // LED (PB4,PB5,PB6 : 출력설정)
	PORTB |= 0x70;    // PB4,5,6 : High(LED OFF)  

	LcdInit();

	LcdCommand(ALLCLR);
	LcdMove(0,0); 
	LcdPuts("CDS value =     ");
	LcdMove(1,0); 
	LcdPuts("LEC count =     ");
 
/*****   AD Converter **********/

	ADMUX &= ~0xE0;     // ADC 기준전압 = AREF, ADC 결과 오른쪽정렬, 채널 AD0
					    // REFS1 = 0, REFS0 = 0(기준전압 AREF), ADLAR = 0(결과 오른쪽 정렬)
	ADCSRA |= 0x87;     // ADC enable, Prescaler = 128
					   	// ADEN = 1(ADC Enable), ADPS2 = 1, ADPS1 = 1, ADPS = 1(Prescaler = 128분주)
						// 16Mkhz / 128 = 125kHz

/**** Timer0 Overflow Interrupt  ******/

	TCCR0 = 0x00; 
    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64use

	TIMSK = 0x01;  // Timer0 overflow interrupt enable 
	sei();         // Global Interrupt Enable 


	TCCR0 |= 0x07; // Clock Prescaler N=1024 (Timer 0 Start)
		 
	while (1) 
	{ 
 	   LcdMove(0,12); 
       Display_Number_LCD( CDS_sensor_ouput, 4 ); 

 	   LcdMove(1,12); 
       Display_Number_LCD( LED_count, 1 ); 

	}
} 

ISR(TIMER0_OVF_vect)   // Timer0 overflow interrupt( 10 msec)  service routine
{

    static unsigned short  time_index = 0,  count1 = 0, CDS_Sum = 0; 
    static unsigned short  CDS_sensor_ouput_buf[Avg_Num ]   ; 

    unsigned char i = 0 ;


    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,  
                             //  오버플로인터럽트 주기 = 10msec
                             //  156 = 10msec/ 64usec

    time_index++ ; 


//  if( time_index == 25 )    // 샘플링주기 =  250 msec = 10msec x 25 
	if( time_index == 5 )    // 샘플링주기 =  50 msec = 10msec x 5 
    {

       time_index = 0; 

    /**************   CDS Sensor signal detection(AD 변환) ************/
/*
	   ADMUX &= ~0x1F;  	// ADC Chanel 0 : ADC0 선택
	   						// MUX4 ~ 0 = 0(ADC0 싱글 엔드)
	   ADMUX &= ~0x1F;
	   ADMUX |= 0x02;		// Chanel 2 : ADC2 선택

	   ADCSRA |= 0x40;   	// ADC start
	   						// ADCS = 1(ADC Start Conversion)

	   while( ( ADCSRA & 0x10 ) == 0x00  ) ;  // Check if ADC Conversion is completed
	   
	   ADCSRA |= 0x10;		// ADIF 플래그 비트 리셋

	   CDS_sensor_ouput = ADC;   
*/
	   CDS_sensor_ouput = ADC_Read(3);	   
 	   	   
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
	   }

       //////////////////////////////////////////////////////////////////

	   if( CDS_sensor_ouput_avg <= 300 )
	   {	
	   	   PORTB |= 0x70;   // PB4,5,6  : High ( LED OFF )  	   
	       PORTB &= ~0x70;   // PB4,5,6  : Low ( LED ON ) 
		   LED_count = 3; 
       }
	   else if( CDS_sensor_ouput_avg <= 400 )
	   {	
		   PORTB |= 0x70;   // PB4,5,6  : High ( LED OFF )  	   
	       PORTB &= ~0x30;   // PB4,5  :Low ( LED ON ) 
		   LED_count = 2; 	
       }
	   else if( CDS_sensor_ouput_avg <= 500 )
	   {	
		   PORTB |= 0x70;   // PB4,5,6  : High ( LED OFF )  	   
	       PORTB &= ~0x10;   // PB4  : Low ( LED ON )
		   LED_count = 1; 	
       }
	   else if( CDS_sensor_ouput_avg > 500 )
	   {	
		   PORTB |= 0x70;   // PB4,5,6  : High ( LED OFF )  	   
		   LED_count = 0; 	
       }
   }
}

unsigned short ADC_Read(unsigned char chan_num)
{

	   unsigned short adc_val = 0;

	   /*****   CDS Sensor signal detection(AD 변환) *****/

	   ADMUX &= ~0x1F;  	// ADC Chanel 0 : ADC0 선택

	   if(chan_num == 1)
	   {
	   		ADMUX |= 0x01;	// Chanel 1 : ADC1 선택
	   }
	   else if(chan_num == 2)
	   {
	   		ADMUX |= 0x02;	// Chanel 2 : ADC2 선택
	   }
	   else if(chan_num == 3)
	   {
	   		ADMUX |= 0x03;	// Chanel 3 : ADC3 선택
	   }
	   else if(chan_num == 4)
	   {
	   		ADMUX |= 0x04;	// Chanel 4 : ADC4 선택
	   }
	   else if(chan_num == 5)
	   {
	   		ADMUX |= 0x05;	// Chanel 5 : ADC5 선택
	   }
	   else if(chan_num == 6)
	   {
	   		ADMUX |= 0x06;	// Chanel 6 : ADC6 선택
	   }
	   else if(chan_num == 7)
	   {
	   		ADMUX |= 0x07;	// Chanel 7 : ADC7 선택
	   }


	   ADCSRA |= 0x40;   	// ADC start
	   						// ADCS = 1(ADC Start Conversion)

	   while( ( ADCSRA & 0x10 ) == 0x00  ) ;  // Check if ADC Conversion is completed
	   
	   ADCSRA |= 0x10;		// ADIF 플래그 비트 리셋

	   adc_val = ADC;   

	   return adc_val;
 	   	   
/******************************************************/ 
}

void Display_Number_LCD( unsigned int num, unsigned char digit )       // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이 
{

	HexToDec( num, 10); //10진수로 변환 

	if( digit == 0 )     digit = 1 ;
	if( digit > 5 )      digit = 5 ;
 
    if( digit >= 5 )     LcdPutchar( NumToAsc(cnumber[4]));  // 10000자리 디스필레이	
	if( digit >= 4 )     LcdPutchar(NumToAsc(cnumber[3]));    // 1000자리 디스필레이 
	if( digit >= 3 )     LcdPutchar(NumToAsc(cnumber[2]));    // 100자리 디스필레이 
	if( digit >= 2 )     LcdPutchar(NumToAsc(cnumber[1]));    // 10자리 디스필레이
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

