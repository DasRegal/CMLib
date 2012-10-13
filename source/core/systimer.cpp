//*****************************************************************************
//
// Имя файла    : 'systimer.cpp'
// Заголовок    : Драйвер модуля системных таймеров 
// Автор        : Крыцкий А.В. 
// Контакты     : kav@nppstels.ru
// Дата         : 04/10/2010
//
//*****************************************************************************

#include "core\systimer.h"
#include "core\processor.h"
#include "core\tc.h"
#include "core\pio.h"
#include "core\core.h"
#include "clib\time.h"
#include "clib\string.h"
#include "def_timer.h"

// ============================================================================
//         Ограничение на максимальное количество 10 мс таймеров 
// ============================================================================

#if ( COUNT_MS10 > 10 )
  #error -------- Не верное количество 10мс таймеров COUNT_MS10[0..10] --------
#endif

    #define QUOTA_MS10     (10 - COUNT_MS10)
    #define QUOTA_MS100   ((10 - COUNT_MS10) * 10) 
    #define QUOTA_SEC    (((10 - COUNT_MS10) * 10) - COUNT_MS10) * 10

    #if (QUOTA_MS10 < 1) && (COUNT_MS100 != 0 || COUNT_SEC != 0 || COUNT_MIN != 0)
        #error -------- Не выделены кванты после 10 мс таймера --------        
    #endif      

    #if QUOTA_MS100 < 1 
        #error -------- Не выделены кванты для секундного таймера --------  
    #endif

    #if QUOTA_SEC < 1 
        #error -------- Не выделены кванты для минутного таймера --------  
    #endif

    // Реализация встроенного таймера защиты 0-х handle
    #define  COUNT_MS_IN     (COUNT_MS + 1)
    // Общее количество таймеров в системе (1 - зарезервирован для 0-го индекса)
    #define  COUNT_TIMERS    (COUNT_MS_IN + COUNT_MS10 + COUNT_MS100 + COUNT_SEC + COUNT_MIN)
    // Выделение памяти хотя бы для одного таймера
    #if (COUNT_TIMERS == 0)
        #undef   COUNT_MIN
        #undef   COUNT_TIMERS
        #define  COUNT_MIN        1
        #define  COUNT_TIMERS     1
    #endif

    #if defined(__CORTEX__)
        // Для отсчета интевалов времени в poll, используется счетчик тактов модуля
        // Data Watchpoint and Trace (DWT), который встроен в ядро Cortex
        #define  DWT_CYCCNT                 *(volatile uint32*)0xE0001004
        #define  DWT_CONTROL                *(volatile uint32*)0xE0001000
        #define  SCB_DEMCR                  *(volatile uint32*)0xE000EDFC
        #define  GET_TICK_COUNT()           DWT_CYCCNT
        #define  SYSTIMER_DISABLE_INT()     AT91C_BASE_SYSTICK->CTRL &= ~AT91C_SYSTICK_INT
        #define  SYSTIMER_ENABLE_INT()      AT91C_BASE_SYSTICK->CTRL |= AT91C_SYSTICK_INT
    #else
        // Для остальных моделей используется 24-х битный таймер/счетчик PIT
        #define  GET_TICK_COUNT()           AT91C_BASE_PITC->PITC_PIIR
        #define  SYSTIMER_DISABLE_INT()     tc0.DisableIrq()
        #define  SYSTIMER_ENABLE_INT()      tc0.EnableIrq()
    #endif

// ============================================================================
//                          Прототипы функций
// ============================================================================

    // Инициализация модуля
    void Init_SysTimer     (void);
    // Тик системного времени
    __RAMFUNC void Tick_SysTime      (void);
    // Увеличение количества дней
    __RAMFUNC void IncDay_SysTimer   (void);
    // Тик таймера 
    __RAMFUNC void TickTimer_SysTime (TTimerRecord* ptr);
    // Увеличение количества дней
    void IncDay_SysTimer   (void);
    

// ============================================================================
//                        Глобальные переменные
// ============================================================================

    CServerTimer  ServerTimer;
    TTimerRecord  timerTable[COUNT_TIMERS];
    uint          g_TickCount_us;
    uint          g_TickCount_ms;    

// ============================================================================
//                             С-функции
// ============================================================================
    
