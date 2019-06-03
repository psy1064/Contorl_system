#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"

/////////////////
void init_serial(void) ;  //  Serial �����Ʈ �ʱ�ȭ

void SerialPutChar(char ch);
void SerialPutString(char str[]);

//////////////////////


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷���


void msec_delay(int n)  ;   // msec ���� �ð�����
void usec_delay(int n)  ;   // usec ���� �ð�����

unsigned char Time_Delay_Polling( unsigned short d_time ) ;   // �ð����� üũ�Լ�(�������)


static volatile unsigned short    distance_1 = 0, distance_2 = 0,  distance_3 = 0, sensor_count = 0, active_sensor_flag = 0  ;

static volatile unsigned short    distance_1_old = 0, distance_2_old = 0,  distance_3_old = 0 ;

static volatile  unsigned char    Warning_Flag = 0 ;
static volatile  unsigned short   Delay_Time = 0;


////////////////////////////////

static volatile char Cmd_Message_1[] = { "led on" } ;     //  Blutooth Command
static volatile char Cmd_Message_2[] = { "led off" } ;  
static volatile char Cmd_Message_3[] = { "led toggle" } ;  
static volatile char Cmd_Message_4[] = { "send distance 1 data" } ;  
static volatile char Cmd_Message_5[] = { "send distance 2 data" } ;  

