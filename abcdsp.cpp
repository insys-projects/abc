#include "isvi.h"
#include "abcdsp.h"
#include "fpga.h"
#include "stream.h"
#include "memory.h"
#include "exceptinfo.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#endif
#include <fcntl.h>
#include <signal.h>

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------

abcdsp::abcdsp()
{
    fprintf(stderr, "====================== Open abcdsp ======================\n");
    m_exit = false;
    m_fpga = new Fpga(0);
    fprintf(stderr, "========================================================\n");
}

//-----------------------------------------------------------------------------

abcdsp::~abcdsp()
{
    if(m_fpga) delete m_fpga;
}

//-----------------------------------------------------------------------------

Fpga* abcdsp::FPGA()
{
    if(!m_fpga) {
        throw except_info("%s, %d: %s() - Invalid FPGA%d pointer!\n", __FILE__, __LINE__, __FUNCTION__);
    }
    return m_fpga;
}

//-----------------------------------------------------------------------------

Memory* abcdsp::DDR3()
{
    return FPGA()->ddr3();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                     REGISTER ACCESS LEVEL INTERFACE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void abcdsp::RegPokeInd(S32 TetrNum, S32 RegNum, U32 RegVal)
{
    FPGA()->FpgaRegPokeInd(TetrNum, RegNum, RegVal);
}

//-----------------------------------------------------------------------------

U32 abcdsp::RegPeekInd(S32 TetrNum, S32 RegNum)
{
    return FPGA()->FpgaRegPeekInd(TetrNum, RegNum);
}

//-----------------------------------------------------------------------------

void abcdsp::RegPokeDir(S32 TetrNum, S32 RegNum, U32 RegVal)
{
    FPGA()->FpgaRegPokeDir(TetrNum,RegNum,RegVal);
}

//-----------------------------------------------------------------------------

U32 abcdsp::RegPeekDir(S32 TetrNum, S32 RegNum)
{
    return FPGA()->FpgaRegPeekDir(TetrNum, RegNum);
}

//-----------------------------------------------------------------------------

bool abcdsp::infoFpga(AMB_CONFIGURATION& info)
{
    return FPGA()->info(info);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                          DMA CHANNEL INTERFACE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int abcdsp::allocateDmaMemory(U32 DmaChan, BRDctrl_StreamCBufAlloc* param)
{
    return FPGA()->allocateDmaMemory(DmaChan, param);
}

//-----------------------------------------------------------------------------

int abcdsp::allocateDmaMemory(U32 DmaChan,
                              void** pBuf,
                              U32 blkSize,
                              U32 blkNum,
                              U32 isSysMem,
                              U32 dir,
                              U32 addr,
                              BRDstrm_Stub **pStub)
{
    return FPGA()->allocateDmaMemory(DmaChan, pBuf, blkSize, blkNum, isSysMem, dir, addr, pStub);
}

//-----------------------------------------------------------------------------

int abcdsp::freeDmaMemory(U32 DmaChan)
{
    return FPGA()->freeDmaMemory(DmaChan);
}

//-----------------------------------------------------------------------------

int abcdsp::startDma(U32 DmaChan, int isCycle)
{
    return FPGA()->startDma(DmaChan, isCycle);
}

//-----------------------------------------------------------------------------

int abcdsp::stopDma(U32 DmaChan)
{
    return FPGA()->stopDma(DmaChan);
}

//-----------------------------------------------------------------------------

int abcdsp::stateDma(U32 DmaChan, U32 msTimeout, int& state, U32& blkNum)
{
    return FPGA()->stateDma(DmaChan, msTimeout, state, blkNum);
}

//-----------------------------------------------------------------------------

int abcdsp::waitDmaBuffer(U32 DmaChan, U32 timeout)
{
    return FPGA()->waitDmaBuffer(DmaChan, timeout);
}

//-----------------------------------------------------------------------------

int abcdsp::waitDmaBlock(U32 DmaChan, U32 timeout)
{
    return FPGA()->waitDmaBlock(DmaChan, timeout);
}

//-----------------------------------------------------------------------------

int abcdsp::resetDmaFifo(U32 DmaChan)
{
    return FPGA()->resetDmaFifo(DmaChan);
}

//-----------------------------------------------------------------------------

int abcdsp::setDmaSource(U32 DmaChan, U32 src)
{
    return FPGA()->setDmaSource(DmaChan, src);
}

//-----------------------------------------------------------------------------

int abcdsp::setDmaDirection(U32 DmaChan, U32 dir)
{
    return FPGA()->setDmaDirection(DmaChan, dir);
}

//-----------------------------------------------------------------------------

int abcdsp::setDmaRequestFlag(U32 DmaChan, U32 flag)
{
    return FPGA()->setDmaRequestFlag(DmaChan, flag);
}

//-----------------------------------------------------------------------------

int abcdsp::adjustDma(U32 DmaChan, U32 adjust)
{
    return FPGA()->adjustDma(DmaChan, adjust);
}

//-----------------------------------------------------------------------------

int abcdsp::doneDma(U32 DmaChan, U32 done)
{
    return FPGA()->doneDma(DmaChan, done);
}

//-----------------------------------------------------------------------------

void abcdsp::resetFifo(U32 trd)
{
    return FPGA()->resetFifo(trd);
}

//-----------------------------------------------------------------------------

void abcdsp::infoDma()
{
    return FPGA()->infoDma();
}

//-----------------------------------------------------------------------------

bool abcdsp::writeBlock(U32 DmaChan, IPC_handle file, int blockNumber)
{
    return FPGA()->writeBlock(DmaChan, file, blockNumber);
}

//-----------------------------------------------------------------------------

bool abcdsp::writeBuffer(U32 DmaChan, IPC_handle file, int fpos)
{
    return FPGA()->writeBuffer(DmaChan, file, fpos);
}

//-----------------------------------------------------------------------------

#define MAIN_TRD 0
#define ADC_TRD  4
#define MEM_TRD  5

//-----------------------------------------------------------------------------

void abcdsp::dataFromAdc(struct app_params_t& params)
{
    fprintf(stderr, "Start testing DMA: %d\n", params.dmaChannel);
    fprintf(stderr, "DMA information:\n" );
    infoDma();

    vector<void*> dmaBlocks(params.dmaBlockCount, 0);

    BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, params.dmaBlockCount, params.dmaBlockSize, dmaBlocks.data(), NULL, };
    fprintf(stderr, "Allocate DMA memory\n");
    allocateDmaMemory(params.dmaChannel, &sSCA);

    // prepare ISVI files for non masked FPGA
    IPC_handle isviFile;
    string flgName;

    IPC_str fname[64];
    BRDC_snprintf(fname, sizeof(fname), _BRDC("data_%d.bin"), 0);
    isviFile = createDataFile(fname);

    IPC_str tmpflg[64];
    BRDC_snprintf(tmpflg, sizeof(tmpflg), _BRDC("data_%d.flg"), 0);
    flgName = tmpflg;
    createFlagFile(flgName.c_str());

    // prepare and start ADC and DMA channels for non masked FPGA
    fprintf(stderr, "Set DMA source\n");
    setDmaSource(params.dmaChannel, ADC_TRD);

    fprintf(stderr, "Set DMA direction\n");
    setDmaDirection(params.dmaChannel, BRDstrm_DIR_IN);

    fprintf(stderr, "Set ADC sync mode\n");
    RegPokeInd(ADC_TRD, 0x18, 1);
    IPC_delay(10);
    RegPokeInd(ADC_TRD, 0x18, 0);
    IPC_delay(10);

    fprintf(stderr, "Program special ADC settings\n");
    specAdcSettings(params);

    fprintf(stderr, "Set ADC channels mask\n");
    RegPokeInd(ADC_TRD, 0x10, params.adcMask);

    fprintf(stderr, "Set ADC clock source\n");
    if(params.clockReference)
        RegPokeInd(ADC_TRD, 0x13, 0x81);
    else
        RegPokeInd(ADC_TRD, 0x13, 0x0);

    fprintf(stderr, "Set ADC test signal forming\n");
    if(params.adcDebug)
        RegPokeInd(ADC_TRD, 0x1A, 0x1);
    else
        RegPokeInd(ADC_TRD, 0x1A, 0x0);

    fprintf(stderr, "Set ADC start mode\n");
    RegPokeInd(ADC_TRD, 0x17, (0x3 << 4));

    fprintf(stderr, "Start DMA channel\n");
    startDma(params.dmaChannel, 0);

    fprintf(stderr, "Start ADC\n");
    RegPokeInd(ADC_TRD, 0, 0x2038);

    unsigned counter = 0;

    while(!exitFlag()) {

        // save ADC data into ISVI files for non masked FPGA
        if( waitDmaBuffer(params.dmaChannel, 2000) < 0 ) {

            u32 status_adc = RegPeekDir(ADC_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X\n", status_adc);
            break;

        } else {

            writeBuffer(params.dmaChannel, isviFile, 0);
            lockDataFile(flgName.c_str(), counter);
        }

        //display 1-st element of each block
        fprintf(stderr, "%d: STATUS = 0x%.4X [", ++counter, (u16)RegPeekDir(ADC_TRD,0x0));
        for(unsigned j=0; j<dmaBlocks.size(); j++) {
            u32* value = (u32*)dmaBlocks.at(j);
            fprintf(stderr, " 0x%.8x ", value[0]);
        }
        fprintf(stderr, "]\r");

        RegPokeInd(ADC_TRD,0,0x0);
        stopDma(params.dmaChannel);
        resetFifo(ADC_TRD);
        resetDmaFifo(params.dmaChannel);
        startDma(params.dmaChannel,0);
        IPC_delay(10);
        RegPokeInd(ADC_TRD,0,0x2038);

        sync();

        IPC_delay(200);
    }

    // stop ADC and DMA channels for non masked FPGA
    // and free DMA buffers
    RegPokeInd(ADC_TRD,0,0x0);
    stopDma(params.dmaChannel);

    IPC_delay(10);

    freeDmaMemory(params.dmaChannel);
    IPC_closeFile(isviFile);
}

//-----------------------------------------------------------------------------

void abcdsp::dataFromAdcToMemAsMem(struct app_params_t& params)
{
    U32 MemBufSize = params.dmaBuffersCount * params.dmaBlockSize * params.dmaBlockCount;
    U32 PostTrigSize = 0;

    // prepare DMA buffers for non masked FPGA
    fprintf(stderr, "Start testing DMA: %d\n", params.dmaChannel);
    fprintf(stderr, "DMA information:\n" );
    infoDma();

    vector<void*> dmaBlocks(params.dmaBlockCount, 0);

    BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, params.dmaBlockCount, params.dmaBlockSize, dmaBlocks.data(), NULL, };
    fprintf(stderr, "Allocate DMA memory\n");
    allocateDmaMemory(params.dmaChannel, &sSCA);


    // prepare ISVI files for non masked FPGA
    IPC_handle isviFile;
    string flgName;

    IPC_str fname[64];
    BRDC_snprintf(fname, sizeof(fname), _BRDC("data_%d.bin"), 0);
    isviFile = createDataFile(fname);

    IPC_str tmpflg[64];
    BRDC_snprintf(tmpflg, sizeof(tmpflg), _BRDC("data_%d.flg"), 0);
    flgName = tmpflg;
    createFlagFile(flgName.c_str());

    unsigned pass_counter = 0;

    fprintf(stderr, "Program special ADC settings\n");
    specAdcSettings(params);

    while(!exitFlag()) {

        // prepare and start ADC and MEM for non masked FPGA
        DDR3()->setMemory(1, 0, PostTrigSize, MemBufSize);
        DDR3()->Enable(true);

        fprintf(stderr, "setDmaSource(MEM_TRD)\n");
        setDmaSource(params.dmaChannel, MEM_TRD);

        fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
        setDmaRequestFlag(params.dmaChannel, BRDstrm_DRQ_HALF);

        fprintf(stderr, "resetFifo(ADC_TRD)\n");
        resetFifo(ADC_TRD);

        fprintf(stderr, "resetFifo(MEM_TRD)\n");
        resetFifo(MEM_TRD);

        fprintf(stderr, "resetDmaFifo()\n");
        resetDmaFifo(params.dmaChannel);

        fprintf(stderr, "setAdcMask(0x%x)\n", params.adcMask);
        RegPokeInd(ADC_TRD, 0x10, params.adcMask);

        fprintf(stderr, "setAdcStartMode(0x%x)\n", (0x3 << 4));
        RegPokeInd(ADC_TRD, 0x17, (0x3 << 4));

        fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
        RegPokeInd(MEM_TRD, 0x0, 0x2038);

        fprintf(stderr, "ADC_TRD: MODE0 = 0x2038\n");
        RegPokeInd(ADC_TRD, 0x0, 0x2038);

        // Save MEM data in ISVI file for non masked FPGA
        fprintf(stderr, "\n");
        for(unsigned counter = 0; counter < params.dmaBuffersCount; counter++) {

            startDma(params.dmaChannel, 0x0); IPC_delay(10);

            if( waitDmaBuffer(params.dmaChannel, 4000) < 0 ) {

                u32 status_adc = RegPeekDir(ADC_TRD, 0x0);
                u32 status_mem = RegPeekDir(MEM_TRD, 0x0);
                fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", status_adc, status_mem);
                break;

            } else {

                writeBuffer(params.dmaChannel, isviFile, counter * params.dmaBlockSize * params.dmaBlockCount);
                fprintf(stderr, "Write DMA buffer: %d\r", counter);
            }

            stopDma(params.dmaChannel);
        }

        lockDataFile(flgName.c_str(), pass_counter);

        fprintf(stderr, "\n");
        RegPokeInd(MEM_TRD, 0x0, 0x0);
        RegPokeInd(ADC_TRD, 0x0, 0x0);
        resetFifo(ADC_TRD);
        resetFifo(MEM_TRD);
        resetDmaFifo(params.dmaChannel);

        ++pass_counter;

        IPC_delay(500);
    }

    // stop ADC, MEM and DMA channels for non masked FPGA
    // and free DMA buffers
    RegPokeInd(MEM_TRD, 0x0, 0x0);
    RegPokeInd(ADC_TRD, 0x0, 0x0);
    stopDma(params.dmaChannel);

    IPC_delay(10);

    freeDmaMemory(params.dmaChannel);
    IPC_closeFile(isviFile);
}

//-----------------------------------------------------------------------------

void abcdsp::dataFromAdcToMemAsFifo(struct app_params_t& params)
{
#if 0
    U32 MemBufSize = params.dmaBuffersCount * params.dmaBlockSize * params.dmaBlockCount;
    U32 PostTrigSize = 0;

    // prepare DMA buffers for non masked FPGA
    vector<void*> Buffers[ADC_FPGA_COUNT];
    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        fprintf(stderr, "Start testing DMA: %d\n", params.dmaChannel);
        fprintf(stderr, "DMA information:\n" );
        infoDma(i);

        vector<void*> dmaBlocks(params.dmaBlockCount, 0);

        BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, params.dmaBlockCount, params.dmaBlockSize, dmaBlocks.data(), NULL, };
        fprintf(stderr, "Allocate DMA memory\n");
        allocateDmaMemory(i, params.dmaChannel, &sSCA);

        Buffers[i] = dmaBlocks;
    }

    // prepare ISVI files for non masked FPGA
    IPC_handle isviFile[ADC_FPGA_COUNT];
