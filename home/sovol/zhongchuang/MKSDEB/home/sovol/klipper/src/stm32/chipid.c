// Support for extracting the hardware chip id on stm32
//
// Copyright (C) 2019  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "generic/canserial.h" // canserial_set_uuid
#include "generic/usb_cdc.h" // usb_fill_serial
#include "generic/usbstd.h" // usb_string_descriptor
#include "internal.h" // UID_BASE
#include "sched.h" // DECL_INIT

#define CHIP_UID_LEN 12

static struct {
    struct usb_string_descriptor desc;
    uint16_t data[CHIP_UID_LEN * 2];
} cdc_chipid;

struct usb_string_descriptor *
usbserial_get_serialid(void)
{
   return &cdc_chipid.desc;
}

//stm32f750     0x8f     canbus_uuid:0d1445047cdd
//stm32f103     0x80     canbus_uuid:61755fe321ac 

void
chipid_init(void)
{
    uint8_t custumUID[] = { 0x12, 0x34, 0x56, 0x78, 0x99, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x8f };
    uint8_t *uid_base = custumUID;
    if (CONFIG_MACH_STM32H750 == 1) 
    {
        custumUID[CHIP_UID_LEN -1] = 0x8f;
    } 
    else if (CONFIG_MACH_STM32F103 == 1)
    {
        custumUID[CHIP_UID_LEN -1] = 0x80;   //多个stm32f103可以修改这个值
    }
    
    if (CONFIG_USB_SERIAL_NUMBER_CHIPID)
        usb_fill_serial(&cdc_chipid.desc, ARRAY_SIZE(cdc_chipid.data)
                        , (void*)uid_base);
    if (CONFIG_CANBUS)
        canserial_set_uuid((void *)uid_base, CHIP_UID_LEN);  //(void*)UID_BASE
}
DECL_INIT(chipid_init);
