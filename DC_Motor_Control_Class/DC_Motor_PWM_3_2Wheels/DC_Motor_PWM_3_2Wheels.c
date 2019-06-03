#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 


void msec_delay(int n)  ;   // msec ���� �ð�����
void usec_delay(int n)  ;   // usec ���� �ð�����

unsigned char Time_Delay_Polling( unsigned short d_time ) ;   // �ð����� üũ�Լ�(�������)

////////////////////////////////////////

void DC_Motor_Run_Fwd_L( short duty );    // ���� DC ���� ��ȸ��(PWM����) �Լ� 
void DC_Motor_Run_Rev_L( short duty );    // ���� DC ���� ��ȸ��(PWM����) �Լ�  
void DC_Motor_Stop_L( void );             // ���� DC ���� ���� �Լ�  
 
void DC_Motor_Run_Fwd_R( short duty );    // ������ DC ���� ��ȸ��(PWM����) �Լ� 
void DC_Motor_Run_Rev_R( short duty );    // ������ DC ���� ��ȸ��(PWM����) �Լ�  
void DC_Motor_Stop_R( void );             // ������ DC ���� ���� �Լ�  

void DC_Motor_PWM_L( short Vref );        // ���� DC ���� PWM ��ȣ �߻� �Լ�  
                                          // ����ũ(Vref>0), ����ũ(Vref<0), ����ũ(Vref=0) ��� ���� 
void DC_Motor_PWM_R( short Vref );        // ������ DC ���� PWM ��ȣ �߻� �Լ�  
                                          // ����ũ(Vref>0), ����ũ(Vref<0), ����ũ(Vref=0) ��� ���� 

static volatile short  Vmax = 0, Vmax_L = 0, Vmax_R = 0  ; 

//////////////////////////////////////


static volatile unsigned short    distance_1 = 0, distance_2 = 0,  distance_3 = 0, sensor_count = 0, active_sensor_flag = 0  ;

static volatile unsigned short    distance_1_old = 0, distance_2_old = 0,  distance_3_old = 0 ;

static volatile  unsigned char    Warning_Flag = 0 ;
static volatile  unsigned short   Delay_Time = 0;


