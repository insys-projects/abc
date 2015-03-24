#ifndef __ABCDSP_H__
#define __ABCDSP_H__

#include "gipcy.h"
#include "utypes.h"
#include "ddwambpex.h"
#include "ctrlstrm.h"
#include "memory.h"
#include "trdprog.h"
#include "dac.h"
#include "adc.h"
#include "i2c.h"
#include "ltc2991.h"
#include "iniparser.h"
#include "sync.h"

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define DSP_FPGA_COUNT            1
#define DMA_CHANNEL_NUM           4

//-----------------------------------------------------------------------------

#define UART_RATE_2400            0x1
#define UART_RATE_4800            0x2
#define UART_RATE_9600            0x4
#define UART_RATE_19200           0x8
#define UART_RATE_38400           0x10
#define UART_RATE_57600           0x20
#define UART_RATE_115200          0x40
#define UART_LOOPBACK_MODE        0x8000

//-----------------------------------------------------------------------------

class Fpga;
class Memory;

//-----------------------------------------------------------------------------

class abcdsp
{
public:
    abcdsp(const struct app_params_t& params);
    virtual ~abcdsp();

    // DATA MAIN
    void dataFromMain(struct app_params_t& params);
    void dataFromMainToMemAsMem(struct app_params_t& params);

    // DATA ADC
    void dataFromAdc(struct app_params_t& params);
    void dataFromAdcToMemAsMem(struct app_params_t& params);
    void dataFromAdcToMSM();

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

    bool fpgaSerialMode(U8 speed, bool loopback);
    bool fpgaSerialWrite(U8 data, int timeout = 10);
    bool fpgaSerialRead(U8& data, int timeout = 10);
    bool serialWrite(const std::string& data, int timeout = 10);
    bool serialRead(std::string& data, int timeout = 10);
    bool uartTest(U8 speed, bool loopback);
    bool ltcTest();
    bool syncTest();
    bool syncStart();

    U8 fpgaSerialRead();

    Fpga *FPGA();
    Memory *DDR3();


private:
    Fpga*                    m_fpga;
    BRDctrl_StreamCBufAlloc  m_sSCA;
    bool                     m_exit;
    dac*                     m_dac;
    adc*                     m_adc;
    abc_sync*                m_sync;
    Memory*                  m_mem;
    ltc2991*                 m_ltc1;
    ltc2991*                 m_ltc2;

    fpga_trd_t               m_mainTrd;
    fpga_trd_t               m_uartTrd;
    fpga_trd_t               m_adcTrd;
    fpga_trd_t               m_dacTrd;
    fpga_trd_t               m_memTrd;

    struct app_params_t      m_params;
};

#endif // __ABCDSP_H__