#ifdef _WIN64
    wstring flgName[ADC_FPGA_COUNT];
#else
    string flgName[ADC_FPGA_COUNT];
#endif
    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        IPC_str fname[64];
        BRDC_snprintf(fname, sizeof(fname), _BRDC("data_%d%d.bin"), m_slotNumber, i);
        isviFile[i] = createDataFile(fname);

        IPC_str tmpflg[64];
        BRDC_snprintf(tmpflg, sizeof(tmpflg), _BRDC("data_%d%d.flg"), m_slotNumber, i);
        flgName[i] = tmpflg;
        createFlagFile(flgName[i].c_str());
    }


    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        DDR3(i)->setMemory(2, 0, PostTrigSize, MemBufSize);
        DDR3(i)->Enable(true);

        fprintf(stderr, "setDmaSource(ADC_TRD)\n");
        setDmaSource(i, params.dmaChannel, ADC_TRD);

        fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
        setDmaRequestFlag(i, params.dmaChannel, BRDstrm_DRQ_HALF);

        fprintf(stderr, "resetFifo(ADC_TRD)\n");
        resetFifo(i, ADC_TRD);

        fprintf(stderr, "resetFifo(MEM_TRD)\n");
        resetFifo(i, MEM_TRD);

        fprintf(stderr, "resetDmaFifo()\n");
        resetDmaFifo(i, params.dmaChannel);

        fprintf(stderr, "startDma(): Cycle = %d\n", 0x0);
        startDma(i, params.dmaChannel, 0x0);

        fprintf(stderr, "setAdcMask(0x%x)\n", params.adcMask);
        RegPokeInd(i, ADC_TRD, 0x10, params.adcMask);

        fprintf(stderr, "setAdcStartMode(0x%x)\n", (0x3 << 4));
        RegPokeInd(i, ADC_TRD, 0x17, (0x3 << 4));

        fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
        RegPokeInd(i, MEM_TRD, 0x0, 0x2038);
        IPC_delay(10);

        fprintf(stderr, "ADC_TRD: MODE0 = 0x2038\n");
        RegPokeInd(i, ADC_TRD, 0x0, 0x2038);
        IPC_delay(10);
    }

    unsigned counter = 0;

    while(1) {

        for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

            if(!(params.fpgaMask & (0x1 << i)))
                continue;

            if( waitDmaBuffer(i, params.dmaChannel, 2000) < 0 ) {

                u32 status_adc = RegPeekDir(i, ADC_TRD, 0x0);
                u32 status_mem = RegPeekDir(i, MEM_TRD, 0x0);
                fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", status_adc, status_mem);
                break;

            } else {

                writeBuffer(i, params.dmaChannel, isviFile[i], 0);
                lockDataFile(flgName[i].c_str(), counter);
            }

            u32 status_adc = RegPeekDir(i, ADC_TRD, 0x0);
            u32 status_mem = RegPeekDir(i, MEM_TRD, 0x0);
            fprintf(stderr, "%d: ADC: 0x%.4X - MEM: 0x%.4X [", ++counter, (u16)status_adc, (u16)status_mem);
            for(unsigned j=0; j<Buffers[i].size(); j++) {
                u32* value = (u32*)Buffers[i].at(j);
                fprintf(stderr, " 0x%.8x ", value[0]);
            }
            fprintf(stderr, " ]\r");


            RegPokeInd(i, ADC_TRD, 0x0, 0x0);
            RegPokeInd(i, MEM_TRD, 0x0, 0x0);
            stopDma(i, params.dmaChannel);
            resetFifo(i, ADC_TRD);
            resetFifo(i, MEM_TRD);
            resetDmaFifo(i, params.dmaChannel);
            startDma(i,params.dmaChannel,0x0);
            IPC_delay(10);
            RegPokeInd(i, ADC_TRD, 0x0, 0x2038);
            RegPokeInd(i, MEM_TRD, 0x0, 0x2038);
        }

        if(exitFlag()) {
            fprintf(stderr, "\n");
            break;
        }

        IPC_delay(250);
    }

    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        RegPokeInd(i, ADC_TRD, 0x0, 0x0);
        RegPokeInd(i, MEM_TRD, 0x0, 0x0);
        stopDma(i, params.dmaChannel);

        freeDmaMemory(i, params.dmaChannel);
        IPC_closeFile(isviFile[i]);
    }
