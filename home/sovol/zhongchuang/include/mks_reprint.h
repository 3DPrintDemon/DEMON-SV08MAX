#ifndef __MKS_REPRINT_H__
#define __MKS_REPRINT_H__

#include "file_list.h"


#define PLR_PARAMETER_PATH "/home/sovol/plr_parameter.txt"


typedef struct
{
    float if_reprint;        //是否断电续打
    float z_pos;             //续打z高度    
    float e_pos;             //续打e值  
    float if_absolute_extrude;  //E轴是否使用绝对坐标      
}MKS_REPRINT_PARAMETER_T;
extern MKS_REPRINT_PARAMETER_T mks_reprint_parameter;









void save_z_e_value(float new_pos_z, float pos_e);
int get_reprint_parameter();
int save_reprint_parameter(const char *key, float value);



#endif
