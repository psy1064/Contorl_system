
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "lcd.h"


#define  Avg_Num     4         //  �̵� ��� ���� 
#define  Amp_Gain   11         //  ������ �̵�  

#define  TMP_Mode   0          //  �µ����� ���÷��� ���   
#define  CDS_Mode   1          //  CDS ���� ���÷��� ���  



void init_serial(void) ;  //  Serial �����Ʈ �ʱ�ȭ

void SerialPutChar(char ch);
void SerialPutString(char str[]);

void HexToDec(unsigned short num, unsigned short radix);
char NumToAsc(char Num);

void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
void Display_TMP_LCD( unsigned int tp  )  ;                           // �µ��� 10���� ���·� LCD �� ���÷��� 

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
 

	 DDRB |= 0x10 ; 	// LED (PB4 ) :��¼���	

	 PORTB |= 0x10;     // LED OFF


   // Push Switch : �ܺ����ͷ�Ʈ 0 (INT0 : PD0 )�� ����   
	DDRD &= ~0x01;     // PD0 (�ܺ����ͷ�Ʈ INT0 ) : �Է¼���   
    PORTD |= 0x01;     // PD0 : ����Ǯ�����   


   ////////  �ܺ� ���ͷ�Ʈ(INT0 ) ����  ///////////


    EICRA &= ~0x01;  // INT0 �ϰ��𼭸����� ���ͷ�Ʈ �ɸ�
    EICRA |=  0x02;  // INT0 �ϰ��𼭸����� ���ͷ�Ʈ �ɸ�

    EIMSK |=  0x01;  // INT0 ���ͷ�Ʈ  ���

  ///////////////////////////////////////////////


    LcdInit();         // LCD �ʱ�ȭ 

	LcdCommand(ALLCLR);
	LcdMove(0,0);  
	LcdPuts("TMP avg =     C");