#endif
}

//-----------------------------------------------------------------------------

void abcdsp::setExitFlag(bool exit)
{
    m_exit = exit;
}

//-----------------------------------------------------------------------------

bool abcdsp::exitFlag()
{
    if(IPC_kbhit())
        return true;
    return m_exit;
}

//-----------------------------------------------------------------------------

void abcdsp::dataFromMain(struct app_params_t& params)
{
    // prepare DMA buffers for non masked FPGA
    fprintf(stderr, "Start testing DMA: %d\n", params.dmaChannel);
    fprintf(stderr, "DMA information:\n" );

    infoDma();

    vector<void*> dmaBlocks(params.dmaBlockCount, 0);
    BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, params.dmaBlockCount, params.dmaBlockSize, dmaBlocks.data(), NULL, };
    fprintf(stderr, "Allocate DMA memory\n");
    allocateDmaMemory(params.dmaChannel, &sSCA);

    // prepare ISVI files for non masked FPGA
    IPC_handle isviFile;
    string flgName;
    IPC_str fname[64];

    BRDC_snprintf(fname, sizeof(fname), _BRDC("data_%d.bin"), 0);
    isviFile = createDataFile(fname);

    IPC_str tmpflg[64];
    BRDC_snprintf(tmpflg, sizeof(tmpflg), _BRDC("data_%d.flg"), 0);
    flgName = tmpflg;
    createFlagFile(flgName.c_str());

    fprintf(stderr, "Set DMA source\n");
    setDmaSource(params.dmaChannel, MAIN_TRD);

    fprintf(stderr, "Set DMA direction\n");
    setDmaDirection(params.dmaChannel, BRDstrm_DIR_IN);

    fprintf(stderr, "Set MAIN test mode\n");
    RegPokeInd(MAIN_TRD, 0xC, 0x1);

    fprintf(stderr, "Start DMA\n");
    startDma(params.dmaChannel, 0);

    fprintf(stderr, "Start MAIN\n");
    RegPokeInd(MAIN_TRD, 0, 0x2038);

    unsigned counter = 0;

    while(!exitFlag()) {

        if( waitDmaBuffer(params.dmaChannel, 2000) < 0 ) {

            u32 status_main = RegPeekDir(MAIN_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! MAIN STATUS = 0x%.4X\n", status_main);
            break;

        } else {

            writeBuffer(params.dmaChannel, isviFile, 0);
            lockDataFile(flgName.c_str(), counter);
        }

        //fprintf(stderr, "%d: STATUS = 0x%.4X [", ++counter, (u16)RegPeekDir(MAIN_TRD,0x0));
        //for(unsigned j=0; j<dmaBlocks.size(); j++) {
        //    u32* value = (u32*)dmaBlocks.at(j);
        //    fprintf(stderr, " 0x%.8x ", value[0]);
        //}
        //fprintf(stderr, " ]\r");

        checkMainDataStream(params.dmaBlockSize, dmaBlocks, 0);

        IPC_delay(100);

        RegPokeInd(MAIN_TRD,0,0x0);
        stopDma(params.dmaChannel);
        resetFifo(MAIN_TRD);
        resetDmaFifo(params.dmaChannel);
        RegPokeInd(MAIN_TRD, 0xC, 0x0);
        startDma(params.dmaChannel,0);
        IPC_delay(10);
        RegPokeInd(MAIN_TRD, 0xC, 0x1);
        RegPokeInd(MAIN_TRD,0,0x2038);
    }


    IPC_delay(50);

    RegPokeInd(MAIN_TRD,0,0x0);
    stopDma(params.dmaChannel);
    freeDmaMemory(params.dmaChannel);
    IPC_closeFile(isviFile);
}