static volatile  char  rdata = 0,  recv_cnt = 0, new_recv_flag = 0  ;                
static volatile  char  recv_data[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
//////////////////////////////

static volatile unsigned char   Command_Error_Flag = 0 ; 


int main() 
{   


    char   eq_count1 = 0, eq_count2 = 0, eq_count3 = 0, eq_count4 = 0, eq_count5 = 0, cmd_data = 0xFF  ;  
    unsigned  char   i = 0 ;   		

	LcdInit();      //  LCd �ʱ�ȭ �Լ� 

	LcdMove(0,0); 
	LcdPuts("Dist_1 =    cm");
	LcdMove(1,0); 
	LcdPuts("Dist_2 =    cm");
//	LcdMove(1,0); 
//	LcdPuts("Dist_3 =    cm"); 


///////////////////////////////////////////////

     init_serial() ;    // Serial Port (USART1) �ʱ�ȭ

     UCSR1B |=  0x80  ;      // �۽�(RX) �Ϸ� ���ͷ�Ʈ ���
	 sei() ;                 // �������ͷ�Ʈ���
/////////////////////////////////////////////////



////  3 ���� �����ļ���( Ultrasonic Sensor) ////////////

// �����Ʈ ���� 
	
	DDRB |= 0x07;     // 3 �����ļ��� Trigger signals( PB0, PB1, PB2 : �����Ʈ ����  )
	PORTB &= ~0x07;   // PB0, PB1, PB2  : Low  ( 3 Trigger signals OFF )  

	DDRB |= 0x08;     // ����(Buzzer) ( PB3 : �����Ʈ ����    )
	PORTB &= ~0x08;   // PB3  : Low  ( ���� OFF )  

	DDRB |= 0x10;     // LED ( PB4 : �����Ʈ ����    )
	PORTB |= 0x10;    // PB4  : High ( LED OFF)    


 ////////////  Timer 0 ����  ( 10 msec �ֱ� Ÿ�̸� 0 ���ͷ�Ʈ )  ///////////////
        
    TCCR0 = 0x00;            // Ÿ�̸� 0 ����(���ֺ� = 1024 ) , Normal mode(Ÿ�̸Ӹ��)

    TCNT0 = 256 - 156;       //  ����Ŭ���ֱ� = 1024/ (16x10^6) = 64 usec,  
                             //  �����÷����ͷ�Ʈ �ֱ� = 10msec
                             //  156 = 10msec/ 64usec

    TIMSK = 0x01;            // Ÿ�̸�0 �����÷����ͷ�Ʈ ���
///////////////////////////////////////////////////////////    


// 3 Echo Signal�� Pulse Width measurment,  Timer3 

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // Ÿ�̸� 3 ����(���ֺ� 8) ,  0.5usec ������ ���� 

/////////////////////////////////////////////////////////

 
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
	PORTB &= 0xFE;    // PB0 : Low 
          
	active_sensor_flag = 1; 
    sensor_count = 1;

  /////////////////////////////////////////////


    TCCR0 |= 0x07;    // Ÿ�̸� 0 ����(���ֺ� = 1024 ) 

	 
	while (1) 
	{ 

//////////////////////////////////


            if( new_recv_flag == 1 )      // ���ڿ� ���ſϷ� �� 
			{ 
                 

		        if( Command_Error_Flag == 1 )    // ���� ��ɿ� ������ �־�����
			    {  
			        Command_Error_Flag = 0 ;     // ���� Command_Error_Flag ���� 
                   
					LcdCommand( ALLCLR ) ;       // LCD ȭ�� ���� 

	                LcdMove(0,0); 
	                LcdPuts("Dist_1 =    cm");
	                LcdMove(1,0); 
	                LcdPuts("Dist_2 =    cm");
//	                LcdMove(1,0); 
//	                LcdPuts("Dist_3 =    cm"); 

               }


               for( i=0; i < recv_cnt ; i++) 
			   {
			      if( recv_data[i] == Cmd_Message_1[i] ) eq_count1++ ;
			      if( recv_data[i] == Cmd_Message_2[i] ) eq_count2++ ; 
			      if( recv_data[i] == Cmd_Message_3[i] ) eq_count3++ ;
			      if( recv_data[i] == Cmd_Message_4[i] ) eq_count4++ ;  
			      if( recv_data[i] == Cmd_Message_5[i] ) eq_count5++ ;  
               }

               if     ( eq_count1 == 6 )  cmd_data = 1 ;     // ��� 1
               else if( eq_count2 == 7 )  cmd_data = 2 ;     // ��� 2   
               else if( eq_count3 == 10)  cmd_data = 3 ;     // ��� 3
               else if( eq_count4 == 20 ) cmd_data = 4 ;     // ��� 4 
               else if( eq_count5 == 20 ) cmd_data = 5 ;     // ��� 5
			   else                       cmd_data = 0xFE ;  // ��� ����

               eq_count1 = 0;  eq_count2 = 0;  eq_count3 = 0;  eq_count4 = 0,  eq_count5 = 0; 

               new_recv_flag = 0 ; 
 
			}

          //////////////////////////////////////


         ////////  ���(Command) ó�� 

			if( cmd_data ==  1 )          // ��� 1 �̸�
			{
                PORTB &= ~0x10;           // LED ON
			}
			else if( cmd_data == 2 )      // ��� 2 �̸�
			{
                PORTB |= 0x10;            // LED OFF
			}
			else if( cmd_data == 3 )      // ��� 3 �̸�
			{
                PORTB ^= 0x10;            // LED Toggle 

			}

			else if( cmd_data == 4 )     // ��� 4 �̸�
			{

		        HexToDec( distance_1,10);                        // �����ļ��� 1�� ���� ������ �Ÿ� distance_1 �������� ��ȯ

                SerialPutString( "measured distance 1 = " );     // �޴������� �޽��� ����

                SerialPutChar( NumToAsc(cnumber[2]));    // ���� distance_1 ���� 10���� ��ȯ�� �� �ڸ����� ���ڵ�����(ASCII)�� ��ȯ�� �޴������� ����
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 

                SerialPutString( "cm" );                 // �޴������� �޽���(�Ÿ� ���� cm) ����

                SerialPutChar('\n');                     // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ��� 


			}

			else if( cmd_data == 5 )     // ��� 5 �̸�
			{

		        HexToDec( distance_2,10);                        // �����ļ��� 2�� ���� ������ �Ÿ� distance_2 �������� ��ȯ

                SerialPutString( "measured distance 2 = " );     // �޴������� �޽��� ����

                SerialPutChar( NumToAsc(cnumber[2]));    // ���� distance_2 ���� 10���� ��ȯ�� �� �ڸ����� ���ڵ�����(ASCII)�� ��ȯ�� �޴������� ����
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 

                SerialPutString( "cm" );                 // �޴������� �޽���(�Ÿ� ���� cm) ����

                SerialPutChar('\n');                     // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ��� 


			}

            else if( cmd_data == 0xFE )      //  ��� ���� �̸� 
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  ��� ���� �޽��� ����

			    LcdCommand( 0x01) ;       // LCD Claear

			    LcdMove(0, 0 );           // LCD�� �����޽��� ���÷���
		        LcdPuts("Cmd Error!!"); 
			    LcdMove(1, 0 );
		        LcdPuts("Try Again."); 

				Command_Error_Flag = 1;                             // ��� ���� �÷��� �� 

			}

      ////////////////////////////////////////////////////////////////

		    cmd_data = 0xFF;                             //  ����� �ʱⰪ���� ����

     ///////////////////////////////////////////////////////////////


		   if( Command_Error_Flag == 0  )                // ��ɿ� ������ ������ �Ÿ����� �����͸� LCD�� ���÷��� 
		   {  

	           LcdMove(0, 9); 
               Display_Number_LCD(distance_1, 3);        // �Ÿ����� ������ LCD�� ���÷���

 	           LcdMove(1, 9); 
               Display_Number_LCD(distance_2, 3); 

//             LcdMove(1, 9); 
//             Display_Number_LCD(distance_3); 

           }







      //////////////////////////////////////////////


     }


} 


