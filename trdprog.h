#ifndef TRDPROG_H
#define TRDPROG_H

#include "fpga.h"
#include <string>
#include <vector>

class trdprog
{
public:
    trdprog(Fpga *fpga, std::string fileName);
    virtual ~trdprog();

private:
    int get_options(std::vector<std::string>& optionsList);
    unsigned get_value(std::string& value);
    bool get_args(std::string& str, std::vector<unsigned>& argList);

    int process_options(std::vector<std::string>& optionsList);
    int process_trd_read(std::vector<unsigned>& argList);
    int process_trd_write(std::vector<unsigned>& argList);
    int process_trd_set_bit(std::vector<unsigned>& argList);
    int process_trd_clr_bit(std::vector<unsigned>& argList);
    int process_trd_pause(std::vector<unsigned>& argList);
    int process_trd_wait_val(std::vector<unsigned>& argList);

    int process_trd_write_spd(std::vector<unsigned>& argList);
    int process_trd_read_spd(std::vector<unsigned>& argList);
    int process_trd_wait_spd_val(std::vector<unsigned>& argList);

    Fpga *m_fpga;
    std::string m_configFile;
};

#endif // TRDPROG_H
