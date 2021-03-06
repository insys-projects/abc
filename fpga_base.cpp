#include "fpga_base.h"
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

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

fpga_base::fpga_base(u32 fpgaNumber) : m_fpgaNumber(fpgaNumber)
{
    m_hwID = 0;
    m_hwAddr = 0;
    m_hwFpgaNum = 0;

    if(!openFpga()) {
        throw except_info("%s, %d: %s() - Error open DEVICE%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }

    fprintf(stderr, "%s(): Open DEVICE %d\n", __FUNCTION__, m_fpgaNumber);

    try {
#ifdef _c6x_
        m_map = new Mapper(0);
#else
        m_map = new Mapper(m_fpga);
#endif
        infoFpga();
        mapFpga();
        initBar();
        core_hw_address(m_hwAddr, m_hwFpgaNum);
        core_device_id(m_hwID);
    } catch(except_info_t err) {
        closeFpga();
        throw err;
    } catch(...) {
        closeFpga();
        throw except_info("%s, %d: %s() - Unknown exception while init DEVICE%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }

    fprintf(stderr, "SLOT: 0x%.2X\tFPGA: 0x%.4X\tID: 0x%.4X\n", m_hwAddr, m_hwFpgaNum, m_hwID );
}

//-----------------------------------------------------------------------------

fpga_base::~fpga_base()
{
    closeFpga();
}

//-----------------------------------------------------------------------------

bool fpga_base::openFpga()
{
    IPC_str name[256];

    m_fpga = IPC_openDevice(name, AmbDeviceName, m_fpgaNumber);
    if(!m_fpga) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

void fpga_base::closeFpga()
{
    fprintf(stderr, "Close FPGA%d\n", m_fpgaNumber);
    if(m_map) delete m_map;
    IPC_closeDevice(m_fpga);
}

//-----------------------------------------------------------------------------

void fpga_base::infoFpga()
{
    memset(&m_info, 0, sizeof(m_info));

    int res = IPC_ioctlDevice( m_fpga, IOCTL_AMB_GET_CONFIGURATION, &m_info, sizeof(AMB_CONFIGURATION), &m_info, sizeof(AMB_CONFIGURATION));
    if(res < 0) {
        throw except_info("%s, %d: %s() - Error get FPGA%d information.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
}

//-----------------------------------------------------------------------------

bool fpga_base::info(AMB_CONFIGURATION& cfgInfo)
{
    cfgInfo = m_info;
    return m_ok;
}

//-----------------------------------------------------------------------------

void fpga_base::mapFpga()
{
    int res_count = 0;
    int map_count = 0;

    for(unsigned i=0; i<sizeof(m_info.PhysAddress)/sizeof(m_info.PhysAddress[0]); i++) {

        if(m_info.PhysAddress[i] && m_info.Size[i]) {

            ++res_count;
#ifdef __linux__
            m_info.VirtAddress[i] = m_map->mapPhysicalAddress(m_info.PhysAddress[i], m_info.Size[i]);
#endif
            if(m_info.VirtAddress[i]) {

                fprintf(stderr, "BAR%d: 0x%x -> %p\n", i, m_info.PhysAddress[i], m_info.VirtAddress[i]);
                ++map_count;
            }
        }
    }

    if(map_count != res_count) {
        throw except_info("%s, %d: %s() - Error map all BARs for FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }

    m_ok = true;
}

//-----------------------------------------------------------------------------

void fpga_base::initBar()
{
    m_bar0 = m_bar1 = m_bar2 = 0;

    if(m_info.VirtAddress[0]) {
        m_bar0 = (u32*)m_info.VirtAddress[0];
        m_bar[0] = m_bar0;
    }
    if(m_info.VirtAddress[1]) {
        m_bar1 = (u32*)m_info.VirtAddress[1];
        m_bar[1] = m_bar1;
    }
    if(m_info.VirtAddress[2]) {
        m_bar2 = (u32*)m_info.VirtAddress[2];
        m_bar[2] = m_bar2;
    }
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_reg_peek_dir( u32 trd, u32 reg )
{
    if( (trd>15) || (reg>3) )
        return -1;

    u32 offset = trd*0x4000 + reg*0x1000;

    return m_bar1[offset/4];
}

//-----------------------------------------------------------------------------

void fpga_base::core_reg_poke_dir( u32 trd, u32 reg, u32 val )
{
    if( (trd>15) || (reg>3) )
        return;

    u32 offset = trd*0x4000 + reg*0x1000;

    m_bar1[offset/4]=val;
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_reg_peek_ind( u32 trd, u32 reg )
{
    if( (trd>15) || (reg>0x3FF) )
        return -1;

    u32 status;
    u32 Address = trd*0x4000;
    u32 Status  = Address;
    u32 CmdAdr  = Address + 0x2000;
    u32 CmdData = Address + 0x3000;
    u32 ret;

    m_bar1[CmdAdr/4] = reg;

    for( int ii=0; ; ii++ ) {

        status = m_bar1[Status/4];
        if( status & 1 )
            break;

        //fprintf(stderr, "status = 0x%x\r", status);

        if( ii>100 )
            IPC_delay( 1 );
        if( ii>200 ) {
            fprintf(stderr, "%s(): TIMEOUT - TRD 0x%.2X REG 0x%.2X\n", __FUNCTION__, trd, reg);
            return 0xFFFF;
        }
    }

    ret = m_bar1[CmdData/4];
    ret &= 0xFFFF;

    return ret;
}

//-----------------------------------------------------------------------------

void fpga_base::core_reg_poke_ind( u32 trd, u32 reg, u32 val )
{
    if( (trd>15) || (reg>0x3FF) )
        return;

    u32 status;
    u32 Address = trd*0x4000;
    u32 Status  = Address;
    u32 CmdAdr  = Address + 0x2000;
    u32 CmdData = Address + 0x3000;

    m_bar1[CmdAdr/4] = reg;

    for( int ii=0; ; ii++ ) {

        status = m_bar1[Status/4];
        if( status & 1 )
            break;

        //fprintf(stderr, "status = 0x%x\r", status);

        if( ii>100 )
            IPC_delay( 1 );
        if( ii>200 ) {
            fprintf(stderr, "%s(): TIMEOUT - TRD 0x%.2X REG 0x%.2X\n", __FUNCTION__, trd, reg);
            return;
        }
    }

    m_bar1[CmdData/4] = val;
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_bar_read( u32 bar, u32 offset )
{
    u32 *base = m_bar[bar];
    return base[2*offset];
}

//-----------------------------------------------------------------------------

void fpga_base::core_bar_write( u32 bar, u32 offset, u32 val )
{
    u32 *base = m_bar[bar];
    base[2*offset] = val;
}

//-----------------------------------------------------------------------------

void fpga_base::core_block_write( u32 nb, u32 reg, u32 val )
{
    if( (nb>7) || (reg>31) )
        return;

    *(m_bar0 + nb*64 + reg*2) = val;
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_block_read( u32 nb, u32 reg )
{
    if( (nb>7) || (reg>31) )
        return -1;

    u32 ret = *(m_bar0 + nb*64 + reg*2);

    if( reg<8 )
        ret &= 0xFFFF;

    return ret;
}

//-----------------------------------------------------------------------------
/*
void fpga_base::core_block_write( u32 nb, u32 reg, u32 val )
{
    if( (nb>7) || (reg>31) )
        return;

    core_bar_write(0, nb*32 + reg, val);
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_block_read( u32 nb, u32 reg )
{
    if( (nb>7) || (reg>31) )
        return -1;

    u32 ret = core_bar_read(0, nb*32 + reg);
    if( reg<8 )
        ret &= 0xFFFF;

    return ret;
}
*/
//-----------------------------------------------------------------------------

u32 fpga_base::core_write_reg_buf(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_WRITE_REG_BUF,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                0,
                0);
    if(res < 0){
        throw except_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
    return 0;
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_write_reg_buf_dir(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_WRITE_REG_BUF_DIR,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                NULL,
                0);
    if(res < 0) {
        throw except_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
    return 0;
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_read_reg_buf(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_READ_REG_BUF,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                &reg_buf,
                sizeof(AMB_BUF_REG));
    if(res < 0) {
        throw except_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
    return 0;
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_read_reg_buf_dir(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_READ_REG_BUF_DIR,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                &reg_buf,
                sizeof(AMB_BUF_REG));
    if(res < 0) {
        throw except_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
    return 0;
}

//-----------------------------------------------------------------------------

void fpga_base::core_hw_address(U08& hwAddr, U08& fpgaNum)
{
    U32 hw = core_block_read(0, 0x1F);
    hwAddr =  ((hw >> 8) & 0xff);
    fpgaNum = (hw & 0xff);
    if(((hw >> 16)&0xffff) == 0x4912) {
        return;
    }
    throw except_info("%s, %d: %s() - Error get hardware addresses for DEVICE%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
}

//-----------------------------------------------------------------------------

void fpga_base::core_device_id(U16& device_id)
{
    device_id = (core_block_read(0, 2) & 0xffff);
    if((device_id != 0) && (device_id != 0xffff)) {
        return;
    }
    throw except_info("%s, %d: %s() - Error get device_id for BOARD%d FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_hwAddr, m_hwFpgaNum);
}

//-----------------------------------------------------------------------------

bool fpga_base::core_temperature(float &t)
{
    t = 0;
    return false;
}