int main() 
{   
	short duty_L = 0, duty_R = 0; 


////  3 ���� �����ļ���( Ultrasonic Sensor) ////////////

// �����Ʈ ���� 
	
	DDRB |= 0x07;     // 3 �����ļ��� Trigger signals( PB0, PB1, PB2 : �����Ʈ ����  )
	PORTB &= ~0x07;   // PB0, PB1, PB2  : Low  ( 3 Trigger signals OFF )  

	DDRB |= 0x08;     // ����(Buzzer) ( PB3 : �����Ʈ ����    )
	PORTB &= ~0x08;   // PB3  : Low  ( ���� OFF )  

	DDRB |= 0x10;     // LED ( PB4 : �����Ʈ ����    )
	PORTB |= 0x10;    // PB4  : High ( LED OFF)    



	DDRB |= 0x20;   // ���� ���ͱ�����ȣ + ����:  PWM ��Ʈ( pin: OC1A(PB5) )   --> ��� ���� 
	DDRA |= 0x01;   // ���� ���ͱ�����ȣ - ���� : ���� ��/�����Ʈ(pin : PA0 ) --> ��� ���� 

	DDRB |= 0x40;   // ������ ���ͱ�����ȣ + ����:  PWM ��Ʈ( pin: OC1A(PB6) )   --> ��� ���� 
	DDRA |= 0x02;   // ������ ���ͱ�����ȣ - ���� : ���� ��/�����Ʈ(pin : PA1 ) --> ��� ���� 
	 

///////////////////////

	LcdInit();

	LcdMove(0,0); 
	LcdPuts("DC Motor Control");

	LcdMove(1,0); 
	LcdPuts("UltrasonicSensor");

    msec_delay(2000) ;   // 2�ʰ� ���÷��� 

    LcdCommand( ALLCLR ) ;   // LCD ȭ�� ���� 
	LcdMove(0,0); 
	LcdPuts("Speed = ");

	LcdMove(1,0); 
	LcdPuts("Dist_1 =    cm");

////////////////////////////////
	

/////////////    Timer1 ���� :  Fast PWM Mode    ////////////////////////

// ����, ������ ���� ���� PWM ��ȣ ( pin: OC1A(PB5)), OC1B(PB6) ),   Timer1,  PWM signal (period= 200 usec )

	TCCR1A = 0xA2;    // OC1A(PB5)), OC1B(PB6) :  PWM ��Ʈ ����,   Fast PWM ( mode 14 )
	TCCR1B = 0x1B;    // 64 ���� Ÿ�̸� 1 ���� (����Ŭ�� �ֱ� =  64/(16*10^6) = 4 usec ),  Fast PWM ( mode 14 ) 
	ICR1 = 50;        // PWM �ֱ� = 50 * 4 usec = 200 usec

    Vmax_L = ICR1; 
    Vmax_R = ICR1; 

    Vmax =  ICR1 ;

//////////////////////////////////////////////////////////////////
    duty_L = 0;
	OCR1A = duty_L;      //  ���� ���� ����: : OC1A(PB5) PWM duty = 0 ���� 
	
    duty_R = 0;
	OCR1B = duty_R;      //  ������ ���� ����: : OC1B(PB6) PWM duty = 0 ���� 	
	 
//////////////////////////////////////////////////////////////////

    duty_L = 50;         // ���� ���� �ӵ� ����,   �ִ� = Vmax = 50,  �ּ� = 0 

    duty_R = 50;         // ������ ���� �ӵ� ����, �ִ� = Vmax = 50,  �ּ� = 0 
///////////////////////////////////////////////////////////////////////////

 ////////////  Timer 0 ����  ( 10 msec �ֱ� Ÿ�̸� 0 ���ͷ�Ʈ )  ///////////////
        
    TCCR0 = 0x00;            // Ÿ�̸� 0 ����(���ֺ� = 1024 ) , Normal mode(Ÿ�̸Ӹ��)

    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64usec

    TIMSK = 0x01;            // Ÿ�̸�0 �����÷����ͷ�Ʈ ���
///////////////////////////////////////////////////////////    


////////////   Timer3  ���� ( 3 Echo Signals Pulse Width measurment )  /////////////

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // Ÿ�̸� 3 ����(���ֺ� 8) ,  0.5usec ������ ���� 

///////////////////////////////////////////////////////////////////////////////////

 
// 3 �����ļ��� Echo Signals : external interrupt 4( pin: INT4 (PE4)),  external interrupt 5( pin: INT5 (PE5)) 
//                           : external interrupt 6( pin: INT4 (PE6)) 

	EICRB |= 0x15;    // Both falling edge and rising edge interrupt
	EICRB &= ~0x2A;   // Both falling edge and rising edge interrupt

	EIMSK |= 0x70;    // INT4 Enable, INT5 Enable, INT6 Enable
	sei(); 

///////////////////////////////////////

   //  ���� �����ļ��� 1 Ʈ���� ��ȣ �߻�(������ 1 �߻�)  
	PORTB |= 0x01;    // PB0 : High
	usec_delay(20) ;  // 20usec ���� High ���� 
	PORTB &= ~0x01;   // PB0 : Low 
          
	active_sensor_flag = 1; 
    sensor_count = 1;

  /////////////////////////////////////////////


    TCCR0 |= 0x07;    // Ÿ�̸� 0 ����(���ֺ� = 1024 ) 

	 
	while (1) 
	{ 

	    LcdMove(0, 8); 
        Display_Number_LCD( duty_L, 2 );          // ���� ���� �ӵ� ���÷��� 

	    LcdMove(1,9); 
        Display_Number_LCD( distance_1, 3 );      // �����ļ��� 1 ���� �Ÿ� ���÷��� 

		DC_Motor_PWM_L( duty_L );      // ���� DC Motor ��ȸ��  
		DC_Motor_PWM_R( -duty_R );     // ������ DC Motor ��ȸ��  
        msec_delay( 5000 );          // 5�ʰ� ȸ��
 
		DC_Motor_PWM_L( 0 );         // ���� DC Motor ����  
		DC_Motor_PWM_R( 0 );         // ������ DC Motor ����  
        msec_delay( 2000 );          // 2�ʰ� ���� 

		DC_Motor_PWM_L( -duty_L );     // ���� DC Motor ��ȸ��  
		DC_Motor_PWM_R( duty_R );      // ������ DC Motor ��ȸ��  
        msec_delay( 5000 );          // 5�ʰ� ȸ��
 
		DC_Motor_PWM_L( 0 );         // ���� DC Motor ����  
		DC_Motor_PWM_R( 0 );         // ������ DC Motor ����  
        msec_delay( 2000 );          // 2�ʰ� ���� 


        msec_delay( 100 ); 

     }


} 

