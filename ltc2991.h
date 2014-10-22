
#ifndef LTC2991_H
#define LTC2991_H

#include <stdint.h>

//----------------------------------------------------------------------------------------------

#define LTC2991_STATUS_LOW_REG              0x00    //!< Data_Valid Bits(V1 Through V8)
#define LTC2991_CHANNEL_ENABLE_REG          0x01    //!< Channel Enable, Vcc, T_internal Conversion Status, Trigger
#define LTC2991_CONTROL_V1234_REG           0x06    //!< V1, V2, V3, and V4 Control Register
#define LTC2991_CONTROL_V5678_REG           0x07    //!< V5, V6, V7, AND V8 Control Register
#define LTC2991_CONTROL_PWM_Tinternal_REG   0x08    //!< PWM Threshold and T_internal Control Register
#define LTC2991_PWM_THRESHOLD_MSB_REG       0x09    //!< PWM Threshold
#define LTC2991_V1_MSB_REG                  0x0A    //!< V1, or T_R1 T MSB
#define LTC2991_V1_LSB_REG                  0x0B    //!< V1, or T_R1 T LSB
#define LTC2991_V2_MSB_REG                  0x0C    //!< V2, V1-V2, or T_R2 Voltage MSB
#define LTC2991_V2_LSB_REG                  0x0D    //!< V2, V1-V2, or T_R2 Voltage LSB
#define LTC2991_V3_MSB_REG                  0x0E    //!< V3, or T_R2 T MSB
#define LTC2991_V3_LSB_REG                  0x0F    //!< V3, or T_R2 T LSB
#define LTC2991_V4_MSB_REG                  0x10    //!< V4, V3-V4, or T_R2 Voltage MSB
#define LTC2991_V4_LSB_REG                  0x11    //!< V4, V3-V4, or T_R2 Voltage LSB
#define LTC2991_V5_MSB_REG                  0x12    //!< V5, or T_R3 T MSB
#define LTC2991_V5_LSB_REG                  0x13    //!< V5, or T_R3 T LSB
#define LTC2991_V6_MSB_REG                  0x14    //!< V6, V5-V6, or T_R3 Voltage MSB
#define LTC2991_V6_LSB_REG                  0x15    //!< V6, V5-V6, or T_R3 Voltage LSB
#define LTC2991_V7_MSB_REG                  0x16    //!< V7, or T_R4 T MSB
#define LTC2991_V7_LSB_REG                  0x17    //!< V7, or T_R4 T LSB
#define LTC2991_V8_MSB_REG                  0x18    //!< V8, V7-V8, or T_R4 Voltage MSB
#define LTC2991_V8_LSB_REG                  0x19    //!< V8, V7-V8, or T_R4 Voltage LSB
#define LTC2991_T_Internal_MSB_REG          0x1A    //!< T_Internal MSB
#define LTC2991_T_Internal_LSB_REG          0x1B    //!< T_Internal LSB
#define LTC2991_Vcc_MSB_REG                 0x1C    //!< Vcc MSB
#define LTC2991_Vcc_LSB_REG                 0x1D    //!< Vcc LSB

#define LTC2991_VCC_TINTERNAL_ENABLE        0x08    //!< Enable Vcc internal voltage measurement

//----------------------------------------------------------------------------------------------
#include "i2c.h"
//----------------------------------------------------------------------------------------------

class ltc2991 {
public:
    ltc2991(int bus, int i2c_addr);
    virtual ~ltc2991();

    float measure_vcc(int timeout = 500);
    float measure_single(int input, int timeout = 500);
    float measure_own_t(int timeout = 500);
    float measure_differential_t(int input_pair, int timeout = 500);
    void  measure_print_enable(bool enable = false) { m_verbose = enable; }

    float code_to_single_ended_voltage(int16_t adc_code);
    float code_to_vcc_voltage(int16_t adc_code);
    float code_to_differential_voltage(int16_t adc_code);
    float code_to_temperature(int16_t adc_code, bool unit = false);
    float code_to_diode_voltage(int16_t adc_code);

private:
    ltc2991();
    i2c m_i2c;
    int m_addr;
    bool m_verbose;
};

//----------------------------------------------------------------------------------------------

#endif  // LTC2991_H