//-----------------------------------------------------------------------------

void abcdsp::dataFromMainToMemAsMem(struct app_params_t& params)
{
    /*
    U32 MemBufSize = params.dmaBuffersCount * params.dmaBlockSize * params.dmaBlockCount;
    U32 PostTrigSize = 0;

    // prepare DMA buffers for non masked FPGA
    vector<void*> Buffers[ADC_FPGA_COUNT];
    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        fprintf(stderr, "Start testing DMA: %d\n", params.dmaChannel);
        fprintf(stderr, "DMA information:\n" );
        infoDma(i);

        vector<void*> dmaBlocks(params.dmaBlockCount, 0);

        BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, params.dmaBlockCount, params.dmaBlockSize, dmaBlocks.data(), NULL, };
        fprintf(stderr, "Allocate DMA memory\n");
        allocateDmaMemory(i, params.dmaChannel, &sSCA);

        Buffers[i] = dmaBlocks;
    }

    // prepare ISVI files for non masked FPGA
    IPC_handle isviFile[ADC_FPGA_COUNT];
#ifdef _WIN64
    wstring flgName[ADC_FPGA_COUNT];
#else
    string flgName[ADC_FPGA_COUNT];
#endif
    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        IPC_str fname[64];
        BRDC_snprintf(fname, sizeof(fname), _BRDC("data_%d%d.bin"), m_slotNumber, i);
        isviFile[i] = createDataFile(fname);

        IPC_str tmpflg[64];
        BRDC_snprintf(tmpflg, sizeof(tmpflg), _BRDC("data_%d%d.flg"), m_slotNumber, i);
        flgName[i] = tmpflg;
        createFlagFile(flgName[i].c_str());
    }

    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        DDR3(i)->setMemory(1, 0, PostTrigSize, MemBufSize);
        DDR3(i)->Enable(true);

        fprintf(stderr, "setDmaSource(MEM_TRD)\n");
        setDmaSource(i, params.dmaChannel, MEM_TRD);

        fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
        setDmaRequestFlag(i, params.dmaChannel, BRDstrm_DRQ_HALF);

        fprintf(stderr, "resetFifo(MAIN_TRD)\n");
        resetFifo(i, MAIN_TRD);

        fprintf(stderr, "resetFifo(MEM_TRD)\n");
        resetFifo(i, MEM_TRD);

        fprintf(stderr, "resetDmaFifo()\n");
        resetDmaFifo(i, params.dmaChannel);

        fprintf(stderr, "setMainTestMode(0x%x)\n", 0x1);
        RegPokeInd(i, MAIN_TRD, 0xC, 0x1);

        fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
        RegPokeInd(i, MEM_TRD, 0x0, 0x2038);
        IPC_delay(10);

        fprintf(stderr, "MAIN_TRD: MODE0 = 0x2038\n");
        RegPokeInd(i, ADC_TRD, 0x0, 0x2038);
        IPC_delay(10);
    }

    fprintf(stderr, "Waiting data DDR3...");
    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        bool ok = true;
        while(!DDR3(i)->AcqComplete()) {
            if(exitFlag()) {
                ok = false;
                break;
            }
        }
        IPC_delay(10);
        if(ok) {
            fprintf(stderr, "OK\n");
        } else {
            fprintf(stderr, "ERROR\n");
            return;
        }

        fprintf(stderr, "MEM_TRD: MODE0 = 0x0\n");
        RegPokeInd(i, MEM_TRD, 0x0, 0x0);

        fprintf(stderr, "MAIN_TRD: MODE0 = 0x0");
        RegPokeInd(i, MAIN_TRD, 0x0, 0x0);

        DDR3(i)->Enable(false);
    }


    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        unsigned counter = 0;

        fprintf(stderr, "\n");
        for(counter = 0; counter < params.dmaBuffersCount; counter++) {

            startDma(i, params.dmaChannel, 0x0);

            IPC_delay(10);

            if( waitDmaBuffer(i, params.dmaChannel, 2000) < 0 ) {

                u32 status_main = RegPeekDir(i, MAIN_TRD, 0x0);
                u32 status_mem = RegPeekDir(i, MEM_TRD, 0x0);
                fprintf( stderr, "ERROR TIMEOUT! MAIN STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", status_main, status_mem);
                break;

            } else {

                writeBuffer(i, params.dmaChannel, isviFile, counter * params.dmaBlockCount * params.dmaBlockSize);
                lockDataFile(flgName[i].c_str(), counter);
                fprintf(stderr, "Write DMA buffer: %d\r", counter);
                //sync();
            }
        }
        fprintf(stderr, "\n");

        RegPokeInd(i, MEM_TRD, 0x0, 0x0);
        RegPokeInd(i, MAIN_TRD, 0x0, 0x0);
        stopDma(i, params.dmaChannel);
    }

    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        freeDmaMemory(i, params.dmaChannel);
        IPC_closeFile(isviFile[i]);
    }
*/
}

