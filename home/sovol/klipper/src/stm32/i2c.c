// I2C functions on stm32
//
// Copyright (C) 2019  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "autoconf.h" // CONFIG_MACH_STM32F1
#include "board/misc.h" // timer_is_before
#include "command.h" // shutdown
#include "gpio.h" // i2c_setup
#include "internal.h" // GPIO
#include "sched.h" // sched_shutdown
#include "board/irq.h" //irq_disable
#include "i2ccmds.h"   // I2C_BUS_SUCCESS

struct i2c_info {
    I2C_TypeDef *i2c;
    uint8_t scl_pin, sda_pin;
};

DECL_ENUMERATION("i2c_bus", "i2c1", 0);
DECL_CONSTANT_STR("BUS_PINS_i2c1", "PB6,PB7");
DECL_ENUMERATION("i2c_bus", "i2c1a", 1);
DECL_CONSTANT_STR("BUS_PINS_i2c1a", "PB8,PB9");
DECL_ENUMERATION("i2c_bus", "i2c2", 2);
DECL_CONSTANT_STR("BUS_PINS_i2c2", "PB10,PB11");
#if CONFIG_MACH_STM32F2 || CONFIG_MACH_STM32F4
DECL_ENUMERATION("i2c_bus", "i2c3", 3);
DECL_CONSTANT_STR("BUS_PINS_i2c3", "PA8,PC9");
  #if CONFIG_MACH_STM32F2 || CONFIG_MACH_STM32F4x5
DECL_ENUMERATION("i2c_bus", "i2c2a", 4);
DECL_CONSTANT_STR("BUS_PINS_i2c2a", "PH4,PH5");
DECL_ENUMERATION("i2c_bus", "i2c3a", 5);
DECL_CONSTANT_STR("BUS_PINS_i2c3a", "PH7,PH8");
  #endif
#endif

static const struct i2c_info i2c_bus[] = {
    { I2C1, GPIO('B', 6), GPIO('B', 7) },
    { I2C1, GPIO('B', 8), GPIO('B', 9) },
    { I2C2, GPIO('B', 10), GPIO('B', 11) },
#if CONFIG_MACH_STM32F2 || CONFIG_MACH_STM32F4
    { I2C3, GPIO('A', 8), GPIO('C', 9) },
  #if CONFIG_MACH_STM32F2 || CONFIG_MACH_STM32F4x5
    { I2C2, GPIO('H', 4), GPIO('H', 5) },
    { I2C3, GPIO('H', 7), GPIO('H', 8) },
  #endif
#endif
};

static void
i2c_us_delay(uint32_t us)
{
    uint32_t t = timer_read_time() + timer_from_us(us);
    while(t > timer_read_time());
}

static void
i2c_init(I2C_TypeDef *i2c)
{
    uint32_t pclk = get_pclock_frequency((uint32_t)i2c);
    i2c->CR2 = pclk / 1000000;
    i2c->CCR = pclk / 100000 / 2;
    i2c->TRISE = (pclk / 1000000) + 1;
    i2c->CR1 = I2C_CR1_PE;
}

// Work around stm32 errata causing busy bit to be stuck
static void
i2c_busy_errata(I2C_TypeDef *i2c)
{
    if(! CONFIG_MACH_STM32F1)
        return ;
    
    const struct i2c_info *ii = container_of((I2C_TypeDef * const *)i2c, struct i2c_info, i2c);
    uint32_t val;
    (void)val;
    val = i2c->SR1;
    val = i2c->SR2;
    val = i2c->DR;
    i2c->CR1 = 0;
    i2c->CR2 = 0;
    i2c->DR = 0;

    gpio_peripheral(ii->scl_pin, GPIO_OUTPUT, 1);
    gpio_peripheral(ii->sda_pin, GPIO_OUTPUT, 1);
    i2c_us_delay(20);
    gpio_peripheral(ii->sda_pin, GPIO_OUTPUT | GPIO_OPEN_DRAIN, -1);
    gpio_peripheral(ii->scl_pin, GPIO_OUTPUT | GPIO_OPEN_DRAIN, -1);
    gpio_peripheral(ii->scl_pin, GPIO_OUTPUT | GPIO_OPEN_DRAIN, 1);
    gpio_peripheral(ii->sda_pin, GPIO_OUTPUT | GPIO_OPEN_DRAIN, 1);

    i2c->CR1 = I2C_CR1_SWRST;
    i2c_us_delay(5);
    i2c->CR1 = 0;
    i2c_init(i2c);
}

