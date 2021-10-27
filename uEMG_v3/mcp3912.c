#include "mcp3912.h"
#include "fft_opt.h"
#include "leds.h"
#include "fast_math.h"

MCP391x_GAIN_REG gain_reg;
MCP391x_STATUSCOM_REG statcom_reg;
MCP391x_CONFIG0_REG conf0_reg;
MCP391x_CONFIG1_REG conf1_reg;

void init_spi_mcp(uint8_t pin_CIPO, uint8_t pin_COPI, uint8_t pin_SCK)
{
	NRF_SPI0->PSELSCK = pin_SCK;
	NRF_SPI0->PSELMOSI = pin_COPI;
	NRF_SPI0->PSELMISO = pin_CIPO;
		
//	NRF_SPI0->INTEN = 0;
//	NRF_SPI0->FREQUENCY = 0x80000000; 
	NRF_SPI0->FREQUENCY = 0x80000000; 
	/* 0x02000000 125 kbps
	 * 0x04000000 250 kbps
	 * 0x08000000 500 kbps
	 * 0x10000000 1 Mbps
	 * 0x20000000 2 Mbps
	 * 0x40000000 4 Mbps
	 * 0x80000000 8 Mbps */
	
	NRF_SPI0->CONFIG = 0b000; // 0bxx0 - SPI mode 0b00x - polarity, 0 - MSB first
	NRF_SPI0->ENABLE = 1;
}

void mcp_write_reg8(uint8_t reg, uint8_t value)
{
	uint8_t tmp;
	while(NRF_SPI0->EVENTS_READY) tmp = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	NRF_SPI0->TXD = reg;
	while(!NRF_SPI0->EVENTS_READY) ;
	tmp = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_SPI0->TXD = value;
	while(!NRF_SPI0->EVENTS_READY) ;
	tmp = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
}

uint8_t mcp_read_reg8(uint8_t reg)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	NRF_SPI0->TXD = reg | (1<<7);
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;
	NRF_SPI0->TXD = 0;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}

void mcp_read_buf(uint8_t addr, uint8_t length, uint8_t *result)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	NRF_SPI0->TXD = addr | (1<<7);
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	for(int x = 0; x < length; x++)
	{
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		result[x] = NRF_SPI0->RXD;
		NRF_SPI0->EVENTS_READY = 0;
	}
	NRF_GPIO->OUTSET = mcp.CS;
}

void mcp_write_buf(uint8_t addr, uint8_t length, uint8_t *data)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;

	NRF_SPI0->TXD = addr;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	for(int x = 0; x < length; x++)
	{
		NRF_SPI0->TXD = data[x];
		while(!NRF_SPI0->EVENTS_READY) ;
		res = NRF_SPI0->RXD;
		NRF_SPI0->EVENTS_READY = 0;
	}
	NRF_GPIO->OUTSET = mcp.CS;
}

void mcp3912_write_reg(uint8_t reg, uint32_t value) //24-bit registers
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY)
	{
		res = NRF_SPI0->RXD; //clear pending
		NRF_SPI0->EVENTS_READY = 0;
	}
	
	NRF_GPIO->OUTCLR = mcp.CS;

	uint8_t treg = ((0b01)<<6) | (reg<<1) | 0;
	NRF_SPI0->EVENTS_READY = 0;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	uint8_t shift = 16;
	for(int x = 0; x < 3; x++)
	{
		NRF_SPI0->TXD = (value>>shift)&0xFF;
		shift -= 8;
		while(!NRF_SPI0->EVENTS_READY) ;
		res = NRF_SPI0->RXD;
		NRF_SPI0->EVENTS_READY = 0;
	}
	NRF_GPIO->OUTSET = mcp.CS;
}

