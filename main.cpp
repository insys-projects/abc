

#include "gipcy.h"
#include "trdprog.h"
#include "abcdsp.h"
#include "iniparser.h"
#include "fpga_base.h"
#include "exceptinfo.h"
#include "fpga.h"
#include "isvi.h"

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#endif

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

#define USE_SIGNAL 1

//-----------------------------------------------------------------------------

#if USE_SIGNAL
abcdsp *pBrd = 0;
void stop_exam(int sig)
{
    if(pBrd) {
        fprintf(stderr, "\nSIGNAL = %d\n", sig);
        pBrd->setExitFlag(true);
    }
}
#endif

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    struct app_params_t params;
    if(!getParams(argc, argv, params)) {
        fprintf(stderr, "Error get parameters from file: %s\n", argv[1]);
        return -1;
    }
    showParams(params);

#if USE_SIGNAL
    signal(SIGINT, stop_exam);
#else
    IPC_initKeyboard();
#endif

    try {

        abcdsp brd(params);

        // used in signal handler to stop test
        pBrd = &brd;

        switch(params.SysTestMode) {
        case 0: {
            if(params.AdcDaqIntoMemory) {
                brd.dataFromAdcToMemAsMem(params);
            } else {
                brd.dataFromAdc(params);
            }
        } break;
        case 1: {
            brd.ltcTest();
        } break;
        case 2: {
            brd.uartTest(UART_RATE_115200, false);
        } break;
        case 3: {
            brd.syncTest();
        } break;
        }

    }
    catch(except_info_t err) {
        fprintf(stderr, "%s", err.info.c_str());
    }
    catch(...) {
        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }

#if !USE_SIGNAL
    IPC_cleanupKeyboard();
#endif
    fprintf(stderr, "STOP\n");

    return 0;
}

//-----------------------------------------------------------------------------
