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

abc_sync::abc_sync(Fpga *fpga, const app_params_t& params) : m_fpga(fpga), m_param(params)
{
    // Search SYNC TRD
    if(!m_fpga->fpgaTrd(0, 0xC2, m_syncTrd)) {
        fprintf(stderr, "%s(): SYNC TRD 0x%.2X not found!\n", __FUNCTION__, 0xC2);
        return;
    }

    reset_fifo();

    set_param(params);
}

//-------------------------------------------------------------------------------------

abc_sync::~abc_sync()
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_syncTrd.number, 0x0);
    mode0 &= ~MODE0_START;      // запрет работы
    mode0 |= MODE0_RST_FIFO;    // сброс FIFO
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, mode0);
}

//-------------------------------------------------------------------------------------

void abc_sync::start()
{
    set_param(m_param);

    // STMODE
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x5, 0x0);

    // MODE0
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, 0x2038);
}

//-------------------------------------------------------------------------------------

void abc_sync::stop()
{
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, 0x0);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x5, 0x0);
}

//-------------------------------------------------------------------------------------

void abc_sync::reset_fifo()
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_syncTrd.number, 0x0);

    mode0 |= MODE0_RST_FIFO;
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, mode0);
    IPC_delay(1);
    mode0 &= ~MODE0_RST_FIFO;
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x0, mode0);
}

//-------------------------------------------------------------------------------------

u16 abc_sync::status()
{
    return (m_fpga->FpgaRegPeekDir(m_syncTrd.number, 0x0) & 0xFFFF);
}

//-------------------------------------------------------------------------------------

bool abc_sync::set_param(const struct app_params_t& params)
{
    if(!check_param(params)) {
        fprintf(stderr, "%s(): Invalid parameters!\n", __FUNCTION__);
        return false;
    }

    stop();

    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_PERIOD, params.Tcycle & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_PERIOD+1, (params.Tcycle >> 16) & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DELAY, params.Tdelay & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_ADC_DELTA, params.deltaAdc & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_DAC_DELTA, params.deltaDac & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_A_DELTA, params.delta_A & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_A_WIDTH, params.width_A & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_B_DELTA, params.delta_B & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_B_WIDTH, params.width_B & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_C_DELTA, params.delta_C & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_C_WIDTH, params.width_C & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_D_DELTA, params.delta_D & 0xffff);
    m_fpga->FpgaRegPokeInd(m_syncTrd.number, SYNC_D_WIDTH, params.width_D & 0xffff);

    if(params.syncCycle)
        m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x9, 0x1);
    else
        m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x9, 0x0);

    // STMODE
    //U32 stmode = (params.DacStopInverting << 14) |
    //        (params.DacStopSource << 8) |
    //        (params.DacStartMode << 7) |
    //        (params.DacStartBaseInverting << 6) | params.DacStartBaseSource;
    //m_fpga->FpgaRegPokeInd(m_syncTrd.number, 0x5, 0x0);

    //start();

    return true;
}

//-------------------------------------------------------------------------------------

bool abc_sync::check_param(const struct app_params_t& params)
{
    bool ok = true;

    if(params.syncCycle == 0) {
        return ok;
    }

    if(params.Tcycle < (params.Tdelay + params.deltaAdc - 2)) {
        fprintf(stderr, "%s(): Error: parameters Tdelay and deltaAdc out of range!\n", __FUNCTION__);
        return ok;
    }

    if(params.Tcycle < (params.Tdelay + params.deltaDac - 2)) {
        fprintf(stderr, "%s(): Error: parameters Tdelay and deltaDac out of range!\n", __FUNCTION__);
        return ok;
    }

    if(params.Tcycle < (params.Tdelay + params.delta_A + params.width_A - 2)) {
        fprintf(stderr, "%s(): Error: parameters Tdelay, delta_A and width_A out of range!\n", __FUNCTION__);
        return ok;
    }

    if(params.Tcycle < (params.Tdelay + params.delta_B + params.width_B - 2)) {
        fprintf(stderr, "%s(): Error: parameters Tdelay, delta_B and width_B out of range!\n", __FUNCTION__);
        return ok;
    }

    if(params.Tcycle < (params.Tdelay + params.delta_C + params.width_C - 2)) {
        fprintf(stderr, "%s(): Error: parameters Tdelay, delta_C and width_C out of range!\n", __FUNCTION__);
        return ok;
    }

    if(params.Tcycle < (params.Tdelay + params.delta_D + params.width_D - 2)) {
        fprintf(stderr, "%s(): Error: parameters Tdelay, delta_D and width_D out of range!\n", __FUNCTION__);
        return ok;
    }

    return ok;
}

//-------------------------------------------------------------------------------------

void abc_sync::reconfig(const IPC_str *iniFile)
{
    IPC_str Buffer[128];
    IPC_str iniFilePath[1024];

    const IPC_str *iniFileName = iniFile;
    IPC_getCurrentDir(iniFilePath, sizeof(iniFilePath)/sizeof(char));
    BRDC_strcat(iniFilePath, _BRDC("/"));
    BRDC_strcat(iniFilePath, iniFileName);

    fprintf(stderr, "inifile = %s\n", iniFilePath);

    int res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("syncCycle"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncCycle - not found. Use default value\n");
    }
    m_param.syncCycle = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("Tcycle"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: Tcycle - not found. Use default value\n");
    }
    m_param.Tcycle = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("Tdelay"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: Tdelay - not found. Use default value\n");
    }
    m_param.Tdelay = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("deltaAdc"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: deltaAdc - not found. Use default value\n");
    }
    m_param.deltaAdc = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("deltaDac"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: deltaDac - not found. Use default value\n");
    }
    m_param.deltaDac = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("delta_A"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: delta_A - not found. Use default value\n");
    }
    m_param.delta_A = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("width_A"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: width_A - not found. Use default value\n");
    }
    m_param.width_A = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("delta_B"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: delta_B - not found. Use default value\n");
    }
    m_param.delta_B = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("width_B"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: width_B - not found. Use default value\n");
    }
    m_param.width_B = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("delta_C"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: delta_C - not found. Use default value\n");
    }
    m_param.delta_C = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("width_C"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: width_C - not found. Use default value\n");
    }
    m_param.width_C = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("delta_D"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: delta_D - not found. Use default value\n");
    }
    m_param.delta_D = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("width_D"), _BRDC("0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: width_D - not found. Use default value\n");
    }
    m_param.width_D = BRDC_strtol(Buffer,0,10);

    //================================================
}