uint8_t mcp3912_read_reg8(uint8_t reg)
{
	uint8_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (reg<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;
	NRF_SPI0->TXD = 0;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}

int16_t mcp3912_read_reg16(uint8_t reg)
{
	int16_t res = 0, rr;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_SPI0->EVENTS_READY = 0;
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (reg<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;

	NRF_SPI0->EVENTS_READY = 0;
	NRF_SPI0->TXD = 0;
	NRF_SPI0->TXD = 0;
	while(!NRF_SPI0->EVENTS_READY) ;
	NRF_SPI0->EVENTS_READY = 0;
	rr = NRF_SPI0->RXD;
	res = rr<<8;
	while(!NRF_SPI0->EVENTS_READY) ;
	rr = NRF_SPI0->RXD;
	res |= rr;

	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}

uint32_t mcp3912_read_reg24(uint8_t reg)
{
	uint32_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (reg<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	res = 0;
	for(int x = 0; x < 3; x++)
	{
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		res = res<<8;
		res |= NRF_SPI0->RXD;
	}
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}


uint32_t mcp3912_read_reg32(uint8_t reg)
{
	uint32_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (reg<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	res = 0;
	for(int x = 0; x < 4; x++)
	{
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		res = res<<8;
		res |= NRF_SPI0->RXD;
	}
	NRF_SPI0->EVENTS_READY = 0;

	NRF_GPIO->OUTSET = mcp.CS;
	return res;
}

void mcp3912_read_all16(int *res_buf)
{
	int16_t res = 0;
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (MCP3912_CH0<<1) | 1;
//	uint8_t treg = ((0b01)<<6) | (MCP3912_GAIN<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	for(int N = 0; N < 4; N++)
	{
		int16_t cur_val = 0;
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		cur_val = NRF_SPI0->RXD;
		cur_val = cur_val<<8;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		cur_val |= NRF_SPI0->RXD;
		res_buf[N] = cur_val;
	}

	NRF_GPIO->OUTSET = mcp.CS;
	return;
}

void mcp3912_read_all24(int *res_buf)
{
	int res = 0;
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (MCP3912_CH0<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	for(int N = 0; N < 4; N++)
	{
		int32_t cur_val = 0;
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = res;
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		cur_val = NRF_SPI0->RXD;
		cur_val = cur_val<<8;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		cur_val |= NRF_SPI0->RXD;
		cur_val = cur_val<<8;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		cur_val |= NRF_SPI0->RXD;
		cur_val = cur_val<<8;
		res_buf[N] = cur_val;
		res_buf[N] >>= 14;
	}

	NRF_GPIO->OUTSET = mcp.CS;
	return;
}

void mcp3912_read_all32(int16_t *res_buf)
{
	int16_t res = 0;
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (MCP3912_CH0<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	for(int N = 0; N < 4; N++)
	{
		int32_t cur_val = 0;
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = res;
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		cur_val = NRF_SPI0->RXD;
		cur_val = cur_val<<8;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		cur_val = NRF_SPI0->RXD;
		cur_val = cur_val<<8;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		cur_val |= NRF_SPI0->RXD;
		cur_val = cur_val<<8;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		cur_val |= NRF_SPI0->RXD;
		res_buf[N] = cur_val>>8;
	}

	NRF_GPIO->OUTSET = mcp.CS;
	return;
}

void mcp3912_continuous_start()
{
	int16_t res = 0;
	while(NRF_SPI0->EVENTS_READY) res = NRF_SPI0->RXD; //clear pending
	
	NRF_GPIO->OUTCLR = mcp.CS;
	uint8_t treg = ((0b01)<<6) | (MCP3912_CH0<<1) | 1;
	NRF_SPI0->TXD = treg;
	while(!NRF_SPI0->EVENTS_READY) ;
	res = NRF_SPI0->RXD;
	return;
}

void mcp3912_continuous_read16(int16_t *res_buf)
{
	for(int N = 0; N < 4; N++)
	{
		int16_t cur_val = 0;
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = 0;
		NRF_SPI0->TXD = 0;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		cur_val = NRF_SPI0->RXD;
		cur_val = cur_val<<8;
		while(!NRF_SPI0->EVENTS_READY) ;
		NRF_SPI0->EVENTS_READY = 0;
		cur_val |= NRF_SPI0->RXD;
		res_buf[N] = cur_val;
	}
	return;
}

void mcp3912_continuous_read24(int16_t *res_buf)
{
	for(int N = 0; N < 4; N++)
	{
		int32_t cur_val = 0;
		for(int x = 0; x < 3; x++)
		{
			NRF_SPI0->EVENTS_READY = 0;
			NRF_SPI0->TXD = 0;
			while(!NRF_SPI0->EVENTS_READY) ;
			cur_val = cur_val<<8;
			cur_val |= NRF_SPI0->RXD;
		}
		res_buf[N] = cur_val>>8;
	}
	return;
}

void start_mcp_clock()
{
	NRF_TIMER1->MODE = 0;
	NRF_TIMER1->BITMODE = 1;
	NRF_TIMER1->PRESCALER = 0b0;
	NRF_TIMER1->CC[0] = 0b01;
	NRF_TIMER1->CC[1] = 0b01;
//	NRF_TIMER1->CC[0] = 4;
//	NRF_TIMER1->CC[1] = 4;
	NRF_TIMER1->SHORTS = 0b10; //clear on compare 1

	NRF_GPIO->DIRSET = 1<<7;
	NRF_GPIOTE->CONFIG[1] = (1<<20) | (0b11 << 16) | (7 << 8) | 0b11; //toggle PIN_NUM<<8 (0) TASK

	NRF_PPI->CHENSET = 1<<5;
	NRF_PPI->CHG[0] |= 1<<5;
	NRF_PPI->CH[5].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];
	NRF_PPI->CH[5].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

	NRF_TIMER1->TASKS_START = 1;
}

void stop_mcp_clock()
{
	NRF_TIMER1->TASKS_STOP = 1;
}


int use_fft = 1;
int fft_size = 8;
float fft_buf1_r[128];
float fft_buf1_i[128];
float fft_buf2_r[128];
float fft_buf2_i[128];
float fft_spectr[128];
float emg_raw[32];
volatile float *fft_buf_r = fft_buf1_r;
volatile float *fft_buf_i = fft_buf1_i;
volatile float *fft_stored_r = fft_buf2_r;
volatile float *fft_stored_i = fft_buf2_i;
volatile int cur_fft_buf = 0;
volatile int cur_fft_pos = 0;
volatile int fft_data_ready = 0;

float *get_cur_fft_buf(int real_im, int channel) //0 real 1 im
{
	if(cur_fft_buf == 0)
	{
		if(real_im == 0) return fft_buf1_r + channel*fft_size;
		else return fft_buf1_i + channel*fft_size;
	} 
	else
	{
		if(real_im == 0) return fft_buf2_r + channel*fft_size;
		else return fft_buf2_i + channel*fft_size;
	}
}

float sin50_w[4] = {0,0,0,0};
float cos50_w[4] = {0,0,0,0};
float sin60_w[4] = {0,0,0,0};
float cos60_w[4] = {0,0,0,0};

float tt = 0;
float tt60 = 0;
float avg_m = 0.995;
float av = 0;

int cur_data[4];

volatile uint8_t mcp_has_new_data = 0;

float filtered_values[4] = {0, 0, 0, 0};
float avg_vals[4] = {0, 0, 0, 0};

float M_PI = 3.1415926;

void mcp_filter_data()
{
	for(int c = 0; c < 4; c++)
		filtered_values[c] = cur_data[c];

if(1)
{
	float sf50 = sin_f(2*M_PI*tt);
	float cf50 = cos_f(2*M_PI*tt);
	float sf60 = sin_f(2*M_PI*tt60);
	float cf60 = cos_f(2*M_PI*tt60);
	
//	tt += 0.204918*1.0;
	tt += 0.0512295 * 2.0; //1 for 976 hz, 2 for 488, 4 for 244
	tt60 += 0.06147541 * 2.0;
	
	if(tt > 0.5) tt -= 1;
	if(tt60 > 0.5) tt60 -= 1;

	for(int c = 0; c < 4; c++)
	{
		float vv = cur_data[c];
		sin50_w[c] *= avg_m;
		cos50_w[c] *= avg_m;
		sin50_w[c] += vv * sf50 * (1.0 - avg_m);// + 10*tt*(c==3)+tt*(c==2);
		cos50_w[c] += vv * cf50 * (1.0 - avg_m);

		sin60_w[c] *= avg_m;
		cos60_w[c] *= avg_m;
		sin60_w[c] += vv * sf60 * (1.0 - avg_m);
		cos60_w[c] += vv * cf60 * (1.0 - avg_m);

		float filt_v;
		if(sin50_w[c]*sin50_w[c] + cos50_w[c]*cos50_w[c] > sin60_w[c]*sin60_w[c] + cos60_w[c]*cos60_w[c])
		{
			filt_v = vv - 2.0*(sin50_w[c] * sf50 + cos50_w[c] * cf50);
		}
		else
		{
			filt_v = vv - 2.0*(sin60_w[c] * sf60 + cos60_w[c] * cf60);
		}

		avg_vals[c] *= 0.99;
		avg_vals[c] += 0.01*filt_v;

		filtered_values[c] = filt_v;// - avg_vals[c];
//		filtered_values[c] = vv;
	}
}
	
	if(use_fft)
	{
//		if(cur_fft_pos < fft_size)
		for(int c = 0; c < 4; c++)
		{
			fft_buf_r[cur_fft_pos + c*fft_size] = filtered_values[c];
			fft_buf_i[cur_fft_pos + c*fft_size] = 0;
		}
		cur_fft_pos++;
		if(cur_fft_pos >= fft_size)
		{
			cur_fft_pos = 0;
			fft_data_ready = 1;
			if(cur_fft_buf == 0)
			{
				cur_fft_buf = 1;
				fft_buf_r = fft_buf2_r;
				fft_buf_i = fft_buf2_i;
				fft_stored_r = fft_buf1_r;
				fft_stored_i = fft_buf1_i;
			}
			else
			{
				cur_fft_buf = 0;
				fft_buf_r = fft_buf1_r;
				fft_buf_i = fft_buf1_i;
				fft_stored_r = fft_buf2_r;
				fft_stored_i = fft_buf2_i;
			}
		}
	}
	return;
}

#define FPU_EXCEPTION_MASK               0x0000009F                      //!< FPU exception mask used to clear exceptions in FPSCR register.
#define FPU_FPSCR_REG_STACK_OFF          0x40                            //!< Offset of FPSCR register stacked during interrupt handling in FPU part stack.

void FPU_IRQHandler(void)
{
    // Prepare pointer to stack address with pushed FPSCR register.
    uint32_t * fpscr = (uint32_t * )(FPU->FPCAR + FPU_FPSCR_REG_STACK_OFF);
    // Execute FPU instruction to activate lazy stacking.
    (void)__get_FPSCR();
    // Clear flags in stacked FPSCR register.
    *fpscr = *fpscr & ~(FPU_EXCEPTION_MASK);
}

float prev_emg_raw[32];

int mcp_fft_process()
{
	if(!fft_data_ready) return 0;
	fft_data_ready = 0;
	for(int c = 0; c < 4; c++)
		fft8_real((float*)fft_stored_r+fft_size*c, (float*)fft_stored_r+64+fft_size*c, (float*)fft_stored_i+64+fft_size*c);

	for(int c = 0; c < 4; c++)
		for(int x = 0; x < 8; x++)
			emg_raw[c*8+x] = fft_stored_r[fft_size*c+x];
	
	for(int c = 0; c < 4; c++)
		for(int x = 0; x < 4; x++)
	{
		float fr = fft_stored_r[64+fft_size*c+x];
		float fi = fft_stored_i[64+fft_size*c+x];
		float vv = fr*fr + fi*fi;
		fr = fft_stored_r[64+fft_size*(c+1)-1-x];
		fi = fft_stored_i[64+fft_size*(c+1)-1-x];
		vv += fr*fr + fi*fi;
		fft_spectr[x+fft_size*c] = sqrt_f(vv);
	}
/*	for(int c = 0; c < 4; c++)
	{
		for(int x = 0; x < 4; x++)
		{
			fft_stored_r[fft_size*c + x] = prev_emg_raw[fft_size*c + x*2];
			fft_stored_r[fft_size*c + 4+x] = emg_raw[fft_size*c + x*2];
		}
		for(int x = 0; x < fft_size; x++)
			prev_emg_raw[fft_size*c + x] = emg_raw[fft_size*c + x];
		fft8_real((float*)fft_stored_r+fft_size*c, (float*)fft_stored_r+64+fft_size*c, (float*)fft_stored_i+64+fft_size*c);

		for(int x = 0; x < 4; x++)
		{
			float fr = fft_stored_r[64+fft_size*c+x];
			float fi = fft_stored_i[64+fft_size*c+x];
			float vv = fr*fr + fi*fi;
			fr = fft_stored_r[64+fft_size*(c+1)-1-x];
			fi = fft_stored_i[64+fft_size*(c+1)-1-x];
			vv += fr*fr + fi*fi;
			fft_spectr[x+4+fft_size*c] = sqrt_f(vv);
		}
	}*/

	return 1;
}
float *mcp_fft_get_spectr()
{
	return fft_spectr;
}
float *mcp_fft_get_raw()
{
	return emg_raw;
}

void mcp_fft_mode(int onoff)
{
    NVIC_SetPriority(FPU_IRQn, 5);
    NVIC_ClearPendingIRQ(FPU_IRQn);
    NVIC_EnableIRQ(FPU_IRQn);	
	use_fft = onoff;
}

//uint8_t mcp_has_new_data = 0;
volatile uint8_t in_mcp_processing = 0;

int ch_to_read = 0;

void GPIOTE_IRQHandler(void)
{
	if (NRF_GPIOTE->EVENTS_IN[0] == 1)
	{
		NRF_GPIOTE->EVENTS_IN[0] = 0;
		if(in_mcp_processing) return;
//		uint8_t sc_state1 = mcp3912_read_reg24(MCP3912_STATCOM)&0x0F;
		in_mcp_processing = 1;
//		mcp3912_continuous_read16(cur_data);
//		mcp3912_continuous_read24(cur_data);
//		mcp3912_read_all24(cur_data);
		mcp3912_read_all16(cur_data);
//		uint8_t sc_state2 = mcp3912_read_reg24(MCP3912_STATCOM)&0x0F;
//		cur_data[0] = sc_state2;
//		cur_data[1] = sc_state1;
		
//		mcp3912_read_all32(cur_data);
//		cur_data[3] = mcp3912_read_reg16(MCP3912_CH3);
//		cur_data[2] = mcp3912_read_reg16(MCP3912_CH2);
//		cur_data[1] = mcp3912_read_reg16(MCP3912_CH1);
//		cur_data[0] = mcp3912_read_reg16(MCP3912_CH0);
//		cur_data[0] = 123;//mcp3912_read_reg16(MCP3912_STATCOM);
//		cur_data[1] = mcp3912_read_reg16(MCP3912_GAIN);
//		cur_data[2] = mcp3912_read_reg16(MCP3912_CONFIG0);
//		cur_data[3] = mcp3912_read_reg16(MCP3912_CONFIG1);
//		cur_data[0] = mcp3912_read_reg24(MCP3912_CH0);
//		cur_data[1] = mcp3912_read_reg24(MCP3912_CH1);
//		cur_data[2] = mcp3912_read_reg24(MCP3912_CH2);
//		cur_data[3] = mcp3912_read_reg24(MCP3912_CH3);

//		cur_data[3] = mcp3912_read_reg32(MCP3912_CH3)>>8;
//		cur_data[2] = mcp3912_read_reg32(MCP3912_CH2)>>8;
//		cur_data[1] = mcp3912_read_reg32(MCP3912_CH1)>>8;
//		cur_data[0] = mcp3912_read_reg32(MCP3912_CH0)>>8;

		mcp_filter_data();
		mcp_has_new_data = 1;
		in_mcp_processing = 0;
	}
}

int init_ok = 0;

void init_mcp3912(uint8_t pin_CIPO, uint8_t pin_COPI, uint8_t pin_SCK, uint8_t pin_CS, uint8_t pin_INT)
{
/*	nrf_gpio_cfg_output(pin_MOSI);
	nrf_gpio_cfg_output(pin_SCK);
	nrf_gpio_cfg_input(pin_MISO, NRF_GPIO_PIN_NOPULL);*/
	nrf_gpio_cfg_output(pin_CS); 
	nrf_gpio_cfg_input(pin_INT, NRF_GPIO_PIN_NOPULL);
	mcp.CS = 1<<pin_CS;
	NRF_GPIO->OUTSET = mcp.CS;

	init_spi_mcp(pin_CIPO, pin_COPI, pin_SCK);
		
	gain_reg.wholeRegister = 0;
//	gain_reg.PGA_CH0 = 0;
//	gain_reg.PGA_CH1 = 0;
//	gain_reg.PGA_CH2 = 0;
//	gain_reg.PGA_CH3 = 0;

	gain_reg.PGA_CH0 = 0b101;
	gain_reg.PGA_CH1 = 0b101;
	gain_reg.PGA_CH2 = 0b101;
	gain_reg.PGA_CH3 = 0b101;

	conf0_reg.wholeRegister = 0;
	conf0_reg.BOOST = 0b10;
	conf0_reg.DITHER = 0b00;
	conf0_reg.EN_GAINCAL = 0;
	conf0_reg.EN_OFFCAL = 0;
//	conf0_reg.OSR = 0b101;
//	conf0_reg.PRE = 0b10; // 0b11 = 8, 0b10 = 4, 0b01 = 2, 0 = 1
	conf0_reg.OSR = 0b110;
	conf0_reg.PRE = 0b01; // 0b11 = 8, 0b10 = 4, 0b01 = 2, 0 = 1
	conf0_reg.VREFCAL = 80;

	conf1_reg.wholeRegister = 0;
	conf1_reg.CLKEXT = 1; //This should be set to zero for oscillator
	conf1_reg.RESET = 0b0000;
	conf1_reg.SHUTDOWN = 0b0000;
	conf1_reg.VREFEXT = 0;


	statcom_reg.wholeRegister = 0;
//	statcom_reg.WIDTH_DATA = 0b10; //32 bits
//	statcom_reg.WIDTH_DATA = 0b11; //32 bits shifted
	statcom_reg.WIDTH_DATA = 0b00; //16 bits
//	statcom_reg.WIDTH_DATA = 0b01; //24 bits
	statcom_reg.DR_HIZ = 1;
	statcom_reg.DR_LINK = 1;
	statcom_reg.EN_CRCCOM = 0;
	statcom_reg.EN_INT = 0;
	statcom_reg.READ = 0b10;
	statcom_reg.WIDTH_CRC = 0;
	statcom_reg.WRITE = 0;

	mcp3912_write_reg(MCP3912_GAIN, gain_reg.wholeRegister);
	mcp3912_write_reg(MCP3912_STATCOM, statcom_reg.wholeRegister);
	mcp3912_write_reg(MCP3912_CONFIG0, conf0_reg.wholeRegister);
	mcp3912_write_reg(MCP3912_CONFIG1, conf1_reg.wholeRegister);

	uint32_t conf_reg_val = mcp3912_read_reg24(MCP3912_CONFIG0);
	if(conf_reg_val == conf0_reg.wholeRegister)
	{
		init_ok = 1;
	}
	
	mcp_fft_mode(1);
//	mcp3912_continuous_start();
	start_mcp_clock();

	NRF_GPIOTE->CONFIG[0] = (0b10 << 16) | (pin_INT << 8) | 0b01; //HiToLow<<16 | PIN_NUM<<8 | EVENT 
	NRF_GPIOTE->INTENSET = 0b001; //interrupt channel 0
	NVIC_EnableIRQ(GPIOTE_IRQn);
	
	return;
}

int *mcp3912_get_data_buf()
{
	return cur_data;
}

uint8_t mcp3912_read()
{
	if(mcp_has_new_data)
	{
		mcp_has_new_data = 0;
		return 1;
	}
	return 0;
}

int mcp3912_is_ok()
{
	return init_ok;
}