//-----------------------------------------------------------------------------

void abcdsp::dataFromMainToMemAsFifo(struct app_params_t& params)
{
    /*
    U32 MemBufSize = params.dmaBlockSize * params.dmaBlockCount;
    U32 PostTrigSize = 0;

    // prepare DMA buffers for non masked FPGA
    vector<void*> Buffers[ADC_FPGA_COUNT];
    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        fprintf(stderr, "Start testing DMA: %d\n", params.dmaChannel);
        fprintf(stderr, "DMA information:\n" );
        infoDma(i);

        vector<void*> dmaBlocks(params.dmaBlockCount, 0);

        BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, params.dmaBlockCount, params.dmaBlockSize, dmaBlocks.data(), NULL, };
        fprintf(stderr, "Allocate DMA memory\n");
        allocateDmaMemory(i, params.dmaChannel, &sSCA);

        Buffers[i] = dmaBlocks;
    }

    // prepare ISVI files for non masked FPGA
    IPC_handle isviFile[ADC_FPGA_COUNT];
#ifdef _WIN64
    wstring flgName[ADC_FPGA_COUNT];
#else
    string flgName[ADC_FPGA_COUNT];
#endif
    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        IPC_str fname[64];
        BRDC_snprintf(fname, sizeof(fname), _BRDC("data_%d%d.bin"), m_slotNumber, i);
        isviFile[i] = createDataFile(fname);

        IPC_str tmpflg[64];
        BRDC_snprintf(tmpflg, sizeof(tmpflg), _BRDC("data_%d%d.flg"), m_slotNumber, i);
        flgName[i] = tmpflg;
        createFlagFile(flgName[i].c_str());
    }

    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        DDR3(i)->setMemory(2, 0, PostTrigSize, MemBufSize);
        DDR3(i)->Enable(true);

        fprintf(stderr, "setDmaSource(MAIN_TRD)\n");
        setDmaSource(i, params.dmaChannel, MAIN_TRD);

        fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
        setDmaRequestFlag(i, params.dmaChannel, BRDstrm_DRQ_HALF);

        fprintf(stderr, "resetFifo(MAIN_TRD)\n");
        resetFifo(i, MAIN_TRD);

        fprintf(stderr, "resetFifo(MEM_TRD)\n");
        resetFifo(i, MEM_TRD);

        fprintf(stderr, "resetDmaFifo()\n");
        resetDmaFifo(i, params.dmaChannel);

        fprintf(stderr, "setMainTestMode(0x%x)\n", 0x1);
        RegPokeInd(i, MAIN_TRD, 0xC, 0x1);

        fprintf(stderr, "startDma(): Cycle = %d\n", 0x0);
        startDma(i, params.dmaChannel, 0x0);

        fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
        RegPokeInd(i, MEM_TRD, 0x0, 0x2038);
        IPC_delay(10);

        fprintf(stderr, "MAIN_TRD: MODE0 = 0x2038\n");
        RegPokeInd(i, MAIN_TRD, 0x0, 0x2038);
        IPC_delay(10);
    }

    unsigned counter = 0;

    while(1) {

        for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

            if(!(params.fpgaMask & (0x1 << i)))
                continue;


            if( waitDmaBuffer(i, params.dmaChannel, 2000) < 0 ) {

                u32 status_main = RegPeekDir(i, MAIN_TRD, 0x0);
                u32 status_mem = RegPeekDir(i, MEM_TRD, 0x0);
                fprintf( stderr, "ERROR TIMEOUT! MAIN STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", (u16)status_main, (u16)status_mem);
                break;

            } else {

                writeBuffer(i, params.dmaChannel, isviFile, 0);
                lockDataFile(flgName[i].c_str(), counter);
            }

            u32 status_main = RegPeekDir(i, MAIN_TRD, 0x0);
            u32 status_mem = RegPeekDir(i, MEM_TRD, 0x0);
            fprintf(stderr, "%d: MAIN: 0x%.4X - MEM: 0x%.4X [", ++counter, (u16)status_main, (u16)status_mem);
            for(unsigned j=0; j<Buffers[i].size(); j++) {
                u32* value = (u32*)Buffers[i].at(j);
                fprintf(stderr, " 0x%.8x ", value[0]);
            }
            fprintf(stderr, " ]\r");

            RegPokeInd(i, MAIN_TRD, 0x0, 0x0);
            RegPokeInd(i, MEM_TRD, 0x0, 0x0);
            stopDma(i, params.dmaChannel);
            resetFifo(i, MAIN_TRD);
            resetFifo(i, MEM_TRD);
            resetDmaFifo(i, params.dmaChannel);
            startDma(i,params.dmaChannel,0x0);
            IPC_delay(10);
            RegPokeInd(i, MAIN_TRD, 0x0, 0x2038);
            RegPokeInd(i, MEM_TRD, 0x0, 0x2038);

        }

        if(exitFlag()) {
            fprintf(stderr, "\n");
            break;
        }

        IPC_delay(250);
    }

    for(unsigned i=0; i<ADC_FPGA_COUNT; ++i) {

        if(!(params.fpgaMask & (0x1 << i)))
            continue;

        RegPokeInd(i, MAIN_TRD, 0x0, 0x0);
        RegPokeInd(i, MEM_TRD, 0x0, 0x0);
        stopDma(i, params.dmaChannel);
        freeDmaMemory(i, params.dmaChannel);
        IPC_closeFile(isviFile[i]);
    }
*/
}

