#ifndef __ABCDSP_H__
#define __ABCDSP_H__

#include "gipcy.h"
#include "utypes.h"
#include "ddwambpex.h"
#include "ctrlstrm.h"
#include "memory.h"
#include "nctable.h"

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define DSP_FPGA_COUNT            1
#define DMA_CHANNEL_NUM           4

//-----------------------------------------------------------------------------

class Fpga;
class Memory;

class abcdsp
{
public:
    abcdsp();
    virtual ~abcdsp();

    // DATA MAIN
    void dataFromMain(struct app_params_t& params);
    void dataFromMainToMemAsMem(struct app_params_t& params);
    void dataFromMainToMemAsFifo(struct app_params_t& params);

    // DATA ADC
    void dataFromAdc(struct app_params_t& params);
    void dataFromAdcToMemAsMem(struct app_params_t& params);
    void dataFromAdcToMemAsFifo(struct app_params_t& params);

    // EXIT
    void setExitFlag(bool exit);
    bool exitFlag();

    // TRD INTERFACE
    void RegPokeInd(S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekInd(S32 trdNo, S32 rgnum);
    void RegPokeDir(S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekDir(S32 trdNo, S32 rgnum);
    void resetFifo(U32 trd);

    //FPGA INTERFACE
    bool infoFpga( AMB_CONFIGURATION& info);

    // DMA INTERFACE
    int allocateDmaMemory( U32 DmaChan, BRDctrl_StreamCBufAlloc* param);
    int allocateDmaMemory( U32 DmaChan,
                          void** pBuf,
                          U32 blkSize,
                          U32 blkNum,
                          U32 isSysMem,
                          U32 dir,
                          U32 addr,
                          BRDstrm_Stub **pStub);
    int freeDmaMemory( U32 DmaChan);
    int startDma( U32 DmaChan, int isCycle);
    int stopDma( U32 DmaChan);
    int stateDma( U32 DmaChan, U32 msTimeout, int& state, U32& blkNum);
    int waitDmaBuffer( U32 DmaChan, U32 timeout);
    int waitDmaBlock( U32 DmaChan, U32 timeout);
    int resetDmaFifo( U32 DmaChan);
    int setDmaSource( U32 DmaChan, U32 src);
    int setDmaDirection( U32 DmaChan, U32 dir);
    int setDmaRequestFlag( U32 DmaChan, U32 flag);
    int adjustDma( U32 DmaChan, U32 adjust);
    int doneDma( U32 DmaChan, U32 done);
    void infoDma();

    // ISVI INTERFACE
    bool writeBlock( U32 DmaChan, IPC_handle file, int blockNumber);
    bool writeBuffer( U32 DmaChan, IPC_handle file, int fpos = 0);

    void getFpgaTemperature(float& t);
    void checkMainDataStream(U32 dmaBlockSize, const std::vector<void*>& Buffers, bool width);

    bool writeSpd(U32 devSpdNum, U32 devSpdReg, U32 devSpdRegData, U32 ctrl);
    bool readSpdDev(U32 devSpdNum, U32 devSpdReg, U32 spdCtrl, U32& devSpdRegData);

    Fpga *FPGA();
    Memory *DDR3();


private:
    Fpga*                    m_fpga;
    BRDctrl_StreamCBufAlloc  m_sSCA;
    bool                     m_exit;

    void createFpgaMemory();
    void deleteFpgaMemory();
};

#endif // __ABCDSP_H__
