#ifndef MKS_GCODE_H
#define MKS_GCODE_H

#include "nlohmann/json.hpp"

extern bool printer_pid_finished;
extern std::string one_time_passcode;
extern std::string one_time_passlink;

void parse_gcode_response(nlohmann::json params);

#endif
