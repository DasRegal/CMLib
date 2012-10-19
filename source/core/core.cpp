//*****************************************************************************
//
// ��� �����    : 'core.cpp'
// ���������    : ���� �����������
// �����        : ���������� �. �.
// ��������     : plexus_bra@rambler.ru
// ����         : 13.10.2012
//
//*****************************************************************************

#include "core\core.h"
#include "core\clock.h"
#include "core\ta.h"

// =============================================================================
//                                 �������
// =============================================================================

// =============================================================================
//                                ��������
// =============================================================================

// =============================================================================
//                                 �������
// =============================================================================

// =============================================================================
//                           ���������� ����������
// =============================================================================

// =============================================================================
//                             ��������� �������
// =============================================================================

    extern  void AppMain (void);

// ============================================================================
///    
///                           ����� ����� main
///
// ============================================================================

int main (void)
{
    WDTCTL = WDTPW + WDTHOLD;
    // ��������� DCO ���������� �� ������� 8 ���
    ConfigDCO(DCO_1MHZ);
    // �������� ������������ CPU - DCO
    ConfigMCLK(MCLK_DIV_1, MCLK_DCO, MCLK_ON);
    // �������� ������������ SMCLK - DCO
    ConfigSMCLK(SMCLK_DIV_1, SMCLK_DCO, SMCLK_ON);
    // ���������� ������������ ������� A �������� SMCLK
    // (��. �������� ta.cpp)
//    ta.Configure(1000, 0);
    
    AppMain();
    
    return 0;
}