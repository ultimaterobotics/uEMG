/**
 */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "nrf.h"
#include "mcp3912.h"
#include "bmi160.h"
#include "leds.h"
#include "urf_star_protocol.h"
#include "urf_timer.h"
#include "imu_fusion.h"

//openocd -f interface/stlink-v2.cfg -f target/nrf52.cfg
//flash write_image erase _build/nrf52832_xxaa.hex

uint8_t pin_button = 28;
uint8_t pin_syson = 27;

float battery_mv = 3500; //by default non zero to prevent power off before actual measurement
int battery_low_threshold = 3200;

uint8_t battery_level = 0;
uint8_t packet_id = 0;
uint8_t data_packet[258];

uint8_t unsent_cnt = 0;

uint8_t mcp_data_id = 0;
int mcp_buffer[130];
uint8_t mcp_buf_pos = 0;
uint8_t mcp_buf_len = 128;
uint8_t mcp_cnt = 0;

uint8_t param_send_id = 0;

uint8_t emg_raw_data_send = 1;

enum param_sends
{
	param_batt = 0,
	param_imu_steps,
	param_end
};

uint8_t part2_packet = 0;

void prepare_data_packet()
{
	uint8_t idx = 0;
	packet_id++;
	if(packet_id > 128) packet_id = 0;
	data_packet[idx++] = packet_id;
	data_packet[idx++] = 0; //fill in the end
	uint32_t unit_id = NRF_FICR->DEVICEID[1];
	data_packet[idx++] = (unit_id>>24)&0xFF;
	data_packet[idx++] = (unit_id>>16)&0xFF;
	data_packet[idx++] = (unit_id>>8)&0xFF;
	data_packet[idx++] = (unit_id)&0xFF;
	uint8_t send_cnt = 16;
	if(emg_raw_data_send)
	{
		send_cnt = 32;
		data_packet[idx++] = 220+send_cnt;
	}
	else
	{
		if(part2_packet)
		{
			data_packet[idx++] = 100+send_cnt;
			part2_packet = 0;
		}
		else
			data_packet[idx++] = 200+send_cnt;
	}

	param_send_id++;
	if(param_send_id == param_end) param_send_id = 0;

	data_packet[idx++] = param_send_id;
	if(param_send_id == param_batt)
	{
		data_packet[idx++] = battery_level;
		data_packet[idx++] = 100; //version_id
		data_packet[idx++] = 0;
	}
	if(param_send_id == param_imu_steps)
	{
		sBMI160 *bmi = bmi160_get_data();
		int steps = bmi->step_cnt;
		data_packet[idx++] = (steps>>8)&0xFF;
		data_packet[idx++] = steps&0xFF;
		data_packet[idx++] = 0;
	}
	
	data_packet[idx++] = mcp_data_id;
	int start_cnt = mcp_buf_pos - send_cnt;
	if(start_cnt < 0) start_cnt += mcp_buf_len;
	for(int n = 0; n < send_cnt; n++)
	{
		data_packet[idx++] = mcp_buffer[start_cnt]>>8;
		data_packet[idx++] = mcp_buffer[start_cnt]&0xFF;
		start_cnt++;
		if(start_cnt >= mcp_buf_len) start_cnt = 0;
	}
	
//	if(!emg_raw_data_send)
	{
		sBMI160 *bmi = bmi160_get_data();
		int16_t b_ax = bmi->raX;
		int16_t b_ay = bmi->raY;
		int16_t b_az = bmi->raZ;
		data_packet[idx++] = (b_ax>>8)&0xFF;
		data_packet[idx++] = b_ax&0xFF;
		data_packet[idx++] = (b_ay>>8)&0xFF;
		data_packet[idx++] = b_ay&0xFF;
		data_packet[idx++] = (b_az>>8)&0xFF;
		data_packet[idx++] = b_az&0xFF;

		int16_t b_wx = bmi->rwX;
		int16_t b_wy = bmi->rwY;
		int16_t b_wz = bmi->rwZ;
		data_packet[idx++] = (b_wx>>8)&0xFF;
		data_packet[idx++] = b_wx&0xFF;
		data_packet[idx++] = (b_wy>>8)&0xFF;
		data_packet[idx++] = b_wy&0xFF;
		data_packet[idx++] = (b_wz>>8)&0xFF;
		data_packet[idx++] = b_wz&0xFF;
		
		sQ *cur_q = imu_fusion_get_q();
		b_wx = cur_q->x * 30000.0f;
		b_wy = cur_q->y * 30000.0f;
		b_wz = cur_q->z * 30000.0f;
		int16_t b_ww = cur_q->w * 30000.0f;
		data_packet[idx++] = (b_wx>>8)&0xFF;
		data_packet[idx++] = b_wx&0xFF;
		data_packet[idx++] = (b_wy>>8)&0xFF;
		data_packet[idx++] = b_wy&0xFF;
		data_packet[idx++] = (b_wz>>8)&0xFF;
		data_packet[idx++] = b_wz&0xFF;
		data_packet[idx++] = (b_ww>>8)&0xFF;
		data_packet[idx++] = b_ww&0xFF;
	}
	data_packet[1] = idx;
}

