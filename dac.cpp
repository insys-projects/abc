#include "gipcy.h"
#include "dac.h"

#include <cstdio>
#include <cstdlib>
#include <math.h>

//-------------------------------------------------------------------------------------

dac::dac(Fpga *fpga, const app_params_t& params) : m_fpga(fpga), m_trdprog(0)
{
    g_aDac[0].aAmpl[0] = params.DacAmplitude0;
    g_aDac[0].aPhase[0] = 0;
    g_aDac[0].aPhaseKee[0] = 0;
    g_aDac[0].aThdac[0] = 0;
    g_aDac[0].aThdac[1] = 0;
    g_aDac[0].chanMask = 0x1;
    g_aDac[0].chanMaxNum = 0x1;
    g_aDac[0].chanNum = 0x1;
    g_aDac[0].dSamplingRate = params.SysSamplingRate;
    g_aDac[0].dSignalFreq = params.DacSignalFreqHz;
    g_aDac[0].nFifoSizeb = 8192*8;
    g_aDac[0].outBufSizeb = 0;
    g_aDac[0].pBuf = 0;
    g_aDac[0].sampleSizeb = 2;
    g_aDac[0].samplesPerChannel = 0;
    g_aDac[0].signalType = 0;
    g_aDac[0].nAutoRestart = params.DacRestart;

    g_nIsDebugMarker = 0;
    g_nDacNum = 1;
    g_nSamplesPerChannel = params.DacSamplesPerChannel;
    g_idx[0] = 0;

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
    for(int ii=0; ii<g_nDacNum; ii++)
    {
        U32 mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);
        mode0 &= ~MODE0_START;      // запрет работы ЦАП
        mode0 |= MODE0_RST_FIFO;    // сброс FIFO ЦАП
        m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0);
        free( g_aDac[ii].pBuf );
    }
    delete m_trdprog;
}

//-------------------------------------------------------------------------------------

