//*****************************************************************************
//
// ��� �����    : 'systimer.cpp'
// ���������    : ������� ������ ��������� �������� 
// �����        : ������� �.�. 
// ��������     : kav@nppstels.ru
// ����         : 04/10/2010
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
//         ����������� �� ������������ ���������� 10 �� �������� 
// ============================================================================

#if ( COUNT_MS10 > 10 )
  #error -------- �� ������ ���������� 10�� �������� COUNT_MS10[0..10] --------
#endif

    #define QUOTA_MS10     (10 - COUNT_MS10)
    #define QUOTA_MS100   ((10 - COUNT_MS10) * 10) 
    #define QUOTA_SEC    (((10 - COUNT_MS10) * 10) - COUNT_MS10) * 10

    #if (QUOTA_MS10 < 1) && (COUNT_MS100 != 0 || COUNT_SEC != 0 || COUNT_MIN != 0)
        #error -------- �� �������� ������ ����� 10 �� ������� --------        
    #endif      

    #if QUOTA_MS100 < 1 
        #error -------- �� �������� ������ ��� ���������� ������� --------  
    #endif

    #if QUOTA_SEC < 1 
        #error -------- �� �������� ������ ��� ��������� ������� --------  
    #endif

    // ���������� ����������� ������� ������ 0-� handle
    #define  COUNT_MS_IN     (COUNT_MS + 1)
    // ����� ���������� �������� � ������� (1 - �������������� ��� 0-�� �������)
    #define  COUNT_TIMERS    (COUNT_MS_IN + COUNT_MS10 + COUNT_MS100 + COUNT_SEC + COUNT_MIN)
    // ��������� ������ ���� �� ��� ������ �������
    #if (COUNT_TIMERS == 0)
        #undef   COUNT_MIN
        #undef   COUNT_TIMERS
        #define  COUNT_MIN        1
        #define  COUNT_TIMERS     1
    #endif

    #if defined(__CORTEX__)
        // ��� ������� ��������� ������� � poll, ������������ ������� ������ ������
        // Data Watchpoint and Trace (DWT), ������� ������� � ���� Cortex
        #define  DWT_CYCCNT                 *(volatile uint32*)0xE0001004
        #define  DWT_CONTROL                *(volatile uint32*)0xE0001000
        #define  SCB_DEMCR                  *(volatile uint32*)0xE000EDFC
        #define  GET_TICK_COUNT()           DWT_CYCCNT
        #define  SYSTIMER_DISABLE_INT()     AT91C_BASE_SYSTICK->CTRL &= ~AT91C_SYSTICK_INT
        #define  SYSTIMER_ENABLE_INT()      AT91C_BASE_SYSTICK->CTRL |= AT91C_SYSTICK_INT
    #else
        // ��� ��������� ������� ������������ 24-� ������ ������/������� PIT
        #define  GET_TICK_COUNT()           AT91C_BASE_PITC->PITC_PIIR
        #define  SYSTIMER_DISABLE_INT()     tc0.DisableIrq()
        #define  SYSTIMER_ENABLE_INT()      tc0.EnableIrq()
    #endif

// ============================================================================
//                          ��������� �������
// ============================================================================

    // ������������� ������
    void Init_SysTimer     (void);
    // ��� ���������� �������
    __RAMFUNC void Tick_SysTime      (void);
    // ���������� ���������� ����
    __RAMFUNC void IncDay_SysTimer   (void);
    // ��� ������� 
    __RAMFUNC void TickTimer_SysTime (TTimerRecord* ptr);
    // ���������� ���������� ����
    void IncDay_SysTimer   (void);
    

// ============================================================================
//                        ���������� ����������
// ============================================================================

    CServerTimer  ServerTimer;
    TTimerRecord  timerTable[COUNT_TIMERS];
    uint          g_TickCount_us;
    uint          g_TickCount_ms;    

// ============================================================================
//                             �-�������
// ============================================================================
    
// ============================================================================
///
///                          ��� ������� 
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
///                    ���������� ���������� �� �������
///
// ============================================================================

#if !defined(__DEBUG__) && !defined(__JTAG__)
    #pragma optimize = speed high
