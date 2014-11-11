#ifndef __INIPARSER_H__
#define __INIPARSER_H__

#include "gipcy.h"

#define SECTION_NAME _BRDC("AZBUKA_325")

struct app_params_t {

    unsigned SysTestMode;
    unsigned SysRefClockSource;
    float    SysSamplingRate;

    unsigned AdcCycle;
    unsigned AdcDaqIntoMemory;
    unsigned AdcSamplesPerChannel;
    unsigned AdcMemSamplesPerChan;
    unsigned AdcTest;
    signed   AdcBias0;
    signed   AdcBias1;

    unsigned AdcStartBaseSource;
    unsigned AdcStartBaseInverting;
    unsigned AdcStartMode;
    unsigned AdcStopSource;
    unsigned AdcStopInverting;

    unsigned DacCycle;
    unsigned DacTest;
    float    DacSignalFreqHz;
    unsigned DacAmplitude0;
    unsigned DacSamplesPerChannel;

    unsigned DacStartBaseSource;
    unsigned DacStartBaseInverting;
    unsigned DacStartMode;
    unsigned DacStopSource;
    unsigned DacStopInverting;
    unsigned DacRestart;
    unsigned DacSincScale;

    unsigned dmaChannel;
    unsigned dmaBlockSize;
    unsigned dmaBlockCount;
    unsigned dmaBuffersCount;

    unsigned syncCycle;
    unsigned Tcycle;
    unsigned Tdelay;
    unsigned deltaAdc;
    unsigned deltaDac;
    unsigned delta_A;
    unsigned width_A;
    unsigned delta_B;
    unsigned width_B;
    unsigned delta_C;
    unsigned width_C;
    unsigned delta_D;
    unsigned width_D;
};

bool getParams(int argc, char *argv[], struct app_params_t& param);
void showParams(struct app_params_t& param);

#endif // __INIPARSER_H__
