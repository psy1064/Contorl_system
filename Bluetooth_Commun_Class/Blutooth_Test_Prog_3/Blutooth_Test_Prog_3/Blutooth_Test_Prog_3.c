
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

static volatile char Cmd_Message_1[] = { "led on" } ;     //  Blutooth Command
static volatile char Cmd_Message_2[] = { "led off" } ;  
static volatile char Cmd_Message_3[] = { "led toggle" } ;  
static volatile char Cmd_Message_4[] = { "send data" } ; 
static volatile char Cmd_Message_5[] = { "buzzer on" } ; 
static volatile char Cmd_Message_6[] = { "buzzer off" };

static volatile  char  rdata = 0,  recv_cnt = 0, new_recv_flag = 0  ;                
static volatile  char  recv_data[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  


static volatile unsigned char   Command_Error_Flag = 0 ; 


/********************************************************************************************************************
                                      					main
********************************************************************************************************************/
int main(void)
{ 
	char    eq_count1=0, eq_count2=0, cmd_data = 0xFF  ;  	  
    unsigned char   i=0 ;
	unsigned short duty = 0;

    init_serial() ;    // Serial Port (USART1) �ʱ�ȭ

    LcdInit();         // LCD �ʱ�ȭ 

    UCSR0B |=  0x80  ;      // �۽�(RX) �Ϸ� ���ͷ�Ʈ ���
	sei() ;                 // �������ͷ�Ʈ���

	// ���ͱ�����ȣ ( pin: OC1A(PB5) ),   Timer1, PWM signal (period= 200 usec )

	TCCR1A = 0x82;    // OC1A(PB5)) :  PWM ��Ʈ ����,   Fast PWM ( mode 14 )
	TCCR1B = 0x1b;    // 64 ���� Ÿ�̸� 1 ���� (����Ŭ�� �ֱ� =  64/(16*10^6) = 4 usec ),  Fast PWM ( mode 14 ) 
	ICR1 = 50;        // PWM �ֱ� = 50 * 4 usec = 200 usec (  PWM ���ļ� = 1/200usec = 5 kHz )

    Vmax = ICR1; 

	OCR1A = duty;      //  OC1A(PB5) PWM duty = 0 ���� : ���� ����

	////////////////////////////////////////////////////////////////////////////////////////////////////////

    LcdCommand( ALLCLR ) ;    // LCD Clear
    LcdMove(0,0);    
	LcdPuts("Bluetooth Prog"); 
 
  	LcdMove(1,0);    
	LcdPuts("Send Command."); 

  
	 while(1)
	 {

            if( new_recv_flag == 1 )      // ���ڿ� ���ſϷ� �� 
			{ 

		        if( Command_Error_Flag == 1 )    // ���� ��ɿ� ������ �־�����
			    {  
			        Command_Error_Flag = 0 ;     // ���� Command_Error_Flag ���� 
                   
					LcdCommand( ALLCLR ) ;       // LCD ȭ�� ���� 

	                LcdMove(0,0); 
	                LcdPuts("Bluetooth Prog"); 

		            LcdMove(1,0);    
		            LcdPuts("Recv cnt = "); 

               }
                 
               for( i=0; i < recv_cnt ; i++) 
			   {
			      if( recv_data[i] == Cmd_Message_1[i] ) eq_count1++ ;
			      if( recv_data[i] == Cmd_Message_2[i] ) eq_count2++ ;

               }

               if     ( eq_count1 == 15 )  cmd_data = 1 ;     // ��� 1
               else if( eq_count2 == 13 )  cmd_data = 2 ;     // ��� 2   
			   else                       cmd_data = 0xFE ;  // ��� ����

               eq_count1 = 0;  eq_count2 = 0;

               new_recv_flag = 0 ; 

			}
     
        
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

		        HexToDec(recv_cnt,10);   // ���ŵ� ����Ʈ�� recv_cnt �������� ��ȯ

                SerialPutString( "Received Data Count = " );     // �޴������� �޽��� ����
//              SerialPutString( Send_Message_1 );               // �޴������� �޽��� ����

                SerialPutChar( NumToAsc(cnumber[2]));  //  ���� recv_cnt ���� �޴������� ����
                SerialPutChar( NumToAsc(cnumber[1])); 
                SerialPutChar( NumToAsc(cnumber[0])); 

                SerialPutChar('\n');                  // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ���

			}
			else if( cmd_data == 5 )      // ��� 5 �̸�
			{
                PORTB |= 0x40;            // Buzzer on
			}

            else if( cmd_data == 6 )      // ��� 6 �̸�
			{
                PORTB &= ~0x40;            // LED Toggle
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

	           LcdMove(1, 11); 
               Display_Number_LCD( recv_cnt, 3 ) ;    // ���ŵ� ����Ʈ�� recv_cnt�� �������� ��ȯ�Ͽ� LCD�� ���÷���

           }

     ////////////////////////////////////////////////////////////////


		   cmd_data = 0xFF;                             //  ����� �ʱⰪ���� ����


     ////////////////////////////////////////////////////////////


	}   // end of while(1)

}     //  End of main()

////

// UART0 ���� ���ͷ�Ʈ ���� ���α׷� 

ISR(  USART0_RX_vect )
{

    static unsigned char r_cnt = 0 ;

    rdata = UDR0; 

    if( rdata != '.' )                      // ���ŵ� �����Ͱ� ������ ���ڸ� ��Ÿ���� ������(��ħǥ)�� �ƴϸ�
    {
        recv_data[r_cnt] = rdata;        //  ���ŵ� ���� ���� 
	    r_cnt++;                         //  ���� ���� ���� ���� 

		new_recv_flag = 0;
    }
    else if(  rdata == '.' )                // ���ŵȵ����Ͱ� ������ ���ڸ� ��Ÿ���� ������(��ħǥ) �̸�
    {
        recv_cnt = r_cnt ;                  // ���ŵ� ������ ����Ʈ�� ����
        r_cnt = 0;  
        
		new_recv_flag = 1;

    }
}





void init_serial(void)
{
    UCSR0A=0x00;                    //�ʱ�ȭ
    UCSR0B = 0x18  ;                //�ۼ������,  �ۼ��� ���ͷ�Ʈ ����
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



char NumToAsc(char Num)
{
	
	if( Num > 9 ) Num += 0x37;
	else	      Num += 0x30;

	return Num;
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


void msec_delay( int n )
{
	for(; n> 0 ; n-- )   _delay_ms(1); 
}

