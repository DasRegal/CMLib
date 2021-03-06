//*****************************************************************************
//
// ��� �����    : 'clock.cpp'
// ���������    : �������� ������ ������������
// �����        : ���������� �. �.
// ��������     : plexus_bra@rambler.ru
// ����         : 12.10.2012
//
//*****************************************************************************

#include "core\clock.h"
#include "core\processor.h"

// =============================================================================
//                                ��������
// =============================================================================

    // ���������� ������������ LFXT
    #define OSC_OFF         true
    #define OSC_ON          false
    #define XTS_LF          false
    #define XTS_HF          true

    // ���������� ������������ XT2
    #define XT2_OFF         true
    #define XT2_ON          false

// =============================================================================
//                           ���������� ����������
// =============================================================================

    // ������� Mck � �������
    uint32  MckClock;

// =============================================================================
//                             ��������� �������
// =============================================================================

    // ������������ LFXTCLK
    void ConfigLFXT(bool oscoff, bool xts);
    // ������������ XT2CLK
    void ConfigXT2(bool xt2off);
      
// ============================================================================
///    
///                          ������������ LFXTCLK
///
// ============================================================================
/// \param  oscoff  ���������� ����������� LFXT
/// \param  xts     ����� HF/LF
// ============================================================================

void ConfigLFXT(bool oscoff, bool xts)
{
    if (oscoff)
    {
        _BIS_SR(OSCOFF);
    }
    
    // ����� ������� ������� (��������� �������������� ������������)
    // xts = 0: ����� ������ �������
    // xts = 1: ����� ������� �������
    if (xts)
    {
        // ���������� ��� XTS
        BCSCTL1 |= XTS;
    }
    else
    {
        // �������� ��� XTS
        BCSCTL1 &= ~XTS;
    }
}

// ============================================================================
///    
///                           ������������ ACLCK
///
// ============================================================================
/// \param  diva - �������� 1/2/4/8
// ============================================================================

void ConfigACLCK(uchar diva)
{
    // ��������� ����������� � ���������� ��������
    ConfigLFXT(OSC_ON, XTS_LF);
    
    // ��������� �������� ��� ACLCK
    BCSCTL1 |= diva;
}

// ============================================================================
///    
///                           ������������ XT2CLK
///
// ============================================================================
/// \param  xt2off  ���������� XT2CLK
// ============================================================================

void ConfigXT2(bool xt2off)
{
    if (xt2off)
    {
        BCSCTL1 |= XT2OFF;
    }
    else
    {
        BCSCTL1 &= ~XT2OFF;
    }
}

// ============================================================================
///    
///             ������������ �������� ��������� ������������ MCLK
///
// ============================================================================
/// \param  divm    �������� 1/2/4/8
/// \param  source  ����� ��������� MCLK
/// \param  cpuoff  ���������� CPU
// ============================================================================

void ConfigMCLK(uchar divm, uchar source, bool cpuoff)
{
    // ��������� ����� SELMx � DIVMx
    BCSCTL2 |= source | divm;
    
    // ���������� CPU
    if (cpuoff)
    {
        _BIS_SR(CPUOFF);
    }
}

// ============================================================================
///    
///                            ������������ DCO
///
// ============================================================================
/// \param  frq  ������� DCO (1/8/12/16 ���)
// ============================================================================

void ConfigDCO(uint frq)
{
    // �������� ��������
    if (CALBC1_1MHZ == 0xFF || CALDCO_1MHZ == 0xFF)                                     
    {  
        while(1);
    } 

    // MSP430G2211 �� ������������ ������� ���� 1 ���
    if (frq != DCO_1MHZ)
    {
        
        #if defined __MSP430G2211__
            BCSCTL1 = CALBC1_1MHZ;
            DCOCTL = CALDCO_1MHZ;
            frq = 0x10FEu;
            #warning ��������� �� ������������ ������� ������ 1 ���
        #else
            DCOCTL = frq;
            BCSCTL1 = (frq + 1);
        #endif
    }
    else
    {
        BCSCTL1 = CALBC1_1MHZ;
        DCOCTL = CALDCO_1MHZ;
    }
    
    switch (frq)
    {
        // ������� CPU 1 ���
        case 0x10FEu: MckClock = 1000000;  break;
        // ������� CPU 8 ���
        case 0x10FCu: MckClock = 8000000;  break;
        // ������� CPU 12 ���
        case 0x10FAu: MckClock = 12000000; break;
        // ������� CPU 16 ���
        case 0x10F8u: MckClock = 16000000; break;
    }
    
    // ����������� ������ ����������� ��� 
    // �������� ��������� �� �����������
    
}

// ============================================================================
///    
///                          ������������ SMCLK
///
// ============================================================================
/// \param  divs    �������� 1/2/4/8
/// \param  source  �������� ������������
/// \param  scg1    ���������� SMCLK
// ============================================================================

void ConfigSMCLK(uchar divs, uchar source, bool scg1)
{
    BCSCTL2 |= source | divs;
    if (scg1)
    {
        _BIS_SR(SCG1);
    }
}