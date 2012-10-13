//*****************************************************************************
//
// Имя файла    : 'lcd3310.h'
// Заголовок    : Драйвер LCD дисплея от Nokia 3310
// Автор        : Барышников Р. А.
// Контакты     : plexus_bra@rambler.ru
// Дата         : 30/06/2012
//
//*****************************************************************************

#ifndef _LCD3310_H
#define _LCD3310_H

#include "core\processor.h"

    #define     SDIN        BIT2
    #define     SCLK        BIT1
    #define     DC          BIT3
    #define     SCE         BIT4
    #define     RES         BIT5


    // Режимы экрана
    // Дисплей пуст
    #define     CLR_MOD     0x00
    // Нормальный режим
    #define     ALLON_MOD   0x01
    // Все сегменты включены
    #define     NORM_MOD    0x04
    // Инверсный режим
    #define     INV_MOD     0x05

    void lcd_init(uchar mode);
    void lcd_com(uchar value);
    void lcd_data(uchar value);
    void lcd_write_byte(uchar value);
    void lcd_clr_scr();
    void lcd_set_xy(uchar x, uchar y);
    void sel_num(uchar *num);
    void print_num(uchar num);

#endif // #ifndef _LCD3310_H