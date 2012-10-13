//*****************************************************************************
//
// ��� �����    : 'systimer.cpp'
// ���������    : ������� ������ ��������� �������� 
// �����        : ������� �.�. 
// ��������     : kav@nppstels.ru
// ����         : 04/10/2010
//
// ������� �������� �� ���� ����������� ������� TC0. 
// ��������� �������, ������������� ������������ ���������� ����������� ��������������
// ��������� ������� ����� ��������, �������, ��������� ��������. ����������� 
// ������������ ������� 1 ��. ������� ����������� �������������� ��������� � �����������:
// 1 ��, 10 ��, 100 ��, 1 ���, 1 ���
// ���������� �������� ������������ ������������� ��� ������� � ����� "def_timer.h"
// ������� ������ �������� ������� �� ���������� ����������. �� ������������� 
// ������ ����������� � ������.
//
// ����         : 07/11/2010 
// ��������     : ��������� ������� delay_ms(), delay_us() � ������ "poll". ���
// ������� ���������� ������������ ������ PITC ��� ����������
// 
//*****************************************************************************

#ifndef _SYSTIMER_H
#define _SYSTIMER_H

#include "clib\time.h"

// ============================================================================
//                             ������� 
// ============================================================================

    // ��������� 2-� ���������� (� �������������)
    // ���� "a" > "b" ��������� true
    #define time_after(a,b)     ((sint)((sint)(b) - (sint)(a)) < 0)
    // ���� "a" < "b" ��������� true
    #define time_before(a,b)    time_after(b,a)
    // ���� "a" >= "b" ��������� true
    #define time_after_eq(a,b)  ((sint)((sint)(a) - (sint)(b)) >= 0)
    // ���� "a" <= "b" ��������� true
    #define time_before_eq(a,b) time_after_eq(b,a)

// ============================================================================
///                            ���������
// ============================================================================

    // ��������� ������������ ��������
    typedef enum ETypeTimer {tMS, tMS10, tMS100, tSEC, tMIN} TTypeTimer;
    // Callback-������� ���������� �������
    typedef uint (TCallbackTimer)(void* pObject);
    // ���������� ������������� �������
    typedef uint            TTimerHandle;
    // ��� �������� �������
    typedef uint            TTimerData;

    // ��������� ������ �������        
    typedef struct _TTimerRecord
    {
        // ������� �������
        TTimerData      count;
        // Callback-������� (���������� �������)
        TCallbackTimer* pFunc; 
        // �������������� ��������
        void*           param;
        
    } TTimerRecord;
    
// ============================================================================
///                             ������
// ============================================================================

class CServerTimer
{
    private:
        // ���������������� ���������� ������� 
        void  ConfigureSysTick   (void);
        // ���������������� ������� poll-����������
        void  ConfigurePollTimer (void);
        // ������ ���������� ������� 
        void  StartSysTimer      (void);
        // ��������� ���������� �������
        void  StopSysTimer       (void);
        // ������ ������� poll-����������
        void  StartPollTimer     (void);
        // ��������� ������� poll-����������
        void  StopPollTimer      (void);
    public:
        // ���������� ��������� ����� (������� �������� �������)
        uint32          jiffies;
        // ���������� ��������� �������
        TPlatformType   ms;
        TPlatformType   ms10;
        TPlatformType   ms100;
        TPlatformType   sec;
        TPlatformType   min;
        TPlatformType   hour;
        TPlatformType   day;
        TPlatformType   month;
        TPlatformType   year;
        TPlatformType   wday;        
        // ���������� ���������� (������ ����������)
        TPlatformType   count_ms10;        
        TPlatformType   count_ms100;
        TPlatformType   count_sec;
        TPlatformType   count_min;
        
        // �����������
        CServerTimer();
        // ������������� ������
        void Configure   (void);
        // ��������������� �������
        void UnInit      (void);
        
        // ������ �������
        void Start       (void);
        // ��������� �������
        void Stop        (void);
        // ��������� ���������� ���������� ������
        void SetPriority (uint priority);
};

// ============================================================================
///                          ��������� �������
// ============================================================================

    // �������� �������
    TTimerHandle CreateTimer      (TTypeTimer type, TCallbackTimer* pFunc = 0, void* pObject = 0);
    // �������� �������
    TTimerHandle CreateTimer      (TTypeTimer type, uint info);
    // ������ �������
    void         StartTimer       (TTimerHandle handle, TTimerData time);
    // ��������� �������
    void         StopTimer        (TTimerHandle handle);
    // �������� ��������� ���������� ��������� �������
    TTimerData   CheckTimer       (TTimerHandle handle);
    
    // ������ �������� ���������� ������� � ����������� �������
    TTimePacked  GetSysTime       (void);
    // ������ �������� ���������� ������� � ������������� �������
    TTimeStamp   GetSysTimeUnpack (void);
    // ��������� ���������� �������
    void         SetSysTime       (uint year, uint month, uint day, uint hour, uint min, uint sec);
    // ��������� ���������� �������
    void         SetSysTime       (TTimePacked time);
    // ��������� �������� ���������� �������
    void         SetSysTimeUnpack (TTimeStamp& time);
    
    // ������������ ������ ��������� �������  
    uchar*       GetSysTimeStr    (ETimeFormat format = TIME_FORMAT_DATE_TIME);                              
    
    // �������������� ��������
    void         delay_ms         (uint time);
    // �������������� ��������
    void         delay_us         (uint time);
    // ������ �������� �������� �������� ������ ����������
    uint         get_cycles       (void);
    // ����������� ������������ ������
    float        cycles_to_time   (uint cycles);
    // ������ ������� ����� 2-� �������
    uint         diff_cycles      (uint cycles_1, uint cycles_2);
    // ������ ������� ����� 2-� �������
    float        diff_cycles_us   (uint cycles_1, uint cycles_2 = 0);
    // �������� ���������� ������ � ������������� ��������
    float        cycles_to_us     (uint cycles);
    // �������� ���������� ������ � �������������� ��������
    float        cycles_to_ms     (uint cycles);
    // ������ ���������� ������ � ������ �������� �������
    uint32       get_jiffies      (void);
    // ������ ������ ���������� ������ � ������ �������� �������
    char*        get_jiffies_str  (void);
    // �������������� ������� ������� � ��������� �������������
    char*        diff_jiffies_str (uint32 after, uint32 before);
    
// ============================================================================
///                          ������� �����������
// ============================================================================

    extern CServerTimer ServerTimer;
    
#endif  // #ifndef _SYSTIMER_H
    