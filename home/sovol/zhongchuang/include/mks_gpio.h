#ifndef MKS_GPIO_H
#define MKS_GPIO_H

static int gpio_config(const char *attr, const char *val, const char *gpio_path);
int set_GPIO1_C5_low();
int set_GPIO2_C4_high();
int init_GPIO2_A3();
void *monitor_GPIO2_A3(void *arg);
int set_GPIO1_B3_low();
void *sled1_ctrl(void *arg);
void *sled2_ctrl(void *arg);
void *sled3_ctrl(void *arg);
int set_GPIO2_B7_low();
int set_GPIO2_A2_low();
int set_GPIO3_A5_low();
int set_GPIO2_B7_high();
int set_GPIO2_A2_high();
int set_GPIO3_A5_high();

#endif