// ============================================================================
///
///                          Тик таймера 
///
// ============================================================================

#if !defined(__DEBUG__) && !defined(__JTAG__)
    #pragma optimize = speed high
#endif
#pragma diag_suppress=Ta022
__RAMFUNC void TickTimer_SysTime (TTimerRecord* ptr)
{
    if (ptr->count && !--ptr->count && ptr->pFunc)
    {
        if ( TTimerData Value = ptr->pFunc(ptr->param) )
        {
            ptr->count = Value;
        }
    }
}    

// ============================================================================
///
///                    Обработчик прерывания от таймера
///
// ============================================================================

#if !defined(__DEBUG__) && !defined(__JTAG__)
    #pragma optimize = speed high
#endif
__RAMFUNC __ARM void handlerSysTimer (void)
{
    // Сброс признака прерывания
    #if !defined(__CORTEX__)
    uint status = AT91C_BASE_TC0->TC_SR;             
    #endif    

    TPlatformType i;
    
    // =================================================
    //    Увеличение системного времени (по 1 mS)
    // =================================================
    Tick_SysTime();
    
    // =================================================
    //           Перебор блока таймеров
    // =================================================  
    
    // Анализ и выполнение 1 мс таймеров
    #if (COUNT_MS_IN > 1)
    i = 1; while (i < COUNT_MS_IN)
    {
        TickTimer_SysTime(&timerTable[i]); i++;
    }
    #endif

    // Анализ и выполнение 10 мс таймеров
    // ==================================
    i = ServerTimer.count_ms10;
    if ( i ) 
    {
        TickTimer_SysTime(&timerTable[i + COUNT_MS_IN - 1]); 
        ServerTimer.count_ms10 = --i;
        return;
    }
    
    // Анализ и выполнение 100 мс таймеров
    // ===================================
    i = ServerTimer.count_ms100;
    if ( i ) 
    {
        TickTimer_SysTime(&timerTable[i + COUNT_MS_IN + COUNT_MS10 - 1]);
        ServerTimer.count_ms100 = --i;
        return;
    }
    
    // Анализ и выполнение 1 с таймеров
    // ===================================
    i = ServerTimer.count_sec;
    if ( i ) 
    {
        TickTimer_SysTime(&timerTable[i + COUNT_MS_IN + COUNT_MS10 + COUNT_MS100 - 1]);
        ServerTimer.count_sec = --i;
        return;
    }
    
    // Анализ и выполнение 1 мин таймеров
    // ===================================
    i = ServerTimer.count_min;
    if ( i ) 
    {
        TickTimer_SysTime(&timerTable[i + COUNT_MS_IN + COUNT_MS10 + COUNT_MS100 + COUNT_SEC - 1]);
        ServerTimer.count_min = --i;
        return;
    }
}

// ============================================================================
///
///                       Тик системного времени
///
// ============================================================================

#if !defined(__DEBUG__) && !defined(__JTAG__)
    #pragma optimize = speed high
#endif
__RAMFUNC void Tick_SysTime (void)
{
    CServerTimer* ptr = &ServerTimer;
    
    if (++ptr->ms > 9) 
    {
        ptr->ms         = 0;                            // Интервал 10 мс
        ptr->count_ms10 = COUNT_MS10;                   //  
        if (++ptr->ms10 > 9)                            //
        {                                               //
            ptr->ms10        = 0;                       // Интервал 100 мс
            ptr->count_ms100 = COUNT_MS100;             //    
            if (++ptr->ms100 > 9)                       //
            {                                           //
                ServerTimer.jiffies++;                  // <Увеличение секундных тиков системы>
                ptr->ms100     = 0;                     // Интервал 1 с            
                ptr->count_sec = COUNT_SEC;             //   
                if (++ptr->sec > 59)                    //
                {                                       //
                    ptr->sec       = 0;                 // Интервал 1 мин
                    ptr->count_min = COUNT_MIN;         // 
                    if (++ptr->min > 59)                //
                    {                                   //    
                        ptr->min = 0;                   // Интервал 1 час
                        if (++ptr->hour > 23)           //
                        {                               //
                            ptr->hour = 0;              // 
                            IncDay_SysTimer();          // Интервал 1 день
                        } // end if (следующий день)    //
                    } // end if (следующий час)         //
                } // end if (следующая минута)          //
            } // enf if (следующая секунда)             //
        } // end if (следующая 100 мс)                  //
    } // end if (следующая 10мс) 
} 