S32 dac::CalcSignalToChan( void *pvBuf, S32 nSamples, S32 signalType,
                      S32 sampleWidth, S32 chanIdx, S32 chanNum,
                      float twiddle, float ampl, float *pPhase )
{
    int			ii;
    float		phase = *pPhase;

    //
    // Проверить аргументы
    //
    if( ampl < 0.0 )
        switch( sampleWidth )
        {
            case 1: ampl = 128/2; break;
            case 2: ampl = 32767/2; break;
            case 4: ampl = 2147483647/2; break;
        }

    fprintf(stderr, "ampl = %.2f\n", ampl);

    //
    // Отсчеты сигнала имеют размер 1 байт
    //
    if( sampleWidth == 1 )
    {
        S08		*pBuf = (S08*)pvBuf;

        if( ampl < 0.0 )
            ampl = 64.0;

        pBuf += chanIdx;
        for( ii=0; ii<nSamples; ii++ )
        {
            *pBuf  = (S08)floor(ampl * sin( phase ) + 0.5);
            if( signalType == 2 )
                *pBuf = (S08)(( *pBuf < 0.0 ) ? -ampl : +ampl);
            pBuf  += chanNum;
            phase += twiddle;
        }
        if( g_nIsDebugMarker )
        {
            pBuf = (S08*)pvBuf;
            pBuf[ chanIdx + (nSamples-1)*chanNum ] = 0x7f;
        }
    }

    //
    // Отсчеты сигнала имеют размер 2 байта
    //
    if( sampleWidth == 2 )
    {
        S16		*pBuf = (S16*)pvBuf;

        if( ampl < 0.0 )
            ampl = 16384.0;

        pBuf += chanIdx;
        for( ii=0; ii<nSamples; ii++ )
        {
            *pBuf  = (S16)floor(ampl * sin( phase ) + 0.5);
            if( signalType == 2 )
                *pBuf = (S16)(( *pBuf < 0.0 ) ? -ampl : +ampl);
            pBuf  += chanNum;
            phase += twiddle;
        }
        if( g_nIsDebugMarker )
        {
            pBuf = (S16*)pvBuf;
            pBuf[ chanIdx + (nSamples-1)*chanNum ] = 0x7fff;
        }
    }

    //
    // Отсчеты сигнала имеют размер 4 байта
    //
    if( sampleWidth == 4 )
    {
        S32		*pBuf = (S32*)pvBuf;

        if( ampl < 0.0 )
            ampl = 1024.0 * 1024.0 * 1024.0;

        pBuf += chanIdx;
        for( ii=0; ii<nSamples; ii++ )
        {
            *pBuf  = (S32)floor(ampl * sin( phase ) + 0.5);
            if( signalType == 2 )
                *pBuf = (S32)(( *pBuf < 0.0 ) ? -ampl : +ampl);
            pBuf  += chanNum;
            phase += twiddle;
        }
        if( g_nIsDebugMarker )
        {
            pBuf = (S32*)pvBuf;
            pBuf[ chanIdx + (nSamples-1)*chanNum ] = 0x7fffffff;
        }
    }

    *pPhase = fmod( phase, PI2 );

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::CorrectOutFreq( int idxDac )
{
    float npi;			// Число периодов выходного синуса (пока не целое)
    float nnp;			// Число периодов выходного синуса (целое гарантировано)
    float delta;		// Поворачивающий фактор
    float freq;         // Скорректированная частота

    delta = PI2 * g_aDac[idxDac].dSignalFreq / g_aDac[idxDac].dSamplingRate;
    npi   = delta * (float)g_aDac[idxDac].samplesPerChannel / PI2;
    nnp   = floor(npi+0.5);
    delta = PI2 * nnp / (float)g_aDac[idxDac].samplesPerChannel;
    freq  = floor(delta * g_aDac[idxDac].dSamplingRate / PI2 + 0.5);

    fprintf( stderr, "Desired Freq = %.2f Hz\n", g_aDac[idxDac].dSignalFreq );
    fprintf( stderr, "Real Freq = %.2f Hz\n", freq );

    g_aDac[idxDac].dSignalFreq  = freq;

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::CalcSignalToBuf( void *pvBuf, S32 nSamples, S32 signalType,
                     S32 sampleWidth, S32 chanMask, S32 chanMaxNum,
                     float twiddle, float *aAmpl, float *aPhase )
{
    int			ii;
    S32			chanNum;
    S32			aChanNo[64];

    //
    // Определить количество и номера выбранных каналов
    //
    chanNum = 0;
    for( ii=0; ii<chanMaxNum; ii++ )
        if( chanMask & (1<<ii) )
            aChanNo[chanNum++] = ii;

    //
    // Сформировать сигнал в каждом выбранном канале
    //
    for( ii=0; ii<chanNum; ii++ )
        CalcSignalToChan( pvBuf, nSamples, signalType, sampleWidth, ii, chanNum,
        twiddle, aAmpl[aChanNo[ii]], &(aPhase[aChanNo[ii]]) );

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::CalcSignal( void *pvBuf, S32 nSamples, int idxDac, int cnt )
{
    S32				err;
    int				ii;
    float			twiddle;

    //
    // Если первый буфер, запомнить начальную фазу
    //
    if( cnt==0 )
        for( ii=0; ii<MAX_CHAN; ii++ )
            g_aDac[idxDac].aPhaseKee[ii] = g_aDac[idxDac].aPhase[ii];


    twiddle = PI2 * g_aDac[idxDac].dSignalFreq / g_aDac[idxDac].dSamplingRate;

    err = CalcSignalToBuf( pvBuf, nSamples,
                            g_aDac[idxDac].signalType,
                            g_aDac[idxDac].sampleSizeb,
                            g_aDac[idxDac].chanMask,
                            g_aDac[idxDac].chanMaxNum,
                            twiddle,
                            g_aDac[idxDac].aAmpl,
                            g_aDac[idxDac].aPhaseKee
                            );

    return err;
}

//-------------------------------------------------------------------------------------

S32 dac::FifoOutputCPUStart( S32 isCycle )
{
    int			ii, idx;

    //
    // Стартовать вывод в FIFO, начиная с последнего ЦАПа так, чтобы закончить МАСТЕРОМ
    //
    U16 status = m_fpga->FpgaRegPeekDir(m_dacTrd.number, 0x0);
    fprintf(stderr, "0) STATUS = 0x%.4X\n", status);



    for( ii=g_nDacNum-1; ii>=0; ii-- )
    {
        idx = g_idx[ii];

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
          m_fpga->FpgaWriteRegBufDir(m_dacTrd.number, 0x1, g_aDac[idx].pBuf, g_aDac[idx].outBufSizeb);
        else
          m_fpga->FpgaWriteRegBufDir(m_dacTrd.number, 0x1, g_aDac[idx].pBuf, g_aDac[idx].outBufSizeb+8);
/*
        // Debug signal
        U16 testBuf[] = { 0x0, 0x5A7F, 0x7FFF, 0x5A7F, 0x0, 0xA583, 0x8000, 0xA583, };
        for(int i=0; i<1000; i++) {
            m_fpga->FpgaWriteRegBufDir(m_dacTrd.number, 0x1, testBuf, sizeof(testBuf));
        }
*/
        // Add Autorestart bit in STMODE after fill FIFO
        if(g_aDac[idx].nAutoRestart) {
          U32 stmode = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x5);
          stmode |= (1 << 15);
          m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x5, stmode);
        }

        mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);
        mode0 |= (MODE0_START | MODE0_MASTER);
        m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0); // разрешение работы ЦАП
    }

    status = m_fpga->FpgaRegPeekDir(m_dacTrd.number, 0x0);
    fprintf(stderr, "1) STATUS = 0x%.4X\n", status);

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::WorkMode3()
{
    int				ii;
    volatile S32	tmp;

    fprintf(stderr, "\nWorkMode 3: -- Single FIFO -- \n\n");

    for( ii=0; ii<g_nDacNum; ii++ )
    {
        //
        // Скорректировать количество отсчетов на канал
        //
        g_aDac[ii].outBufSizeb = g_nSamplesPerChannel
                               * g_aDac[ii].sampleSizeb
                               * g_aDac[ii].chanNum;
        tmp = g_aDac[ii].outBufSizeb / 16;
        g_aDac[ii].outBufSizeb = tmp * 16;
        if( g_aDac[ii].outBufSizeb > (S32)g_aDac[ii].nFifoSizeb )
        {
            g_aDac[ii].outBufSizeb = g_aDac[ii].nFifoSizeb;
            fprintf(stderr, "WARNING: OutBufSizeb > DacFifoSizeb !\n");
        }

        g_aDac[ii].samplesPerChannel = g_aDac[ii].outBufSizeb
                                        / g_aDac[ii].sampleSizeb
                                        / g_aDac[ii].chanNum;

        fprintf(stderr, "SamplesPerChannel = %d,  ", g_aDac[ii].samplesPerChannel );
        fprintf(stderr, "SignalFreq = %.2f Hz\n", g_aDac[ii].dSignalFreq );

        //
        // Создать буфер для сигнала
        //
        g_aDac[ii].pBuf = malloc( g_aDac[ii].outBufSizeb + 8);
        if( !g_aDac[ii].pBuf )
        {
            BRDC_printf( _BRDC("ERROR: No enougth memory, dacNo = %d"), ii );
            return -1;
        }

        memset(g_aDac[ii].pBuf, 0, g_aDac[ii].outBufSizeb + 8);

        CalcSignal( g_aDac[ii].pBuf, g_aDac[ii].samplesPerChannel, ii, 0 );
    }

    //
    // Выводить циклически данные в FIFO с помощью процессора
    //
    fprintf(stderr, "Press any key to stop ...\n");
    FifoOutputCPUStart(0);
    //IPC_getch();

    //
    // Остановит все ЦАПы и освободить все буфера
    //
    //for( ii=0; ii<g_nDacNum; ii++ )
    //{
    //    U32 mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);
    //    mode0 &= ~MODE0_START;      // запрет работы ЦАП
    //    m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0);
    //    free( g_aDac[ii].pBuf );
    //}

    printf("\n");

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::WorkMode5()
{
    int				ii;
    volatile S32	tmp;

    fprintf(stderr, "\nWorkMode 5: -- Cycle FIFO -- \n\n");

    for( ii=0; ii<g_nDacNum; ii++ )
    {
        //
        //
        //
        g_aDac[ii].outBufSizeb = g_nSamplesPerChannel
                               * g_aDac[ii].sampleSizeb
                               * g_aDac[ii].chanNum;
        tmp = g_aDac[ii].outBufSizeb / 16;
        g_aDac[ii].outBufSizeb = tmp * 16;
        if( g_aDac[ii].outBufSizeb > (S32)g_aDac[ii].nFifoSizeb )
        {
            g_aDac[ii].outBufSizeb = g_aDac[ii].nFifoSizeb;
            fprintf(stderr, "WARNING: OutBufSizeb > DacFifoSizeb !\n");
        }

        g_aDac[ii].samplesPerChannel = g_aDac[ii].outBufSizeb
                                     / g_aDac[ii].sampleSizeb
                                     / g_aDac[ii].chanNum;
        CorrectOutFreq( ii );

        fprintf(stderr, "SamplesPerChannel = %d,  ", g_aDac[ii].samplesPerChannel );
        fprintf(stderr, "SignalFreq = %.2f Hz\n", g_aDac[ii].dSignalFreq );

        //
        //
        //
        g_aDac[ii].pBuf = malloc( g_aDac[ii].outBufSizeb);
        if( !g_aDac[ii].pBuf )
        {
            BRDC_printf( _BRDC("ERROR: No enougth memory, dacNo = %d"), ii );
            return -1;
        }

        CalcSignal( g_aDac[ii].pBuf, g_aDac[ii].samplesPerChannel, ii, 0 );
    }

    //
    //     FIFO
    //
    FifoOutputCPUStart(1);
    //fprintf(stderr, "Press any key to stop ...\n");
    //IPC_getch();

    //
    //
    //
    //U32 mode0 = m_fpga->FpgaRegPeekInd(m_dacTrd.number, 0x0);
    //mode0 &= ~MODE0_START;
    //m_fpga->FpgaRegPokeInd(m_dacTrd.number, 0x0, mode0);


    //
    //
    //
    //for( ii=0; ii<g_nDacNum; ii++ )
    //    free( g_aDac[ii].pBuf );

    //fprintf(stderr, "\n");

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

    m_fpga->readSpdDev(m_dacTrd.number, 0x1, 0x21, 0, val);
    //fprintf(stderr, "DAC REG 0x21: 0x%.2X\n", val & 0xff);
    m_fpga->readSpdDev(m_dacTrd.number, 0x1, 0x14, 0, val);
    //fprintf(stderr, "DAC REG 0x14: 0x%.2X\n", val & 0xff);

//    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x06, 0x00);
//    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x07, 0x02);
//    m_fpga->writeSpdDev(m_dacTrd.number, 0x1, 0x08, 0x00);

    return true;
}

//-----------------------------------------------------------------------------
