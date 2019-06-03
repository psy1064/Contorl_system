
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "lcd.h"


void init_serial(void) ;  //  Serial �����Ʈ �ʱ�ȭ

void SerialPutChar(char ch);
void SerialPutString(char str[]);


void HexToDec(unsigned short num, unsigned short radix);

char NumToAsc( unsigned char Num );       // ���ڸ� ASCII �ڵ�(����)�� ��ȯ�ϴ� �Լ� 

unsigned char AscToNum( char Asc ) ;      // ����(ASCII �ڵ�)�� ���ڷ� ��ȯ�ϴ� �Լ� 

void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷���

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


	 DDRB |= 0x10 ; 	// LED (PB4 ) :��¼���	

	 PORTB |= 0x10 ;    // LED OFF

     init_serial() ;    // Serial Port (USART1) �ʱ�ȭ

     LcdInit();         // LCD �ʱ�ȭ 


     UCSR1B |=  0x80  ;      // �۽�(RX) �Ϸ� ���ͷ�Ʈ ���
	 sei() ;                 // �������ͷ�Ʈ���

     LcdCommand( ALLCLR ) ;    // LCD Clear
  	 LcdMove(0,0);    
	 LcdPuts("Blutooth Master"); 
 

  
	 while(1)
	 {

            if( new_recv_flag == 1 )      // ���ڿ� ���ſϷ� �� 
			{ 

		        if( Command_Error_Flag == 1 )    // ���� ��ɿ� ������ �־�����
			    {  
			        Command_Error_Flag = 0 ;     // ���� Command_Error_Flag ���� 
                   
					LcdCommand( ALLCLR ) ;       // LCD ȭ�� ���� 

////    �����Ÿ� ���÷���  //////////////

	                LcdMove(0,0); 
	                LcdPuts("Dist_1 = ");
                    Display_Number_LCD( distance_1, 3 );      // �����ļ��� 1 ���� �Ÿ� ���÷��� 
	                LcdPuts("cm");

	                LcdMove(1,0); 
	                LcdPuts("Dist_2 = ");
                    Display_Number_LCD( distance_2, 3 );      // �����ļ��� 2 ���� �Ÿ� ���÷��� 
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

               if     ( eq_count1 == 13 )  cmd_data = 1 ;     // ��� 1
               else if( eq_count2 == 13 )  cmd_data = 2 ;     // ��� 2   
			   else                       cmd_data = 0xFE ;  // ��� ����

               eq_count1 = 0;  eq_count2 = 0;  

               new_recv_flag = 0 ; 

			}


         /////////////////////////////////
        
         ////////  ���(Command) ó�� ( Slave --> Master ) : Slave�� Master���� ������ ������(���) ó��


			if( cmd_data == 1 )     // ��� 1 �̸�
			{

				 for( i = 13 ; i < recv_cnt ; i++)
			     {
					  result *= 10 ; 
			          result += AscToNum( recv_data[i] )  ; 
                 }

                 distance_1  = result  ;
                 result = 0 ; 

//               SerialPutString( "Received Data Count = " );     // �޴������� �޽��� ����
//               SerialPutChar('\n');                  // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ���

             }
           
			else if( cmd_data == 2 )     // ��� 2 �̸�
			{

				 for( i = 13 ; i < recv_cnt ; i++)
			     {
					  result *= 10 ; 
			          result += AscToNum( recv_data[i] )  ; 
                 }

                 distance_2  = result  ;
                 result = 0 ;

             }

            else if( cmd_data == 0xFE )      //  ��� ���� �̸� 
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  ��� ���� �޽��� ����

			    LcdCommand( 0x01) ;                                 // LCD Claear

			    LcdMove(0, 0 );                                     // LCD�� �����޽��� ���÷���
		        LcdPuts("Cmd Error!!"); 
			    LcdMove(1, 0 );
		        LcdPuts("Try Again."); 

				Command_Error_Flag = 1;                              // ��� ���� �÷��� ��

			}


     ///////////////////////////////////////////////////////////////


		   if( Command_Error_Flag == 0  &&  cmd_data != 0xFF  )   // ��ɿ� ������ ���� ����ʱ����(0xFF)�� �ƴϸ�   
		   {  

////    �����Ÿ� ���÷���

	           LcdMove(0,0); 
	           LcdPuts("Dist_1 = ");
               Display_Number_LCD( distance_1, 3 );      // �����ļ��� 1 ���� �Ÿ� ���÷��� 
	           LcdPuts("cm");

	           LcdMove(1,0); 
	           LcdPuts("Dist_2 = ");
               Display_Number_LCD( distance_2, 3 );      // �����ļ��� 2 ���� �Ÿ� ���÷��� 
	           LcdPuts("cm");

////////////////////////////////

           }

     ////////////////////////////////////////////////////////////////


		   cmd_data = 0xFF;                             //  ����� �ʱⰪ���� ����


     ////////////////////////////////////////////////////////////


	       k++ ;

		   if( k == 1 )  /////  ���(Command) ���� ( Master --> Slave ) : Master�� Slave���� ��� ����
		   {

                SerialPutString( "Read USonic Sensor1." );     // Slave �� �޽��� ����
 
		   }

		   else if( k == 4 )   //     
		   {
                SerialPutString( "Read USonic Sensor2." );     // Slave �� �޽��� ����
 
		   }

		   else if( k == 7 )   //   
		   { 
              k = 0; 
 
		   }


		   msec_delay(10) ;


	}   // end of while(1)

}     //  End of main()

////

// UART1 ���� ���ͷ�Ʈ ���� ���α׷� 

ISR(  USART1_RX_vect )
{

    static unsigned char r_cnt = 0 ;

    rdata = UDR1; 

    if( rdata != '.' )                      // ���ŵ� �����Ͱ� ������ ���ڸ� ��Ÿ���� ������(��ħǥ)�� �ƴϸ�
    {
//        SerialPutChar( rdata);               // Echo  ���ŵ� �����͸� �ٷ� �۽��Ͽ� ���ŵ� �����Ͱ� ��Ȯ���� Ȯ�� 
   	    recv_data[r_cnt] = rdata;        //  ���ŵ� ���� ���� 
	    r_cnt++;                         //  ���� ���� ���� ���� 

		new_recv_flag = 0;

    }
    else if(  rdata == '.' )                // ���ŵȵ����Ͱ� ������ ���ڸ� ��Ÿ���� ������(��ħǥ) �̸�
    {
//        SerialPutChar('\n');                // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ��� 
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


// ���ڸ� ASCII �ڵ�(����)�� ��ȯ�ϴ� �Լ�

char NumToAsc( unsigned char Num )
{
	if( Num <10 ) Num += 0x30; 
	else          Num += 0x37; 

	return Num ;
}


 // ����(ASCII �ڵ�)�� ���ڷ� ��ȯ�ϴ� �Լ� 

unsigned char AscToNum( char Asc )
{
	if( Asc <= '9' )       Asc -= 0x30; 
	else if( Asc >= 'A' )  Asc -= 0x37; 

	return Asc ;
}


void msec_delay(int n)
{	
	for(; n>0; n--)		// 1msec �ð� ������ nȸ �ݺ�
		_delay_ms(1);		// 1msec �ð� ����
}



