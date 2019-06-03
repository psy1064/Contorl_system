#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num )  ;       // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
void Display_Distance_LCD( unsigned int num )  ;     // �����İ� ������ �Ÿ��� 10���� ���·� LCD �� ���÷��� 

void msec_delay(int n)  ;   // msec ���� �ð�����
void usec_delay(int n)  ;   // usec ���� �ð�����

unsigned char Time_Delay_Polling( unsigned short d_time ) ;   // �ð����� üũ�Լ�(�������)


static volatile unsigned short    distance_1 = 0  ;

static volatile unsigned short    distance_1_old = 0 ;

static volatile  unsigned char    Warning_Flag = 0 ;
static volatile  unsigned short   Delay_Time = 0;



int main() 
{   


	LcdInit();      //  LCd �ʱ�ȭ �Լ� 

	LcdMove(0,0); 
	LcdPuts("UltrasonicSensor"); 
	LcdMove(1,0); 
	LcdPuts("Dist_1 =      cm");


////   �����ļ���( Ultrasonic Sensor) ////////////

// �����Ʈ ���� 
	
	DDRB |= 0x01;     // �����ļ��� Trigger signal( PB0 : �����Ʈ ����  )
	PORTB &= ~0x01;   // PB0  : Low  ( Trigger signal OFF )  

	DDRB |= 0x10;     // ����(Buzzer) ( PB4 : �����Ʈ ����)
	PORTB &= ~0x10;   // PB4  : Low  ( ���� OFF )  

	DDRB |= 0x20;     // LED ( PB5 : �����Ʈ ����)
	PORTB |= 0x20;   // PB5  : High ( LED OFF)    


 ////////////  Timer 0 ����  ( 10 msec �ֱ� Ÿ�̸� 0 ���ͷ�Ʈ )  ///////////////
        
    TCCR0 = 0x00;            // Ÿ�̸� 0 ����(���ֺ� = 1024 ) , Normal mode(Ÿ�̸Ӹ��)

    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64usec

    TIMSK = 0x01;            // Ÿ�̸�0 �����÷����ͷ�Ʈ ���
///////////////////////////////////////////////////////////    


// Echo Signal Pulse Width measurment,  Timer3 

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // Ÿ�̸� 3 ����(���ֺ� 8) ,  0.5usec ������ ���� 

/////////////////////////////////////////////////////////

 
// �����ļ��� Echo Signals : external interrupt 4( pin: INT4 (PE4)),  

	EICRB = 0x01;      // INT4 Both falling edge and rising edge interrupt
	EIMSK |= 0x10;     // INT4 Enable 
	sei(); 

///////////////////////////////////////

   //  ���� �����ļ��� 1 Ʈ���� ��ȣ �߻�(������ 1 �߻�)  
	PORTB |= 0x01;    // PB0 : High
	usec_delay(20) ;  // 20usec ���� High ���� 
	PORTB &= 0xFE;    // PB0 : Low 
          

  /////////////////////////////////////////////


    TCCR0 |= 0x07;    // Ÿ�̸� 0 ����(���ֺ� = 1024 )  

	 
	while (1) 
	{ 

	    LcdMove(1, 9); 
        Display_Distance_LCD( distance_1 ); 


    }


} 



