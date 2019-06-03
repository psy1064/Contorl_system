#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num )  ;     // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 


void msec_delay(int n)  ;   // msec ���� �ð�����
void usec_delay(int n)  ;   // usec ���� �ð�����

unsigned char Time_Delay_Polling( unsigned short d_time ) ;   // �ð����� üũ�Լ�(�������)


static volatile unsigned short    distance_1 = 0, distance_2 = 0,  distance_3 = 0, sensor_count = 0, active_sensor_flag = 0  ;
static volatile unsigned short    distance_1_old = 0, distance_2_old = 0,  distance_3_old = 0 ;

static volatile  unsigned char    Warning_Flag = 0;
static volatile  unsigned short   Delay_Time = 0, Delay_Time1 = 0, Delay_Time2 = 0;


int main() 
{   


	LcdInit();      //  LCd �ʱ�ȭ �Լ� 

	LcdMove(0,0); 
	LcdPuts("Dist_1 =    cm");
	LcdMove(1,0); 
	LcdPuts("Dist_2 =    cm");
//	LcdMove(1,0); 
//	LcdPuts("Dist_3 =    cm"); 


////  3 ���� �����ļ���( Ultrasonic Sensor) ////////////

// �����Ʈ ���� 
	
	DDRB |= 0x07;     // 3 �����ļ��� Trigger signals( PB0, PB1, PB2 : �����Ʈ ����  )
	PORTB &= ~0x07;   // PB0, PB1, PB2  : Low  ( 3 Trigger signals OFF )  

	DDRB |= 0x10;     // ����(Buzzer) ( PB4 : �����Ʈ ����)
	PORTB &= ~0x10;   // PB4  : Low  ( ���� OFF )  

	DDRB |= 0x20;     // LED(PB5 : �����Ʈ ����)
	PORTB |= 0x20;    // PB5 : High(LED OFF)    


////////////  Timer 0 ����  ( 10 msec �ֱ� Ÿ�̸� 0 ���ͷ�Ʈ )  ///////////////
        
    TCCR0 = 0x00;            // Ÿ�̸� 0 ����(���ֺ� = 1024 ) , Normal mode(Ÿ�̸Ӹ��)

    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64usec

    TIMSK = 0x01;            // Ÿ�̸�0 �����÷����ͷ�Ʈ ���
///////////////////////////////////////////////////////////    


// 3 Echo Signal Pulse Width measurment,  Timer3 

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // Ÿ�̸� 3 ����(���ֺ� 8) ,  0.5usec ������ ���� 

/////////////////////////////////////////////////////////

 
// 3 �����ļ��� Echo Signals : external interrupt 4( pin: INT4 (PE4)),  external interrupt 5( pin: INT5 (PE5)) 
//                           : external interrupt 6( pin: INT4 (PE6)) 
	
	DDRE &= ~0x70;	  // PE4(INT4), PE5(INT5), PE6(INT6) set input

	EICRB |= 0x15;    // Both falling edge and rising edge interrupt
	EICRB &= ~0x2A;   // Both falling edge and rising edge interrupt

	EIMSK |= 0x70;    // INT4 Enable, INT5 Enable, INT6 Enable

	sei(); 

///////////////////////////////////////

   //  ���� �����ļ��� 1 Ʈ���� ��ȣ �߻�(������ 1 �߻�)  
	PORTB |= 0x01;    // PB0 : High
	usec_delay(20) ;  // 20usec ���� High ���� 
	PORTB &= 0xFE;    // PB0 : Low 
          
	active_sensor_flag = 1; 
    sensor_count = 1;

  /////////////////////////////////////////////


    TCCR0 |= 0x07;    // Ÿ�̸� 0 ����(���ֺ� = 1024 ) 

	 
	while (1) 
	{ 
	    LcdMove(0, 9); 
        Display_Number_LCD(distance_1); 
	
 	    LcdMove(1, 9); 
        Display_Number_LCD(distance_2); 

//      LcdMove(1, 9); 
//      Display_Number_LCD(distance_3); 

     }
} 