///////////////////////////////////////////////////////////////////////



void DC_Motor_Run_Fwd_L( short duty )   // DC ���� ��ȸ�� �Լ� 
{

    if( duty > Vmax_L )     duty = Vmax_L ;

    PORTA &= ~0x01;     //  ���� ���� ������ȣ - ���� : 0 V �ΰ�( PA0 = 0 );  
	OCR1A = duty;       //  ���� ���� ������ȣ + ���� : OC1A(PB5) PWM duty ���� 

}

void DC_Motor_Run_Rev_L( short duty )   // DC ���� ��ȸ�� �Լ� 
{

    if( duty > Vmax_L )     duty = Vmax_L ;

    PORTA |= 0x01;              //  ���� ���� ������ȣ - ���� : 5 V �ΰ�( PA0 = 1 );  
	OCR1A = Vmax_L - duty;      //  ���� ���� ������ȣ + ���� : OC1A(PB5) PWM duty ���� 

}


void DC_Motor_Stop_L( void )   // ���� DC ���� ���� �Լ� 
{

    PORTA &= ~0x01;     //  ���� ���� ������ȣ - ���� : 0 V �ΰ�( PA0 = 0 );  
	OCR1A = 0;          //  ���� ���� ������ȣ + ���� : OC1A(PB5) PWM duty = 0 ���� 

}



void DC_Motor_Run_Fwd_R( short duty )   // ������ DC ���� ��ȸ�� �Լ� 
{

    if( duty > Vmax_R )     duty = Vmax_R ;

    PORTA &= ~0x02;     //  ������ ���� ������ȣ - ���� : 0 V �ΰ�( PA1 = 0 );  
	OCR1B = duty;       //  ������ ���� ������ȣ + ���� : OC1B(PB6) PWM duty ���� 

}

void DC_Motor_Run_Rev_R( short duty )   // ������ DC ���� ��ȸ�� �Լ� 
{

    if( duty > Vmax_R )     duty = Vmax_R ;

    PORTA |= 0x02;                //  ������ ���� ������ȣ - ���� : 5 V �ΰ�( PA1 = 1 );  
	OCR1B = Vmax_R - duty ;       //  ������ ���� ������ȣ + ���� : OC1B(PB6) PWM duty ���� 

}


void DC_Motor_Stop_R( void )   // ������ DC ���� ���� �Լ� 
{

    PORTA &= ~0x02;     //  ������ ���� ������ȣ - ���� : 0 V �ΰ�( PA1 = 0 );  
	OCR1B = 0;          //  ������ ���� ������ȣ + ���� : OC1B(PB6) PWM duty = 0 ���� 

}