// ============================================================================
///
///                       Увеличение количества дней
///
// ============================================================================

#if !defined(__DEBUG__) && !defined(__JTAG__)
    #pragma optimize = speed high
#endif
__RAMFUNC void IncDay_SysTimer (void)
{
    TPlatformType day   = ServerTimer.day;
    TPlatformType month = ServerTimer.month;

    switch (++day)
    {
        // Переход с 31 
        case 32: month++; day = 1; break;
        // Переход с 30
        case 31: 
            if ((month == 4) || (month == 6) || (month == 9) || (month == 11))
            {
                month++; day = 1;
            }
            break;
        // Переход с 29
        case 30: 
            if (month == 2)
            {
                month++; day = 1;
            }
            break;
        // Переход с 28
        case 29: 
            if ((month == 2) && ((ServerTimer.year & 3) == 0))
            {
                month++; day = 1;
            }
            break;
    } // end switch (анализ дня)

    // Увеличение года  
    if (month == 13)
    {
        month = 1; ++ServerTimer.year;
    } 
  
    ServerTimer.day   = day;
    ServerTimer.month = month;
    
    // Увеличение дня недели
    if (++ServerTimer.wday > 7) ServerTimer.wday = 1;
}

// ============================================================================
///
///                       Инициализация модуля
///
// ============================================================================
/// Начальная настройка блока таймеров
// ============================================================================

void Init_SysTimer (void)
{
    TTimerRecord* pTimer = timerTable;
    
    for (uint i = 0; i < COUNT_TIMERS; i++, pTimer++)
        pTimer->param = (void*)(-1); 
    
    ServerTimer.day   = 20;
    ServerTimer.month = 6;
    ServerTimer.year  = 2000;

    // ===========================================    
    // 20111004 - Резервирование 0-го таймера
    // ===========================================    
    // В случае сбоя программы модули работают в основном с Handle = 0, и если
    // валидный модуль использует получанный handle = 0, его работу может прервать 
    // другой модуль
    //
        timerTable[0].param = (void*)(0xBABA); 
    //
    // ===========================================        
}

// ============================================================================
///
///                       Создание таймера
///
// ============================================================================
/// \param  type     Тип таймера (дискретность)
/// \param  pFunc    Указатель на callback-фукнцию
/// \param  param  Указатель на объект/Дополнительное поле информации
/// \return Уникальный идентификатор таймера
// ============================================================================

TTimerHandle CreateTimer (TTypeTimer type, TCallbackTimer* pFunc, void* param)
{
    uint          count;  
    uint          index;    
    TTimerRecord* pTimer;
    
    switch (type) 
    {
        case tMS: 
            count  = COUNT_MS;
            index  = 1;         
            pTimer = &timerTable[1];
            break;
            
        case tMS10: 
            count  = COUNT_MS10;  
            index  = COUNT_MS_IN;
            pTimer = &timerTable[COUNT_MS_IN];
            break;
        case tMS100: 
            count  = COUNT_MS100;
            index  = COUNT_MS_IN + COUNT_MS10;         
            pTimer = &timerTable[COUNT_MS_IN + COUNT_MS10];
            break;
            
        case tSEC: 
            count  = COUNT_SEC;
            index  = COUNT_MS_IN + COUNT_MS10 + COUNT_MS100;                  
            pTimer = &timerTable[COUNT_MS_IN + COUNT_MS10 + COUNT_MS100];
            break;
        
        case tMIN: 
            count  = COUNT_MIN;
            index  = COUNT_MS_IN + COUNT_MS10 + COUNT_MS100 + COUNT_SEC;                           
            pTimer = &timerTable[COUNT_MS_IN + COUNT_MS10 + COUNT_MS100 + COUNT_SEC];
            break;
            
        default:
            while(1);
    } // end switch (анализ типа таймера)
    
    // Поиск свободного таймера
    while (count) 
    {
        if (pTimer->param == (void*)(-1)) 
        {
            // Свободный таймер найден: инициализация структуры
            pTimer->count = 0;
            pTimer->pFunc = pFunc;  
            pTimer->param = param;
            
            // Таймер выделен
            return (TTimerHandle)index;                                              
        } // end if ( ptr )      
        
        // Поиск следующего таймера    
        count--; pTimer++; index++;                                                   
    } // end while (i)
    
    // Не хватило таймеров
    while(1);
}