//	LcdPuts("TMP value =    ");
	LcdMove(1,0); 
	LcdPuts("CDS avg =     ");


  /////////////////////////////////////////////////

     init_serial() ;    // Serial Port (USART1) �ʱ�ȭ
     UCSR1B |=  0x80  ;      // UART1 �۽�(RX) �Ϸ� ���ͷ�Ʈ ���


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



	  while(1)
	  {

         if( new_recv_flag == 1 )      // 1 ���� ���ſϷ� �� 
		 { 


		  //////////////  ��ɾ� ó��   //////////////

			if( rdata == '0' )          // ���� 0 �� ���ŵǸ� 
			{
                PORTB |= 0x10;          // LED OFF 
			}
			else if( rdata == '1' )     // ���� 1 �� ���ŵǸ�
			{
                PORTB &= ~0x10;         // LED ON
			}
			else if( rdata == '2')      // ���� 2 �� ���ŵǸ� 
			{

		        HexToDec(TMP_sensor_ouput_C,10);   // ���� �µ� �� TMP_sensor_ouput_C �������� ��ȯ

                SerialPutString( "Temperature = " );   //  �޽��� ����
                SerialPutChar( NumToAsc(cnumber[2]));  //  ���� �µ���, ���� TMP_sensor_ouput_C �� ����
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( '.'); 
                SerialPutChar( NumToAsc(cnumber[0]));  // �Ҽ��� ���� �ڸ� ���� 
                SerialPutChar( 'C'); 
                SerialPutChar('\n');                   // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ���

			} 

			else if( rdata == '3')      // ���� 3 �� ���ŵǸ� 
			{

		        HexToDec( CDS_sensor_ouput, 10);   // ���� ����(CDS)���� �� CDS_sensor_ouput �������� ��ȯ

                SerialPutString( "CDS Value = " );     //  �޽��� ����
                SerialPutChar( NumToAsc(cnumber[3]));  //  ���� ����(CDS)���� ��, ���� CDS_sensor_ouput �� ����
                SerialPutChar( NumToAsc(cnumber[2])); 
                SerialPutChar( NumToAsc(cnumber[1]));   
                SerialPutChar( NumToAsc(cnumber[0]));   
                SerialPutChar('\n');                   // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ���

			} 


			else if( rdata != 0xFF)    //  ��� ���� �̸�
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  ��� ���� �޽��� ����


			}


		    rdata = 0xFF;
            new_recv_flag = 0;      // �� ����(���) ���� �÷��� Reset
  

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



// UART1 ���� ���ͷ�Ʈ ���� ���α׷�

ISR(  USART1_RX_vect )
{

    rdata = UDR1; 
 
    SerialPutChar( rdata);           // Echo  ���ŵ� �����͸� �ٷ� �۽��Ͽ� ���ŵ� �����Ͱ� ��Ȯ���� Ȯ�� 
    SerialPutChar('\n');             // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ���

    recv_cnt++ ;                     // ���ŵ� ������ ����Ʈ�� ����

    new_recv_flag = 1;               // �� ����(���) ���� �÷��� Set

}



///////////////////////////////////////////////////////////////


ISR(TIMER0_OVF_vect)   // Timer0 overflow interrupt( 10 msec)  service routine
{

    static unsigned short  time_index = 0,  count1 = 0, TMP_Sum = 0, CDS_Sum = 0 ; 
    static unsigned short  TMP_sensor_ouput_buf[Avg_Num ], CDS_sensor_ouput_buf[Avg_Num ]   ; 


    unsigned char i = 0 ;

    sei();            // �������ͷ�Ʈ ���( �ٸ����ͷ�Ʈ(Ÿ�̸����ͷ�Ʈ) ����ϰ� ������) 

    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64usec

    time_index++ ; 


    if( time_index == 25 )    // ���ø��ֱ� =  250 msec = 10msec x 25 
    {

       time_index = 0; 


      /**************   CDS Sensor signal detection(AD ��ȯ) ************/

	   ADMUX &= ~0x1F;    //  ADC Chanel 0 : ADC0 ����

	   ADCSRA |= 0x40;   // ADC start 

	   while( ( ADCSRA & 0x10 ) == 0x00  ) ;  // Check if ADC Conversion is completed 

	   CDS_sensor_ouput = ADC;   
 
     /******************************************************/ 


      /**************   Temperature Sensor signal detection(AD ��ȯ) ************/

	   ADMUX &= ~0x1F;    //  ADC Chanel ���� Clear
	   ADMUX |= 0x01;     //  ADC Chanel 1 : ADC 1 ����

	   ADCSRA |= 0x40;    // ADC start 

	   while( ( ADCSRA & 0x10 ) == 0x00  ) ;  // Check if ADC Conversion is completed 

	   TMP_sensor_ouput = ADC;                // ADC Conversion �� �Ϸ�Ǿ����� ADC ��� ���� 
 
     /******************************************************/ 


   ////////////////////////////////////////////////////////////////////
   //////////                                               /////////// 
   //////////  Avg_Num(4��) ���� �̵� ���(Moving Average)  ///////////
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

             CDS_Sum +=  CDS_sensor_ouput  ;	       // ���� �ֱ� �� ���ϰ�  
             CDS_Sum -=  CDS_sensor_ouput_buf[ 0 ] ;   // ���� ������ �� ���� 

             CDS_sensor_ouput_avg = CDS_Sum / Avg_Num ;     // 4�� �̵� ��� 

             for( i = 0; i <= (Avg_Num - 2) ; i++ )
			 {
                 CDS_sensor_ouput_buf[ i ]  = CDS_sensor_ouput_buf[ i+1 ] ;
			 } 

             CDS_sensor_ouput_buf[ Avg_Num - 1 ]  = CDS_sensor_ouput ;  

            ////////////////////////////////////////////////////////////////


             TMP_Sum +=  TMP_sensor_ouput  ;	       // ���� �ֱ� �� ���ϰ�  
             TMP_Sum -=  TMP_sensor_ouput_buf[ 0 ] ;   // ���� ������ �� ���� 

             TMP_sensor_ouput_avg = TMP_Sum / Avg_Num ;     // 4�� �̵� ��� 

             //  �����µ� ��� : ������(������ �̵� = Amp_Gain ) ��������� 
               TMP_sensor_ouput_avg_C =   ( unsigned short) ( (unsigned long) 1250 * TMP_sensor_ouput_avg  / (256 * Amp_Gain)  )  ;    // �µ� ��� [C] ����
               TMP_sensor_ouput_C =   ( unsigned short) ( (unsigned long) 1250 * TMP_sensor_ouput  / (256 * Amp_Gain)  )  ;    // �µ� ��� [C] ����

             // �����µ� ��� : ������ ������� �ʾ�����  
             //  TMP_sensor_ouput_avg_C =   ( unsigned short) ( (unsigned long) 1250 * TMP_sensor_ouput_avg  / 256  )  ;           // �µ� ��� [C] ����
 

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

ISR(INT0_vect)    //  INT0 ���� ���α׷�
{


      sei();            // �������ͷ�Ʈ ���( �ٸ����ͷ�Ʈ(Ÿ�̸����ͷ�Ʈ) ����ϰ� ������) 
      EIMSK = 0x00;     // INT0 ���ͷ�Ʈ ����


     int_num++;           // ����ġ�� �ѹ� ������ ������ ������ Ƚ�� 1 ���� 

     if( int_num == 2 ) int_num = 0 ;

	 if( int_num == 0 )       Sensor_Flag = TMP_Mode ;
	 else if( int_num == 1 )  Sensor_Flag = CDS_Mode ;
     

//////////////////// ä�͸� ���� ///////////////////

	  msec_delay( 20 );
	  while( ~PIND & 0x01 );
	  msec_delay( 20 );

	  EIFR = 0x01;   // �÷��׺�Ʈ ����	

///////////////////////////////////////////////////

      EIMSK = 0x01;     // INT0 ���ͷ�Ʈ ���

 

}


///////////////////////////////////////////////////////////



// UART1 ��� �ʱ�ȭ ���α׷� 

void init_serial(void)
{
    UCSR1A = 0x00;                    //�ʱ�ȭ
    UCSR1B = 0x18  ;                  //�ۼ������,  �ۼ��� ���ͷ�Ʈ ����
    UCSR1C = 0x06;                    //������ ���ۺ�Ʈ �� 8��Ʈ�� ����.
    
    UBRR1H = 0x00;
    UBRR1L = 103;                     //Baud Rate 9600 
}




//======================================
// �� ���ڸ� �۽��Ѵ�.
//======================================

void SerialPutChar(char ch)
{
	while(!(UCSR1A & (1<<UDRE)));			// ���۰� �� ���� ��ٸ�
  	UDR1 = ch;								// ���ۿ� ���ڸ� ����
}


//=============================================
// ���ڿ��� �۽��Ѵ�.
// �Է�   : str - �۽��� ���ڿ��� ������ ������ �ּ�
//=============================================

 void SerialPutString(char *str)
 {

    while(*str != '\0')         // ���ŵ� ���ڰ� Null ����( 0x00 )�� �ƴϸ� 
    {

        SerialPutChar(*str++);
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