//-----------------------------------------------------------------------------

void abcdsp::getFpgaTemperature(float& t)
{
    unsigned temp = 0;

    RegPokeInd( 0, 0x210, 0 );
    temp = RegPeekInd( 0, 0x211 );

    temp >>= 6;
    temp &= 0x3FF;
    t = (temp*503.975)/1024-273.15;
}

//-----------------------------------------------------------------------------

void abcdsp::checkMainDataStream(U32 dmaBlockSize, const std::vector<void*>& Buffer, bool width)
{
    __int64 data_rd = 0ULL;
    __int64 data_wr = 2ULL;
    __int64 data_wrh = 0xAA55ULL;
    U32 cnt_err = 0;

    for(U32 iBlock = 0; iBlock < Buffer.size(); iBlock++)
    {
        fprintf(stderr, "BLOCK%d\n", iBlock);

        __int64 *pBlockData = (__int64*)Buffer.at(iBlock);

        for(U32 i = 0; i < dmaBlockSize/8; i++)
        {
            data_rd = pBlockData[i];
            if(data_wr != data_rd)
            {
                cnt_err++;
                if(cnt_err < 16)
                    fprintf(stderr, _BRDC("%.2d: (%d, %d): wr %016llX: rd %016llX\n"), cnt_err, iBlock, i, data_wr, data_rd);
                data_wr = data_rd;
            }
            U32 data_h = U32(data_wr>>32);
            U32 f63 = data_h >> 31;
            U32 f62 = data_h >> 30;
            U32 f60 = data_h >> 28;
            U32 f59 = data_h >> 27;
            U32 f0 = (f63 ^ f62 ^ f60 ^ f59)&1;

            data_wr <<= 1;
            data_wr &= ~1;
            data_wr |=f0;

            if(width)
            {
                data_rd = pBlockData[i+1];
                if(data_wrh != data_rd)
                {
                    cnt_err++;
                    if(cnt_err < 10)
                        fprintf(stderr, _BRDC("Error (%d, %d): wr %016llX: rd %016llX\n"), iBlock, i, data_wrh, data_rd);
                    data_wrh = data_rd;
                }
                U32 data_h = U32(data_wrh>>32);
                U32 f63 = data_h >> 31;
                U32 f62 = data_h >> 30;
                U32 f60 = data_h >> 28;
                U32 f59 = data_h >> 27;
                U32 f0 = (f63 ^ f62 ^ f60 ^ f59)&1;

                data_wrh <<= 1;
                data_wrh &= ~1;
                data_wrh |=f0;
                i++;
            }
        }

        fprintf(stderr, "\n");
    }
}

