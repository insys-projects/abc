#include "gipcy.h"
#include "sync.h"

#include <cstdio>
#include <cstdlib>
#include <math.h>

//-------------------------------------------------------------------------------------

#define MODE0_RST_TRD           0x1
#define MODE0_RST_FIFO          0x2
#define MODE0_MASTER            0x10
#define MODE0_START             0x20
#define MODE0_RT                0x80

//-------------------------------------------------------------------------------------

sync::sync(Fpga *fpga, const app_params_t& params) : m_fpga(fpga), m_param(params)
{
    // Search SYNC TRD
    if(!m_fpga->fpgaTrd(0, 0xC1, m_syncTrd)) {
        fprintf(stderr, "%s(): SYNC TRD 0x%.2X not found!\n", __FUNCTION__, 0xB9);
        return;
    }

    // STMODE
    U32 stmode = 0;
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x5, stmode);
}

//-------------------------------------------------------------------------------------

sync::~sync()
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_syncTrd.number, 0x0);
    mode0 &= ~MODE0_START;      // запрет работы
    mode0 |= MODE0_RST_FIFO;    // сброс FIFO
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, mode0);
}

//-------------------------------------------------------------------------------------

void sync::start()
{
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, 0x2038);
}

//-------------------------------------------------------------------------------------

void sync::stop()
{
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, 0x0);
}

//-------------------------------------------------------------------------------------

void sync::reset_fifo()
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_syncTrd.number, 0x0);

    mode0 |= MODE0_RST_FIFO;
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, mode0);
    IPC_delay(1);
    mode0 &= ~MODE0_RST_FIFO;
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, mode0);
}

//-------------------------------------------------------------------------------------

u16 sync::status()
{
    return (m_fpga->FpgaRegPeekDir(m_syncTrd.number, 0x0) & 0xFFFF);
}

//-------------------------------------------------------------------------------------

bool sync::set_param(const struct app_params_t& params)
{
    if(!check_param(params)) {
      return false;
    }

    stop();

    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_PERIOD, params.Tcycle & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_PERIOD+1, (params.Tcycle >> 16) & 0xffff);

    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DELAY, params.Tdelay & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_ADC_DELAY, params.deltaAdc & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DAC_DELAY, params.deltaDac & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_A_DELAY, params.deltaA & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_A_WIDTH, params.widthA & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DELAY, params.deltaB & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DELAY, params.widthB & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DELAY, params.deltaC & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DELAY, params.widthC & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DELAY, params.deltaD & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DELAY, params.widthD & 0xffff);

    if(params.syncCycle)
      m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x9, 0x1);
    else
      m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x9, 0x0);

    // STMODE
    U32 stmode = (params.DacStopInverting << 14) |
            (params.DacStopSource << 8) |
            (params.DacStartMode << 7) |
            (params.DacStartBaseInverting << 6) | params.DacStartBaseSource;
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x5, stmode);

    start();

    return true;
}

//-------------------------------------------------------------------------------------

bool sync::check_param(const struct app_params_t& params)
{
    if(params.Tcycle < params.Tdelay + params.deltaAdc) {
      fprintf(stderr, "%s(): Error: parameters Tdelay and deltaAdc out of range!\n", __FUNCTION__);
      return false;
    }

    if(params.Tcycle < params.Tdelay + params.deltaDac) {
      fprintf(stderr, "%s(): Error: parameters Tdelay and deltaDac out of range!\n", __FUNCTION__);
      return false;
    }

    if(params.Tcycle < params.Tdelay + params.deltaA + params.widthA) {
      fprintf(stderr, "%s(): Error: parameters Tdelay, deltaA and widthA out of range!\n", __FUNCTION__);
      return false;
    }

    if(params.Tcycle < params.Tdelay + params.deltaB + params.widthB) {
      fprintf(stderr, "%s(): Error: parameters Tdelay, deltaB and widthB out of range!\n", __FUNCTION__);
      return false;
    }

    if(params.Tcycle < params.Tdelay + params.deltaC + params.widthC) {
      fprintf(stderr, "%s(): Error: parameters Tdelay, deltaC and widthC out of range!\n", __FUNCTION__);
      return false;
    }

    if(params.Tcycle < params.Tdelay + params.deltaD + params.widthD) {
      fprintf(stderr, "%s(): Error: parameters Tdelay, deltaD and widthD out of range!\n", __FUNCTION__);
      return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------
