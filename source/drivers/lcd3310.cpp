//*****************************************************************************
//
// Имя файла    : 'lcd3310.cpp'
// Заголовок    : Драйвер LCD дисплея от Nokia 3310
// Автор        : Барышников Р. А.
// Контакты     : plexus_bra@rambler.ru
// Дата         : 30/06/2012
//
//*****************************************************************************

#include "drivers\lcd3310.h"
#include "core\pio.h"
#include "types.h"

    uchar N0 [] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
    uchar N1 [] = {0x00, 0x42, 0x7F, 0x40, 0x00};
    uchar N2 [] = {0x42, 0x61, 0x51, 0x49, 0x46};
    uchar N3 [] = {0x21, 0x41, 0x45, 0x4B, 0x31};
    uchar N4 [] = {0x18, 0x14, 0x12, 0x7F, 0x10};
    uchar N5 [] = {0x27, 0x45, 0x45, 0x45, 0x39};
    uchar N6 [] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
    uchar N7 [] = {0x01, 0x71, 0x09, 0x05, 0x03};
    uchar N8 [] = {0x36, 0x49, 0x49, 0x49, 0x36};
    uchar N9 [] = {0x06, 0x49, 0x49, 0x29, 0x1E};
    uchar SWPoint [] = {0x00, 0x36, 0x36, 0x00, 0x00};

void print_num(uchar num)
{
    switch (num)
    {
          case 0: 
                sel_num(N0);
                break;
          case 1: 
                sel_num(N1);
                break;
          case 2: 
                sel_num(N2);
                break;
          case 3: 
                sel_num(N3);
                break;
          case 4: 
                sel_num(N4);
                break;
          case 5: 
                sel_num(N5);
                break;
          case 6: 
                sel_num(N6);
                break;
          case 7: 
                sel_num(N7);
                break;
          case 8: 
                sel_num(N8);
                break;
          case 9: 
                sel_num(N9);
                break;
          case 10: 
                sel_num(SWPoint);
                break;                
    }
}

void sel_num(uchar *num)
{
    for(uchar row = 0; row < 5; row++)
    {
        lcd_data(num[row]);
    }
}

void lcd_init(uchar mode)
{
    cfg_pin(SDIN, PIN_OUTPUT);
    cfg_pin(SCLK, PIN_OUTPUT);
    cfg_pin(DC, PIN_OUTPUT);
    cfg_pin(SCE, PIN_OUTPUT);
    cfg_pin(RES, PIN_OUTPUT);
    
    clr_pin(RES);
    __delay_cycles(10);
    set_pin(RES);
    
    set_pin(SCE);
    __delay_cycles(10);
    
    // 0x21 = b00100001 Функциональные установки
    // Чип включен, горизонтальная адресация,
    // используются расширеные инструкции
    lcd_com(0x21);
    
    // 0xC8 = b11001000
    // Установка Vop
    lcd_com(0xC8);
    
    // 0x13 = b00010011
    // Система Bias 1 : 48
    lcd_com(0x13);
    
    // 0x20 = b00100000
    // Использовать основные инструкции
    lcd_com(0x20);
    
    // Нормальный режим дисплея
    lcd_com(0x0C);
    lcd_clr_scr();
}

void lcd_clr_scr()
{
    lcd_set_xy(0, 0);
    for (uint pixel = 0; pixel < 504; pixel++)
    {
        lcd_data(0x00);
    }
}

void lcd_set_xy(uchar x, uchar y)
{
    lcd_com(0x40 | (y & 0x07));
    lcd_com(0x80 | (x & 0x7F));
}

void lcd_com(uchar value)
{
    set_pin(DC, 0);
    lcd_write_byte(value);
}

void lcd_data(uchar value)
{
    set_pin(DC, 1);
    lcd_write_byte(value);
}

void lcd_write_byte(uchar value)
{
    set_pin(SCE, 0);
    for(uchar i = 0; i < 8; i++)
    {
        set_pin(SCLK, 0);
        if ((value & 0x80) == 0)
        {
            set_pin(SDIN, 0);
        }
        else
        {
            set_pin(SDIN, 1);
        }
        set_pin(SCLK, 1);
        value = value << 1;
    }
    set_pin(SCE, 1);
}