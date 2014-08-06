
#include "ltc2991.h"
#include "i2c.h"
#include "gipcy.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//----------------------------------------------------------------------------------------------

ltc2991::ltc2991(int bus, int i2c_addr) : m_i2c(bus), m_addr(i2c_addr), m_verbose(false)
{
    m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, 0x00);
    m_i2c.i2c_write_byte_data(m_addr, LTC2991_CONTROL_V1234_REG, 0x00);
    m_i2c.i2c_write_byte_data(m_addr, LTC2991_CONTROL_V5678_REG, 0x00);
}

//----------------------------------------------------------------------------------------------

ltc2991::~ltc2991()
{
    m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, 0x00);
}

//----------------------------------------------------------------------------------------------

#define uV (1.e-6)

//----------------------------------------------------------------------------------------------

float ltc2991::measure_vcc(int timeout)
{
    uint8_t h_status = 0;

    // disable all channels
    int res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, 0x00);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
        return 0;
    }

    // enable Vcc measure channel
    res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
        return 0;
    }

    // wait until new measure will be triggered
    IPC_delay(50);

    // wait status bit
    while(1) {

        res = m_i2c.i2c_read_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, &h_status);
        if(res) {
            fprintf(stderr, "%s(): Error in i2c_read_byte_data(0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG);
            return 0;
        }

        if(h_status & 0x1) {
            break;
        }

        --timeout;

        if(timeout <= 0) {
            fprintf(stderr, "%s(): Vcc measuring timeout!\n", __FUNCTION__);
            return 0;
        }

        IPC_delay(1);
    }

    // read measured data
    uint16_t vcc = 0;
    uint8_t msb = 0;
    uint8_t lsb = 0;

    res |= m_i2c.i2c_read_byte_data(m_addr, LTC2991_Vcc_MSB_REG, &msb);
    res |= m_i2c.i2c_read_byte_data(m_addr, LTC2991_Vcc_LSB_REG, &lsb);
    if(res) {
        fprintf(stderr, "%s(): Error read Vcc LSB or MSB value!\n", __FUNCTION__);
        return 0;
    }

    // Data valid bit
    if(!(msb & 0x80)) {
        fprintf(stderr, "%s(): Error - Data Valid bit not set!\n", __FUNCTION__);
        return 0;
    }

    // SIGN bit
    //int sign = (msb & 0x40);

    msb &= ~0x80;
    msb &= ~0x40;

    vcc = ((msb << 8) | lsb);

    float Vcc = (2.5 + vcc * 305.18 * uV);

    if(m_verbose)
        fprintf(stderr, "Vcc = %.2f V\n", Vcc);

    return Vcc;
}

//----------------------------------------------------------------------------------------------

float ltc2991::measure_single(int input, int timeout)
{
    // disable all channels
    int res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, 0x00);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
        return 0;
    }

    // enable V measure channel
    uint8_t mask = 0;
    switch(input) {
    case 1:
    case 2:
        mask |= 0x10;
        break;
    case 3:
    case 4:
        mask |= 0x20;
        break;
    case 5:
    case 6:
        mask |= 0x40;
        break;
    case 7:
    case 8:
        mask |= 0x80;
        break;
    default: {
        fprintf(stderr, "%s(): Invalid input channel: 0x%X\n", __FUNCTION__, input);
        return 0;
    }
    }

    res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, mask);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
        return 0;
    }

    // wait until new measure will be triggered
    IPC_delay(50);

    uint8_t l_status = 0;

    // wait status bit
    while(1) {

        res = m_i2c.i2c_read_byte_data(m_addr, LTC2991_STATUS_LOW_REG, &l_status);
        if(res) {
            fprintf(stderr, "%s(): Error in i2c_read_byte_data(0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG);
            return 0;
        }

        if(l_status & (1 << (input-1))) {
            break;
        }

        --timeout;

        if(timeout <= 0) {
            fprintf(stderr, "%s(): V measuring timeout!\n", __FUNCTION__);
            return 0;
        }

        IPC_delay(1);
    }

    // read measured data
    uint16_t v = 0;
    uint8_t msb = 0;
    uint8_t lsb = 0;
    uint8_t msb_offset = 0xA + 2*(input - 1) + 0;
    uint8_t lsb_offset = 0xA + 2*(input - 1) + 1;

    res |= m_i2c.i2c_read_byte_data(m_addr, msb_offset, &msb);
    res |= m_i2c.i2c_read_byte_data(m_addr, lsb_offset, &lsb);
    if(res) {
        fprintf(stderr, "%s(): Error read V LSB or MSB value!\n", __FUNCTION__);
        return 0;
    }

    // Data valid bit
    if(!(msb & 0x80)) {
        fprintf(stderr, "%s(): Error - Data Valid bit not set!\n", __FUNCTION__);
        return 0;
    }

    // SIGN bit
    int sign = (msb & 0x40);

    msb &= ~0x80;
    msb &= ~0x40;

    v = ((msb << 8) | lsb);

    float V = (v * 305.18 * uV);

    if(m_verbose)
        fprintf(stderr, "V[%d] = %.2f V\n", input, sign ? -V : V);

    return V;
}

//----------------------------------------------------------------------------------------------

