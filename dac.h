#ifndef DAC_H
#define DAC_H

#include "fpga.h"
#include "trdprog.h"
#include "iniparser.h"

//-------------------------------------------------------------------------------------

#define PI2   6.2831853071795864
#define MAX_DEVICE   1
#define MAX_DAC      (MAX_DEVICE*2)
#define MAX_CHAN     1
#define	MAX_SECTNAME 32

//-------------------------------------------------------------------------------------

typedef struct
{
    S32				sampleSizeb;
    S32				samplesPerChannel;
    S32				chanMask;
    S32				chanNum;
    S32				chanMaxNum;
    float			aThdac[2];
    float			aAmpl[MAX_CHAN];
    float			aPhase[MAX_CHAN];
    float			aPhaseKee[MAX_CHAN];
    S32				signalType;
    S32				outBufSizeb;

    U32				nFifoSizeb;
    float			dSamplingRate;
    float			dSignalFreq;
    U32				nAutoRestart;

    void			*pBuf;
    BRDctrl_StreamCBufAlloc rBufAlloc;
} TDacParam;

//-------------------------------------------------------------------------------------

#define MODE0_RST_TRD           0x1
#define MODE0_RST_FIFO          0x2
#define MODE0_MASTER            0x10
#define MODE0_START             0x20
#define MODE0_RT                0x80

//-------------------------------------------------------------------------------------

class dac
{
public:
    dac(Fpga *fpga, const struct app_params_t& params );
    virtual ~dac();

    S32 CalcSignalToChan( void *pvBuf, S32 nSamples, S32 signalType,
                          S32 sampleWidth, S32 chanIdx, S32 chanNum,
                          float twiddle, float ampl, float *pPhase );
    S32 CorrectOutFreq( int idxDac );
    S32 CalcSignalToBuf( void *pvBuf, S32 nSamples, S32 signalType,
                         S32 sampleWidth, S32 chanMask, S32 chanMaxNum,
                         float twiddle, float *aAmpl, float *aPhase );
    S32 CalcSignal( void *pvBuf, S32 nSamples, int idxDac, int cnt );
    S32 FifoOutputCPUStart( S32 isCycle );
    bool defaultDacSettings();
    S32 WorkMode3();
    S32 WorkMode5();

private:
    dac();
    Fpga* m_fpga;
    fpga_trd_t m_dacTrd;
    trdprog*   m_trdprog;
    TDacParam g_aDac[MAX_DEVICE];
    int g_nIsDebugMarker;
    int g_nDacNum;
    int g_nSamplesPerChannel;
    int g_idx[MAX_DAC];
};

#endif // DAC_H
