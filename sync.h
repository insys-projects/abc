#ifndef SYNC_H
#define SYNC_H

#include "fpga.h"
#include "iniparser.h"

//-------------------------------------------------------------------------------------

#define SYNC_PERIOD		0xA
#define SYNC_DELAY		0xC
#define SYNC_ADC_DELAY		0xD
#define SYNC_DAC_DELAY		0xE
#define SYNC_A_DELAY		0xF
#define SYNC_A_WIDTH		0x10
#define SYNC_B_DELAY		0x11
#define SYNC_B_WIDTH		0x12
#define SYNC_C_DELAY		0x13
#define SYNC_C_WIDTH		0x14
#define SYNC_D_DELAY		0x15
#define SYNC_D_WIDTH		0x16

//-------------------------------------------------------------------------------------

class sync
{
public:
    sync(Fpga *fpga, const struct app_params_t& params);
    virtual ~sync();

    void start();
    void stop();
    void reset_fifo();

    u16  status();

    bool set_param(const struct app_params_t& params);
    bool check_param(const struct app_params_t& params);

private:
    sync();

    Fpga* m_fpga;
    fpga_trd_t m_syncTrd;
    struct app_params_t m_param;
};

#endif // SYNC_H
