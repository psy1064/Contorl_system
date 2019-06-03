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

static volatile unsigned short    distance_1 = 0  ;
static volatile unsigned short    distance_1_old = 0 ;

int main() 
{   

	unsigned short dist_1 = 0  ;


	LcdInit();      //  LCd �ʱ�ȭ �Լ� 

	LcdMove(0,0); 
	LcdPuts("UltrasonicSensor"); 
	LcdMove(1,0); 
	LcdPuts("Dist_1 =    cm");


//// �����ļ���( Ultrasonic Sensor) ////////////

//// �����Ʈ ���� 
	
	DDRB |= 0x01;     	// Data Direct Register B(0�̸� �Է�, 1�̸� ���)
						// �����ļ��� Trigger signal( PB0 : �����Ʈ ����  )
					  	// Trigger signal�� �����־�� �ϱ� ������ ��� ��Ʈ�� ����
						// ���ϴ� ��Ʈ�� 1�� ������ �ִ� ��
					  	// DDRB = XXXX XXXX
						// 0x01 = 0000 0001
						//|------------------
						//        XXXX XXX1
	DDRB |= 0x10;		// DDRB = XXX1 XXXX ���� ���
	DDRB |= 0x20;		// DDRB = XX1X XXXX LED ��� 

	PORTB &= ~0x01;   	// PB0  : Low  ( Trigger signal OFF )  
						// ���ϴ� ��Ʈ�� 0�� ������ �ִ� ��
						// PORTB = XXXX XXXX
						// ~0x01 = 1111 1110
						//&--------------------
						//         XXXX XXX0  
						// PORTB = XXXX XXX0
	PORTB &= ~0x10;		// ���� OFF(LOW)
	PORTB |= 0x20;		// LED OFF(HIGH)


////////////  Timer 0 ����  ( 10 msec �ֱ� Ÿ�̸� 0 ���ͷ�Ʈ )  ///////////////
// 50msec���� Trigger ��ȣ�� ������ ��⿡ ����
// p. 133
        
    TCCR0 = 0x00;            // Ÿ�̸� 0 ����(���ֺ� = 1024 ) , Normal mode(Ÿ�̸Ӹ��)
						
    TCNT0 = 256 - 156;       // �ʱ갪 ���� 	
						     // ����Ŭ���ֱ� = 1024(=���ֺ�) / 16x10^6(=���� Ŭ��) = 64 usec,  
                             // �����÷����ͷ�Ʈ �ֱ� = 10msec
							 // ����Ŭ���ֱ� x n = �����÷����ͷ�Ʈ �ֱ�
                             // 64usec x n = 10mesc
							 // n = 10msec / 64usec
							 // therefore n = 156
							 // 64usec Ŭ���� 156�� �ݺ��������� 10msec��
							 // �����÷ο찡 �߻������� ���ͷ�Ʈ�� �ɸ��� ������ 8bit�� �����÷ο찡 �ɸ��� 256������ n���� ���ָ�
							 // n�� �ݺ������� �����÷ο찡 �ɸ���

    TIMSK |= 0x01;            // Ÿ�̸� 0 �����÷����ͷ�Ʈ ���

///////////////////////////////////////////////////////////    


// Echo Signal Pulse Width measurment,  Timer3 
// ���� ��ȣ�� �� �ð��� �����ϱ� ���� Ÿ�̸� ���ͷ�Ʈ
// p.238

	TCCR3A = 0x00; 
	TCCR3B = 0x02;     // Ÿ�̸� 3 ����(���ֺ� 8) ,  0.5usec ������ ���� 

/////////////////////////////////////////////////////////

/* 
// �����ļ��� Echo Signals : external interrupt 4( pin: INT4 (PE4)),  
// p.109 

	DDRE &= ~0x10;		// PE4(INT4)�� �Է����� ����

	EICRB |= 0x01;     	// INT4 Both falling edge and rising edge interrupt
						// EICRB = XXXX XXX1
	EICRB &= ~0x02;		// EICRB = XXXX XX0X
	EIMSK |= 0x10;     	// INT4 Enable 
	sei(); 				// ���ͷ�Ʈ ���  <-> ���� = cli();

///////////////////////////////////////
*/

// �����ļ��� Echo Signals : external interrupt 0( pin: INT0 (PD0)),  
// p.109 

	DDRD &= ~0x01;		// PD0(INT0)�� �Է����� ����

	EICRA |= 0x03;     	// INT0 rising edge interrupt
						// EICRA = XXXX XX11
	EIMSK |= 0x01;     	// INT0 Enable 
	sei(); 				// ���ͷ�Ʈ ���  <-> ���� = cli();

///////////////////////////////////////

// ���� �����ļ��� 1 Ʈ���� ��ȣ �߻�(������ 1 �߻�)  


	PORTB |= 0x01;    	// PB0 : High
	usec_delay(20);  	// 20usec ���� High ���� 
	PORTB &= ~0x01;    	// PB0 : Low
						// = PORTB &= 0xFE;
          
//////////////////////////////////////

    TCCR0 |= 0x07;    // Ÿ�̸� 0 ����(���ֺ� = 1024)

////////////////////////////////////// 
 
	while (1) 
	{ 
		cli();			// �����͸� ������ ���ͷ�Ʈ ����
 	    dist_1 = distance_1 ;
 		sei();			// ���ͷ�Ʈ ��� 

	    LcdMove(1, 9); 
        Display_Number_LCD(dist_1); 
    }
} // ���α׷� ����

//////////////////////////////////////

ISR(TIMER0_OVF_vect)    //  10 msec �ֱ� Ÿ�̸�1 �����÷� ���ͷ�Ʈ �������α׷�
{
    static unsigned short  time_index = 0 ; 

    TCNT0 = 256 - 156;   // �ʱⰪ�� �� �������־�� �� 

    time_index++ ; 

    if( time_index == 5 )   // 50 msec �ֱ� 
    {

       time_index = 0; 

       //  �����ļ��� 1 Ʈ���� ��ȣ �߻�(������ 1 �߻�) 

	   PORTB |= 0x01;    // PB0 : High
	   usec_delay(20) ;  // 20usec ���� High ���� 
	   PORTB &= ~0x01;    // PB0 : Low 
	   if( distance_1 < 40)
	   {
   	   		PORTB |= 0x10;		// ���� ON(LOW)
			PORTB &= ~0x20;		// LED ON(HIGH)
       }
	   else
	   {
   	   		PORTB &= ~0x10;		// ���� OFF(LOW)
			PORTB |= 0x20;		// LED OFF(HIGH)
	   }

   }
} // ���� ���� ��
/*ISR(INT4_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;


	if(flag == 0) 
	{
		count1 = TCNT3; 
	  	flag = 1;

	} // ��� ���ͷ�Ʈ 
	else if(flag == 1)
	{ 
	  	count2 = TCNT3; 
		del_T = count2 - count1;
    	distance_1 = del_T/(2*58);		  	// �Ÿ� ��� ��

        if( distance_1 > 380 )              // �ݻ�Ǵ� �����İ� ������� ������ 
		{
			distance_1 = distance_1_old ;   // ���� ������ ��� 
		} 

        distance_1_old = distance_1 ;       // ���� ������ ���� ���� ������Ʈ  

		flag = 0; 
	} // �ϰ� ���ͷ�Ʈ 
}*/

ISR(INT0_vect)
{

    static unsigned short count1 = 0, count2 = 0, del_T = 0, flag = 0 ;


	if(flag == 0) 
	{
		count1 = TCNT3; 
	  	flag = 1;
		EICRA |= 0x02;						
		EICRA &= ~0x01;						// �ϰ� �������� ���ͷ�Ʈ�� �ɸ��� ���� ����
	} // ��� ���ͷ�Ʈ 
	else if(flag == 1)
	{ 
	  	count2 = TCNT3; 
		del_T = count2 - count1;
    	distance_1 = del_T/(2*58);		  	// �Ÿ� ��� ��

        if( distance_1 > 380 )              // �ݻ�Ǵ� �����İ� ������� ������ 
		{
			distance_1 = distance_1_old ;   // ���� ������ ��� 
		} 

        distance_1_old = distance_1 ;       // ���� ������ ���� ���� ������Ʈ  

		flag = 0; 
		EICRA |= 0x03;						// ��� �������� ���ͷ�Ʈ�� �ɸ��� ���� ����
	} // �ϰ� ���ͷ�Ʈ 
}
void Display_Number_LCD( unsigned int num )       // ��ȣ���� ������ ������ 10���� ���·� LCD �� ���÷��� 
{

	HexToDec( num, 10); //10������ ��ȯ


	LcdPutchar(NumToAsc(cnumber[2]));    // 100�ڸ� ���÷��� 

	LcdPutchar(NumToAsc(cnumber[1]));    // 10�ڸ� ���÷���

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
		_delay_ms(1);	// 1msec �ð� ����
}
void usec_delay(int n)
{	
	for(; n>0; n--)		// 1usec �ð� ������ nȸ �ݺ�
		_delay_us(1);	// 1usec �ð� ����
}