// ============================================================================
///
///                       Создание таймера
///
// ============================================================================
/// \param  type  Тип таймера (дискретность)
/// \param  info  Дополнительное поле информации
/// \return Уникальный идентификатор таймера
// ============================================================================

TTimerHandle CreateTimer (TTypeTimer type, uint info)
{
    return CreateTimer(type, 0, (void*)info);
}

// ============================================================================
///
///                          Запуск таймера
///
// ============================================================================
/// \param  handle  Уникальный идентификатор таймера
/// \param  time    Время на которое запускается таймер (в дискретах таймера)
// ============================================================================

void StartTimer (TTimerHandle handle, TTimerData time)
{
    timerTable[handle].count = time;
}

// ============================================================================
///
///                          Остановка таймера
///
// ============================================================================
/// \param  handle  Уникальный идентификатор таймера
// ============================================================================
  
void StopTimer (TTimerHandle handle)
{
    timerTable[handle].count = 0;
}
  
// ============================================================================
///
///          Проверка окончания ожидаемого интервала времени
///
// ============================================================================
/// \param  handle  Уникальный идентификатор таймера
/// \return Текущее значение счетчика таймера (0 - таймер закончил отсчет)
// ============================================================================
  
TTimerData CheckTimer (TTimerHandle handle)
{
    return timerTable[handle].count;
}

// ============================================================================
///
///                    Чтение текущего системного времени
///
// ============================================================================
/// \return  Текущее системное время в упакованном формате
// ============================================================================

TTimePacked GetSysTime (void)
{
    // Запрет прерываний
    SYSTIMER_DISABLE_INT();

    TTimePacked time;
    
    uint year = ServerTimer.year;
    if (year >= 2000) year -= 2000;
    
    time.year  = year;
    time.month = ServerTimer.month;
    time.day   = ServerTimer.day;
    time.hour  = ServerTimer.hour;
    time.min   = ServerTimer.min;
    time.sec   = ServerTimer.sec;

    // Разрешение прерываний    
    SYSTIMER_ENABLE_INT();    
    
    return time;
}

// ============================================================================
///
///                    Чтение текущего системного времени
///
// ============================================================================
/// \return  Текущее системное время в неупакованном формате
// ============================================================================

TTimeStamp GetSysTimeUnpack (void)
{
    TTimePacked pack = GetSysTime();
    TTimeStamp  t    = UnpackTime(pack);
    
    return t;    
}

// ============================================================================
///
///                      Установка системного времени
///
// ============================================================================
/// \param  year   Год (2000 - 2xxx)
/// \param  month  Месяц (1 - 12)
/// \param  day    День (1 - 31)
/// \param  hour   Час
/// \param  min    Минута
/// \param  sec    Секунда
// ============================================================================

void SetSysTime (uint year, uint month, uint day, uint hour, uint min, uint sec)
{
    // Запрет прерываний
    SYSTIMER_DISABLE_INT();

    if (year < 2000)  year += 2000;
    
    ServerTimer.year  = year;
    ServerTimer.month = month;
    ServerTimer.day   = day;
    ServerTimer.hour  = hour;
    ServerTimer.min   = min;
    ServerTimer.sec   = sec;

    // Разрешение прерываний    
    SYSTIMER_ENABLE_INT();    
    
    // Корректировка дня недели
    ServerTimer.wday  = WhatDayTime(ServerTimer.day, ServerTimer.month, ServerTimer.year);
}

// ============================================================================
///
///                     Установка системного времени
///
// ============================================================================
/// \param  time  Новое системное время
// ============================================================================

void SetSysTime (TTimePacked time)
{
    SetSysTime(time.year, time.month, time.day, time.hour, time.min, time.sec);
}

// ============================================================================
///
///                    Установка текущего системного времени
///
// ============================================================================
/// \param  time  Новое системное время в неупакованном формате
// ============================================================================

void SetSysTimeUnpack (TTimeStamp& time)
{
    TTimePacked t = PackTime(time);
    
    SetSysTime(t);    
}