float avg_emg[4] = {0, 0, 0, 0};
int emg_lvl[4] = {0, 0, 0, 0};
int spectr_part2_pending = 0;
uint32_t spectr_part2_time = 0;
int mcp_buf_sp2[16];

int push_mcp_data()
{
//	if(emg_mode)	
	if(mcp_fft_process())
	{
		if(emg_raw_data_send)
		{
			float *dat_base = mcp_fft_get_raw();
			for(int c = 0; c < 4; c++)
			{
				float *dat = dat_base + c*8;
				for(int x = 0; x < 8; x++)
				{
					mcp_buffer[mcp_buf_pos++] = dat[x];
					if(mcp_buf_pos >= mcp_buf_len) mcp_buf_pos = 0;
				}
			}
		}
		else
		{
//			spectr_part2_pending = 1;
//			spectr_part2_time = millis();
			float *sp_base = mcp_fft_get_spectr();
			for(int c = 0; c < 4; c++)
			{
				float *sp = sp_base + c*8;

				float avg_center = 0;
				float low_part = 0;
				float high_part = 0;

				for(int x = 0; x < 4; x++)
				{
					if(x > 0) avg_center += sp[x];
					if(x == 0) low_part += sp[x];
					else high_part += sp[x];
					float vv = sp[x];
					if(vv < 0) vv = 0;
					if(vv > 65535) vv = 65535;
					mcp_buffer[mcp_buf_pos++] = vv;
					if(mcp_buf_pos >= mcp_buf_len) mcp_buf_pos = 0;

					float vv2 = sp[4+x];
					if(vv2 < 0) vv2 = 0;
					if(vv2 > 65535) vv2 = 65535;
					mcp_buf_sp2[c*4+x] = vv2;
				}
				
				
				low_part /= 2.0f;
				high_part /= 6.0f;
			
				float h_coeff = 2.0f*high_part / (low_part + high_part + 0.001f);

				avg_center *= 0.03f * h_coeff;
			
				avg_emg[c] *= 0.8f;
				avg_emg[c] += 0.2f*avg_center;

				emg_lvl[c] = avg_emg[c] * 100.0f;
				if(avg_emg[c] > 650) emg_lvl[c] = 65000;

			}
			static uint32_t last_led_ms = 0;
			uint32_t ms = millis();
			if(ms - last_led_ms > 30)
			{
				last_led_ms = ms;
				float led_scale = 1.0;
				int l1 = avg_emg[0] * led_scale;
				int l2 = avg_emg[1] * led_scale;
				int l3 = avg_emg[2] * led_scale;
				int l4 = avg_emg[3] * led_scale;
				if(l1 > 255) l1 = 255;
				if(l2 > 255) l2 = 255;
				if(l3 > 255) l3 = 255;
				if(l4 > 255) l4 = 255;
				leds_pulse(l1, l2, l3, l4, 100);
//				leds_pulse(255, 255, 0, 0, 100);
			}
		}
		mcp_data_id++;
//		leds_set(255*((ms%2000)>1000), 0, 0, 0);
		return 1;
	}
	return 0;
}


void fast_clock_start()
{
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {}
}
void slow_clock_start()
{
	NRF_CLOCK->LFCLKSRC = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {}
}
void fast_clock_stop()
{
	slow_clock_start();
//	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
//	NRF_CLOCK->TASKS_LFCLKSTART = 1;
//	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {}
	NRF_CLOCK->TASKS_HFCLKSTOP = 1;
}


