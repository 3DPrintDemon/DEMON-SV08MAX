#include "ws2811.h"
#include "generic/armcm_timer.h" // udelay
#include "stm32/internal.h"
#include "stm32/gpio.h"

#define GPIO_CS GPIO('C', 10)

void RGB_IOInit(void)
{
	gpio_peripheral(GPIO_CS, GPIO_OUTPUT, 0);
}

void GPIO_SetBits(uint32_t pin)
{
	gpio_out_setup(pin, 1);
}

void GPIO_ResetBits(uint32_t pin)
{
	gpio_out_setup(pin, 0);
}

void DelayNops(uint32_t n)
{
    uint32_t clk_freq = SystemCoreClock;
    uint32_t cycles = (clk_freq / 1000000000) * n;
    for (uint32_t i = 0; i < cycles; i++) {
        __NOP();
    }
}

void SendOne(void){//T1H -> T1L
	
	GPIO_SetBits(GPIO_CS);
	DelayNops(2);
	
	GPIO_ResetBits(GPIO_CS);
	DelayNops(1);
}

void SendZero(void){//T0H -> T0L
	GPIO_SetBits(GPIO_CS);
	DelayNops(1);
	
	GPIO_ResetBits(GPIO_CS);
	DelayNops(2);
}

void SendRGB(uint8_t red, uint8_t green, uint8_t blue) {
	uint8_t i = 0;
	for(i = 0;i < 8; i++){
		if(red&0x80){
			SendOne();
		}
		else{
			SendZero();
		}
		red = red<<1;
	}
	for(i = 0;i < 8; i++){
		if(green&0x80){
			SendOne();
		}
		else{
			SendZero();
		}
		green = green<<1;
	}
	for(i = 0;i < 8; i++){
		if(blue&0x80){
			SendOne();
		}
		else{
			SendZero();
		}
		blue = blue<<1;
	}
}

void RGB_Reset(void){							
	GPIO_ResetBits(GPIO_CS);	
	DelayNops(5);
	GPIO_SetBits(GPIO_CS);		
}

void lcd_init(void) {
	RGB_IOInit();
	RGB_Reset();
  SendRGB(255, 255, 255);
}