// ============================================================================
///
///      Чтение текущего системного времени (строковое представление)
///
// ============================================================================
/// \return  Указатель на сформированную строку
// ============================================================================

uchar* GetSysTimeStr (ETimeFormat format)
{
    // Запрет прерываний
    SYSTIMER_DISABLE_INT();
    
    TTimeStamp time;
    
    time.year  = ServerTimer.year;
    time.month = ServerTimer.month;
    time.day   = ServerTimer.day;
    time.hour  = ServerTimer.hour;
    time.min   = ServerTimer.min;
    time.sec   = ServerTimer.sec;
    time.ms    = ServerTimer.ms + 
                 ServerTimer.ms10 * 10 + 
                 ServerTimer.ms100 * 100;    
    
    // Разрешение прерываний    
    SYSTIMER_ENABLE_INT();     
    
    uchar* str = TimeToStr(time, format);
    
    return str;
}

// ============================================================================
///
///                        Конструктор объекта
///
// ============================================================================

CServerTimer::CServerTimer()
{
}

// ============================================================================
///
///                        Инициализация объекта
///
// ============================================================================
/// Функция вызывается для начальной настройке драйвера. С учетом частоты MCK
/// драйвер настраивается для отсчета 1 мс интервала, разрешаются прерывания и 
/// регистрируются в AIC, включается тактирование в модуле PMC и запускается 
/// таймер
// ============================================================================

void CServerTimer::Configure (void)
{
    // Инициализация переменных
    Init_SysTimer();
    
    // Инициализация системного таймера
    ConfigureSysTick();
    // Инициализация таймера отсчета poll-интервалов
    ConfigurePollTimer();
    
    // Запуск сервера таймеров
    Start();
}

// ============================================================================
///
///                       Деинициализация объекта
///
// ============================================================================
/// Функция вызывается для завершения работы драйвера. Выключается тактирование
// ============================================================================

void CServerTimer::UnInit (void)
{
    StopPollTimer();
    StopSysTimer();
}

// ============================================================================
///
///                       Запуск сервера
///
// ============================================================================

void CServerTimer::Start (void)
{
    StartSysTimer();    
    StartPollTimer();
}

// ============================================================================
///
///                       Остановка сервера
///
// ============================================================================

void CServerTimer::Stop (void)
{
    StopPollTimer();
    StopSysTimer();
}

// ============================================================================
///
///               Установка приоритера прерываний модуля
///
// ============================================================================
/// \param  priority  Приоритет прерывания 
// ============================================================================

void CServerTimer::SetPriority (uint priority)
{
    #if defined(__CORTEX__)
        #warning Не раализовано
    #else
    tc0.SetPriority(priority);    
    #endif
}

// ============================================================================
///
///             Конфигурирование системного таймера 
///
// ============================================================================

void CServerTimer::ConfigureSysTick (void)
{
    // Инициализация и основного системного таймера
    #if defined(__CORTEX__)
        // Счетчик таймера, для генерации 1 мс прерываний
        // Важно: Для отсчета N интервалов, значение счетчика должно быть N-1
        #define  SYSTICK_RELOAD_VALUE       ((MckClock / 1000) - 1)
    
        // Выключение таймера и настройка частоты тактирования (MCK)
        AT91C_BASE_SYSTICK->CTRL = AT91C_SYSTICK_MCK;
        // Сброс текущего значения
        AT91C_BASE_SYSTICK->VAL &= ~AT91C_SYSTICK_VAL_MASK;
        // Инициализация значения перезагрузки счетчика
        AT91C_BASE_SYSTICK->LOAD = SYSTICK_RELOAD_VALUE;
        // Установка обработчика прерываний
        set_handler_aic(AT91C_ID_SYSTICK, handlerSysTimer);
    #else
    tc0.Configure(1000, handlerSysTimer, INT_PRIORITY_BACKGROUND, 0);
    #endif
}

// ============================================================================
///
///             Конфигурирование таймера poll-интервалов
///
// ============================================================================

