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

abcdsp::abcdsp(const struct app_params_t& params) : m_params(params)
{
    fprintf(stderr, "====================== ABCDSP ======================\n");
    m_exit = false;
    m_fpga = new Fpga(0);

    fprintf( stderr, "ABCDSP TRD LIST\n" );
    if(m_fpga->fpgaTrd(0, 0x01, m_mainTrd)) {
        fprintf( stderr, "%d) MAIN:     0x%.2X\n", m_mainTrd.number, m_mainTrd.id );
    }
    if(m_fpga->fpgaTrd(0, 0xBB, m_uartTrd)) {
        fprintf( stderr, "%d) UART:     0x%.2X\n", m_uartTrd.number, m_uartTrd.id );
    }
    if(m_fpga->fpgaTrd(0, 0xB8, m_adcTrd)) {
        fprintf( stderr, "%d) ADC:      0x%.2X\n", m_adcTrd.number, m_adcTrd.id );
    }
    if(m_fpga->fpgaTrd(0, 0xB9, m_dacTrd)) {
        fprintf( stderr, "%d) DAC:      0x%.2X\n", m_dacTrd.number, m_dacTrd.id );
    }
    if(m_fpga->fpgaTrd(0, 0x9B, m_memTrd)) {
        fprintf( stderr, "%d) MEM:      0x%.2X\n", m_memTrd.number, m_memTrd.id );
    }
    m_dac = new dac(m_fpga, params);
    fprintf(stderr, "====================================================\n");
}

//-----------------------------------------------------------------------------

