

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

        abcdsp brd;

        // used in signal handler to stop test
        pBrd = &brd;

//        return 0;
/*
        brd.writeSpd(4, 0x0, 0x1, 0x8, (1 << 12));

        fprintf(stderr, "MAIN: 0x%.4X\n", brd.RegPeekInd(0x0, 0x100));
        fprintf(stderr, "MAIN: 0x%.4X\n", brd.RegPeekInd(0x0, 0x108));

        fprintf(stderr, "ADC: 0x%.4X\n", brd.RegPeekInd(0x4, 0x100));
        fprintf(stderr, "DAC: 0x%.4X\n", brd.RegPeekInd(0x5, 0x100));
        fprintf(stderr, "MEM: 0x%.4X\n", brd.RegPeekInd(0x6, 0x100));

        brd.writeSpd(0x5, 0x1, 0x0, 0x0);
        brd.readSpdDev(0x5, 0x1, 0x0, 0x1, val);
        fprintf(stderr, "0x%X\n", val & 0xff);

        while(!brd.exitFlag()) {

            brd.writeSpd(0x5, 0x1, 0x29, 0x8B);
            brd.readSpdDev(0x5, 0x1, 0x29, 0x1, val);

            fprintf(stderr, "0x%X\n", val & 0xff);

            fprintf(stderr, "--------------------\n");
            IPC_delay(100);
        }
*/
        //---------------------------------------------------- DATA FROM MAIN STREAM

        if(params.testMode == 0)
            brd.dataFromMain(params);

        //---------------------------------------------------- DDR3 FPGA AS MEMORY

        if(params.testMode == 1)
            brd.dataFromMainToMemAsMem(params);

        //---------------------------------------------------- DDR3 FPGA AS FIFO

        if(params.testMode == 2)
            brd.dataFromMainToMemAsFifo(params);


        //---------------------------------------------------- DATA FROM STREAM ADC

        if(params.testMode == 3)
            brd.dataFromAdc(params);

        //---------------------------------------------------- DDR3 FPGA AS MEMORY

        if(params.testMode == 4)
            brd.dataFromAdcToMemAsMem(params);

        //---------------------------------------------------- DDR3 FPGA AS FIFO

        if(params.testMode == 5)
            brd.dataFromAdcToMemAsFifo(params);

        //----------------------------------------------------

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
