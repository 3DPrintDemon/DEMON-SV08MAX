// Custom Math Library
// 
// Portions of this code are derived from the TI IQmath Library.
// Copyright (C) Texas Instruments Incorporated - http://www.ti.com/
// 
// Modifications and additional code have been added by Sovol3d.
// These modifications are distributed under the MIT License.
// Copyright (C) 2024-2025 Sovol3d <info@sovol3d.com>
// 
// TI IQmath Library components remain subject to the original TI proprietary license.
// This file may be distributed under the terms of the GNU GPLv3 license.

#ifndef __FAST_CAL_H__
#define __FAST_CAL_H__

#include <stdlib.h>
#include <stdint.h>

#define GLOBAL_IQ 12

#define TYPE_DEFAULT    (0)
#define TYPE_UNSIGNED   (1)

const uint8_t _IQ6div_lookup[65] = {
    0x7F, 0x7D, 0x7B, 0x79, 0x78, 0x76, 0x74, 0x73, 
    0x71, 0x6F, 0x6E, 0x6D, 0x6B, 0x6A, 0x68, 0x67, 
    0x66, 0x65, 0x63, 0x62, 0x61, 0x60, 0x5F, 0x5E, 
    0x5D, 0x5C, 0x5B, 0x5A, 0x59, 0x58, 0x57, 0x56, 
    0x55, 0x54, 0x53, 0x52, 0x52, 0x51, 0x50, 0x4F, 
    0x4E, 0x4E, 0x4D, 0x4C, 0x4C, 0x4B, 0x4A, 0x49, 
    0x49, 0x48, 0x48, 0x47, 0x46, 0x46, 0x45, 0x45, 
    0x44, 0x43, 0x43, 0x42, 0x42, 0x41, 0x41, 0x40, 0x40
};
#if GLOBAL_IQ == 12
#define  _IQ12(A) ((int32_t)((A) * ((int32_t)1 << 12)))
#define  _IQ(A)  _IQ12(A)
#elif GLOBAL_IQ == 14
#define  _IQ14(A) ((int32_t)((A) * ((int32_t)1 << 14)))
#define  _IQ(A)  _IQ14(A)
#elif GLOBAL_IQ == 16
#define  _IQ16(A) ((int32_t)((A) * ((int32_t)1 << 16)))
#define  _IQ(A)   _IQ16(A)
#elif GLOBAL_IQ == 24
#define  _IQ24(A) ((int32_t)((A) * ((int32_t)1 << 24)))
#define  _IQ(A)   _IQ24(A)
#endif

/* mpy */
static inline int32_t __IQNmpy(int32_t iqNInput1, int32_t iqNInput2, const int8_t q_value)
{
    int32_t uLow,vLow;
    int32_t uHigh,vHigh;
    int64_t iqNResult;
    
    /* Implement MULHS, high side of 64-bit 32bitx32bit multiplication */
    uLow = iqNInput1 & 0xFFFF;
    vLow = iqNInput2 & 0xFFFF;
    
    uHigh = iqNInput1 >> 16;
    vHigh = iqNInput2 >> 16;
    
    iqNResult = (uLow*vLow);
    iqNResult += (int64_t)(uHigh*vLow + uLow*vHigh) <<16 ;
    iqNResult += (int64_t)(uHigh*vHigh) << 32;
    iqNResult = iqNResult >> q_value;
    
    return (int32_t)iqNResult;
}
#if GLOBAL_IQ == 12
int32_t _IQ12mpy(int32_t a, int32_t b)
{
    return __IQNmpy(a, b, 12);
}
#define  _IQmpy(A, B)  _IQ12mpy(A, B)
#elif GLOBAL_IQ == 14
int32_t _IQ14mpy(int32_t a, int32_t b)
{
    return __IQNmpy(a, b, 14);
}
#define  _IQmpy(A, B)  _IQ14mpy(A, B)
#elif GLOBAL_IQ == 16
int32_t _IQ16mpy(int32_t a, int32_t b)
{
    return __IQNmpy(a, b, 16);
}
#define  _IQmpy(A, B)  _IQ16mpy(A, B)
#elif GLOBAL_IQ == 24
int32_t _IQ24mpy(int32_t a, int32_t b)
{
    return __IQNmpy(a, b, 24);
}
#define  _IQmpy(A, B)  _IQ24mpy(A, B)
#endif

/* div */
static inline void __mpyf_start(uint16_t *ui16IntState, uint16_t *ui16MPYState)
{
    /* Do nothing. */
    return;
}

static inline void __mpy_stop(uint16_t *ui16IntState, uint16_t *ui16MPYState)
{
    /* Do nothing. */
    return;
}