void DC_Motor_PWM_L( short Vref )   // ���� DC ���� PWM ��ȣ �߻� �Լ�  
{

   if ( Vref > Vmax_L )       Vref = Vmax_L ;
   else if( Vref < -Vmax_L )  Vref = -Vmax_L ;

   if( Vref > 0 )  
   {
      DC_Motor_Run_Fwd_L( Vref ) ;
   }
   else if( Vref == 0 )  
   {
      DC_Motor_Stop_L() ;
   }
   else if( Vref < 0 )  
   {
      DC_Motor_Run_Rev_L( -Vref ) ;
   }

}


void DC_Motor_PWM_R( short Vref )   // ������ DC ���� PWM ��ȣ �߻� �Լ�  
{

   if ( Vref > Vmax_R )       Vref = Vmax_R ;
   else if( Vref < -Vmax_R )  Vref = -Vmax_R ;

   if( Vref > 0 )  
   {
      DC_Motor_Run_Fwd_R( Vref ) ;
   }
   else if( Vref == 0 )  
   {
      DC_Motor_Stop_R() ;
   }
   else if( Vref < 0 )  
   {
      DC_Motor_Run_Rev_R( -Vref ) ;
   }


}

//////////////////////////////////////////////////////////////////////////////


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
	       
	   if( sensor_count == 4 )  sensor_count = 1; 


       if ( sensor_count == 1 )        //  �����ļ��� 1 Ʈ���� ��ȣ �߻�(������ 1 �߻�) 
	   {
	      PORTB |= 0x01;    // PB0 : High
		  usec_delay(20) ;  // 20usec ���� High ���� 
	      PORTB &= ~0x01;   // PB0 : Low 
          
		  active_sensor_flag = 1;

	   }
       else if ( sensor_count == 2 )   //  �����ļ��� 2 Ʈ���� ��ȣ �߻�(������ 2 �߻�)
	   {
	      PORTB |= 0x02;    // PB1 : High
	 	  usec_delay(20) ;  // 20usec ���� High ���� 
	      PORTB &= ~0x02;   // PB1 : Low 

		  active_sensor_flag = 2;

	   }
       else if ( sensor_count == 3 )   //  �����ļ��� 3 Ʈ���� ��ȣ �߻�(������ 3 �߻�)
	   {
	      PORTB |= 0x04;    // PB2 : High
		  usec_delay(20) ;  // 20usec ���� High ���� 
	      PORTB &= ~0x04;   // PB2 : Low 

		  active_sensor_flag = 3;

	   }



       ////////  ����� �߻�   /////////////

 
       if( distance_1 <=  40 )    Warning_Flag = 1 ;     // ������ �Ÿ��� 40 cm �����̸� ����� �߻� �÷��� set
       else                       Warning_Flag = 0 ;    
		
       Delay_Time =  distance_1 / 10 + 1;            // �Ÿ��� ���ϴ� �ֱ�(= Delay_Time * 50 msec )�� ���� ����� �߻�
        
	   if( Delay_Time <= 1)   Delay_Time = 1 ;   // ������ֱ� ���� : 0.1��
	   if( Delay_Time >= 4)   Delay_Time = 4 ;   // ������ֱ� ���� : 0.4�� 
 

       if( Warning_Flag == 1 )
	   {
           if( Time_Delay_Polling( Delay_Time ) == 1 )     // 50msec * Delay_Time ��� �� 
	       {
               PORTB ^= 0x08  ;    // PB3(����) toggle : ���� �ܼ��� 
			   PORTB ^= 0x10  ;    // PB4(LED) toggle :  LED ON, OFF �ݺ� 
	       }
	   }
       else if( Warning_Flag == 0 )
	   {
           PORTB &= ~0x08  ;    // PB3(����) OFF : ���� OFF 
		   PORTB |= 0x10  ;     // PB4(LED) OFF :  LED  OFF 
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



ISR(INT6_vect)
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