#endif
__RAMFUNC __ARM void handlerSysTimer (void)
{
    // ����� �������� ����������
    #if !defined(__CORTEX__)
    uint status = AT91C_BASE_TC0->TC_SR;             
    #endif    

    TPlatformType i;
    
    // =================================================
    //    ���������� ���������� ������� (�� 1 mS)
    // =================================================
    Tick_SysTime();
    
    // =================================================
    //           ������� ����� ��������
    // =================================================  
    
    // ������ � ���������� 1 �� ��������
    #if (COUNT_MS_IN > 1)
    i = 1; while (i < COUNT_MS_IN)
    {
        TickTimer_SysTime(&timerTable[i]); i++;
    }
    #endif

    // ������ � ���������� 10 �� ��������
    // ==================================
    i = ServerTimer.count_ms10;
    if ( i ) 
    {
        TickTimer_SysTime(&timerTable[i + COUNT_MS_IN - 1]); 
        ServerTimer.count_ms10 = --i;
        return;
    }
    
    // ������ � ���������� 100 �� ��������
    // ===================================
    i = ServerTimer.count_ms100;
    if ( i ) 
    {
        TickTimer_SysTime(&timerTable[i + COUNT_MS_IN + COUNT_MS10 - 1]);
        ServerTimer.count_ms100 = --i;
        return;
    }
    
    // ������ � ���������� 1 � ��������
    // ===================================
    i = ServerTimer.count_sec;
    if ( i ) 
    {
        TickTimer_SysTime(&timerTable[i + COUNT_MS_IN + COUNT_MS10 + COUNT_MS100 - 1]);
        ServerTimer.count_sec = --i;
        return;
    }
    
    // ������ � ���������� 1 ��� ��������
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
///                       ��� ���������� �������
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
        ptr->ms         = 0;                            // �������� 10 ��
        ptr->count_ms10 = COUNT_MS10;                   //  
        if (++ptr->ms10 > 9)                            //
        {                                               //
            ptr->ms10        = 0;                       // �������� 100 ��
            ptr->count_ms100 = COUNT_MS100;             //    
            if (++ptr->ms100 > 9)                       //
            {                                           //
                ServerTimer.jiffies++;                  // <���������� ��������� ����� �������>
                ptr->ms100     = 0;                     // �������� 1 �            
                ptr->count_sec = COUNT_SEC;             //   
                if (++ptr->sec > 59)                    //
                {                                       //
                    ptr->sec       = 0;                 // �������� 1 ���
                    ptr->count_min = COUNT_MIN;         // 
                    if (++ptr->min > 59)                //
                    {                                   //    
                        ptr->min = 0;                   // �������� 1 ���
                        if (++ptr->hour > 23)           //
                        {                               //
                            ptr->hour = 0;              // 
                            IncDay_SysTimer();          // �������� 1 ����
                        } // end if (��������� ����)    //
                    } // end if (��������� ���)         //
                } // end if (��������� ������)          //
            } // enf if (��������� �������)             //
        } // end if (��������� 100 ��)                  //
    } // end if (��������� 10��) 
} 

// ============================================================================
///
///                       ���������� ���������� ����
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
        // ������� � 31 
        case 32: month++; day = 1; break;
        // ������� � 30
        case 31: 
            if ((month == 4) || (month == 6) || (month == 9) || (month == 11))
            {
                month++; day = 1;
            }
            break;
        // ������� � 29
        case 30: 
            if (month == 2)
            {
                month++; day = 1;
            }
            break;
        // ������� � 28
        case 29: 
            if ((month == 2) && ((ServerTimer.year & 3) == 0))
            {
                month++; day = 1;
            }
            break;
    } // end switch (������ ���)

    // ���������� ����  
    if (month == 13)
    {
        month = 1; ++ServerTimer.year;
    } 
  
    ServerTimer.day   = day;
    ServerTimer.month = month;
    
    // ���������� ��� ������
    if (++ServerTimer.wday > 7) ServerTimer.wday = 1;
}

// ============================================================================
///
///                       ������������� ������
///
// ============================================================================
/// ��������� ��������� ����� ��������
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
    // 20111004 - �������������� 0-�� �������
    // ===========================================    
    // � ������ ���� ��������� ������ �������� � �������� � Handle = 0, � ����
    // �������� ������ ���������� ���������� handle = 0, ��� ������ ����� �������� 
    // ������ ������
    //
        timerTable[0].param = (void*)(0xBABA); 
    //
    // ===========================================        
}