ISR( TIMER0_OVF_vect )    //  10 msec �ֱ� Ÿ�̸�1 �����÷� ���ͷ�Ʈ �������α׷�
{

    static unsigned short  time_index = 0 ; 


    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64usec

    time_index++ ; 


    if( time_index == 5 )    // 50 msec �ֱ� 
    {

       time_index = 0; 

       ////////  �����ļ��� 1 Ʈ���� ��ȣ �߻�(������ 1 �߻�)  /////////

	   PORTB |= 0x01;    // PB0 : High
	   usec_delay(20) ;  // 20usec ���� High ���� 
	   PORTB &= 0xFE;    // PB0 : Low 



       ////////  ����� �߻�   /////////////

/***
        if( distance_1 >  400 ) 
		{
		   Warning_Flag = 0 ;     

		}
		else if(  distance_1 >= 300 )
		{
		   Warning_Flag = 1 ;
		   Delay_Time =  5;    // 0.5�� �ֱ� �ܼ��� 
		}

		else if(  distance_1 >= 200 )
		{
		   Warning_Flag = 1 ;
		   Delay_Time =  3;     // 0.3�� �ֱ� �ܼ��� 
		}

		else if(  distance_1 >= 100 )
		{
		   Warning_Flag = 1 ;
		   Delay_Time =  2;     // 0.2�� �ֱ� �ܼ���
		}

		else if(  distance_1 >= 0 )
		{
		   Warning_Flag = 1 ;
		   Delay_Time =  1;    // 0.1�� �ֱ� �ܼ���  
		}
 
***/
 
       if( distance_1 <=  400 )   Warning_Flag = 1 ;     // ������ �Ÿ��� 40 cm �����̸� ����� �߻� �÷��� set
       else                       Warning_Flag = 0 ;    
		
       Delay_Time =  distance_1 / 100 + 1;            // �Ÿ��� ���ϴ� �ֱ�(= Delay_Time * 50 msec )�� ���� ����� �߻�
        
	   if( Delay_Time <= 1)   Delay_Time = 1 ;   // ������ֱ� ���� : 0.1��
	   if( Delay_Time >= 4)   Delay_Time = 4 ;   // ������ֱ� ���� : 0.


       if( Warning_Flag == 1 )
	   {
           if( Time_Delay_Polling( Delay_Time ) == 1 )     // 50msec * Delay_Time ��� �� 
	       {
               PORTB ^= 0x10  ;    // PB1(����) toggle : ���� �ܼ��� 
			   PORTB ^= 0x20  ;    // PB2(LED) toggle :  LED ON, OFF �ݺ� 
	       }
	   }
       else if( Warning_Flag == 0 )
	   {
           PORTB &= ~0x10  ;    // PB1(����) OFF : ���� OFF 
		   PORTB |= 0x20  ;     // PB2(LED) OFF :  LED  OFF 
	   }
      
       /////////////////////////////////////  

   }


}


ISR(INT4_vect)
{

    static unsigned short count1 = 0, count2 = 0, flag = 0 ;
    unsigned short   del_T = 0, del_T_usec  = 0 ;


	  if(flag == 0) 
	  {
		  count1 = TCNT3; 
		  flag = 1;
	  } 
	  else 
	  { 
		  count2 = TCNT3; 

		  del_T = count2 - count1;

 		  del_T_usec = del_T / 2 ;            // ������ �պ��ð� [ usec ] ���� 
 
      	  distance_1 = (unsigned short)((unsigned long)17 * del_T_usec/100);		// S = 340[m/s[ * T/2[usec]
		  																			// �Ÿ� ��� [ mm ] ����
 
          if( distance_1 > 3800 )              // �ݻ�Ǵ� �����İ� ������� ������ 
		  {
		      distance_1 = distance_1_old ;   // ���� ������ ��� 
		  } 

          distance_1_old = distance_1 ;       // ���� ������ ���� ���� ������Ʈ  

		  flag = 0; 

	  } 


} 



void Display_Number_LCD( unsigned int num )       // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
{

	HexToDec( num, 10); //10������ ��ȯ


	LcdPutchar(NumToAsc(cnumber[4]));    // 10000�ڸ� ���ʷ��� 

	LcdPutchar(NumToAsc(cnumber[3]));    // 1000�ڸ� ���ʷ���

	LcdPutchar(NumToAsc(cnumber[2]));    //  100�ڸ� ���÷���

	LcdPutchar(NumToAsc(cnumber[1]));    //  10�ڸ� ���÷��� 

	LcdPutchar(NumToAsc(cnumber[0]));    //  1�ڸ� ���÷���
}


void Display_Distance_LCD( unsigned int num )       // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
{

	HexToDec( num, 10); //10������ ��ȯ


	LcdPutchar(NumToAsc(cnumber[3]));    // 100�ڸ� ���ʷ��� 

	LcdPutchar(NumToAsc(cnumber[2]));    // 10�ڸ� ���ʷ���

	LcdPutchar(NumToAsc(cnumber[1]));    //  1�ڸ� ���÷���

	LcdPuts("." );                       //  �Ҽ��� ���÷���

	LcdPutchar(NumToAsc(cnumber[0]));    //  �Ҽ� 1�ڸ� ���÷���
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
	for(; n>0; n--)		// 1msec �ð� ������ nȸ �ݺ�
		_delay_ms(1);		// 1msec �ð� ����
}



void usec_delay(int n)
{	
	for(; n>0; n--)		// 1usec �ð� ������ nȸ �ݺ�
		_delay_us(1);		// 1usec �ð� ����
}


//////////////////////////////////////////////////////////

unsigned char Time_Delay_Polling( unsigned short d_time )
{

    static unsigned short  curr_delay = 0; 
	unsigned char  ret_val = 0;


    curr_delay++ ;  

    if( curr_delay >= d_time )   // 50msec * d_time ��� �� 
	{
       ret_val = 1; 
       curr_delay = 0 ;
	} 


    return  ret_val ;


}