float ltc2991::measure_own_t(int timeout)
{
    uint8_t h_status = 0;

    // disable all channels
    int res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, 0x00);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
        return 0;
    }

    // enable internal T measure channel
    res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
        return 0;
    }

    // wait until new measure will be triggered
    IPC_delay(50);

    // wait status bit
    while(1) {

        res = m_i2c.i2c_read_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, &h_status);
        if(res) {
            fprintf(stderr, "%s(): Error in i2c_read_byte_data(0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG);
            return 0;
        }

        if(h_status & 0x2) {
            break;
        }

        --timeout;

        if(timeout <= 0) {
            fprintf(stderr, "%s(): temperature measuring timeout!\n", __FUNCTION__);
            return 0;
        }

        IPC_delay(1);
    }

    // read measured data
    uint16_t vcc = 0;
    uint8_t msb = 0;
    uint8_t lsb = 0;

    res |= m_i2c.i2c_read_byte_data(m_addr, LTC2991_T_Internal_MSB_REG, &msb);
    res |= m_i2c.i2c_read_byte_data(m_addr, LTC2991_T_Internal_LSB_REG, &lsb);
    if(res) {
        fprintf(stderr, "%s(): Error read temperature LSB or MSB value!\n", __FUNCTION__);
        return 0;
    }

    // Data valid bit
    if(!(msb & 0x80)) {
        fprintf(stderr, "%s(): Error - Data Valid bit not set!\n", __FUNCTION__);
        return 0;
    }

    msb &= ~(0x80 | 0x40 | 0x20);

    vcc = ((msb << 8) | lsb);

    float T = (vcc * 0.0625);

    if(m_verbose)
        fprintf(stderr, "T = %.2f C\n", T);

    return T;
}

//----------------------------------------------------------------------------------------------

float ltc2991::measure_differential_t(int input_pair, int timeout)
{
    // disable all channels
    int res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, 0x00);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG, LTC2991_VCC_TINTERNAL_ENABLE);
        return 0;
    }

    // enable V measure channel
    uint8_t enable_mask = 0;
    uint8_t ctrl_mask0 = 0;
    uint8_t ctrl_mask1 = 0;
    switch(input_pair) {
    case 1:
        enable_mask |= 0x10;
        ctrl_mask0 |= 0x02;
        break;
    case 2:
        enable_mask |= 0x20;
        ctrl_mask0 |= 0x20;
        break;
    case 3:
        enable_mask |= 0x40;
        ctrl_mask1 |= 0x02;
        break;
    case 4:
        enable_mask |= 0x80;
        ctrl_mask1 |= 0x20;
        break;
    default: {
        fprintf(stderr, "%s(): Invalid channels pair: 0x%X\n", __FUNCTION__, input_pair);
        return 0;
    }
    }

    res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CONTROL_V1234_REG, ctrl_mask0);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CONTROL_V1234_REG, ctrl_mask0);
        return 0;
    }

    res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CONTROL_V5678_REG, ctrl_mask1);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CONTROL_V5678_REG, ctrl_mask1);
        return 0;
    }

    res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CONTROL_PWM_Tinternal_REG, 0);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CONTROL_PWM_Tinternal_REG, 0);
        return 0;
    }

    res = m_i2c.i2c_write_byte_data(m_addr, LTC2991_CHANNEL_ENABLE_REG, enable_mask);
    if(res) {
        fprintf(stderr, "%s(): Error in i2c_write_byte_data(0x%x, 0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG, enable_mask);
        return 0;
    }

    // wait until new measure will be triggered
    IPC_delay(50);

    uint8_t l_status = 0;

    // wait status bit
    while(1) {

        res = m_i2c.i2c_read_byte_data(m_addr, LTC2991_STATUS_LOW_REG, &l_status);
        if(res) {
            fprintf(stderr, "%s(): Error in i2c_read_byte_data(0x%x)\n", __FUNCTION__, LTC2991_CHANNEL_ENABLE_REG);
            return 0;
        }

        if((l_status & ctrl_mask0) || (l_status & ctrl_mask1)) {
            break;
        }

        --timeout;

        if(timeout <= 0) {
            fprintf(stderr, "%s(): Vcc measuring timeout!\n", __FUNCTION__);
            return 0;
        }

        IPC_delay(1);
    }

    // read measured data
    uint16_t Vx = 0;

    uint8_t msb = 0;
    uint8_t lsb = 0;

    res |= m_i2c.i2c_read_byte_data(m_addr, LTC2991_V1_MSB_REG + 4*(input_pair-1), &msb);
    res |= m_i2c.i2c_read_byte_data(m_addr, LTC2991_V1_LSB_REG + 4*(input_pair-1), &lsb);
    if(res) {
        fprintf(stderr, "%s(): Error read Vx LSB or MSB value!\n", __FUNCTION__);
        return 0;
    }

    // Data valid bit
    if(!(msb & 0x80)) {
        fprintf(stderr, "%s(): Error - Data Valid bit not set in MSB1!\n", __FUNCTION__);
        return 0;
    }

    msb &= ~0xE0;

    Vx = ((msb << 8) | lsb);

    float Tx = Vx / 16;

    if(m_verbose)
        fprintf(stderr, "T[%d-%d] = %.2f C\n", 2*(input_pair-1)+1, 2*(input_pair-1)+2, Tx);

    return Tx;
}
