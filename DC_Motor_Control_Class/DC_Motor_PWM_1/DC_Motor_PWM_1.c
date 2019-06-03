#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#include "lcd.h"


void HexToDec( unsigned short num, unsigned short radix); 

char NumToAsc( unsigned char Num ); 

static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0}; 
	 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 


void msec_delay(int n) ;
 
void DC_Motor_Run_Fwd( short duty );    // DC ���� ��ȸ��(PWM����) �Լ� 
void DC_Motor_Run_Rev( short duty );    // DC ���� ��ȸ��(PWM����) �Լ�  
void DC_Motor_Stop( void );             // DC ���� ���� �Լ�  
 
void DC_Motor_PWM( short Vref );        // DC ���� PWM ��ȣ �߻� �Լ�  
                                        // ����ũ(Vref>0), ����ũ(Vref<0), ����ũ(Vref=0) ��� ���� 

static volatile short  Vmax = 3200 ;    //  Vmax = 3200  :  �ִ� ����( PWM duty = 100 % )


int main() 
{   
	short duty = 0;       


	DDRB |= 0x20;   // ���ͱ�����ȣ + ����:  PWM ��Ʈ( pin: OC1A(PB5) )   --> ��� ���� 
	DDRA |= 0x01;   // ���ͱ�����ȣ - ���� : ���� ��/�����Ʈ(pin : PA0 ) --> ��� ���� 

	LcdInit();
 

// ���ͱ�����ȣ ( pin: OC1A(PB5) ),   Timer1, PWM signal (period= 200 usec )

	TCCR1A = 0x82;    // OC1A(PB5)) :  PWM ��Ʈ ����,   Fast PWM ( mode 14 )
	TCCR1B = 0x19;    // 1 ����( No Prescale ) Ÿ�̸� 1 ���� (����Ŭ�� �ֱ� =  1/(16*10^6) = 1/16 usec ),  Fast PWM ( mode 14 ) 
	ICR1 = Vmax;      // PWM �ֱ� = 3200 * 1/16 usec = 200 usec


	OCR1A = duty;      //  OC1A(PB5) PWM duty = 0 ���� : ���� ����
//////////////////////////////////////////////////////////////////

    duty = 3200;      // ���� �ӵ� ����, �ִ� = Vmax = 50,  �ּ� = 0 

	LcdMove(0,0); 
	LcdPuts("DC Motor Control");

	LcdMove(1,0); 
	LcdPuts("Duty = ");

	 
	 
	while (1) 
	{ 

	    LcdMove(1,7); 
        Display_Number_LCD(duty, 4); 

 
		DC_Motor_Run_Fwd( duty );     // DC Motor ��ȸ�� 5��
        msec_delay( 5000 ); 
 
		DC_Motor_Stop();              // DC Motor ���� 2��
        msec_delay( 2000 ); 


		DC_Motor_Run_Rev( duty );     // DC Motor ��ȸ�� 5��
        msec_delay( 5000 ); 
 
		DC_Motor_Stop();              // DC Motor ���� 2��
        msec_delay( 2000 ); 


     }


} 



void DC_Motor_Run_Fwd( short duty )   // DC ���� ��ȸ�� �Լ� 
{

    if( duty > Vmax )     duty = Vmax ;

    PORTA &= ~0x01;     //  ���ͱ�����ȣ - ���� : 0 V �ΰ�( PA0 = 0 );  
	OCR1A = duty;       //  ���ͱ�����ȣ + ���� : OC1A(PB5) PWM duty ���� 


}

void DC_Motor_Run_Rev( short duty )   // DC ���� ��ȸ�� �Լ� 
{

    if( duty > Vmax )     duty = Vmax ;

    PORTA |= 0x01;            //  ���ͱ�����ȣ - ���� : 5 V �ΰ�( PA0 = 1 );  
	OCR1A = Vmax - duty;      //  ���ͱ�����ȣ + ���� : OC1A(PB5) PWM duty ���� 


}


void DC_Motor_Stop( void )   // DC ���� ���� �Լ� 
{

    PORTA &= ~0x01;     //  ���ͱ�����ȣ - ���� : 0 V �ΰ�( PA0 = 0 );  
	OCR1A = 0;          //  ���ͱ�����ȣ + ���� : OC1A(PB5) PWM duty = 0 ���� 


}



void DC_Motor_PWM( short Vref )   // DC ���� PWM ��ȣ �߻� �Լ�  
{

   if ( Vref > Vmax )       Vref = Vmax ;
   else if( Vref < -Vmax )  Vref = -Vmax ;

   if( Vref > 0 )  
   {
      DC_Motor_Run_Fwd( Vref ) ;
   }
   else if( Vref == 0 )  
   {
      DC_Motor_Stop() ;
   }
   else if( Vref < 0 )  
   {
      DC_Motor_Run_Rev( -Vref ) ;
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





