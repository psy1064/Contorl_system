//
//================================================
//  ���ϸ� : lcd.h - LCD���� ����� �������
//================================================
//

#ifndef __LCD_H__
#define __LCD_H__

// LCD ���� ���

#define ALLCLR			0x01	// ȭ���� �����.
#define HOME			0x02	// Ŀ���� Ȩ���� ������.
#define LN21			0xc0	// Ŀ���� 2��° ������ ù��°�� ��ġ��Ŵ
#define ENTMOD			0x06	// entry mode
#define FUNSET			0x28	// function set   
#define DISP_ON			0x0c	// ���÷��̸� �Ҵ�.
#define DISP_OFF		0x08	// ���÷��̸� ����.
#define CURSOR_ON		0x0e	// Ŀ���� �Ҵ�.
#define CURSOR_OFF		0x0c	// Ŀ���� ����.
#define CURSOR_LSHIFT  	0x10	// Ŀ���� ������ �̵���Ų��.
#define CURSOR_RSHIFT	0x14	// Ŀ���� ���������� �̵���Ų��.
#define DISP_LSHIFT		0x18	// ���÷��̸� �������� �̵���Ų��.
#define DISP_RSHIFT		0x1c	// ���÷��̸� ���������� �̵���Ų��.

// LCD ������ �Լ�

void LcdInit(void);			
void LcdCommand(char command);
void LcdMove(char line, char pos);
void LcdPutchar(char ch);
void LcdPuts(char* str);
void LcdNewchar(char ch, char font[]);


#endif	// __LCD_H__
