
#include <avr/io.h>

#include "lcd.h"


void init_serial(void) ;  //  Serial �����Ʈ �ʱ�ȭ

void SerialPutChar(char ch);
void SerialPutString(char str[]);


char Send_Message_1[] = { "Tx Success!" } ;                    

/********************************************************************************************************************
                                      					main
********************************************************************************************************************/
int main(void)
{ 
 
   	 unsigned char rdata=0 ;  	  
 

	 DDRB |= 0x10; 	   // LED (PB4 ) :��¼���	

	 PORTB |= 0x10;    // LED OFF

     init_serial() ;   // Serial Port (USART1) �ʱ�ȭ

     LcdInit();       // LCD �ʱ�ȭ 


     LcdCommand( ALLCLR ) ;    // LCD Clear
  	 LcdMove(0,0);    
	 LcdPuts("Blutooth Module"); 
 
  	 LcdMove(1,0);    
	 LcdPuts("HC-06 Test Prog"); 
  
	 while(1)
	 {
       		if( (UCSR0A & 0x80) != 0)       // 1000 0000    ���� ������ �Ϸ�Ǹ�( UCSR1A�� RXC�� ��Ʈ1�� �Ǹ�) 
        	{                               // : Polling ������� üũ(���ͷ�Ʈ ������)

            	rdata = UDR0;               //  ���ŵ� ���� ���� 
        	}
        	else rdata = rdata;             //  ���ŵ� ���ڰ� ������ ���� ������ ���

 
			if( rdata == '0' )              // ���� 0 �� ���ŵǸ� 
			{
                PORTB |= 0x10;              // LED OFF 

				rdata = 0xFF;
			}
			else if( rdata == '1' )         // ���� 1 �� ���ŵǸ� 
			{
                PORTB &= ~0x10;             // LED ON

				rdata = 0xFF;
			}
			else if( rdata == '2')          // ���� 2 �� ���ŵǸ� 
			{
				SerialPutString("Tx Success!");  // �޴������� �޽��� ����
                SerialPutChar('\n');             // �޴������� ������(����) ���۽� '\n'�� �׻� ���� �����ؾ��� 

				rdata = 0xFF;
			}

 
	}

} 


void init_serial(void)
{
    UCSR0A=0x00;                    //�ʱ�ȭ
    UCSR0B=0x18;                    //�ۼ������,�������ͷ�Ʈ ����
    UCSR0C=0x06;                    //������ ���ۺ�Ʈ �� 8��Ʈ�� ����.
    
    UBRR0H=0x00;
    UBRR0L=103;                     //Baud Rate 9600 
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

    while(*str != '\0')          // ���ŵ� ���ڰ� Null ����( 0x00 )�� �ƴϸ� 
    {
        SerialPutChar(*str++);
    }
}



