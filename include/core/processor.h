//*****************************************************************************
//
// Имя файла    : 'processor.h'
// Заголовок    : Подключение файлов описаний для выбранного контроллера
// Автор        : Барышников Р. А. 
// Контакты     : plexus_bra@rambler.ru
// Дата         : 29/06/2012
//
//*****************************************************************************

#ifndef _PROCESSOR_H
#define _PROCESSOR_H

#include "types.h"

    #if defined(__MSP430__)
        #if   defined(__MSP430G2211__)
            #include "core/io/msp430g2211.h"
        #elif defined(__MSP430F2002__)
            #include "core/io/msp430f2002.h"
        #elif defined(__MSP430F2001__)
            #include "core/io/msp430f2001.h"
        #elif defined(__MSP430F2012__)
            #include "core/io/msp430f2012.h"
        #else
            #error ------- Неизвестный тип контроллера -------
        #endif
    #else
        #error ------ Неизвестный тип контроллера -------
    #endif

#endif // #ifndef _PROCESSOR_H