// ============================================================================
///
///                       �������� �������
///
// ============================================================================
/// \param  type     ��� ������� (������������)
/// \param  pFunc    ��������� �� callback-�������
/// \param  param  ��������� �� ������/�������������� ���� ����������
/// \return ���������� ������������� �������
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
    } // end switch (������ ���� �������)
    
    // ����� ���������� �������
    while (count) 
    {
        if (pTimer->param == (void*)(-1)) 
        {
            // ��������� ������ ������: ������������� ���������
            pTimer->count = 0;
            pTimer->pFunc = pFunc;  
            pTimer->param = param;
            
            // ������ �������
            return (TTimerHandle)index;                                              
        } // end if ( ptr )      
        
        // ����� ���������� �������    
        count--; pTimer++; index++;                                                   
    } // end while (i)
    
    // �� ������� ��������
    while(1);
}

// ============================================================================
///
///                       �������� �������
///
// ============================================================================
/// \param  type  ��� ������� (������������)
/// \param  info  �������������� ���� ����������
/// \return ���������� ������������� �������
// ============================================================================

TTimerHandle CreateTimer (TTypeTimer type, uint info)
{
    return CreateTimer(type, 0, (void*)info);
}

// ============================================================================
///
///                          ������ �������
///
// ============================================================================
/// \param  handle  ���������� ������������� �������
/// \param  time    ����� �� ������� ����������� ������ (� ��������� �������)
// ============================================================================

void StartTimer (TTimerHandle handle, TTimerData time)
{
    timerTable[handle].count = time;
}

// ============================================================================
///
///                          ��������� �������
///
// ============================================================================
/// \param  handle  ���������� ������������� �������
// ============================================================================
  
void StopTimer (TTimerHandle handle)
{
    timerTable[handle].count = 0;
}
  
// ============================================================================
///
///          �������� ��������� ���������� ��������� �������
///
// ============================================================================
/// \param  handle  ���������� ������������� �������
/// \return ������� �������� �������� ������� (0 - ������ �������� ������)
// ============================================================================
  
TTimerData CheckTimer (TTimerHandle handle)
{
    return timerTable[handle].count;
}

// ============================================================================
///
///                    ������ �������� ���������� �������
///
// ============================================================================
/// \return  ������� ��������� ����� � ����������� �������
// ============================================================================

TTimePacked GetSysTime (void)
{
    // ������ ����������
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

    // ���������� ����������    
    SYSTIMER_ENABLE_INT();    
    
    return time;
}

// ============================================================================
///
///                    ������ �������� ���������� �������
///
// ============================================================================
/// \return  ������� ��������� ����� � ������������� �������
// ============================================================================

TTimeStamp GetSysTimeUnpack (void)
{
    TTimePacked pack = GetSysTime();
    TTimeStamp  t    = UnpackTime(pack);
    
    return t;    
}

// ============================================================================
///
///                      ��������� ���������� �������
///
// ============================================================================
/// \param  year   ��� (2000 - 2xxx)
/// \param  month  ����� (1 - 12)
/// \param  day    ���� (1 - 31)
/// \param  hour   ���
/// \param  min    ������
/// \param  sec    �������
// ============================================================================

void SetSysTime (uint year, uint month, uint day, uint hour, uint min, uint sec)
{
    // ������ ����������
    SYSTIMER_DISABLE_INT();

    if (year < 2000)  year += 2000;
    
    ServerTimer.year  = year;
    ServerTimer.month = month;
    ServerTimer.day   = day;
    ServerTimer.hour  = hour;
    ServerTimer.min   = min;
    ServerTimer.sec   = sec;

    // ���������� ����������    
    SYSTIMER_ENABLE_INT();    
    
    // ������������� ��� ������
    ServerTimer.wday  = WhatDayTime(ServerTimer.day, ServerTimer.month, ServerTimer.year);
}

// ============================================================================
///
///                     ��������� ���������� �������
///
// ============================================================================
/// \param  time  ����� ��������� �����
// ============================================================================

void SetSysTime (TTimePacked time)
{
    SetSysTime(time.year, time.month, time.day, time.hour, time.min, time.sec);
}

// ============================================================================
///
///                    ��������� �������� ���������� �������
///
// ============================================================================
/// \param  time  ����� ��������� ����� � ������������� �������
// ============================================================================

