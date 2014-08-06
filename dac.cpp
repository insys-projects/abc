#include "gipcy.h"
#include "dac.h"

#include <cstdio>
#include <cstdlib>
#include <math.h>

//-------------------------------------------------------------------------------------

dac::dac(Fpga *fpga, const app_params_t& params) : m_fpga(fpga), m_trdprog(0)
{
    g_aDac.aAmpl = params.DacAmplitude0;
    g_aDac.aPhase = 0;
    g_aDac.aPhaseKee = 0;
    g_aDac.aThdac = 0;
    g_aDac.dSamplingRate = params.SysSamplingRate;
    g_aDac.dSignalFreq = params.DacSignalFreqHz;
    g_aDac.nFifoSizeb = 8192*8;
    g_aDac.outBufSizeb = 0;
    g_aDac.pBuf = 0;
    g_aDac.sampleSizeb = 2;
    g_aDac.samplesPerChannel = 0;
    g_aDac.nAutoRestart = params.DacRestart;
    g_aDac.nDacSincScale = params.DacSincScale;
    g_aDac.nDacCycle = params.DacCycle;

    g_nSamplesPerChannel = params.DacSamplesPerChannel;

    // Search DAC TRD
    if(!m_fpga->fpgaTrd(0, 0xB9, m_dacTrd)) {
        fprintf(stderr, "%s(): DAC TRD 0x%.2X not found!\n", __FUNCTION__, 0xB9);
        return;
    }

    // Reset and Sync DAC
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x18, 0x0);
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x18, 0x1);
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x18, 0x0);

    // Test mode
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x1A, params.DacTest);

    // STMODE
    U32 stmode = (params.DacStopInverting << 14) |
            (params.DacStopSource << 8) |
            (params.DacStartMode << 7) |
            (params.DacStartBaseInverting << 6) |
            params.DacStartBaseSource;
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x5, stmode);

    // DAC default settings
    defaultDacSettings();

    //m_trdprog = new trdprog(m_fpga, "dac.ini");
}

//-------------------------------------------------------------------------------------

dac::~dac()
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);
    mode0 &= ~MODE0_START;      // запрет работы ЦАП
    mode0 |= MODE0_RST_FIFO;    // сброс FIFO ЦАП
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0);
    free( g_aDac.pBuf );
    delete m_trdprog;
}

//-------------------------------------------------------------------------------------