void mode_lowbatt()
{
//	set_led(0);
//	nrf_delay_ms(1);
//	mcp3912_turnoff();
	time_stop();
	fast_clock_stop();
	NRF_SPI0->ENABLE = 0;
	leds_pulse(0, 0, 0, 0, 1);
	delay_ms(2);
	NRF_GPIO->OUTCLR = 1<<pin_syson;
//	nrf_delay_ms(1);
	NRF_POWER->TASKS_LOWPWR = 1;
//	NRF_POWER->DCDCEN = 1;
	NRF_POWER->SYSTEMOFF = 1;
}
void stop_rtc()
{
	NRF_RTC1->TASKS_STOP = 1;
}
void start_rtc()
{
	stop_rtc();
	NRF_RTC1->TASKS_CLEAR = 1;
	NRF_RTC1->CC[0] = 220;
	NRF_RTC1->CC[1] = 0xFFFF;
	NRF_RTC1->CC[2] = 0xFFFF;
	NRF_RTC1->CC[3] = 0xFFFF;
	NRF_RTC1->PRESCALER = 0x3CFFFF;
	NRF_RTC1->INTENSET = (1<<16); //CC0 event
	NVIC_EnableIRQ(RTC1_IRQn);
	NRF_RTC1->TASKS_START = 1;
}
void RTC1_IRQHandler(void)
{
	/* Update compare counter */
	if (NRF_RTC1->EVENTS_COMPARE[0] != 0)
	{
		NRF_RTC1->EVENTS_COMPARE[0] = 0;
		NRF_RTC1->TASKS_CLEAR = 1;  // Clear Counter		    
//		if(U32_delay_ms) U32_delay_ms--; // used in V_hw_delay_ms()
		__SEV(); // to avoid race condition
		stop_rtc();
	}
}

