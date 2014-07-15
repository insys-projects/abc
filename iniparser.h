#ifndef __INIPARSER_H__
#define __INIPARSER_H__

#include "gipcy.h"

#define SECTION_NAME _BRDC("VARIABLE")

struct app_params_t {
    unsigned dmaChannel;
    unsigned adcMask;
    unsigned adcStart;
    unsigned adcStartInv;
    unsigned dmaBlockSize;
    unsigned dmaBlockCount;
    unsigned dmaBuffersCount;
    unsigned testMode;
    float syncFd;
    unsigned clockReference;
    unsigned adcDebug;
    unsigned analogOffset;
};

bool getParams(int argc, char *argv[], struct app_params_t& param);
void showParams(struct app_params_t& param);

#endif // __INIPARSER_H__