//-----------------------------------------------------------------------------

bool abcdsp::writeSpd(U32 devSpdNum, U32 devSpdReg, U32 devSpdRegData, U32 spdCtrl)
{
    int timeout = 0;

    // ожидаем готовности тетрады
    U32 status = 0;
    while(1) {
        status = RegPeekDir(4,0);
        if(status & 0x1)
            break;
        timeout++;
        if(timeout > 100) {
            fprintf(stderr, "%s(): TIMEOUT\n", __FUNCTION__);
            return false;
        }
        IPC_delay(10);
    }

    // выбираем устройство
    RegPokeInd(4, 0x203, devSpdNum);

    // записываем адрес
    RegPokeInd(4, 0x205, devSpdReg);

    // записываем данные
    RegPokeInd(4, 0x206, devSpdRegData);

    // посылаем команду записи
    RegPokeInd(4, 0x204, spdCtrl|0x2);

    // ожидаем готовности тетрады
    status = 0;
    timeout = 0;
    while(1) {
        status = RegPeekDir(4,0);
        if(status & 0x1)
            break;
        timeout++;
        if(timeout > 100) {
            fprintf(stderr, "%s(): TIMEOUT\n", __FUNCTION__);
            return false;
        }
        IPC_delay(10);
    }
    return true;
}

