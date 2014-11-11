
#include "iniparser.h"
#include "utypes.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

bool getParams(int argc, char *argv[], struct app_params_t& param)
{
    int res = 0;
    IPC_str Buffer[128];
    IPC_str iniFilePath[1024];

    if(argc > 2) {
        fprintf(stderr, "usage: %s [alternate ini file name. azbuka.ini by default]\n", argv[0]);
        return false;
    }

    memset(&param, 0, sizeof(param));

    if(argc == 2) {
        BRDC_snprintf(iniFilePath, 1024, "%s", argv[1]);
    } else {
        const IPC_str *iniFileName = _BRDC("azbuka_325.ini");
        IPC_getCurrentDir(iniFilePath, sizeof(iniFilePath)/sizeof(char));
        BRDC_strcat(iniFilePath, _BRDC("/"));
        BRDC_strcat(iniFilePath, iniFileName);
    }

    fprintf(stderr, "inifile = %s\n", iniFilePath);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("SysTestMode"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: SysTestMode - not found. Use default value\n");
    }
    param.SysTestMode = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("SysRefClockSource"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: SysRefClockSource - not found. Use default value\n");
    }
    param.SysRefClockSource = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("SysSamplingRate"), _BRDC("1000000000.0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: SysSamplingRate - not found. Use default value\n");
    }
    param.SysSamplingRate = float(BRDC_strtod(Buffer,0));

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcCycle"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcCycle - not found. Use default value\n");
    }
    param.AdcCycle = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcDaqIntoMemory"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcDaqIntoMemory - not found. Use default value\n");
    }
    param.AdcDaqIntoMemory = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcSamplesPerChannel"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcSamplesPerChannel - not found. Use default value\n");
    }
    param.AdcSamplesPerChannel = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcMemSamplesPerChan"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcMemSamplesPerChan - not found. Use default value\n");
    }
    param.AdcMemSamplesPerChan = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcTest"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcMemSamplesPerChannel - not found. Use default value\n");
    }
    param.AdcTest = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcBias0"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcBias0 - not found. Use default value\n");
    }
    param.AdcBias0 = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcBias1"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcBias1 - not found. Use default value\n");
    }
    param.AdcBias1 = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcStartBaseSource"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcStartBaseSource - not found. Use default value\n");
    }
    param.AdcStartBaseSource = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcStartBaseInverting"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcStartBaseInverting - not found. Use default value\n");
    }
    param.AdcStartBaseInverting = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcStartMode"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcStartMode - not found. Use default value\n");
    }
    param.AdcStartMode = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcStopSource"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcStopSource - not found. Use default value\n");
    }
    param.AdcStopSource = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("AdcStopInverting"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: AdcStopInverting - not found. Use default value\n");
    }
    param.AdcStopInverting = BRDC_strtol(Buffer,0,10);

    //================================================

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacCycle"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacCycle - not found. Use default value\n");
    }
    param.DacCycle = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacTest"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacTest - not found. Use default value\n");
    }
    param.DacTest = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacSignalFreqHz"), _BRDC("10000000.0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacSignalFreqHz - not found. Use default value\n");
    }
    param.DacSignalFreqHz = float(BRDC_strtod(Buffer,0));

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacAmplitude0"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacAmplitude0 - not found. Use default value\n");
    }
    param.DacAmplitude0 = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacSamplesPerChannel"), _BRDC("4096"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacSamplesPerChannel - not found. Use default value\n");
    }
    param.DacSamplesPerChannel = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacStartBaseSource"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacStartBaseSource - not found. Use default value\n");
    }
    param.DacStartBaseSource = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacStartBaseInverting"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacStartBaseInverting - not found. Use default value\n");
    }
    param.DacStartBaseInverting = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacStartMode"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacStartMode - not found. Use default value\n");
    }
    param.DacStartMode = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacStopSource"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacStopSource - not found. Use default value\n");
    }
    param.DacStopSource = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacStopInverting"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacStopInverting - not found. Use default value\n");
    }
    param.DacStopInverting = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacRestart"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacRestart - not found. Use default value\n");
    }
    param.DacRestart = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("DacSincScale"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: DacSincScale - not found. Use default value\n");
    }
    param.DacSincScale = BRDC_strtol(Buffer,0,10);

    //================================================

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("dmaChannel"), _BRDC("0x0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaChannel - not found. Use default value\n");
    }
    param.dmaChannel = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("dmaBlockSize"), _BRDC("0x10000"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBlockSize - not found. Use default value\n");
    }
    param.dmaBlockSize = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("dmaBlockCount"), _BRDC("0x4"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBlockCount - not found. Use default value\n");
    }
    param.dmaBlockCount = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("dmaBuffersCount"), _BRDC("1"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBuffersCount - not found. Use default value\n");
    }
    param.dmaBuffersCount = BRDC_strtol(Buffer,0,10);

    //================================================

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("syncCycle"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncCycle - not found. Use default value\n");
    }
    param.syncCycle = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("Tcycle"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: Tcycle - not found. Use default value\n");
    }
    param.Tcycle = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("Tdelay"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: Tdelay - not found. Use default value\n");
    }
    param.Tdelay = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("deltaAdc"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: deltaAdc - not found. Use default value\n");
    }
    param.deltaAdc = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("deltaDac"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: deltaDac - not found. Use default value\n");
    }
    param.deltaDac = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("delta_A"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: deltaA - not found. Use default value\n");
    }
    param.delta_A = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("width_A"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: widthA - not found. Use default value\n");
    }
    param.width_A = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("delta_B"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: deltaB - not found. Use default value\n");
    }
    param.delta_B = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("width_B"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: widthB - not found. Use default value\n");
    }
    param.width_B = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("delta_C"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: deltaC - not found. Use default value\n");
    }
    param.delta_C = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("width_C"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: widthC - not found. Use default value\n");
    }
    param.width_C = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("delta_D"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: deltaD - not found. Use default value\n");
    }
    param.delta_D = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("width_D"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: widthD - not found. Use default value\n");
    }
    param.width_D = BRDC_strtol(Buffer,0,10);

    //================================================

    return true;
}

