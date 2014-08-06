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
    float			aThdac;
    float			aAmpl;
    float			aPhase;
    float			aPhaseKee;
    S32				outBufSizeb;

    U32				nFifoSizeb;
    float			dSamplingRate;
    float			dSignalFreq;
    U32				nAutoRestart;
    U32				nDacSincScale;

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

    S32 CalcSignalToChan( void *pvBuf, S32 nSamples, S32 sampleWidth, float twiddle, float ampl, float *pPhase );
    S32 CorrectOutFreq();
    S32 CalcSignalToBuf( void *pvBuf, S32 nSamples, S32 sampleWidth, float twiddle,
                         float aAmpl, float *aPhase );
    S32 CalcSignal( void *pvBuf, S32 nSamples );
    S32 FifoOutputCPUStart( S32 isCycle );
    bool defaultDacSettings();
    S32 WorkMode3();
    S32 WorkMode5();
    float dacScale();

private:
    dac();
    Fpga* m_fpga;
    fpga_trd_t m_dacTrd;
    trdprog*   m_trdprog;
    TDacParam g_aDac;
    int g_nSamplesPerChannel;
};

#endif // DAC_H
