#include "gipcy.h"
#include "adc.h"

#include <cstdio>
#include <cstdlib>
#include <math.h>

//-------------------------------------------------------------------------------------

adc::adc(Fpga *fpga, const app_params_t& params) : m_fpga(fpga), m_trdprog(0), m_params(params)
{
    // Search ADC TRD
    if(!m_fpga->fpgaTrd(0, 0xB8, m_adcTrd)) {
        fprintf(stderr, "%s(): ADC TRD 0x%.2X not found!\n", __FUNCTION__, 0xB8);
        return;
    }

    // Reset and Sync ADC
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x18, 0x0);
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x18, 0x1);
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x18, 0x0);

    // Test mode
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x1A, params.AdcTest);

    // Channel mask
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x10, 0x3);

    // ADC clock source
    if(params.SysRefClockSource)
        m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x13, 0x81);
    else
        m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x13, 0x0);

    // STMODE
    U32 stmode = (params.AdcStopInverting << 14) |
                 (params.AdcStopSource << 8) |
                 (params.AdcStartMode << 7) |
                 (params.AdcStartBaseInverting << 6) |
                  params.AdcStartBaseSource;
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x5, stmode);

    // ADC default settings
    default_settings(params.AdcBias0, params.AdcBias1);
    //m_trdprog = new trdprog(m_fpga, "adc.ini");

    // CNT0, CNT1, CNT2
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x6, 0x0);
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x7, params.AdcCnt1);
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x8, 0x1);
}

//-------------------------------------------------------------------------------------

adc::~adc()
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_adcTrd.number, 0x0);
    mode0 &= ~MODE0_START;      // запрет работы АЦП
    mode0 |= MODE0_RST_FIFO;    // сброс FIFO АЦП
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x0, mode0);
    delete m_trdprog;
}

//-------------------------------------------------------------------------------------

void adc::default_settings(signed AdcBias0, signed AdcBias1)
{
    // ADC0
    unsigned sign0 = (AdcBias0 < 0) ? 1 : 0;
    m_fpga->writeSpdDev(m_adcTrd.number, 0x0, 0x1, 0x8);
    m_fpga->writeSpdDev(m_adcTrd.number, 0x0, 0x3, sign0);
    m_fpga->writeSpdDev(m_adcTrd.number, 0x0, 0x4, (abs(AdcBias0) & 0xFF));
    m_fpga->writeSpdDev(m_adcTrd.number, 0x0, 0x5, 0x28);

    // ADC1
    unsigned sign1 = (AdcBias1 < 0) ? 1 : 0;
    m_fpga->writeSpdDev(m_adcTrd.number, 0x0, 0x1, 0x8, (1<<4));
    m_fpga->writeSpdDev(m_adcTrd.number, 0x0, 0x3, sign1, (1<<4));
    m_fpga->writeSpdDev(m_adcTrd.number, 0x0, 0x4, (abs(AdcBias1) & 0xFF), (1<<4));
    m_fpga->writeSpdDev(m_adcTrd.number, 0x0, 0x5, 0x28, (1<<4));
}

//-----------------------------------------------------------------------------

void adc::start()
{
    unsigned bit = m_params.AdcEnableCnt;
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x0, 0x2038 | (bit << 9) | (bit << 10));
    //fprintf(stderr, "%s(): MODE0 = 0x%.4X\n", __FUNCTION__, m_fpga->FpgaRegPeekInd(m_adcTrd.number, 0x0));
    //fprintf(stderr, "%s(): STMODE = 0x%.4X\n", __FUNCTION__, m_fpga->FpgaRegPeekInd(m_adcTrd.number, 0x5));
}

//-------------------------------------------------------------------------------------

void adc::stop()
{
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x0, 0x0);
}

//-------------------------------------------------------------------------------------

void adc::reset_fifo()
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_adcTrd.number, 0x0);

    mode0 |= 0x2;
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x0, mode0);
    IPC_delay(1);
    mode0 &= ~0x2;
    m_fpga->FpgaRegPokeInd(m_adcTrd.number, 0x0, mode0);
}

//-------------------------------------------------------------------------------------

u16 adc::status()
{
    return (m_fpga->FpgaRegPeekDir(m_adcTrd.number, 0x0) & 0xFFFF);
}

//-------------------------------------------------------------------------------------