//-----------------------------------------------------------------------------

void showParams(struct app_params_t& param)
{
    fprintf(stderr, "SysTestMode:               %d\n", param.SysTestMode);
    fprintf(stderr, "SysRefClockSource:         %d\n", param.SysRefClockSource);
    fprintf(stderr, "SysSamplingRate:           %f\n", param.SysSamplingRate);
    fprintf(stderr, " \n");
    fprintf(stderr, "AdcCycle:                  %d\n", param.AdcCycle);
    fprintf(stderr, "AdcDaqIntoMemory:          %d\n", param.AdcDaqIntoMemory);
    fprintf(stderr, "AdcSamplesPerChannel:      %d\n", param.AdcSamplesPerChannel);
    fprintf(stderr, "AdcMemSamplesPerChan:      %d\n", param.AdcMemSamplesPerChan);
    fprintf(stderr, "AdcTest:                   %d\n", param.AdcTest);
    fprintf(stderr, "AdcBias0:                  %d\n", param.AdcBias0);
    fprintf(stderr, "AdcBias1:                  %d\n", param.AdcBias1);
    fprintf(stderr, "AdcStartBaseSource:        %d\n", param.AdcStartBaseSource);
    fprintf(stderr, "AdcStartBaseInverting:     %d\n", param.AdcStartBaseInverting);
    fprintf(stderr, "AdcStartMode:              %d\n", param.AdcStartMode);
    fprintf(stderr, "AdcStopSource:             %d\n", param.AdcStopSource);
    fprintf(stderr, "AdcStopInverting:          %d\n", param.AdcStopInverting);
    fprintf(stderr, " \n");
    fprintf(stderr, "DacCycle:                  %d\n", param.DacCycle);
    fprintf(stderr, "DacTest:                   %d\n", param.DacTest);
    fprintf(stderr, "DacSignalFreqHz:           %f\n", param.DacSignalFreqHz);
    fprintf(stderr, "DacAmplitude0:             %d\n", param.DacAmplitude0);
    fprintf(stderr, "DacSamplesPerChannel:      %d\n", param.DacSamplesPerChannel);
    fprintf(stderr, "DacStartBaseSource:        %d\n", param.DacStartBaseSource);
    fprintf(stderr, "DacStartBaseInverting:     %d\n", param.DacStartBaseInverting);
    fprintf(stderr, "DacStartMode:              %d\n", param.DacStartMode);
    fprintf(stderr, "DacStopSource:             %d\n", param.DacStopSource);
    fprintf(stderr, "DacStopInverting:          %d\n", param.DacStopInverting);
    fprintf(stderr, "DacRestart:                %d\n", param.DacRestart);
    fprintf(stderr, "DacSincScale:              %d\n", param.DacSincScale);
    fprintf(stderr, " \n");
    fprintf(stderr, "dmaChannel:                0x%x\n", param.dmaChannel);
    fprintf(stderr, "dmaBlockSize:              0x%x\n", param.dmaBlockSize);
    fprintf(stderr, "dmaBlockCount:             0x%x\n", param.dmaBlockCount);
    fprintf(stderr, "dmaBuffersCount:           0x%x\n", param.dmaBuffersCount);
    fprintf(stderr, " \n");
    fprintf(stderr, "syncCycle:                 0x%x\n", param.syncCycle);
    fprintf(stderr, "Tcycle:                    0x%x\n", param.Tcycle);
    fprintf(stderr, "Tdelay:                    0x%x\n", param.Tdelay);
    fprintf(stderr, "deltaAdc:                  0x%x\n", param.deltaAdc);
    fprintf(stderr, "deltaDac:                  0x%x\n", param.deltaDac);
    fprintf(stderr, "delta_A:                    0x%x\n", param.delta_A);
    fprintf(stderr, "width_A:                    0x%x\n", param.width_A);
    fprintf(stderr, "delta_B:                    0x%x\n", param.delta_B);
    fprintf(stderr, "width_B:                    0x%x\n", param.width_B);
    fprintf(stderr, "delta_C:                    0x%x\n", param.delta_C);
    fprintf(stderr, "width_C:                    0x%x\n", param.width_C);
    fprintf(stderr, "delta_D:                    0x%x\n", param.delta_D);
    fprintf(stderr, "width_D:                    0x%x\n", param.width_D);
}

//-----------------------------------------------------------------------------

