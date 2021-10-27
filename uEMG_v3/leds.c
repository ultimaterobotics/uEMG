#include "leds.h"

uint8_t led_pins[4];
uint32_t led_pin_mask[4];
uint32_t led_set_mask = 0;

volatile int led_pulse_length = 0;

int led_pwm_vals[4] = {0, 512, 1023, 0};
uint16_t pwm_seq[8];

void start_leds_pwm(int ms_length)
{
	if(NRF_PWM0->ENABLE)
	{
		NRF_PWM0->ENABLE = 0;
	}
	NRF_PWM0->PSEL.OUT[3] = 0xFFFFFFFF;
	for(int x = 0; x < 4; x++)
	{
		NRF_PWM0->PSEL.OUT[x] = led_pins[x];
		pwm_seq[x] = led_pwm_vals[x];
		pwm_seq[4+x] = 1023;
	}
	NRF_PWM0->ENABLE = 1;
	NRF_PWM0->MODE = 0;
	NRF_PWM0->COUNTERTOP = 1024;
	NRF_PWM0->PRESCALER = 0;
	NRF_PWM0->DECODER = 2;
	NRF_PWM0->LOOP = 0;
	NRF_PWM0->SEQ[0].PTR = (uint32_t)pwm_seq;
	NRF_PWM0->SEQ[0].CNT = 8;
	NRF_PWM0->SEQ[0].REFRESH = 16*ms_length;
	NRF_PWM0->INTEN = 1<<4; //SEQ0 END
//	if(ms_length == 0) NRF_PWM0->INTEN = 0;
	NRF_PWM0->TASKS_SEQSTART[0] = 1;
	NRF_PWM0->SHORTS = 1; //SEQ0END -> STOP
//	if(ms_length == 0) NRF_PWM0->SHORTS = 0;
}

void PWM0_IRQHandler()
{
	if(NRF_PWM0->EVENTS_SEQEND[0])
	{
		NRF_PWM0->ENABLE = 0;
		NRF_GPIO->OUTCLR = led_pin_mask[0];
		NRF_PWM0->EVENTS_SEQEND[0] = 0;
	}
}

void leds_init(int pin_r, int pin_g, int pin_b, int pin_driver)
{
	uint8_t lp[8] = {16, 15, 13, 12};
	
	for(int x = 0; x < 4; x++)
	{
		led_pins[x] = lp[x];
		led_pin_mask[x] = 1<<led_pins[x];
		NRF_GPIO->DIRSET = led_pin_mask[x];
	}	
}
 
int val_to_cc(int val)
{
	int v2 = val*val;
//	v2 >>= 2;
	v2 >>= 6;
//	if(v2 == 0) v2 = 1;
	if(v2 > 1023) v2 = 1023; 
//	if(v2 > 16384) v2 = 16384; 
	return v2;
}

void leds_pulse(int l1, int l2, int l3, int l4, int length)
{
	led_pwm_vals[0] = val_to_cc(l1);
	led_pwm_vals[1] = val_to_cc(l2);
	led_pwm_vals[2] = val_to_cc(l3);
	led_pwm_vals[3] = val_to_cc(l4);
	start_leds_pwm(length);
}