//-----------------------------------------------------------------------------

bool abcdsp::readSpdDev(U32 devSpdNum, U32 devSpdReg, U32 spdCtrl, U32& devSpdRegData)
{
    int timeout = 0;

    // ожидаем готовности тетрады
    U32 status = 0;
    while(1) {
        status = RegPeekDir(4,0);
        if(status & 0x1)
            break;
        timeout++;
        if(timeout > 100) {
            fprintf(stderr, "%s(): TIMEOUT\n", __FUNCTION__);
            return false;
        }
        IPC_delay(10);
    }

    // выбираем устройство
    RegPokeInd(4, 0x203, devSpdNum);

    // записываем адрес
    RegPokeInd(4, 0x205, devSpdReg);

    // посылаем команду чтения
    RegPokeInd(4, 0x204, spdCtrl|0x1);

    // ожидаем готовности тетрады
    status = 0;
    timeout = 0;
    while(1) {
        status = RegPeekDir(4,0);
        if(status & 0x1)
            break;
        timeout++;
        if(timeout > 100) {
            fprintf(stderr, "%s(): TIMEOUT\n", __FUNCTION__);
            return false;
        }
        IPC_delay(10);
    }

    // считываем данные
    devSpdRegData = RegPeekInd(4, 0x206);

    return true;
}

//-----------------------------------------------------------------------------

void abcdsp::specAdcSettings(struct app_params_t& params)
{
    writeSpd(0x0, 0x1, 0x8, (1 << 12));
    writeSpd(0x0, 0x3, (params.analogOffset >> 8) & 0x1, (1 << 12));
    writeSpd(0x0, 0x4, (params.analogOffset & 0xFF), (1 << 12));
    writeSpd(0x0, 0x5, 0x28, (1 << 12));
}

//-----------------------------------------------------------------------------

void abcdsp::specDacSettings(struct app_params_t& params)
{
}

//-----------------------------------------------------------------------------