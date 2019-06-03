#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void init_serial(void) ;  //  Serial �����Ʈ �ʱ�ȭ

void SerialPutChar(char ch);
void SerialPutString(char str[]);

void Servo_Move( short sv_pos_cmd ) ;                     // Servo Move �Լ� 

void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 

	 
void Display_Number_LCD( unsigned int num, unsigned char digit )  ;     // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 


void msec_delay(int n)  ; 


static volatile short     Servo_Pos_CMD = 0 ;    // ���� ��ġ ��� ( ���� : 0 - 180,  ����:  �� )
static volatile short     Pos_max = 180 ;        // ���� �ִ� ��ġ ��� ( 180 �� )
static volatile short     Pos_min = 0 ;          // ���� �ּ� ��ġ ��� ( 0   �� )
static volatile short     Pos_center = 90 ;      // ���� �߰� ��ġ ��� ( 90  �� )

static volatile unsigned char   Command_Error_Flag = 0 ; 

static volatile  char  recv_cnt = 0, rdata = 0, new_recv_flag = 0, rdata_old = 0 ; 


int main() 
{   


	DDRB |= 0x80;    //  PWM ��Ʈ: OC2( PB7 ) ��¼��� 
 

	DDRB |= 0x08;     // ����(Buzzer) ( PB3 : �����Ʈ ����    )
	PORTB &= ~0x08;   // PB3  : Low  ( ���� OFF )  

	DDRB |= 0x10;     // LED ( PB4 : �����Ʈ ����    )
	PORTB |= 0x10;    // PB4  : High ( LED OFF)    


    init_serial() ;    // Serial Port (USART1) �ʱ�ȭ

    UCSR0B |=  0x80  ;      // UART1 �۽�(RX) �Ϸ� ���ͷ�Ʈ ���
	sei() ;                 // �������ͷ�Ʈ���


	LcdInit();    //  LCD �ʱ�ȭ 

	LcdMove(0,0); 
	LcdPuts("RC Servo Motor");
	LcdMove(1,0); 
	LcdPuts("Servo_Pos = ");

    msec_delay(2000); 


// PWM ��ȣ  pin: OC2(PB7), Timer2, PWM signal (period= 16.384msec )

	TCCR2 |= 0x68;   //  Trigger signal (OC2)   �߻� :  WGM20(bit6)=1,  WGM21(bit3)=1,  COM21(bit5)=1, COM20(bit4)=0 ,  
	TCCR2 |= 0x05;   //  1024����,  ����Ŭ���ֱ� = 64usec  : CS22(bit2) = 1, CS21(bit1) = 0,  CS20(bit0) = 1 


    Servo_Pos_CMD = Pos_center ;                        // �������� �߰� ��ġ�� ȸ���ϴ� ��� ����  
    Servo_Move( Servo_Pos_CMD );                        // �־��� ��ɴ�� ���� ���� ȸ��

	LcdMove(1, 12); 
    Display_Number_LCD( Servo_Pos_CMD , 3); 

	 
	while (1) 
	{ 
 

         if( new_recv_flag == 1 )      // �� ���� ���ſϷ� �� 
		 { 

		    if( Command_Error_Flag == 1 )    // ���� ��ɿ� ������ �־�����
			{  
			    Command_Error_Flag = 0 ;     // ���� Command_Error_Flag ���� 

			    LcdCommand( ALLCLR ) ;       // LCD Claear
 	            LcdMove(0,0); 
	            LcdPuts("RC Servo Motor");
	            LcdMove(1,0); 
	            LcdPuts("Servo_Pos = ");

            }

		  //////////////  ��ɾ� ó��   //////////////

			if( rdata == '0' )          // ���� 0 �� ���ŵǸ� 
			{
                PORTB |= 0x10 ;          // LED OFF 
			}
			else if( rdata == '1' )     // ���� 1 �� ���ŵǸ�
			{
                PORTB &= ~0x10 ;         // LED ON
			}

			else if( rdata == '2' )            // ���� 2 �� ���ŵǸ�
			{
		        Servo_Pos_CMD += 10 ;                                         // �������� ��ġ 10 ���� ����  
                if( Servo_Pos_CMD >= Pos_max )   Servo_Pos_CMD = Pos_max ;    // �������� �ִ� ��ġ Pos_max = 180 ��
		        Servo_Move( Servo_Pos_CMD );                                  // �־��� ��ɴ�� ���� ���� ȸ��  
 
				rdata_old = rdata ; 
			}

			else if( rdata == '3' )            // ���� 3 �� ���ŵǸ�
			{
		        Servo_Pos_CMD -= 10 ;                                        // �������� ��ġ 10 ���� ����  
                if( Servo_Pos_CMD < Pos_min )   Servo_Pos_CMD = Pos_min ;    // �������� �ּ� ��ġ Pos_min = 0 ��
		        Servo_Move( Servo_Pos_CMD );                                 // �־��� ��ɴ�� ���� ���� ȸ��  
 
				rdata_old = rdata ; 
			}

			else if( rdata == '4' )            // ���� 4 �� ���ŵǸ�
			{
                Servo_Pos_CMD = Pos_max ;
 		        Servo_Move( Servo_Pos_CMD );                // �������� �ִ� ��ġ Pos_max = 180 ���� ���� ���� ȸ��  
										 
			}

			else if( rdata == '5' )            // ���� 5 �� ���ŵǸ�
			{

                Servo_Pos_CMD = Pos_center ;
 		        Servo_Move( Servo_Pos_CMD );               // �������� ��� ��ġ Pos_center = 90 ���� ���� ���� ȸ��  
										 
			}

			else if( rdata == '6' )            // ���� 6 �� ���ŵǸ�
			{
                Servo_Pos_CMD = Pos_min ;
 		        Servo_Move( Servo_Pos_CMD );               // �������� �ּ� ��ġ Pos_min = 0 ���� ���� ���� ȸ��  
										 
			}


			else if( rdata == '7')      // ���� 7 �� ���ŵǸ�
			{

		        HexToDec( Servo_Pos_CMD,10);   // ���� ��ġ(���) Servo_Pos_CMD �������� ��ȯ

                SerialPutString( "Servo Moror Position = " );   //  �޽��� ���� 

                SerialPutChar( NumToAsc(cnumber[2]));           //  ���� Servo_Pos_CMD �� ����
                SerialPutChar( NumToAsc(cnumber[1]));            
                SerialPutChar( NumToAsc(cnumber[0])); 

                SerialPutString( "deg" );                       //  �޽��� ����
                SerialPutChar('\n');                            // �޴������� ������ ���۽� Line Feed('\n')�� �׻� ���� �����ؾ���

			} 

			else if( rdata != 0xFF)    //  ��� ���� �̸�
			{

                SerialPutString( "Command Error!!  Try again.\n" ); //  ��� ���� �޽��� ����

			    LcdCommand( ALLCLR ) ;    // LCD Claear

			    LcdMove(0, 0 );           // LCD�� �����޽��� ���÷���
		        LcdPuts("Cmd Error!!"); 
			    LcdMove(1, 0 );
		        LcdPuts("Try Again."); 

				Command_Error_Flag = 1;  

			}


		    rdata = 0xFF;
            new_recv_flag = 0;      // �� ����(���) ���� �÷��� Reset
  

		   if( Command_Error_Flag == 0 )                // ��ɿ� ������ ������ 
		   {  
	           LcdMove(1, 12); 
               Display_Number_LCD( Servo_Pos_CMD , 3 ); 
           }


        }              


     }           // end of while(1) loop


}                // end of main() loop



/////////////////////////////////////////////////////////////////////////////////////////



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


////////////////////////////////////////////////////////////////////////////////



void Servo_Move( short sv_pos_cmd )
{

      OCR2 = ( 135 * sv_pos_cmd )/900  + 10  ;  

      //  �޽��� = 0.64msec = 64usec * 10,   ���� ��(0 ��)  (�޽��� = 0.66msec )
      //  �޽��� = 1.47msec = 64usec * 23 ,  ���(90 ��) (�޽��� = 1.5msec )
      //  �޽��� = 2.37msec = 64usec * 37 ,  ������ ��(180 ��) (�޽��� = 2.45msec ) 

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



