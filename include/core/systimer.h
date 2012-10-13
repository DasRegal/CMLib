//*****************************************************************************
//
// Имя файла    : 'systimer.cpp'
// Заголовок    : Драйвер модуля системных таймеров 
// Автор        : Крыцкий А.В. 
// Контакты     : kav@nppstels.ru
// Дата         : 04/10/2010
//
// Драйвер построен на базе аппаратного таймера TC0. 
// Системные таймера, предоставляют пользователю библиотеки возможность контролировать
// интервалы времени путем создания, запуска, остановки таймеров. Минимальная 
// дискретность таймера 1 мс. Имеется возможность манипулировать таймераме с разрешением:
// 1 мс, 10 мс, 100 мс, 1 сек, 1 мин
// Количество таймеров определяется индивидуально для проекта в файле "def_timer.h"
// Принцип работы драйвера основан на уникальной реализации. За подробностями 
// работы обращайтесь к автору.
//
// Дата         : 07/11/2010 
// Описание     : Добавлены функции delay_ms(), delay_us() в режиме "poll". Для
// отсчета интервалов используется таймер PITC без прерываний
// 
//*****************************************************************************

#ifndef _SYSTIMER_H
#define _SYSTIMER_H

#include "clib\time.h"

// ============================================================================
//                             Макросы 
// ============================================================================

    // Сравнение 2-х переменных (с переполнением)
    // Если "a" > "b" результат true
    #define time_after(a,b)     ((sint)((sint)(b) - (sint)(a)) < 0)
    // Если "a" < "b" результат true
    #define time_before(a,b)    time_after(b,a)
    // Если "a" >= "b" результат true
    #define time_after_eq(a,b)  ((sint)((sint)(a) - (sint)(b)) >= 0)
    // Если "a" <= "b" результат true
    #define time_before_eq(a,b) time_after_eq(b,a)

// ============================================================================
///                            Структуры
// ============================================================================

    // Возможные дискретности таймеров
    typedef enum ETypeTimer {tMS, tMS10, tMS100, tSEC, tMIN} TTypeTimer;
    // Callback-функция обработчик таймера
    typedef uint (TCallbackTimer)(void* pObject);
    // Уникальный идентификатор таймера
    typedef uint            TTimerHandle;
    // Тип счетчика таймера
    typedef uint            TTimerData;

    // Структура данных таймера        
    typedef struct _TTimerRecord
    {
        // Счетчик таймера
        TTimerData      count;
        // Callback-фукнция (обработчик таймера)
        TCallbackTimer* pFunc; 
        // Дополнительный параметр
        void*           param;
        
    } TTimerRecord;
    
// ============================================================================
///                             Классы
// ============================================================================

class CServerTimer
{
    private:
        // Конфигурирование системного таймера 
        void  ConfigureSysTick   (void);
        // Конфигурирование таймера poll-интервалов
        void  ConfigurePollTimer (void);
        // Запуск системного таймера 
        void  StartSysTimer      (void);
        // Остановка системного таймера
        void  StopSysTimer       (void);
        // Запуск таймера poll-интервалов
        void  StartPollTimer     (void);
        // Остановка таймера poll-интервалов
        void  StopPollTimer      (void);
    public:
        // Количество секундных тиков (сначала загрузки системы)
        uint32          jiffies;
        // Переменные систеного времени
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
        // Внутренние переменные (отсчет интервалов)
        TPlatformType   count_ms10;        
        TPlatformType   count_ms100;
        TPlatformType   count_sec;
        TPlatformType   count_min;
        
        // Конструктор
        CServerTimer();
        // Инициализация модуля
        void Configure   (void);
        // Деинициализация объекта
        void UnInit      (void);
        
        // Запуск сервера
        void Start       (void);
        // Остановка сервера
        void Stop        (void);
        // Установка приоритера прерываний модуля
        void SetPriority (uint priority);
};

// ============================================================================
///                          Прототипы фукнций
// ============================================================================

    // Создание таймера
    TTimerHandle CreateTimer      (TTypeTimer type, TCallbackTimer* pFunc = 0, void* pObject = 0);
    // Создание таймера
    TTimerHandle CreateTimer      (TTypeTimer type, uint info);
    // Запуск таймера
    void         StartTimer       (TTimerHandle handle, TTimerData time);
    // Остановка таймера
    void         StopTimer        (TTimerHandle handle);
    // Проверка окончания ожидаемого интервала времени
    TTimerData   CheckTimer       (TTimerHandle handle);
    
    // Чтение текущего системного времени в упакованном формате
    TTimePacked  GetSysTime       (void);
    // Чтение текущего системного времени в неупакованном формате
    TTimeStamp   GetSysTimeUnpack (void);
    // Установка системного времени
    void         SetSysTime       (uint year, uint month, uint day, uint hour, uint min, uint sec);
    // Установка системного времени
    void         SetSysTime       (TTimePacked time);
    // Установка текущего системного времени
    void         SetSysTimeUnpack (TTimeStamp& time);
    
    // Формирование строки реального времени  
    uchar*       GetSysTimeStr    (ETimeFormat format = TIME_FORMAT_DATE_TIME);                              
    
    // Миллисекундная задержка
    void         delay_ms         (uint time);
    // Микросекундная задержка
    void         delay_us         (uint time);
    // Чтение текущего значения счетчика циклов процессора
    uint         get_cycles       (void);
    // Определение длительности циклов
    float        cycles_to_time   (uint cycles);
    // Расчет разницы между 2-я тактами
    uint         diff_cycles      (uint cycles_1, uint cycles_2);
    // Расчет разницы между 2-я тактами
    float        diff_cycles_us   (uint cycles_1, uint cycles_2 = 0);
    // Пересчет количества тактов в микросекундый интервал
    float        cycles_to_us     (uint cycles);
    // Пересчет количества тактов в миллисекундный интервал
    float        cycles_to_ms     (uint cycles);
    // Чтение количества секунд с начала загрузки системы
    uint32       get_jiffies      (void);
    // Строка общего количество секунд с начала загрузки системы
    char*        get_jiffies_str  (void);
    // Преобразование разницы времени в строковое представление
    char*        diff_jiffies_str (uint32 after, uint32 before);
    
// ============================================================================
///                          Внешине зависимости
// ============================================================================

    extern CServerTimer ServerTimer;
    
#endif  // #ifndef _SYSTIMER_H
    