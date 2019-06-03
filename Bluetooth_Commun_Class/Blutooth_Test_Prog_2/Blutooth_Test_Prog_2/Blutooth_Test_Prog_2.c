
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "lcd.h"


void init_serial(void) ;  //  Serial �����Ʈ �ʱ�ȭ

void SerialPutChar(char ch);
void SerialPutString(char str[]);

void HexToDec(unsigned short num, unsigned short radix);
char NumToAsc(char Num);

void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷���

void msec_delay( int n );

static volatile unsigned char cnumber[5] = { 0, 0, 0, 0, 0};


static volatile char Send_Message_1[] = { "Received Data Count = " } ;     

static volatile  char  recv_cnt = 0, rdata=0, new_recv_flag = 0  ;  
               

static volatile unsigned char   Command_Error_Flag = 0 ; 


/********************************************************************************************************************
                                      					main
********************************************************************************************************************/
int main(void)
{ 	  
 

	 DDRB |= 0x10 ; 	// LED (PB4 ) :��¼���	

	 PORTB |= 0x10 ;    // LED OFF

     init_serial() ;    // Serial Port (USART1) �ʱ�ȭ

     LcdInit();         // LCD �ʱ�ȭ 


     UCSR0B |=  0x80  ;      // UART1 �۽�(RX) �Ϸ� ���ͷ�Ʈ ���
	 sei() ;                 // �������ͷ�Ʈ���


     LcdCommand( ALLCLR ) ;    // LCD Clear
  	 LcdMove(0,0);    
	 LcdPuts("Bluetooth Prog"); 
 
  	 LcdMove(1,0);    
	 LcdPuts("Send Command."); 

	  while(1)
	  {

         if( new_recv_flag == 1 )      // 1 ���� ���ſϷ� �� 
		 { 


		    if( Command_Error_Flag == 1 )    // ���� ��ɿ� ������ �־�����
			{  
			      Command_Error_Flag = 0 ;     // ���� Command_Error_Flag ���� 
            }

            /////  ���ŵ� ����Ʈ��(���� ��) LCD ���÷���  /////////////////

            LcdCommand( ALLCLR ) ;    // LCD Clear

		    LcdMove(0,0);    
		    LcdPuts("Recv cnt = "); 
            Display_Number_LCD( recv_cnt, 3 ) ;  // ���ŵ� ����Ʈ�� recv_cnt�� �������� ��ȯ�Ͽ� LCD�� ���÷���


		    LcdMove(1,0);    
		    LcdPuts("Recv data = "); 

		    LcdPutchar( rdata );   // ���ŵ� ���� LCD�� ���÷���

          ////////////////////////////////////////////////////////

		  //////////////  ��ɾ� ó��   //////////////

			if( rdata == '0' )          // ���� 0 �� ���ŵǸ� 
			{
                PORTB |= 0x10 ;          // LED OFF 
			}
			else if( rdata == '1' )     // ���� 1 �� ���ŵǸ�
			{
                PORTB &= ~0x10 ;         // LED ON
			}
			else if( rdata == '2')      // ���� 2 �� ���ŵǸ�
			{

		        HexToDec(recv_cnt,10);   // ���ŵ� ����Ʈ�� recv_cnt �������� ��ȯ

                SerialPutString( "Received Data Count = " );     //  �޽��� ���� 
//              SerialPutString( Send_Message_1 );               //  �޽��� ���� 

                SerialPutChar( NumToAsc(cnumber[2]));            //  ���� recv_cnt �� ����
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 
                SerialPutChar('\n');                    // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ���
 
			} 

			else if( rdata != 0xFF)    //  ��� ���� �̸�
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  ��� ���� �޽��� ����

			    LcdCommand( 0x01) ;       // LCD Claear

			    LcdMove(0, 0 );           // LCD�� �����޽��� ���÷���
		        LcdPuts("Cmd Error!!"); 
			    LcdMove(1, 0 );
		        LcdPuts("Try Again."); 


				Command_Error_Flag = 1;                             // ��� ���� �÷��� ��

			}


     ///////////////////////////////////////////////////////////////

		   if( Command_Error_Flag == 0  )          // ��ɿ� ������ ������  
		   {  

	           LcdMove(0, 11); 
               Display_Number_LCD( recv_cnt, 3 ) ;  // ���ŵ� ����Ʈ�� recv_cnt�� �������� ��ȯ�Ͽ� LCD�� ���÷���

		       LcdMove(1,12);    
		       LcdPutchar( rdata );                 // ���ŵ� ���� rdata�� LCD�� ���÷���

           }

      ////////////////////////////////////////////////////////////////


		   rdata = 0xFF;                           // ���ŵ� ����� �ʱⰪ���� ����
           new_recv_flag = 0;                      // �� ����(���) ���� �÷��� Reset
  

        }
    //////////////////////////////////////////////////

 
	}   // end of while(1)

}     //  End of main()



// UART1 ���� ���ͷ�Ʈ ���� ���α׷�

ISR(  USART0_RX_vect )
{

    rdata = UDR0; 
 
    SerialPutChar( rdata);           // Echo  ���ŵ� �����͸� �ٷ� �۽��Ͽ� ���ŵ� �����Ͱ� ��Ȯ���� Ȯ�� 
    SerialPutChar('\n');             // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ���

    recv_cnt++ ;                     // ���ŵ� ������ ����Ʈ�� ����

    new_recv_flag = 1;               // �� ����(���) ���� �÷��� Set

}



// UART1 ��� �ʱ�ȭ ���α׷� 

void init_serial(void)
{
    UCSR0A = 0x00;                    //�ʱ�ȭ
    UCSR0B = 0x18  ;                  //�ۼ������,  �ۼ��� ���ͷ�Ʈ ����
    UCSR0C = 0x06;                    //������ ���ۺ�Ʈ �� 8��Ʈ�� ����.
    
    UBRR0H = 0x00;
    UBRR0L = 103;                     //Baud Rate 9600 
}




//======================================
// �� ���ڸ� �۽��Ѵ�.
//======================================

void SerialPutChar(char ch)
{
	while(!(UCSR0A & (1<<UDRE)));			// ���۰� �� ���� ��ٸ�
  	UDR0 = ch;								// ���ۿ� ���ڸ� ����
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



