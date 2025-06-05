#include "../include/KlippyGcodes.h"
#include "KlippyGcodes.h"

// chris add
std::string set_pin_value(int pin)
{
    return SET_PIN + (std::string)" PIN=main_led VALUE=" + std::to_string(pin);
}

std::string set_bed_temp(int temp) {
    return SET_BED_TEMP + (std::string)" S" + std::to_string(temp);
}

std::string set_ext_temp(int temp, int tool) {
    return SET_EXT_TEMP + (std::string)" T" + std::to_string(tool) + (std::string)" S" + std::to_string(temp);
}

std::string set_heater_temp(std::string heater, int temp) {
    return "SET_HEATER_TEMPERATURE heater=" + heater + " target=" + std::to_string(temp);
}

std::string set_temp_fan_temp(std::string temp_fan, int temp) {
    return "SET_TEMPERATURE_FAN_TARGET temperature_fan=" + temp_fan + " target=" + std::to_string(temp);
}

std::string set_chamber_temp(int temp) {
    return "SET_HEATER_TEMPERATURE" + (std::string)" HEATER=chamber_temp TARGET=" + std::to_string(temp);  //chris todo
}

std::string set_model_fan_speed(int speed) {
    std::string speed_temp = std::to_string(float((float)(speed) / 100));
    std::cout << speed_temp << std::endl;
    return "SET_FAN_SPEED FAN=fan_model SPEED=" + speed_temp;
}



std::string set_extrusion_rate(std::string rate) {
    return SET_EXT_FACTOR + (std::string)" S" + rate;
}

std::string set_speed_rate(std::string rate) {
    return SET_SPD_FACTOR + (std::string)" S" + rate;
}

std::string testz_move(std::string dist) {
    return TESTZ + dist;
}

std::string extrude(std::string dist, int speed) {
    return MOVE + (std::string)" E" + dist + (std::string)" F" + std::to_string(speed);
}

std::string bed_mesh_load(std::string profile) {
    return "BED_MESH_PROFILE LOAD=" + profile;
}

std::string bed_mesh_remove(std::string profile) {
    return "BED_MESH_PROFILE REMOVE=" + profile;
}

std::string bed_mesh_save(std::string profile) {
    return "BED_MESH_PROFILE SAVE=" + profile;
}