void SetSysTimeUnpack (TTimeStamp& time)
{
    TTimePacked t = PackTime(time);
    
    SetSysTime(t);    
}

// ============================================================================
///
///      ������ �������� ���������� ������� (��������� �������������)
///
// ============================================================================
/// \return  ��������� �� �������������� ������
// ============================================================================

uchar* GetSysTimeStr (ETimeFormat format)
{
    // ������ ����������
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
    
    // ���������� ����������    
    SYSTIMER_ENABLE_INT();     
    
    uchar* str = TimeToStr(time, format);
    
    return str;
}

// ============================================================================
///
///                        ����������� �������
///
// ============================================================================

CServerTimer::CServerTimer()
{
}

// ============================================================================
///
///                        ������������� �������
///
// ============================================================================
/// ������� ���������� ��� ��������� ��������� ��������. � ������ ������� MCK
/// ������� ������������� ��� ������� 1 �� ���������, ����������� ���������� � 
/// �������������� � AIC, ���������� ������������ � ������ PMC � ����������� 
/// ������
// ============================================================================

void CServerTimer::Configure (void)
{
    // ������������� ����������
    Init_SysTimer();
    
    // ������������� ���������� �������
    ConfigureSysTick();
    // ������������� ������� ������� poll-����������
    ConfigurePollTimer();
    
    // ������ ������� ��������
    Start();
}

// ============================================================================
///
///                       ��������������� �������
///
// ============================================================================
/// ������� ���������� ��� ���������� ������ ��������. ����������� ������������
// ============================================================================

void CServerTimer::UnInit (void)
{
    StopPollTimer();
    StopSysTimer();
}

// ============================================================================
///
///                       ������ �������
///
// ============================================================================

void CServerTimer::Start (void)
{
    StartSysTimer();    
    StartPollTimer();
}

// ============================================================================
///
///                       ��������� �������
///
// ============================================================================

void CServerTimer::Stop (void)
{
    StopPollTimer();
    StopSysTimer();
}

// ============================================================================
///
///               ��������� ���������� ���������� ������
///
// ============================================================================
/// \param  priority  ��������� ���������� 
// ============================================================================

void CServerTimer::SetPriority (uint priority)
{
    #if defined(__CORTEX__)
        #warning �� �����������
    #else
    tc0.SetPriority(priority);    
    #endif
}

// ============================================================================
///
///             ���������������� ���������� ������� 
///
// ============================================================================

void CServerTimer::ConfigureSysTick (void)
{
    // ������������� � ��������� ���������� �������
    #if defined(__CORTEX__)
        // ������� �������, ��� ��������� 1 �� ����������
        // �����: ��� ������� N ����������, �������� �������� ������ ���� N-1
        #define  SYSTICK_RELOAD_VALUE       ((MckClock / 1000) - 1)
    
        // ���������� ������� � ��������� ������� ������������ (MCK)
        AT91C_BASE_SYSTICK->CTRL = AT91C_SYSTICK_MCK;
        // ����� �������� ��������
        AT91C_BASE_SYSTICK->VAL &= ~AT91C_SYSTICK_VAL_MASK;
        // ������������� �������� ������������ ��������
        AT91C_BASE_SYSTICK->LOAD = SYSTICK_RELOAD_VALUE;
        // ��������� ����������� ����������
        set_handler_aic(AT91C_ID_SYSTICK, handlerSysTimer);
    #else
    tc0.Configure(1000, handlerSysTimer, INT_PRIORITY_BACKGROUND, 0);
    #endif
}

// ============================================================================
///
///             ���������������� ������� poll-����������
///
// ============================================================================