ISR( TIMER0_OVF_vect )    //  10 msec �ֱ� Ÿ�̸�1 �����÷� ���ͷ�Ʈ �������α׷�
{

    static unsigned short  time_index = 0 ; 


    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64usec


    time_index++ ; 


    if( time_index == 5 )   // 50 msec �ֱ� 
    {

       time_index = 0; 

       sensor_count++;          // ������ ���� ī���� �� ���� 
	       
	   if( sensor_count == 3 )  sensor_count = 1; 


       if ( sensor_count == 1 )        //  �����ļ��� 1 Ʈ���� ��ȣ �߻�(������ 1 �߻�) 
	   {
	      PORTB |= 0x01;    // PB0 : High
		  usec_delay(20) ;  // 20usec ���� High ���� 
	      PORTB &= 0xFE;    // PB0 : Low 
          
		  active_sensor_flag = 1;

	   }
       else if ( sensor_count == 2 )   //  �����ļ��� 2 Ʈ���� ��ȣ �߻�(������ 2 �߻�)
	   {
	      PORTB |= 0x02;    // PB1 : High
	 	  usec_delay(20) ;  // 20usec ���� High ���� 
	      PORTB &= 0xFD;    // PB1 : Low 

		  active_sensor_flag = 2;

	   }
       /*else if ( sensor_count == 3 )   //  �����ļ��� 3 Ʈ���� ��ȣ �߻�(������ 3 �߻�)
	   {
	      PORTB |= 0x04;    // PB2 : High
		  usec_delay(20) ;  // 20usec ���� High ���� 
	      PORTB &= 0xFB;    // PB2 : Low 

		  active_sensor_flag = 3;

	   }*/



       ////////  ����� �߻�   /////////////

 
       if( distance_1 <=  40 || distance_2 <= 40)    Warning_Flag = 1 ;     // ������ �Ÿ��� 40 cm �����̸� ����� �߻� �÷��� set
       else     					                 Warning_Flag = 0 ;    
		
       Delay_Time1 =  distance_1 / 10 + 1;            // �Ÿ��� ���ϴ� �ֱ�(= Delay_Time * 50 msec )�� ���� ����� �߻�
	   Delay_Time2 =  distance_2 / 10 + 1;            
       
	    
	   if( Delay_Time1 >= Delay_Time2)   Delay_Time = Delay_Time2 ; 
	   else								 Delay_Time = Delay_Time1 ;

	   if( Delay_Time <= 1) Delay_Time = 1;
	   if( Delay_Time >= 4) Delay_Time = 4;
 

       if( Warning_Flag == 1 )
	   {
           if( Time_Delay_Polling( Delay_Time ) == 1 )     // 50msec * Delay_Time ��� �� 
	       {
               PORTB ^= 0x10  ;    // PB4(����) toggle : ���� �ܼ��� 
			   PORTB ^= 0x20  ;    // PB5(LED) toggle :  LED ON, OFF �ݺ� 
	       }
	   }
       else if( Warning_Flag == 0 )
	   {
           PORTB &= ~0x10  ;    // PB4(����) OFF : ���� OFF 
		   PORTB |= 0x20  ;     // PB5(LED) OFF :  LED  OFF 
	   }
	         
       /////////////////////////////////////  

   }

}

ISR(INT4_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;

    if ( active_sensor_flag == 1 )
 	{

	   if(flag == 0) 
	   {
		  count1 = TCNT3; 
		  flag = 1;
	  } 
	  else 
	  { 
		  count2 = TCNT3; 
		  del_T = count2 - count1;

    	  distance_1 = del_T/(2*58); 

          if( distance_1 > 380 )  // �ݻ�Ǵ� �����İ� ������� ������ 
		  {
		      distance_1 = distance_1_old ;   // ���� ������ ��� 
		  } 

          distance_1_old = distance_1 ;    // ���� ������ ���� ���� ������Ʈ  

		  flag = 0; 

	 	  active_sensor_flag = 0;
	  } 

    }

} 


ISR(INT5_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;


    if ( active_sensor_flag == 2 )
	{

	   if(flag == 0) 
	   {
		  count1 = TCNT3; 
		  flag = 1;
	  } 
	  else 
	  { 
		  count2 = TCNT3; 
		  del_T = count2 - count1;
    	  distance_2 = del_T/(2*58); 

          if( distance_2 > 380 )  // �ݻ�Ǵ� �����İ� ������� ������ 
		  {
		      distance_2 = distance_2_old ;   // ���� ������ ��� 
		  } 

          distance_2_old = distance_2 ;    // ���� ������ ���� ���� ������Ʈ  

		  flag = 0; 

	 	  active_sensor_flag = 0;
	  } 

    }

} 



/*ISR(INT6_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;

    if ( active_sensor_flag == 3 )
	{

	   if(flag == 0) 
	   {
		  count1 = TCNT3; 
		  flag = 1;
	  } 
	  else 
	  { 
		  count2 = TCNT3; 
		  del_T = count2 - count1;
    	  distance_3 = del_T/(2*58); 

          if( distance_3 > 380 )  // �ݻ�Ǵ� �����İ� ������� ������ 
		  {
		      distance_3 = distance_3_old ;   // ���� ������ ��� 
		  } 

          distance_3_old = distance_3 ;    // ���� ������ ���� ���� ������Ʈ  

		  flag = 0; 

	 	  active_sensor_flag = 0;
	  } 

    }

} 
*/

void Display_Number_LCD( unsigned int num )       // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
{

	HexToDec( num, 10); //10������ ��ȯ


	LcdPutchar(NumToAsc(cnumber[2]));    // 100�ڸ� ���ʷ��� 

	LcdPutchar(NumToAsc(cnumber[1]));    // 10�ڸ� ���ʷ���

	LcdPutchar(NumToAsc(cnumber[0]));    //  1�ڸ� ���÷���

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