static inline int32_t __mpyf_ul_reuse_arg1(uint32_t arg1, uint32_t arg2)
{
    /* This is identical to __mpyf_ul */
    return (uint32_t)(((uint64_t)arg1 * (uint64_t)arg2) >> 31);
}

static inline uint32_t __mpyf_ul(uint32_t arg1, uint32_t arg2)
{
    return (uint32_t)(((uint64_t)arg1 * (uint64_t)arg2) >> 31);
}

static inline int32_t __IQNdiv(int32_t iqNInput1, int32_t iqNInput2, const uint8_t type, const int8_t q_value)
{
    uint8_t ui8Index, ui8Sign = 0;
    uint32_t ui32Temp;
    uint32_t uiq30Guess;
    uint32_t uiqNInput1;
    uint32_t uiqNInput2;
    uint32_t uiqNResult;
    uint64_t uiiqNInput1;
    uint16_t ui16IntState;
    uint16_t ui16MPYState;

    if (type == TYPE_DEFAULT) {
        /* save sign of denominator */
        if (iqNInput2 <= 0) {
            /* check for divide by zero */
            if (iqNInput2 == 0) {
                return INT32_MAX;
            }
            else {
                ui8Sign = 1;
                iqNInput2 = -iqNInput2;
            }
        }

        /* save sign of numerator */
        if (iqNInput1 < 0) {
            ui8Sign ^= 1;
            iqNInput1 = -iqNInput1;
        }
    }
    else {
        /* Check for divide by zero */
        if (iqNInput2 == 0) {
            return INT32_MAX;
        }
    }

    /* Save input1 and input2 to unsigned IQN and IIQN (64-bit). */
    uiiqNInput1 = (uint64_t)iqNInput1;
    uiqNInput2 = (uint32_t)iqNInput2;

    /* Scale inputs so that 0.5 <= uiqNInput2 < 1.0. */
    while (uiqNInput2 < 0x40000000) {
        uiqNInput2 <<= 1;
        uiiqNInput1 <<= 1;
    }

    /*
     * Shift input1 back from iq31 to iqN but scale by 2 since we multiply
     * by result in iq30 format.
     */
    if (q_value < 31) {
        uiiqNInput1 >>= (31 - q_value - 1);
    }
    else {
        uiiqNInput1 <<= 1;
    }

    /* Check for saturation. */
    if (uiiqNInput1 >> 32) {
        if (ui8Sign) {
            return INT32_MIN;
        }
        else {
            return INT32_MAX;
        }
    }
    else {
        uiqNInput1 = (uint32_t)uiiqNInput1;
    }

    /* use left most 7 bits as ui8Index into lookup table (range: 32-64) */
    ui8Index = uiqNInput2 >> 24;
    ui8Index -= 64;
    uiq30Guess = (uint32_t)_IQ6div_lookup[ui8Index] << 24;

    /*
     * Mark the start of any multiplies. This will disable interrupts and set
     * the multiplier to fractional mode. This is designed to reduce overhead
     * of constantly switching states when using repeated multiplies (MSP430
     * only).
     */
    __mpyf_start(&ui16IntState, &ui16MPYState);

    /* 1st iteration */
    ui32Temp = __mpyf_ul(uiq30Guess, uiqNInput2);
    ui32Temp = -((uint32_t)ui32Temp - 0x80000000);
    ui32Temp = ui32Temp << 1;
    uiq30Guess = __mpyf_ul_reuse_arg1(uiq30Guess, ui32Temp);

    /* 2nd iteration */
    ui32Temp = __mpyf_ul(uiq30Guess, uiqNInput2);
    ui32Temp = -((uint32_t)ui32Temp - 0x80000000);
    ui32Temp = ui32Temp << 1;
    uiq30Guess = __mpyf_ul_reuse_arg1(uiq30Guess, ui32Temp);

    /* 3rd iteration */
    ui32Temp = __mpyf_ul(uiq30Guess, uiqNInput2);
    ui32Temp = -((uint32_t)ui32Temp - 0x80000000);
    ui32Temp = ui32Temp << 1;
    uiq30Guess = __mpyf_ul_reuse_arg1(uiq30Guess, ui32Temp);

    /* Multiply 1/uiqNInput2 and uiqNInput1. */
    uiqNResult = __mpyf_ul(uiq30Guess, uiqNInput1);

    /*
     * Mark the end of all multiplies. This restores MPY and interrupt states
     * (MSP430 only).
     */
    __mpy_stop(&ui16IntState, &ui16MPYState);

    /* Saturate, add the sign and return. */
    if (type == TYPE_DEFAULT) {
        if (uiqNResult > INT32_MAX) {
            if (ui8Sign) {
                return INT32_MIN;
            }
            else {
                return INT32_MAX;
            }
        }
        else {
            if (ui8Sign) {
                return -(int32_t)uiqNResult;
            }
            else {
                return (int32_t)uiqNResult;
            }
        }
    }
    else {
        return uiqNResult;
    }
}
#if GLOBAL_IQ == 12
int32_t _IQ12div(int32_t a, int32_t b)
{
    return __IQNdiv(a, b, TYPE_DEFAULT, 12);
}
#define  _IQdiv(A, B)   _IQ12div(A, B)
#elif GLOBAL_IQ == 14
int32_t _IQ14div(int32_t a, int32_t b)
{
    return __IQNdiv(a, b, TYPE_DEFAULT, 14);
}
#define  _IQdiv(A, B)   _IQ14div(A, B)
#elif GLOBAL_IQ == 16
int32_t _IQ16div(int32_t a, int32_t b)
{
    return __IQNdiv(a, b, TYPE_DEFAULT, 16);
}
#define  _IQdiv(A, B)   _IQ16div(A, B)
#elif GLOBAL_IQ == 24
int32_t _IQ24div(int32_t a, int32_t b)
{
    return __IQNdiv(a, b, TYPE_DEFAULT, 24);
}
#define  _IQdiv(A, B)   _IQ24div(A, B)
#endif