void CServerTimer::ConfigurePollTimer (void)
{
    #if !defined(__CORTEX__)
    // � �������� ������� poll-���������� ������������ ������ PIT (SAM7)
    // ��� �������� �������: fpoll = fPIT = MCK / 16 (��. �������� AT91SAM7S)
    // �����, 
    //    Tpoll = 1 / fpoll
    //    X���  = 1 ��� / Tpoll = fpoll * 1 ��� = (MCK / 16) / 1000000
    // ���������� ��� ��
    //    X���  = (MCK / 16) / 1000000
    //    X�c   = (MCK / 16) / 1000
    
    // ������ ���������� ����� ��� 1 ��
    g_TickCount_us = (MckClock / 16) / 1000000;
    g_TickCount_ms = (MckClock / 16) / 1000;   

    // ������������ ������� PIT �� ������ ����
    AT91C_BASE_PITC->PITC_PIMR = 0x000FFFFF;     
    #else
    SCB_DEMCR   |= 0x01000000;
    DWT_CYCCNT   = 0; 
    
    // ������ ���������� ����� ��� 1 ��
    g_TickCount_us = (MckClock) / 1000000;
    g_TickCount_ms = (MckClock) / 1000;   
    #endif
    
    if (g_TickCount_us == 0) g_TickCount_us = 1;
    if (g_TickCount_ms == 0) g_TickCount_ms = 1;    
}

// ============================================================================
///
///                   ������ ������� poll-����������
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
///                   ��������� ������� poll-����������
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
///                   ������ ���������� ������� 
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
///                   ��������� ���������� �������
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
///                      �������������� ��������
///
// ============================================================================
/// \param  time  ������������ �������� � �� [0..60000]
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
///                      �������������� ��������
///
// ============================================================================
/// ������������� ������������ ��� ������� ��� ��������� ������� 10 ���, ��-��
/// ����������� ������ � ����������� ����������� �������. ���� �������� �������
/// � ��� ��������, ����������� ��� �����������
// ============================================================================
/// \param  time  ������������ �������� � ��� [0..60000] (����������� 1 ���)
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
///                  ������ �������� ���������� ������ 
///
// ============================================================================
/// � �������� ��������� ������ ���������� ������������ ������� PIT_IR
// ============================================================================
/// \return   ������� ���������� ������ ������� 
// ============================================================================

uint get_cycles (void)
{
    return GET_TICK_COUNT();    
}

// ============================================================================
///
///                  ������ ������� ����� 2-� �������
///
// ============================================================================
/// \param   cycles_1  �������� ������ 1
/// \param   cycles_2  �������� ������ 2 (��� 0 ������� ������� �������� ������)
/// \return            ������� ����� ����� ���������� 
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
///                  ������ ������� ����� 2-� �������
///
// ============================================================================
/// \param   cycles_1  �������� ������ 1
/// \param   cycles_2  �������� ������ 2 (��� -1 ������� ������� �������� ������)
/// \return            ������� ����� ����� ���������� (us)
// ============================================================================

float diff_cycles_us (uint cycles_1, uint cycles_2)
{
    uint diff = diff_cycles(cycles_1, cycles_2);
    
    float result = cycles_to_us(diff);
    
    return result;
}

// ============================================================================
///
///                          ������ ������������ ������
///
// ============================================================================
/// \param   cycles  �������� ������
/// \return          ���������� ������
// ============================================================================

float cycles_to_time (uint cycles)
{
    float time = ((float)(cycles * 16)) / MckClock;
    
    return time;
}

// ============================================================================
///
///            �������� ���������� ������ � ������������� ��������
///
// ============================================================================
/// \param   cycles  �������� ������
/// \return          ���������� �����������
// ============================================================================

float cycles_to_us (uint cycles)
{
    float time = cycles_to_time(cycles);
    
    time *= 1000000;
    
    return time;
}

// ============================================================================
///
///            �������� ���������� ������ � �������������� ��������
///
// ============================================================================
/// \param   cycles  �������� ������
/// \return          ���������� �����������
// ============================================================================

float cycles_to_ms (uint cycles)
{
    float time = cycles_to_time(cycles);
    
    time *= 1000;
    
    return time;
}

// ============================================================================
///
///               ������ ���������� �� � ������ �������� �������
///
// ============================================================================
/// \return  ���������� ������ � ������ �������� �������
// ============================================================================

uint32 get_jiffies (void)
{
    return ServerTimer.jiffies;
}

// ============================================================================
///
///       ������ ������ ���������� ������ � ������ �������� �������
///
// ============================================================================
/// \return         ��������� �� ������
// ============================================================================

char* get_jiffies_str (void)
{
    return diff_jiffies_str(ServerTimer.jiffies, 0); 
}

// ============================================================================
///
/// �������������� ������� ������� � ��������� �������������
///
// ============================================================================
/// \param  after   �������� ������� ����� (�������)
/// \param  before  �������� ������� �� (�������)
/// \return         ��������� �� ������
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