void CServerTimer::ConfigurePollTimer (void)
{
    #if !defined(__CORTEX__)
    // В качестве таймера poll-интервалов используется таймер PIT (SAM7)
    // Его тактовая частота: fpoll = fPIT = MCK / 16 (см. описание AT91SAM7S)
    // Тогда, 
    //    Tpoll = 1 / fpoll
    //    Xмкс  = 1 мкс / Tpoll = fpoll * 1 мкс = (MCK / 16) / 1000000
    // Аналогично для мс
    //    Xмкс  = (MCK / 16) / 1000000
    //    Xмc   = (MCK / 16) / 1000
    
    // Расчет количество тиков для 1 мс
    g_TickCount_us = (MckClock / 16) / 1000000;
    g_TickCount_ms = (MckClock / 16) / 1000;   

    // Конфигурация таймера PIT на полный цикл
    AT91C_BASE_PITC->PITC_PIMR = 0x000FFFFF;     
    #else
    SCB_DEMCR   |= 0x01000000;
    DWT_CYCCNT   = 0; 
    
    // Расчет количество тиков для 1 мс
    g_TickCount_us = (MckClock) / 1000000;
    g_TickCount_ms = (MckClock) / 1000;   
    #endif
    
    if (g_TickCount_us == 0) g_TickCount_us = 1;
    if (g_TickCount_ms == 0) g_TickCount_ms = 1;    
}

// ============================================================================
///
///                   Запуск таймера poll-интервалов
///
// ============================================================================

void CServerTimer::StartPollTimer (void)
{
    #if defined(__CORTEX__)
    DWT_CONTROL |= 1;
    #else
    AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITEN;        
    #endif
}

// ============================================================================
///
///                   Остановка таймера poll-интервалов
///
// ============================================================================

void CServerTimer::StopPollTimer (void)
{
    #if defined(__CORTEX__)
    DWT_CONTROL &= ~1;
    #else
    AT91C_BASE_PITC->PITC_PIMR &= ~AT91C_PITC_PITEN;            
    #endif
}

// ============================================================================
///
///                   Запуск системного таймера 
///
// ============================================================================

void CServerTimer::StartSysTimer (void)
{
    #if defined(__CORTEX__)
    AT91C_BASE_SYSTICK->CTRL = AT91C_SYSTICK_MCK | AT91C_SYSTICK_ENABLE | AT91C_SYSTICK_INT;    
    #else
    tc0.Start();
    #endif
}

// ============================================================================
///
///                   Остановка системного таймера
///
// ============================================================================

void CServerTimer::StopSysTimer (void)
{
    #if defined(__CORTEX__)
    AT91C_BASE_SYSTICK->CTRL = AT91C_SYSTICK_MCK;        
    #else
    tc0.Stop();
    #endif
}

// ============================================================================
///
///                      Миллисекундная задержка
///
// ============================================================================
/// \param  time  Длительность задержки в мс [0..60000]
// ============================================================================

__RAMFUNC void delay_ms (uint time)
{
    #if !defined(__DEBUG__)
    uint curr    = GET_TICK_COUNT();    
    uint timeout = curr + time * g_TickCount_ms;
    
    while ( time_before(curr, timeout) )
        curr = GET_TICK_COUNT();
    #endif
}

// ============================================================================
///
///                      Микросекундная задержка
///
// ============================================================================
/// Рекомендуется использовать эту функцию при задержках больших 10 мкс, из-за
/// погрешности вызова и погрешности внутреннего расчета. Либо вызывать функцию
/// с уже временем, учитывающим эту погрешность
// ============================================================================
/// \param  time  Длительность задержки в мкс [0..60000] (погрешность 1 мкс)
// ============================================================================

__RAMFUNC void delay_us (uint time)
{
    #if !defined(__DEBUG__)    
    uint curr    = GET_TICK_COUNT();    
    uint timeout = curr + time * g_TickCount_us;
    
    while ( time_before(curr, timeout) )
        curr = GET_TICK_COUNT();
    #endif
}

// ============================================================================
///
///                  Чтение текущего количества тактов 
///
// ============================================================================
/// В качестве хранилища тактов процессора используется регистр PIT_IR
// ============================================================================
/// \return   Текущее количество тактов таймера 
// ============================================================================

uint get_cycles (void)
{
    return GET_TICK_COUNT();    
}

// ============================================================================
///
///                  Расчет разницы между 2-я тактами
///
// ============================================================================
/// \param   cycles_1  Значение тактов 1
/// \param   cycles_2  Значение тактов 2 (при 0 берется текущее значение тактов)
/// \return            Разница между двумя значениями 
// ============================================================================