abcdsp::~abcdsp()
{
    if(m_dac) delete m_dac;
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

U32 calc_stmode(const struct app_params_t& params)
{
    U32 stmode = 0x0;

    if(params.AdcStartBaseSource) {
        stmode |= 0x87;
    } else {
        stmode &= ~0x80;
    }

    if(params.AdcStartBaseInverting) {
        stmode |= 0x40;
    } else {
        stmode &= ~0x40;
    }

    return stmode;
}

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

    string isvi_hdr;
    createIsviHeader(isvi_hdr, 0, params);

    // prepare and start ADC and DMA channels for non masked FPGA
    fprintf(stderr, "Set DMA source\n");
    setDmaSource(params.dmaChannel, m_adcTrd.number);

    fprintf(stderr, "Set DMA direction\n");
    setDmaDirection(params.dmaChannel, BRDstrm_DIR_IN);

    fprintf(stderr, "Set ADC sync mode\n");
    RegPokeInd(m_adcTrd.number, 0x18, 1);
    IPC_delay(10);
    RegPokeInd(m_adcTrd.number, 0x18, 0);
    IPC_delay(10);

    fprintf(stderr, "Program special ADC settings\n");
    specAdcSettings(params);

    fprintf(stderr, "Set ADC channels mask\n");
    RegPokeInd(m_adcTrd.number, 0x10, 0x3);

    fprintf(stderr, "Set ADC clock source\n");
    if(params.SysRefClockSource)
        RegPokeInd(m_adcTrd.number, 0x13, 0x81);
    else
        RegPokeInd(m_adcTrd.number, 0x13, 0x0);

    fprintf(stderr, "Set ADC test signal forming\n");
    if(params.AdcTest)
        RegPokeInd(m_adcTrd.number, 0x1A, 0x1);
    else
        RegPokeInd(m_adcTrd.number, 0x1A, 0x0);

    //fprintf(stderr, "Set ADC start mode\n");
    //RegPokeInd(m_adcTrd.number, 0x17, (0x3 << 4));

    fprintf(stderr, "Start DMA channel\n");
    startDma(params.dmaChannel, 0);

    fprintf(stderr, "Start ADC\n");
    RegPokeInd(m_adcTrd.number, 0, 0x2038);

    unsigned counter = 0;

    while(!exitFlag()) {

        // save ADC data into ISVI files for non masked FPGA
        if( waitDmaBuffer(params.dmaChannel, 2000) < 0 ) {

            u32 status_adc = RegPeekDir(m_adcTrd.number, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X\n", status_adc & 0xFFFF);
            break;

        } else {

            writeBuffer(params.dmaChannel, isviFile, 0);
            IPC_writeFile(isviFile, (void*)isvi_hdr.c_str(), isvi_hdr.size());
            lockDataFile(flgName.c_str(), counter);
        }

        //display 1-st element of each block
        fprintf(stderr, "%d: STATUS = 0x%.4X [", ++counter, (u16)RegPeekDir(m_adcTrd.number,0x0));
        for(unsigned j=0; j<dmaBlocks.size(); j++) {
            u32* value = (u32*)dmaBlocks.at(j);
            fprintf(stderr, " 0x%.8x ", value[0]);
        }
        fprintf(stderr, "]\r");

        RegPokeInd(m_adcTrd.number,0,0x0);
        stopDma(params.dmaChannel);
        resetFifo(m_adcTrd.number);
        resetDmaFifo(params.dmaChannel);
        startDma(params.dmaChannel,0);
        IPC_delay(10);
        RegPokeInd(m_adcTrd.number,0,0x2038);
        IPC_delay(10);

        if(params.AdcCycle)
            continue;
        else
            break;
    }

    // stop ADC and DMA channels for non masked FPGA
    // and free DMA buffers
    RegPokeInd(m_adcTrd.number,0,0x0);
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

    string isvi_hdr;
    createIsviHeader(isvi_hdr, 0, params);

    unsigned pass_counter = 0;

    fprintf(stderr, "Program special ADC settings\n");
    specAdcSettings(params);

    while(!exitFlag()) {

        DDR3()->Enable(false);

        setDmaSource(params.dmaChannel, m_memTrd.number);
        setDmaRequestFlag(params.dmaChannel, BRDstrm_DRQ_HALF);

        //brd->RegPokeInd(j, m_adcTrd.number, 0xC, 0x100);

        resetFifo(m_adcTrd.number);
        resetFifo(m_memTrd.number);
        resetDmaFifo(params.dmaChannel);

        RegPokeInd(m_adcTrd.number, 0x10, 0x3);

        DDR3()->setMemory(1, 0, PostTrigSize, MemBufSize);

        U32 stmode = calc_stmode(params);

        RegPokeInd(m_adcTrd.number, 0x5, stmode);
        RegPokeInd(m_adcTrd.number, 0x17, (0x3 << 4));
        resetFifo(m_adcTrd.number);
        resetFifo(m_memTrd.number);
        RegPokeInd(m_memTrd.number, 0x0, 0x2038);
        RegPokeInd(m_adcTrd.number, 0x0, 0x2038);

        IPC_delay(1);

        U32 mode01 = RegPeekInd(m_adcTrd.number, 0);
        U32 mode02 = RegPeekInd(m_memTrd.number, 0);
        mode01 = mode01;
        mode02 = mode02;

        // Save MEM data in ISVI file for non masked FPGA
        //fprintf(stderr, "\n");
        for(unsigned counter = 0; counter < params.dmaBuffersCount; counter++) {

            startDma(params.dmaChannel, 0x0); IPC_delay(10);

            if( waitDmaBuffer(params.dmaChannel, 4000) < 0 ) {

                u32 status_adc = RegPeekDir(m_adcTrd.number, 0x0);
                u32 status_mem = RegPeekDir(m_memTrd.number, 0x0);
                fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", status_adc, status_mem);
                break;

            } else {

                writeBuffer(params.dmaChannel, isviFile, counter * params.dmaBlockSize * params.dmaBlockCount);
                fprintf(stderr, "Write DMA buffer: %d\r", counter);
            }

            stopDma(params.dmaChannel);
        }

        IPC_writeFile(isviFile, (void*)isvi_hdr.c_str(), isvi_hdr.size());
        lockDataFile(flgName.c_str(), pass_counter);

        //fprintf(stderr, "\n");
        RegPokeInd(m_memTrd.number, 0x0, 0x0);
        RegPokeInd(m_adcTrd.number, 0x0, 0x0);
        resetFifo(m_adcTrd.number);
        resetFifo(m_memTrd.number);
        resetDmaFifo(params.dmaChannel);

        ++pass_counter;

        IPC_delay(500);

        if(params.AdcCycle)
            continue;
        else
            break;
    }

    // stop ADC, MEM and DMA channels for non masked FPGA
    // and free DMA buffers
    RegPokeInd(m_memTrd.number, 0x0, 0x0);
    RegPokeInd(m_adcTrd.number, 0x0, 0x0);
    stopDma(params.dmaChannel);

    IPC_delay(10);

    freeDmaMemory(params.dmaChannel);
    IPC_closeFile(isviFile);
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
    setDmaSource(params.dmaChannel, m_mainTrd.number);

    fprintf(stderr, "Set DMA direction\n");
    setDmaDirection(params.dmaChannel, BRDstrm_DIR_IN);

    fprintf(stderr, "Set MAIN test mode\n");
    RegPokeInd(m_mainTrd.number, 0xC, 0x1);

    fprintf(stderr, "Start DMA\n");
    startDma(params.dmaChannel, 0);

    fprintf(stderr, "Start MAIN\n");
    RegPokeInd(m_mainTrd.number, 0, 0x2038);

    unsigned counter = 0;

    while(!exitFlag()) {

        if( waitDmaBuffer(params.dmaChannel, 2000) < 0 ) {

            u32 status_main = RegPeekDir(m_mainTrd.number, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! MAIN STATUS = 0x%.4X\n", status_main);
            break;

        } else {

            writeBuffer(params.dmaChannel, isviFile, 0);
            lockDataFile(flgName.c_str(), counter);
        }

        //fprintf(stderr, "%d: STATUS = 0x%.4X [", ++counter, (u16)RegPeekDir(m_mainTrd.number,0x0));
        //for(unsigned j=0; j<dmaBlocks.size(); j++) {
        //    u32* value = (u32*)dmaBlocks.at(j);
        //    fprintf(stderr, " 0x%.8x ", value[0]);
        //}
        //fprintf(stderr, " ]\r");

        checkMainDataStream(params.dmaBlockSize, dmaBlocks, 0);

        IPC_delay(100);

        RegPokeInd(m_mainTrd.number,0,0x0);
        stopDma(params.dmaChannel);
        resetFifo(m_mainTrd.number);
        resetDmaFifo(params.dmaChannel);
        RegPokeInd(m_mainTrd.number, 0xC, 0x0);
        startDma(params.dmaChannel,0);
        IPC_delay(10);
        RegPokeInd(m_mainTrd.number, 0xC, 0x1);
        RegPokeInd(m_mainTrd.number,0,0x2038);
    }


    IPC_delay(50);

    RegPokeInd(m_mainTrd.number,0,0x0);
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
        setDmaSource(i, params.dmaChannel, m_memTrd.number);

        fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
        setDmaRequestFlag(i, params.dmaChannel, BRDstrm_DRQ_HALF);

        fprintf(stderr, "resetFifo(MAIN_TRD)\n");
        resetFifo(i, m_mainTrd.number);

        fprintf(stderr, "resetFifo(MEM_TRD)\n");
        resetFifo(i, m_memTrd.number);

        fprintf(stderr, "resetDmaFifo()\n");
        resetDmaFifo(i, params.dmaChannel);

        fprintf(stderr, "setMainTestMode(0x%x)\n", 0x1);
        RegPokeInd(i, m_mainTrd.number, 0xC, 0x1);

        fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
        RegPokeInd(i, m_memTrd.number, 0x0, 0x2038);
        IPC_delay(10);

        fprintf(stderr, "MAIN_TRD: MODE0 = 0x2038\n");
        RegPokeInd(i, m_mainTrd.number, 0x0, 0x2038);
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
        RegPokeInd(i, m_memTrd.number, 0x0, 0x0);

        fprintf(stderr, "MAIN_TRD: MODE0 = 0x0");
        RegPokeInd(i, m_mainTrd.number, 0x0, 0x0);

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
                u32 status_mem = RegPeekDir(i, m_memTrd.number, 0x0);
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

        RegPokeInd(i, m_memTrd.number, 0x0, 0x0);
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

void abcdsp::specAdcSettings(struct app_params_t& params)
{
    FPGA()->writeSpdDev(m_adcTrd.number, 0x0, 0x1, 0x8, (1 << 12));
    FPGA()->writeSpdDev(m_adcTrd.number, 0x0, 0x3, (params.AdcBias0 >> 8) & 0x1, (1 << 12));
    FPGA()->writeSpdDev(m_adcTrd.number, 0x0, 0x4, (params.AdcBias0 & 0xFF), (1 << 12));
    FPGA()->writeSpdDev(m_adcTrd.number, 0x0, 0x5, 0x28, (1 << 12));
}

//-----------------------------------------------------------------------------

bool abcdsp::fpgaSerialMode(U8 speed, bool loopback)
{
    if(m_uartTrd.id != 0xBB) {
        return false;
    }

    fprintf(stderr, "UART: TRDID = 0x%X, TRDNUM = %d\n", m_uartTrd.id, m_uartTrd.number);

    U32 mode = speed;

    if(loopback)
        mode |= UART_LOOPBACK_MODE;

    FPGA()->FpgaRegPokeInd(m_uartTrd.number, 0x18, mode);

    return true;
}

//-----------------------------------------------------------------------------

bool abcdsp::serialWrite(const std::string& data, int timeout)
{
    for(unsigned i=0; i<data.size(); i++) {
        bool ok = fpgaSerialWrite(data.at(i), timeout);
        if(!ok) {
            fprintf(stderr, "UART: WRITE BUFFER ERROR\n");
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

bool abcdsp::serialRead(std::string& data, int timeout)
{
    data.clear();

    while(!exitFlag()) {

        U8 rdata = 0;
        if( fpgaSerialRead(rdata, timeout) ) {
            data.push_back(rdata);
            if(rdata == 0xD)
                break;
        }
    }

    if(exitFlag())
        return false;

    return true;
}

//-----------------------------------------------------------------------------

bool abcdsp::fpgaSerialWrite(U8 data, int timeout)
{
    if(m_uartTrd.id != 0xBB) {
        return false;
    }

    U32 uart_status = 0;
    while(timeout >= 0) {
        uart_status = FPGA()->FpgaRegPeekInd(m_uartTrd.number, 0x206);
        if(uart_status & 0x4) { // UART_TX_FIFO FULL
            timeout -= 1;
            IPC_delay(1);
            if(timeout <= 0) {
                return false;
            }
        } else {
            break;
        }
    }

    FPGA()->FpgaRegPokeInd(m_uartTrd.number, 0x202, data);

    return true;
}

//-----------------------------------------------------------------------------

bool abcdsp::fpgaSerialRead(U8& data, int timeout)
{
    if(m_uartTrd.id != 0xBB) {
        return false;
    }

    U32 uart_status = 0;

    while(timeout >= 0) {
        uart_status = FPGA()->FpgaRegPeekInd(m_uartTrd.number, 0x206);
        if(uart_status & 0x8) {
            FPGA()->FpgaRegPokeInd(m_uartTrd.number, 0x204, 0x00);
            break;
        }
        IPC_delay(1);
        timeout -= 1;
    }

    if(timeout <= 0) {
        //fprintf(stderr, "TIMEOUT! STATUS: 0x%X\n", uart_status);
        return false;
    }

    data = FPGA()->FpgaRegPeekInd(m_uartTrd.number, 0x204);

    return true;
}

//-----------------------------------------------------------------------------

bool abcdsp::uartTest(U8 speed, bool loopback)
{
    fpgaSerialMode(speed, loopback);

    serialWrite("UART TEST START\n\r", 100);
    serialWrite("\n\r", 100);
    serialWrite("INSYS\n\r", 100);
    serialWrite("ABC BOARD\n\r", 100);
    serialWrite("\n\r", 100);

    serialWrite("Type any text in minicom window and press Enter\n\r", 100);
    serialWrite("\n\r", 100);

    while(!exitFlag()) {

        U8 rdata = 0x0;
        if(fpgaSerialRead(rdata, 100)) {
            fprintf(stderr, "%c", rdata);
            if(rdata == 0xD) {
                fprintf(stderr, "\n");
                fprintf(stderr, "\r");
                fpgaSerialWrite(0xD);
                fpgaSerialWrite(0xA);
                break;
            } else {
                fpgaSerialWrite(rdata);
            }
        }
/*
        std::string buf;
        if(serialRead(buf,100)) {
            serialWrite(buf);
            break;
        }
*/
    }

    serialWrite("UART TEST STOP\n\r", 100);

    return true;
}

//-----------------------------------------------------------------------------

bool abcdsp::ltcTest()
{
    ltc2991 ltc(0x1, 0x48);

    ltc.measure_vcc(1000);
    ltc.measure_own_t(1000);
    ltc.measure_single(3, 1000);
    ltc.measure_single(4, 1000);
    ltc.measure_single(5, 1000);
    ltc.measure_single(6, 1000);
    ltc.measure_single(7, 1000);

    fprintf(stderr, "\n");

    ltc2991 ltc1(0x1, 0x49);

    ltc1.measure_vcc(1000);
    ltc1.measure_own_t(1000);
    ltc1.measure_single(1, 1000);
    ltc1.measure_single(2, 1000);
    ltc1.measure_single(3, 1000);
    ltc1.measure_single(4, 1000);
    ltc1.measure_single(5, 1000);
    ltc1.measure_single(6, 1000);

    return true;
}