S32 dac::CalcSignalToChan( void *pvBuf, S32 nSamples, S32 sampleWidth, float twiddle, float ampl, float *pPhase )
{
    int			ii;
    float		phase = *pPhase;

    //
    // Проверить аргументы
    //
    if( ampl < 0.0 )
        ampl = 32767;

    if(g_aDac.nDacSincScale) {
        ampl *= dacScale();
    }

    fprintf(stderr, "ampl = %.2f\n", ampl);

    //
    // Отсчеты сигнала имеют размер 2 байта
    //
    if( sampleWidth == 2 )
    {
        S16		*pBuf = (S16*)pvBuf;
        for( ii=0; ii<nSamples; ii++ )
        {
            pBuf[ii] = (S16)floor(ampl * sin( twiddle * ii ) + 0.5);
        }
    }

    *pPhase = fmod( phase, PI2 );

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::CorrectOutFreq()
{
    float npi;			// Число периодов выходного синуса (пока не целое)
    float nnp;			// Число периодов выходного синуса (целое гарантировано)
    float delta;		// Поворачивающий фактор
    float freq;         // Скорректированная частота

    delta = g_aDac.dSignalFreq / g_aDac.dSamplingRate;
    npi   = delta * (float)g_aDac.samplesPerChannel;
    nnp   = floor(npi+0.5);
    delta = nnp / (float)g_aDac.samplesPerChannel;
    freq  = floor(delta * g_aDac.dSamplingRate + 0.5);

    fprintf( stderr, "Desired Freq = %.2f Hz\n", g_aDac.dSignalFreq );
    fprintf( stderr, "Real Freq = %.2f Hz\n", freq );

    g_aDac.dSignalFreq  = freq;

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::CalcSignalToBuf( void *pvBuf, S32 nSamples, S32 sampleWidth,
                          float twiddle, float aAmpl, float *aPhase )
{
    //
    // Сформировать сигнал
    //
    CalcSignalToChan( pvBuf, nSamples, sampleWidth, twiddle, aAmpl, aPhase );

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::CalcSignal( void *pvBuf, S32 nSamples )
{
    S32				err;
    float			twiddle;

    twiddle = PI2 * g_aDac.dSignalFreq / g_aDac.dSamplingRate;

    err = CalcSignalToBuf( pvBuf, nSamples,
                           g_aDac.sampleSizeb,
                           twiddle,
                           g_aDac.aAmpl,
                           &g_aDac.aPhaseKee
                           );

    return err;
}

//-------------------------------------------------------------------------------------

S32 dac::FifoOutputCPUStart( S32 isCycle )
{
    U16 status = m_fpga->FpgaRegPeekDir(m_dacTrd.number, 0x0);
    fprintf(stderr, "0) STATUS = 0x%.4X\n", status);

    U32 mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);

    if(isCycle) {
        mode0 |= MODE0_RT;      // зацикливание FIFO ЦАП
    } else {
        mode0 &= ~MODE0_RT;
    }
    mode0 &= ~MODE0_START;      // запрет работы ЦАП
    mode0 |= MODE0_RST_FIFO;    // сброс FIFO ЦАП

    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0);

    IPC_delay(100);

    mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);
    mode0 &= ~MODE0_RST_FIFO;
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0);

    IPC_delay(100);

    if(isCycle)
        m_fpga->FpgaWriteRegBufDir(m_dacTrd.number, 0x1, g_aDac.pBuf, g_aDac.outBufSizeb);
    else
        m_fpga->FpgaWriteRegBufDir(m_dacTrd.number, 0x1, g_aDac.pBuf, g_aDac.outBufSizeb+8);
    /*
        // Debug signal
        U16 testBuf[] = { 0x0, 0x5A7F, 0x7FFF, 0x5A7F, 0x0, 0xA583, 0x8000, 0xA583, };
        for(int i=0; i<1000; i++) {
            m_fpga->FpgaWriteRegBufDir(m_dacTrd.number, 0x1, testBuf, sizeof(testBuf));
        }
*/
    // Add Autorestart bit in STMODE after fill FIFO
    if(g_aDac.nAutoRestart) {
        U32 stmode = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x5);
        stmode |= (1 << 15);
        m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x5, stmode);
    }

    mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);
    mode0 |= (MODE0_START | MODE0_MASTER);
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0); // разрешение работы ЦАП

    status = m_fpga->FpgaRegPeekDir(m_dacTrd.number, 0x0);
    fprintf(stderr, "1) STATUS = 0x%.4X\n", status);

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::WorkMode3()
{
    volatile S32	tmp;

    fprintf(stderr, "\nWorkMode 3: -- Single FIFO -- \n\n");

    //
    // Скорректировать количество отсчетов на канал
    //
    g_aDac.outBufSizeb = g_nSamplesPerChannel
            * g_aDac.sampleSizeb;
    tmp = g_aDac.outBufSizeb / 16;
    g_aDac.outBufSizeb = tmp * 16;
    if( g_aDac.outBufSizeb > (S32)g_aDac.nFifoSizeb )
    {
        g_aDac.outBufSizeb = g_aDac.nFifoSizeb;
        fprintf(stderr, "WARNING: OutBufSizeb > DacFifoSizeb !\n");
    }

    g_aDac.samplesPerChannel = g_aDac.outBufSizeb / g_aDac.sampleSizeb;

    fprintf(stderr, "SamplesPerChannel = %d,  ", g_aDac.samplesPerChannel );
    fprintf(stderr, "SignalFreq = %.2f Hz\n", g_aDac.dSignalFreq );

    //
    // Создать буфер для сигнала
    //
    g_aDac.pBuf = malloc( g_aDac.outBufSizeb + 8);
    if( !g_aDac.pBuf )
    {
        BRDC_printf( _BRDC("ERROR: No enougth memory!") );
        return -1;
    }

    memset(g_aDac.pBuf, 0, g_aDac.outBufSizeb + 8);

    CalcSignal( g_aDac.pBuf, g_aDac.samplesPerChannel );

    //
    // Выводить циклически данные в FIFO с помощью процессора
    //
    FifoOutputCPUStart(0);

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::WorkMode5()
{
    volatile S32	tmp;

    fprintf(stderr, "\nWorkMode 5: -- Cycle FIFO -- \n\n");

    g_aDac.outBufSizeb = g_nSamplesPerChannel
            * g_aDac.sampleSizeb;
    tmp = g_aDac.outBufSizeb / 16;
    g_aDac.outBufSizeb = tmp * 16;
    if( g_aDac.outBufSizeb > (S32)g_aDac.nFifoSizeb )
    {
        g_aDac.outBufSizeb = g_aDac.nFifoSizeb;
        fprintf(stderr, "WARNING: OutBufSizeb > DacFifoSizeb !\n");
    }

    g_aDac.samplesPerChannel = g_aDac.outBufSizeb
            / g_aDac.sampleSizeb;

    CorrectOutFreq();

    fprintf(stderr, "SamplesPerChannel = %d,  ", g_aDac.samplesPerChannel );
    fprintf(stderr, "SignalFreq = %.2f Hz\n", g_aDac.dSignalFreq );

    //
    //
    //
    g_aDac.pBuf = malloc( g_aDac.outBufSizeb);
    if( !g_aDac.pBuf )
    {
        BRDC_printf( _BRDC("ERROR: No enougth memory!") );
        return -1;
    }

    CalcSignal( g_aDac.pBuf, g_aDac.samplesPerChannel );

    //
    //     FIFO
    //
    FifoOutputCPUStart(1);

    return 0;
}

//-------------------------------------------------------------------------------------

bool dac::defaultDacSettings()
{
    U32 val = 0;

    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x00, 0x0);
    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x00, 0x20);
    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x00, 0x0);

    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x22, 0xf);
    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x23, 0x7);

    m_fpga->readSpdDev(m_dacTrd.number, 0x1, 0x35, 0, val);
    fprintf(stderr, "DAC ID: 0x%.2X\n", val & 0xFF);

    IPC_delay(200);

    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x24, 0x30);
    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x25, 0x80);
    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x27, 0x45);
    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x28, 0x6C);

    int attempt = 0;

    while(1) {

        m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x29, 0xCB);
        m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x26, 0x42);
        m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x26, 0x43);

        IPC_delay(100);

        U32 val = 0;
        bool ok = m_fpga->readSpdDev(m_dacTrd.number, 0x1, 0x2A, 0, val);

        //fprintf(stderr, "DAC REG 0x2A: 0x%.2X\n", val & 0xff);

        if(ok) {
            if((val & 0xff) == 0x01) {
                //fprintf(stderr, "%s(): Ok (1) DAC !!!!!\n", __FUNCTION__);
                break;
            }
        } else {
            fprintf(stderr, "%s(): Error (1) read answer from DAC\n", __FUNCTION__);
            return false;
        }

        if(attempt == 3) {
            fprintf(stderr, "%s(): Error (1) DAC programming\n", __FUNCTION__);
            return false;
            //break;
        }

        attempt++;
    }

    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x13, 0x72);
    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x14, 0xCA);

    while(1) {

        m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x10, 0x00);
        m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x10, 0x02);
        m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x10, 0x03);

        IPC_delay(100);

        U32 val = 0;
        bool ok = m_fpga->readSpdDev(m_dacTrd.number, 0x1, 0x21, 0, val);
        fprintf(stderr, "DAC REG 0x21: 0x%.2X\n", val & 0xff);
        if(ok) {
            if((val & 0xff) == 0x9) {
                //fprintf(stderr, "%s(): Ok (2) DAC !!!!!\n", __FUNCTION__);
                break;
            }
        } else {
            fprintf(stderr, "%s(): Error (2) read answer from DAC\n", __FUNCTION__);
            return false;
        }

        if(attempt == 3) {
            fprintf(stderr, "%s(): Error (2) DAC programming\n", __FUNCTION__);
            return false;
            //break;
        }

        attempt++;
    }

    //m_fpga->readSpdDev(m_dacTrd.number, 0x1, 0x21, 0, val);
    //fprintf(stderr, "DAC REG 0x21: 0x%.2X\n", val & 0xff);
    //m_fpga->readSpdDev(m_dacTrd.number, 0x1, 0x14, 0, val);
    //fprintf(stderr, "DAC REG 0x14: 0x%.2X\n", val & 0xff);

    //m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x06, 0x00);
    //m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x07, 0x02);
    //m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x08, 0x00);

    return true;
}

//-----------------------------------------------------------------------------

float dac::dacScale()
{
    float df = g_aDac.dSignalFreq/g_aDac.dSamplingRate;
    float sinc0 = sin(M_PI*0.5)/(M_PI*0.5);
    float sincx = ((1.0 - (sin(M_PI*df)/(M_PI*df)) + sinc0));
    fprintf(stderr, "f: %.2f --- f/fs: %.2f --- SCALE: %.2f\n", g_aDac.dSignalFreq, df, sincx);
    return sincx;
}

//-----------------------------------------------------------------------------

void dac::start()
{
    if(g_aDac.nDacCycle)
        WorkMode5();
    else
        WorkMode3();
}

//-------------------------------------------------------------------------------------

void dac::stop()
{
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, 0x0);
}

//-------------------------------------------------------------------------------------

void dac::reset_fifo()
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);

    mode0 |= MODE0_RST_FIFO;
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0);
    IPC_delay(1);
    mode0 &= ~MODE0_RST_FIFO;
    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0);
}

//-------------------------------------------------------------------------------------

u16 dac::status()
{
    return (m_fpga->FpgaRegPeekDir(m_dacTrd.number, 0x0) & 0xFFFF);
}

//-------------------------------------------------------------------------------------