uint diff_cycles (uint cycles_1, uint cycles_2)
{
    uint result;
    
    if (cycles_2 == 0)
        cycles_2 = GET_TICK_COUNT();
    
    if (cycles_1 < cycles_2)
        result = cycles_2 - cycles_1;
    else
        result = cycles_2 - cycles_1;
    
    return result;
}

// ============================================================================
///
///                  Расчет разницы между 2-я тактами
///
// ============================================================================
/// \param   cycles_1  Значение тактов 1
/// \param   cycles_2  Значение тактов 2 (при -1 берется текущее значение тактов)
/// \return            Разница между двумя значениями (us)
// ============================================================================

float diff_cycles_us (uint cycles_1, uint cycles_2)
{
    uint diff = diff_cycles(cycles_1, cycles_2);
    
    float result = cycles_to_us(diff);
    
    return result;
}

// ============================================================================
///
///                          Расчет длительность тактов
///
// ============================================================================
/// \param   cycles  Значение тактов
/// \return          Количество секунд
// ============================================================================

float cycles_to_time (uint cycles)
{
    float time = ((float)(cycles * 16)) / MckClock;
    
    return time;
}

// ============================================================================
///
///            Пересчет количества тактов в микросекундый интервал
///
// ============================================================================
/// \param   cycles  Значение тактов
/// \return          Количество микросекунд
// ============================================================================

float cycles_to_us (uint cycles)
{
    float time = cycles_to_time(cycles);
    
    time *= 1000000;
    
    return time;
}

// ============================================================================
///
///            Пересчет количества тактов в миллисекундный интервал
///
// ============================================================================
/// \param   cycles  Значение тактов
/// \return          Количество миллисекунд
// ============================================================================

float cycles_to_ms (uint cycles)
{
    float time = cycles_to_time(cycles);
    
    time *= 1000;
    
    return time;
}

// ============================================================================
///
///               Чтение количества мс с начала загрузки системы
///
// ============================================================================
/// \return  Количество секунд с начала загрузки системы
// ============================================================================

uint32 get_jiffies (void)
{
    return ServerTimer.jiffies;
}

// ============================================================================
///
///       Строка общего количество секунд с начала загрузки системы
///
// ============================================================================
/// \return         Указатель на строку
// ============================================================================

char* get_jiffies_str (void)
{
    return diff_jiffies_str(ServerTimer.jiffies, 0); 
}

// ============================================================================
///
/// Преобразование разницы времени в строковое представление
///
// ============================================================================
/// \param  after   Значение времени ПОСЛЕ (секунды)
/// \param  before  Значение времени ДО (секунды)
/// \return         Указатель на строку
// ============================================================================

char* diff_jiffies_str (uint32 after, uint32 before)
{
    static uchar str[sizeof("-49710d:23h:59m:59s")];
    uint32 delta;
    bool   minus;
    uint   temp;
    uint   day;
    uint   hour;
    uint   min;
    uint   sec;    
    
    if (after > before) { delta = after - before; minus = false; }
    else                { delta = before - after; minus = true;  }
    
    day    = delta / (24 * 60 * 60);
    temp   = delta - day * (24 * 60 * 60);

    hour   = temp / (60 * 60);    
    temp   = temp - hour * (60 * 60);
    
    min    = temp / (60);    
    sec    = temp - min * (60);
    
    uchar* ptr  = str;
    bool   next = false;
    
    if (minus)
    {
        *ptr++ = '-';
    }
    if (day)
    {
        ptr = PutInt(ptr, day); 
        ptr[0] = 'd'; ptr[1] = ':'; ptr += 2;
        next = true;
    }
    if (hour || next)
    {
        ptr = PutInt(ptr, hour, 2); 
        ptr[0] = 'h'; ptr[1] = ':'; ptr += 2;
        next = true;
    }
    if (min || next)
    {
        ptr = PutInt(ptr, min, 2); 
        ptr[0] = 'm'; ptr[1] = ':'; ptr += 2;
        next = true;
    }
    if (1)
    {
        ptr = PutInt(ptr, sec, 2); 
        ptr[0] = 's'; ptr += 1;
    }
    
    ptr[0] = '\0';
    
    return (char*)str;
}