struct i2c_config
i2c_setup(uint32_t bus, uint32_t rate, uint8_t addr)
{
    // Lookup requested i2c bus
    if (bus >= ARRAY_SIZE(i2c_bus))
        shutdown("Unsupported i2c bus");
    const struct i2c_info *ii = &i2c_bus[bus];
    I2C_TypeDef *i2c = ii->i2c;

    if (!is_enabled_pclock((uint32_t)i2c)) {
        // Enable i2c clock and gpio
        enable_pclock((uint32_t)i2c);
        i2c_busy_errata(i2c);
        gpio_peripheral(ii->scl_pin, GPIO_FUNCTION(4) | GPIO_OPEN_DRAIN, 1);
        gpio_peripheral(ii->sda_pin, GPIO_FUNCTION(4) | GPIO_OPEN_DRAIN, 1);
        i2c->CR1 = I2C_CR1_SWRST;
        i2c->CR1 = 0;

        // Set 100Khz frequency and enable
        i2c_init(i2c);
    }

    return (struct i2c_config){ .i2c=i2c, .addr=addr<<1 };
}

static void
i2c_logging_info(I2C_TypeDef *i2c, uint32_t ret_code)
{
    sendf("ldc1612_i2c_report cr1_data=%u cr2_data=%u sr1_data=%u sr2_data=%u dr_data=%u err_code=%u",  
          (uint32_t)(i2c->CR1), (uint32_t)(i2c->CR2),
          (uint32_t)(i2c->SR1), (uint32_t)(i2c->SR2),
          (uint32_t)(i2c->DR),  (uint32_t)(ret_code));
}

static int
i2c_wait(I2C_TypeDef *i2c, uint32_t set, uint32_t clear, uint32_t timeout)
{
    for (;;) {
        int ret = 0;
        I2C_TypeDef i2c_register = {
            .SR1 = i2c->SR1, .SR2 = i2c->SR2, 
            .CR1 = i2c->CR1, .CR2 = i2c->CR2
        };
        
        if ((i2c_register.SR1 & set) == set && (i2c_register.SR1 & clear) == 0)
            return (int)I2C_BUS_SUCCESS;

        if (i2c_register.SR1 & I2C_SR1_AF)
            ret |= (1 << I2C_BUS_NACK);
        if (!timer_is_before(timer_read_time(), timeout))
            ret |= (1 << I2C_BUS_TIMEOUT);

        if (ret != 0) {
            if (i2c_register.SR2 & I2C_SR2_BUSY)
                ret |= (1 << I2C_BUS_BUSY);
            if (i2c_register.SR1 & I2C_SR1_BERR)
                ret |= (1 << I2C_BUS_ERR);
            i2c_busy_errata(i2c);
            i2c_register.DR = i2c->DR;
            i2c_logging_info(&i2c_register, ret);
            return ret;
        }
    }
}

