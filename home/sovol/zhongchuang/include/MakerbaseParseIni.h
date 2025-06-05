#ifndef MAKERBASEPARSEINI_H
#define MAKERBASEPARSEINI_H



#include <iostream>
#include "./dictionary.h"
#include "./iniparser.h"


#define INIPATH "/home/sovol/zhongchuang/mksini.ini"

// std::string get_cfg_serial();
int mksini_load();
void mksini_free();
std::string mksini_getstring(std::string section, std::string key, std::string def);
int mksini_getint(std::string section, std::string key, int notfound);
double mksini_getdouble(std::string section, std::string key, double notfound);
bool mksini_getboolean(std::string section, std::string key, int notfound);
int mksini_set(std::string section, std::string key, std::string value);
void mksini_unset(std::string section, std::string key);
void mksini_save();

void mksversion_free();
std::string mksversion_mcu(std::string def);
std::string mksversion_ui(std::string def);
std::string mksversion_soc(std::string def);

#endif
