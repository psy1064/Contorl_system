#include <avr/io.h>
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"

#define  Avg_Num   	60         //  �̵� ��� ���� 
#define  Amp_Gain   11         //  ������ �̵�  

void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
void Display_TMP_LCD( unsigned int tp  )  ;                           // �µ��� 10���� ���·� LCD �� ���÷��� 

void msec_delay(unsigned int n);
void usec_delay(unsigned int n);

static volatile unsigned short TMP_sensor_ouput= 0,  TMP_sensor_ouput_avg = 0, TMP_sensor_ouput_avg_C = 0 ; 

int main() 
{   
	DDRB |= 0x10;     // LED (PB4 : ��¼��� )
	PORTB &= ~0x10;   // PB4  : High ( LED OFF)  

	LcdInit();

	LcdCommand(ALLCLR);
	LcdMove(0,0);  
	LcdPuts("TMP =     C");
//	LcdPuts("TMP value =    ");
	LcdMove(1,0); 
	LcdPuts("TMP avg =     ");
 
/*****   AD Converter **********/

	ADMUX &= ~0xE0;    //  ADC �������� = AREF ,   ADC ��� ���������� 
	ADCSRA |= 0x87;     // ADC enable, Prescaler = 128

/**** Timer0 Overflow Interrupt  ******/
/**************************************/
	TCCR0 = 0x00; 
    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64use

	TIMSK = 0x01;  // Timer0 overflow interrupt enable 
	sei();         // Global Interrupt Enable 


	TCCR0 |= 0x07; // Clock Prescaler N=1024 (Timer 0 Start)
		 
	while (1) 
	{ 

// 	   LcdMove(0,12); 
//     Display_Number_LCD( TMP_sensor_ouput, 4 ); 

 	   LcdMove(0,6); 
       Display_TMP_LCD( TMP_sensor_ouput_avg_C  );  
 	   LcdMove(1,12); 
       Display_Number_LCD( TMP_sensor_ouput_avg, 4 ); 

	}
} 

ISR(TIMER0_OVF_vect)   // Timer0 overflow interrupt( 10 msec)  service routine
{

    static unsigned short  time_index = 0,  count1 = 0, TMP_Sum = 0; 
    static unsigned short  TMP_sensor_ouput_buf[Avg_Num ]   ; 

    unsigned char i = 0 ;


    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64usec

    time_index++ ; 


    if( time_index == 10 )    // ���ø��ֱ� =  250 msec = 10msec x 25 
    {

       time_index = 0; 


      /**************   Temperature Sensor signal detection(AD ��ȯ) ************/

	   ADMUX &= ~0x1F;    //  WADC Chanel 1 : ADC 1 ����
	   ADMUX |= 0x00;     //  ADC Chanel 1 : ADC 1 ����

	   ADCSRA |= 0x40;    // ADC start 

	   while( ( ADCSRA & 0x10 ) == 0x00  ) ;  // Check if ADC Conversion is completed 

	   ADCSRA |= 0x10;						  // ADIF �÷��� ��Ʈ ����

	   TMP_sensor_ouput = ADC;                // ADC Conversion �� �Ϸ�Ǿ����� ADC ��� ���� 
 
     /******************************************************/ 

     ////////////////////////////////////////////////////////////////////
     //////////                                               /////////// 
     //////////  Avg_Num(60��) ���� �̵� ���(Moving Average)  ///////////
     //////////                                               ///////////
     ////////////////////////////////////////////////////////////////////

	   if( count1 <= ( Avg_Num -1 ) )
	   {
             TMP_sensor_ouput_buf[ count1 ] = TMP_sensor_ouput ;
			 TMP_Sum +=  TMP_sensor_ouput_buf[ count1 ] ; 
	         count1++ ; 
	   } 
	   else
	   {
             TMP_Sum +=  TMP_sensor_ouput  ;	       // ���� �ֱ� �� ���ϰ�  
             TMP_Sum -=  TMP_sensor_ouput_buf[ 0 ] ;   // ���� ������ �� ���� 

             TMP_sensor_ouput_avg = TMP_Sum / Avg_Num ;     // 4�� �̵� ��� 

             //  �����µ� ��� : ������(������ �̵� = Amp_Gain ) ��������� 
             // TMP_sensor_ouput_avg_C =   ( unsigned short) ( (unsigned long) 1250 * TMP_sensor_ouput_avg  / (256 * Amp_Gain)  )  ;    // �µ� ��� [C] ����

             // �����µ� ��� : ������ ������� �ʾ�����  
               TMP_sensor_ouput_avg_C =   ( unsigned short) ( (unsigned long) 1250 * TMP_sensor_ouput_avg  / 256  )  ;           // �µ� ��� [C] ����


             for( i = 0; i <= (Avg_Num - 2) ; i++ )
			 {
                 TMP_sensor_ouput_buf[ i ]  = TMP_sensor_ouput_buf[ i+1 ] ;
			 } 

             TMP_sensor_ouput_buf[ Avg_Num - 1 ]  = TMP_sensor_ouput ;  

	   }

       //////////////////////////////////////////////////////////////////
   }
}

void Display_Number_LCD( unsigned int num, unsigned char digit )       // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
{

	HexToDec( num, 10); //10������ ��ȯ 

	if( digit == 0 )     digit = 1 ;
	if( digit > 5 )      digit = 5 ;
 
    if( digit >= 5 )     LcdPutchar( NumToAsc(cnumber[4]) );  // 10000�ڸ� ���÷���
	
	if( digit >= 4 )     LcdPutchar(NumToAsc(cnumber[3]));    // 1000�ڸ� ���÷��� 

	if( digit >= 3 )     LcdPutchar(NumToAsc(cnumber[2]));    // 100�ڸ� ���÷��� 

	if( digit >= 2 )     LcdPutchar(NumToAsc(cnumber[1]));    // 10�ڸ� ���÷���

	if( digit >= 1 )     LcdPutchar(NumToAsc(cnumber[0]));    //  1�ڸ� ���÷���

}


void Display_TMP_LCD( unsigned int tp  )       // �µ��� 10���� ���·� LCD �� ���÷��� 
{

	HexToDec( tp, 10); //10������ ��ȯ 

 
    LcdPutchar(NumToAsc(cnumber[2]) );   // 10�ڸ� ���÷���
	
    LcdPutchar(NumToAsc(cnumber[1]));    // 1�ڸ� ���÷��� 

    LcdPuts( ".");                       // �Ҽ���(.) ���÷��� 

    LcdPutchar(NumToAsc(cnumber[0]));    // 0.1 �ڸ� ���÷��� 

 

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
	for(; n>0; n--)		// 1msec �ð� ������ nȸ �ݺ�
		_delay_ms(1);		// 1msec �ð� ����
}

void usec_delay(unsigned int n)
{	
	for(; n>0; n--)		// 1usec �ð� ������ nȸ �ݺ�
		_delay_us(1);		// 1usec �ð� ����
}



