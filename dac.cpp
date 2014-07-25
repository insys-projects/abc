#include "gipcy.h"
#include "dac.h"

#include <cstdio>
#include <cstdlib>
#include <math.h>

//-------------------------------------------------------------------------------------

dac::dac(Fpga *fpga) : m_fpga(fpga)
{
    g_aDac[0].aAmpl[0] = 16384;
    g_aDac[0].aPhase[0] = 0;
    g_aDac[0].aPhaseKee[0] = 0;
    g_aDac[0].aThdac[0] = 0;
    g_aDac[0].aThdac[1] = 0;
    g_aDac[0].chanMask = 0x1;
    g_aDac[0].chanMaxNum = 0x1;
    g_aDac[0].chanNum = 0x1;
    g_aDac[0].dSamplingRate = 1000000000.0;
    g_aDac[0].dSignalFreq = 100000000.0;
    g_aDac[0].nFifoSizeb = 8192*8;
    g_aDac[0].outBufSizeb = 0;
    g_aDac[0].pBuf = 0;
    g_aDac[0].sampleSizeb = 2;
    g_aDac[0].samplesPerChannel = 0;
    g_aDac[0].signalType = 0;

    g_nIsDebugMarker = 0;
    g_nDacNum = 1;
    g_nSamplesPerChannel = 0x1000;
    g_idx[0] = 0;

    m_trdprog = new trdprog(m_fpga, "dac.ini");

    WorkMode5();
}

//-------------------------------------------------------------------------------------

dac::~dac()
{
    for(int ii=0; ii<g_nDacNum; ii++)
    {
        U32 mode0 = m_fpga->FpgaRegPeekInd(0x5, 0x0);
        mode0 &= ~MODE0_START;      // запрет работы ЦАП
        mode0 |= MODE0_RST_FIFO;    // сброс FIFO ЦАП
        m_fpga->FpgaRegPokeInd(0x5, 0x0, mode0);
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
    U16 status = m_fpga->FpgaRegPeekDir(0x5, 0x0);
    fprintf(stderr, "0) STATUS = 0x%.4X\n", status);

    for( ii=g_nDacNum-1; ii>=0; ii-- )
    {
        idx = g_idx[ii];

        U32 mode0 = m_fpga->FpgaRegPeekInd(0x5, 0x0);

        if(isCycle) {
            mode0 |= MODE0_RT;      // зацикливание FIFO ЦАП
        } else {
            mode0 &= ~MODE0_RT;
        }
        mode0 &= ~MODE0_START;      // запрет работы ЦАП
        mode0 |= MODE0_RST_FIFO;    // сброс FIFO ЦАП

        m_fpga->FpgaRegPokeInd(0x5, 0x0, mode0);

        IPC_delay(100);

        mode0 = m_fpga->FpgaRegPeekInd(0x5, 0x0);
        mode0 &= ~MODE0_RST_FIFO;
        m_fpga->FpgaRegPokeInd(0x5, 0x0, mode0);

        IPC_delay(100);

        m_fpga->FpgaWriteRegBufDir(0x5, 0x1, g_aDac[idx].pBuf, g_aDac[idx].outBufSizeb);
/*
        U16 testBuf[] = { 0x0, 0x5A7F, 0x7FFF, 0x5A7F, 0x0, 0xA583, 0x8000, 0xA583, };
        for(int i=0; i<1000; i++) {
            m_fpga->FpgaWriteRegBufDir(0x5, 0x1, testBuf, sizeof(testBuf));
        }
*/
        mode0 = m_fpga->FpgaRegPeekInd(0x5, 0x0);
        mode0 |= MODE0_START;
        m_fpga->FpgaRegPokeInd(0x5, 0x0, mode0); // разрешение работы ЦАП
    }

    status = m_fpga->FpgaRegPeekDir(0x5, 0x0);
    fprintf(stderr, "1) STATUS = 0x%.4X\n", status);

    return 0;
}

//-------------------------------------------------------------------------------------

S32 dac::WorkMode3()
{
    int				ii;
    volatile S32	tmp;

    fprintf(stderr, "\nWorkMode 3: -- Restart FIFO -- \n\n");

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
        g_aDac[ii].pBuf = malloc( g_aDac[ii].outBufSizeb );
        if( !g_aDac[ii].pBuf )
        {
            BRDC_printf( _BRDC("ERROR: No enougth memory, dacNo = %d"), ii );
            return -1;
        }

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
    //    U32 mode0 = m_fpga->FpgaRegPeekInd(0x5, 0x0);
    //    mode0 &= ~MODE0_START;      // запрет работы ЦАП
    //    m_fpga->FpgaRegPokeInd(0x5, 0x0, mode0);
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
        g_aDac[ii].pBuf = malloc( g_aDac[ii].outBufSizeb );
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
    FifoOutputCPUStart( 1 );
    //fprintf(stderr, "Press any key to stop ...\n");
    //IPC_getch();

    //
    //
    //
    //U32 mode0 = m_fpga->FpgaRegPeekInd(0x5, 0x0);
    //mode0 &= ~MODE0_START;
    //m_fpga->FpgaRegPokeInd(0x5, 0x0, mode0);


    //
    //
    //
    //for( ii=0; ii<g_nDacNum; ii++ )
    //    free( g_aDac[ii].pBuf );

    //fprintf(stderr, "\n");

    return 0;
}

//-------------------------------------------------------------------------------------