void read_battery()
{
	
	uint32_t result = 0;
	
	// Configure SAADC singled-ended channel, Internal reference (0.6V) and 1/6 gain.
	NRF_SAADC->CH[0].CONFIG = (SAADC_CH_CONFIG_GAIN_Gain1_6    << SAADC_CH_CONFIG_GAIN_Pos) |
							(SAADC_CH_CONFIG_MODE_SE         << SAADC_CH_CONFIG_MODE_Pos) |
							(SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
							(SAADC_CH_CONFIG_RESN_Bypass     << SAADC_CH_CONFIG_RESN_Pos) |
							(SAADC_CH_CONFIG_RESP_Bypass     << SAADC_CH_CONFIG_RESP_Pos) |
							(SAADC_CH_CONFIG_TACQ_3us        << SAADC_CH_CONFIG_TACQ_Pos);

	// Configure the SAADC channel with VDD as positive input, no negative input(single ended).
	NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_AnalogInput0 << SAADC_CH_PSELP_PSELP_Pos;
	NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos;

	// Configure the SAADC resolution.
	NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_14bit << SAADC_RESOLUTION_VAL_Pos;

	// Configure result to be put in RAM at the location of "result" variable.
	NRF_SAADC->RESULT.MAXCNT = 1;
	NRF_SAADC->RESULT.PTR = (uint32_t)&result;

	// No automatic sampling, will trigger with TASKS_SAMPLE.
	NRF_SAADC->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task << SAADC_SAMPLERATE_MODE_Pos;

	// Enable SAADC (would capture analog pins if they were used in CH[0].PSELP)
	NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos;


	static int cal_cnt = 0;
	cal_cnt++;
	if(0)if(cal_cnt > 1000)
	{
		cal_cnt = 0;
		// Calibrate the SAADC (only needs to be done once in a while)
		NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
		while (NRF_SAADC->EVENTS_CALIBRATEDONE == 0);
		NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
		while (NRF_SAADC->STATUS == (SAADC_STATUS_STATUS_Busy <<SAADC_STATUS_STATUS_Pos));
	}

	// Start the SAADC and wait for the started event.
	NRF_SAADC->TASKS_START = 1;
	while (NRF_SAADC->EVENTS_STARTED == 0);
	NRF_SAADC->EVENTS_STARTED = 0;

	// Do a SAADC sample, will put the result in the configured RAM buffer.
	NRF_SAADC->TASKS_SAMPLE = 1;
	while (NRF_SAADC->EVENTS_END == 0);
	NRF_SAADC->EVENTS_END = 0;

// Convert the result to voltage
// Result = [V(p) - V(n)] * GAIN/REFERENCE * 2^(RESOLUTION)
// Result = (VDD - 0) * ((1/6) / 0.6) * 2^14
// VDD = Result / 4551.1
//  precise_result = (float)result / 4551.1f;
//  precise_result; // to get rid of set but not used warning

	// Stop the SAADC, since it's not used anymore.
	NRF_SAADC->TASKS_STOP = 1;
	while (NRF_SAADC->EVENTS_STOPPED == 0);
	NRF_SAADC->EVENTS_STOPPED = 0;
	
	float res = result;
	//16384 = 3600 mV on A0
	res = res * 3600.0f / 16384.0f; //now res is in mv
	res *= 2.0f; //1:2 divider on the battery measurement
	battery_mv = res;
	res -= 2000.0f; //everything below 2V is too low for a battery
	if(res < 0)
	{
		battery_level = 0;
		return;
	}
	if(res > 2550) //shouldn't ever happen - 4.55v on the battery is way too much
	{
		battery_level = 255;
		return;
	}
	battery_level = res / 10;
	return;
}


void mode_idle()
{
//	leds_pause();
	time_pause();
	NRF_POWER->TASKS_LOWPWR = 1;
}
void mode_resume_idle()
{
	NRF_POWER->TASKS_CONSTLAT = 1;
	time_resume();
//	leds_resume();
}

void low_power_cycle()
{
	NRF_RADIO->POWER = 0;
//	mode_idle();
	__WFI();
//	mode_resume_idle();
	NRF_RADIO->POWER = 1;
}


enum
{
	radio_mode_fast = 0,
	radio_mode_ble,
	radio_mode_fast64
};

uint8_t radio_mode = radio_mode_fast64;//radio_mode_ble;
/*
void RADIO_IRQHandler()
{
	if(radio_mode == radio_mode_fast || radio_mode == radio_mode_fast64)
		fr_irq_handler();
	if(radio_mode == radio_mode_ble)
		ble_irq_handler();
}*/

void switch_to_ble()
{
//	fr_disable();
//	fr_poweroff();
//	config_ble_adv();
}

void switch_to_fr32()
{
//	fr_disable();
//	fr_poweroff();
//	fr_init(32);
//	fr_listen();	
}

void switch_to_fr48()
{
//	fr_disable();
//	fr_poweroff();
//	fr_init(48);
//	fr_listen();	
}
void switch_to_fr64()
{
//	fr_disable();
//	fr_poweroff();
//	fr_init(64);
//	fr_listen();	
}

void process_btn_short()
{
	emg_raw_data_send = !emg_raw_data_send;
/*	if(radio_mode == radio_mode_ble)
	{
		radio_mode = radio_mode_fast64;
		switch_to_fr64();
	}
	else if(radio_mode == radio_mode_fast64)
	{
		radio_mode = radio_mode_fast;
		switch_to_fr32();
	}
	else
	{
		radio_mode = radio_mode_ble;
		switch_to_ble();
	}*/
}

void process_btn_long()
{
	NRF_GPIO->OUTCLR = 1<<pin_syson;
}

int main(void)
{
	int init_ok = 1;
	
	NRF_UICR->NFCPINS = 0;

	NRF_GPIO->PIN_CNF[pin_button] = 0;

	fast_clock_start();
	time_start();
	NRF_GPIO->DIRSET = 1<<pin_syson;
	NRF_GPIO->OUTSET = 1<<pin_syson;
	leds_init();

//	NRF_POWER->DCDCEN = 1;

	for(int x = 0; x < 4; x++)
	{
		leds_pulse((x==0)*255, (x==1)*255, (x==2)*255, (x==3)*255, 250);
		delay_ms(300);
	}
//	leds_set_driver(55);
//	leds_set(255, 60, 120);
//	leds_set(0, 0, 0, 0);
//	delay_ms(300);

//	if(radio_mode == radio_mode_ble)
//		config_ble_adv();
//	if(radio_mode == radio_mode_fast)
//	{
//		fr_init(32);
//		fr_listen();
//	}
//	if(radio_mode == radio_mode_fast64)
//	{
//		fr_init(64);
//		fr_listen();
//	}
	star_init(21, 1000, 2000, 0);

	leds_pulse(255, 255, 255, 0, 200);
	
	init_mcp3912(4, 3, 5, 6, 8);
	leds_pulse(0, 0, 0, 0, 10);
	if(!mcp3912_is_ok())
	{
		init_ok = 0;
		leds_pulse(255, 0, 0, 0, 1000);
		delay_ms(1200);
		leds_pulse(255, 0, 255, 0, 1000);
		delay_ms(1200);
	}
	bmi160_init(4, 3, 5, 1, 20);
	if(!bmi160_is_ok())
	{
		init_ok = 0;
		leds_pulse(255, 0, 0, 0, 1000);
		delay_ms(1200);
		leds_pulse(255, 255, 0, 0, 1000);
		delay_ms(1200);
	}
	if(NRF_GPIO->IN & 1<<pin_button)
	{
		leds_pulse(255, 0, 0, 0, 1000);
		delay_ms(1200);
		leds_pulse(0, 0, 255, 0, 1000);
		delay_ms(1200);
	}
	

//	leds_set(0, 255, 0);

	delay_ms(100);

	uint32_t last_sent_ms = 0;
	uint32_t btn_on = 0;
	uint32_t btn_off = 0;

	NRF_RNG->TASKS_START = 1;
	NRF_RNG->SHORTS = 0;
	NRF_RNG->CONFIG = 1;
	
	int ble_dt1 = 5;
	int ble_dt2 = 5;
	int send_iter = 0;
	int low_bat_cnt = 0;
	
	int bmi_skip_cnt = 0;
		
	if(init_ok)
	{
		for(int x = 0; x < 3; x++)
		{
			leds_pulse(255, 255, 0, 0, 200);
			delay_ms(200);
			leds_pulse(0, 0, 255, 255, 200);
			delay_ms(200);
		}
	}
	
	uint32_t unit_id = NRF_FICR->DEVICEID[1];
	uint32_t last_rx_packet_time = 0;
	
	star_set_id(unit_id);
	
	int btn_pressed = 0;
	
	imu_fusion_init();

	NRF_WDT->CRV = 1*32768; //1 second timeout
	NRF_WDT->TASKS_START = 1;

	while(1)
	{
		NRF_WDT->RR[0] = 0x6E524635; //reload watchdog
		star_loop_step();
//		low_power_cycle();
		uint32_t ms = millis();
		
		if(NRF_GPIO->IN & 1<<pin_button)
		{
			if(!btn_pressed)
			{
				btn_pressed = 1;
				btn_on = ms;
			}
			if(ms - btn_on > 25 && ms - btn_on < 1000)
				leds_pulse(0, 0, 255, 0, 30);
			if(ms - btn_on > 1000 && ms - btn_on < 5000)
				leds_pulse(0, 255, 0, 0, 30);
			if(ms - btn_on > 5000)
				leds_pulse(255, 255, 255, 255, 30);
		}
		else
		{
			if(btn_pressed)
			{
				btn_pressed = 0;
				btn_off = ms;
				leds_pulse(0, 0, 0, 0, 10);
				uint32_t btn_time = btn_off - btn_on;
				if(btn_time > 25) //ignore too short presses - noise
				{
					if(btn_time < 1000)
						process_btn_short();
					else if(btn_time < 5000)
						process_btn_long();
				}
//				mcp_set_filter_mode(radio_mode == radio_mode_ble);
			}
		}

/*		if(spectr_part2_pending && millis() - spectr_part2_time > 4)
		{
			for(int x = 0; x < 16; x++)
			{
				mcp_buffer[mcp_buf_pos++] = mcp_buf_sp2[x];
				if(mcp_buf_pos >= mcp_buf_len) mcp_buf_pos = 0;
			}
			spectr_part2_pending = 0;
			part2_packet = 1;
			if(unsent_cnt < 30) unsent_cnt++;
//			return 1;
		}*/
		
		if(mcp3912_read())
		{
			if(push_mcp_data())
			{
				mcp_cnt++;
//				data_id++;
				int mcp_cnt_led = (mcp_cnt>>6)%4;
//				leds_set(255*(mcp_cnt_led==0), 255*(mcp_cnt_led==1), 255*(mcp_cnt_led==2), 255*(mcp_cnt_led==3));
				
				if(unsent_cnt < 30) unsent_cnt++;
				read_battery();
/*				if(battery_mv < battery_low_threshold)
				{
					low_bat_cnt++;
					if(low_bat_cnt > 100)
						mode_lowbatt();
				}
				else
					low_bat_cnt = 0;*/
			}
//			if(!emg_mode)
			{
				imu_fusion_process();
//				bmi160_read();
				bmi_skip_cnt++;
				if(bmi_skip_cnt == 100)
				{
					bmi160_read_temp();
				}
				if(bmi_skip_cnt > 200)
				{
					bmi160_read_steps();
					bmi_skip_cnt = 0;
				}
			}
/*			if(0)if(fft_pending())
			{
				fft_skip_cnt++; 
				if(fft_skip_cnt > 0)
				{
//					sfft_butterfly();
					int cv = 10 + get_avg_sp();
					//if(cv > spectr_val && cv < 400) 
						spectr_val = cv;
					fft_skip_cnt = 0;
				}
			}*/
			
		}		

		if(radio_mode == radio_mode_fast64)
		{
			if(unsent_cnt > 0)
			{
				prepare_data_packet();
				star_queue_send(data_packet, data_packet[1]);
//				rf_send_and_listen(data_packet, data_packet[1]);
				last_sent_ms = ms;
				unsent_cnt = 0;
			}
		}
	}

}