#if GLOBAL_IQ == 12
#define  _IQ12abs(A) (((A) < 0) ? - (A) : (A))
#define  _IQabs(A)   _IQ12abs(A)
#elif GLOBAL_IQ == 14
#define  _IQ14abs(A) (((A) < 0) ? - (A) : (A))
#define  _IQabs(A)   _IQ14abs(A)
#elif GLOBAL_IQ == 16
#define  _IQ16abs(A) (((A) < 0) ? - (A) : (A))
#define  _IQabs(A)   _IQ16abs(A)
#elif GLOBAL_IQ == 24
#define  _IQ24abs(A) (((A) < 0) ? - (A) : (A))
#define  _IQabs(A)   _IQ24abs(A)
#endif

static inline float __IQNtoF(int32_t iqNInput, int8_t q_value)
{
    uint16_t ui16Exp;
    uint32_t uiq31Input;
    union {
        uint32_t _uiq23Result;
        float _fValue;
    }converter;

    /* Initialize exponent to the offset iq value. */
    ui16Exp = 0x3f80 + ((31 - q_value) * ((uint32_t) 1 << (23 - 16)));

    /* Save the sign of the iqN input to the exponent construction. */
    if (iqNInput < 0) {
        ui16Exp |= 0x8000;
        uiq31Input = -iqNInput;
    } else if (iqNInput == 0) {
        return (0);
    } else {
        uiq31Input = iqNInput;
    }

    /* Scale the iqN input to uiq31 by keeping track of the exponent. */
    while ((uint16_t)(uiq31Input >> 16) < 0x8000) {
        uiq31Input <<= 1;
        ui16Exp -= 0x0080;
    }

    /* Round the uiq31 result and and shift to uiq23 */
    converter._uiq23Result = (uiq31Input + 0x0080) >> 8;

    /* Remove the implied MSB bit of the mantissa. */
    converter._uiq23Result &= ~0x00800000;

    /*
     * Add the constructed exponent and sign bit to the mantissa. We must use
     * an add in the case where rounding would cause the mantissa to overflow.
     * When this happens the mantissa result is two where the MSB is zero and
     * the LSB of the exp is set to 1 instead. Adding one to the exponent is the
     * correct handling for a mantissa of two. It is not required to scale the
     * mantissa since it will always be equal to zero in this scenario.
     */
    converter._uiq23Result += (uint32_t) ui16Exp << 16;

    /* Return the mantissa + exp + sign result as a floating point type. */
    return converter._fValue;
}

#if GLOBAL_IQ == 12
float _IQ12toF(int32_t a)
{
    return __IQNtoF(a, 12);
}
#define  _IQtoF(A) _IQ12toF(A)
#elif GLOBAL_IQ == 14
float _IQ14toF(int32_t a)
{
    return __IQNtoF(a, 14);
}
#define  _IQtoF(A) _IQ14toF(A)
#elif GLOBAL_IQ == 16
float _IQ16toF(int32_t a)
{
    return __IQNtoF(a, 16);
}
#define  _IQtoF(A) _IQ16toF(A)
#elif GLOBAL_IQ == 24
float _IQ24toF(int32_t a)
{
    return __IQNtoF(a, 24);
}
#define  _IQtoF(A) _IQ24toF(A)
#endif

#endif