static int
i2c_start(I2C_TypeDef *i2c, uint8_t addr, uint8_t xfer_len,
          uint32_t timeout)
{
    int ret = 0;
    i2c->CR1 = I2C_CR1_START | I2C_CR1_PE;
    ret = i2c_wait(i2c, I2C_SR1_SB, 0, timeout);
    if (ret != I2C_BUS_SUCCESS) {
        if (ret == I2C_BUS_BUSY) {
            if (i2c_wait(i2c, I2C_SR1_SB, 0, timeout) != I2C_BUS_SUCCESS) {
                ret = I2C_BUS_BUSY;
                goto abrt;
            }
        } else 
            goto abrt;
    }
    i2c->DR = addr;
    if (addr & 0x01)
        i2c->CR1 |= I2C_CR1_ACK;
    ret = i2c_wait(i2c, I2C_SR1_ADDR, 0, timeout);
    if (ret != I2C_BUS_SUCCESS) {
        if (ret == I2C_BUS_BUSY) {
            if (i2c_wait(i2c, I2C_SR1_ADDR, 0, timeout) != I2C_BUS_SUCCESS) {
                ret = I2C_BUS_BUSY;
                goto abrt;
            }
        } else 
            goto abrt;
    }
    irqstatus_t flag = irq_save();
    uint32_t sr2 = i2c->SR2;
    if (addr & 0x01 && xfer_len == 1)
        i2c->CR1 = I2C_CR1_STOP | I2C_CR1_PE;
    irq_restore(flag);
    if (!(sr2 & I2C_SR2_MSL))
        shutdown("Failed to send i2c addr");
abrt:
    return ret;
}

static int
i2c_send_byte(I2C_TypeDef *i2c, uint8_t b, uint32_t timeout)
{
    int ret = 0;
    i2c->DR = b;
    ret = i2c_wait(i2c, I2C_SR1_TXE, 0, timeout);
    return ret;
}

static int
i2c_read_byte(I2C_TypeDef *i2c, uint8_t *rb, uint32_t timeout, uint8_t remaining)
{
    int ret = 0;
    ret = i2c_wait(i2c, I2C_SR1_RXNE, 0, timeout);
    if (ret != I2C_BUS_SUCCESS)
        goto abrt;
    irqstatus_t flag = irq_save();
    uint8_t b = i2c->DR;
    *rb = b;
    if (remaining == 1)
        i2c->CR1 = I2C_CR1_STOP | I2C_CR1_PE;
    irq_restore(flag);
abrt:
    return ret;
}

static void
i2c_stop(I2C_TypeDef *i2c, uint32_t timeout)
{
    i2c->CR1 = I2C_CR1_STOP | I2C_CR1_PE;
    i2c_wait(i2c, 0, I2C_SR1_TXE, timeout);
}

int
i2c_write(struct i2c_config config, uint8_t write_len, uint8_t *write)
{
    int ret = 0;
    I2C_TypeDef *i2c = config.i2c;
    uint32_t timeout = timer_read_time() + timer_from_us(5000);

    ret = i2c_start(i2c, config.addr, write_len, timeout);
    if (ret != I2C_BUS_SUCCESS)
        goto abrt;
    while (write_len--) {
        ret = i2c_send_byte(i2c, *write++, timeout);
    }
    i2c_stop(i2c, timeout);
abrt:
    return ret;
}

int
i2c_read(struct i2c_config config, uint8_t reg_len, uint8_t *reg
         , uint8_t read_len, uint8_t *read)
{
    int ret = 0;
    I2C_TypeDef *i2c = config.i2c;
    uint32_t timeout = timer_read_time() + timer_from_us(5000);
    uint8_t addr = config.addr | 0x01;

    if (reg_len) {
        // write the register
        ret = i2c_start(i2c, config.addr, reg_len, timeout);
        if (ret != I2C_BUS_SUCCESS)
            goto abrt;
        while(reg_len--) {
            ret = i2c_send_byte(i2c, *reg++, timeout);
        }
    }
    // start/re-start and read data
    ret = i2c_start(i2c, addr, read_len, timeout);
    if (ret != I2C_BUS_SUCCESS)
        goto abrt;
    while(read_len--) {
        ret = i2c_read_byte(i2c, read, timeout, read_len);
        read++;
    }
    ret = i2c_wait(i2c, 0, I2C_SR1_RXNE, timeout);
    if (ret != I2C_BUS_SUCCESS)
        goto abrt;
abrt:
    return ret;
}