//////////////////////////////////////////////////////////////////


// UART1 ���� ���ͷ�Ʈ ���� ���α׷� 

ISR(  USART1_RX_vect )
{

    static unsigned char r_cnt = 0 ; 


    rdata = UDR1; 

    if( rdata != '.' )                      // ���ŵ� �����Ͱ� ������ ���ڸ� ��Ÿ���� ������(��ħǥ)�� �ƴϸ�
    {
        SerialPutChar( rdata);               // Echo  ���ŵ� �����͸� �ٷ� �۽��Ͽ� ���ŵ� �����Ͱ� ��Ȯ���� Ȯ�� 
   	    recv_data[r_cnt] = rdata;        //  ���ŵ� ���� ���� 
	    r_cnt++;                         //  ���� ���� ���� ���� 

		new_recv_flag = 0;

    }
    else if(  rdata == '.' )                // ���ŵȵ����Ͱ� ������ ���ڸ� ��Ÿ���� ������(��ħǥ) �̸�
    {
        SerialPutChar('\n');                // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ��� 
        recv_cnt = r_cnt ;                  // ���ŵ� ������ ����Ʈ�� ����
        r_cnt = 0;  
        
		new_recv_flag = 1;

    }


}




void init_serial(void)
{
    UCSR1A=0x00;                    //�ʱ�ȭ
    UCSR1B = 0x18  ;                //�ۼ������,  �ۼ��� ���ͷ�Ʈ ����
    UCSR1C=0x06;                    //������ ���ۺ�Ʈ �� 8��Ʈ�� ����.
    
    UBRR1H=0x00;
    UBRR1L=103;                     //Baud Rate 9600 
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

    while(*str != '\0')
    {

        SerialPutChar(*str++);
    }
}



///////////////////////////////////////////////////////////////



ISR( TIMER0_OVF_vect )    //  10 msec �ֱ� Ÿ�̸�1 �����÷� ���ͷ�Ʈ �������α׷�
{

    static unsigned short  time_index = 0 ; 


    sei();            // �������ͷ�Ʈ ���( �ٸ����ͷ�Ʈ( USART1 ���ͷ�Ʈ) ����ϰ� ������) 


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
       else if ( sensor_count == 3 )   //  �����ļ��� 3 Ʈ���� ��ȣ �߻�(������ 3 �߻�)
	   {
	      PORTB |= 0x04;    // PB2 : High
		  usec_delay(20) ;  // 20usec ���� High ���� 
	      PORTB &= 0xFB;    // PB2 : Low 

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
//			   PORTB ^= 0x10  ;    // PB4(LED) toggle :  LED ON, OFF �ݺ� 
	       }
	   }
       else if( Warning_Flag == 0 )
	   {
           PORTB &= ~0x08  ;    // PB3(����) OFF : ���� OFF 
//		   PORTB |= 0x10  ;     // PB4(LED) OFF :  LED  OFF 
	   }
      
       /////////////////////////////////////  


   }


}



ISR(INT4_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;


    sei();            // �������ͷ�Ʈ ���( �ٸ����ͷ�Ʈ( USART1 ���ͷ�Ʈ) ����ϰ� ������) 

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


    sei();            // �������ͷ�Ʈ ���( �ٸ����ͷ�Ʈ( USART1 ���ͷ�Ʈ) ����ϰ� ������) 

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


    sei();            // �������ͷ�Ʈ ���( �ٸ����ͷ�Ʈ( USART1 ���ͷ�Ʈ) ����ϰ� ������) 

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




