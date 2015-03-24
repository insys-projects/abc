#ifndef ADC_H
#define ADC_H

#include "fpga.h"
#include "trdprog.h"
#include "iniparser.h"

//-------------------------------------------------------------------------------------

#define MODE0_RST_TRD           0x1
#define MODE0_RST_FIFO          0x2
#define MODE0_MASTER            0x10
#define MODE0_START             0x20
#define MODE0_RT                0x80

//-------------------------------------------------------------------------------------

class adc
{
public:
    adc(Fpga *fpga, const struct app_params_t& params);
    virtual ~adc();

    void start();
    void stop();
    void reset_fifo();
    u16  status();

private:
    adc();
    Fpga* m_fpga;
    fpga_trd_t m_adcTrd;
    trdprog*   m_trdprog;
    struct app_params_t m_params;

    void default_settings(signed AdcBias0, signed AdcBias1);
};

#endif // ADC_H
