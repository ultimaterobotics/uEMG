//#define WIN_BUILD

#ifdef WIN_BUILD
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "device_serial.h"
#include "device_functions.h"
#include "quat_math.h"

uint32_t uemg_data_id = 0;
int prev_data_id = -1;
uint8_t prev_pack_id = 0;

uint8_t *response_buf;
int response_pos = 0;
int response_buf_len = 4096;
int response_inited = 0;

uint8_t uart_prefix[2] = {79, 213};
uint8_t uart_suffix[2] = {76, 203};

sUEMG_data_frame uemg_dat;
int uemg_batt_level = 0;


int uemg_packet_parse(sUEMG_data_frame *data, uint8_t *pack)
{
	int pp = 0;
	data->unit_id = pack[pp++];
	for(int x = 0; x < 3; x++)
	{
		data->unit_id <<= 8; 
		data->unit_id += pack[pp++];
	}
	data->packet_type = pack[pp++];
	if(data->packet_type > 220)
	{
		data->data_count = data->packet_type - 220;
		data->packet_type = 220;
	}
	else if(data->packet_type > 200)
	{
		data->data_count = data->packet_type - 200;
		data->packet_type = 200;
	}
	else if(data->packet_type > 100)
	{
		data->data_count = data->packet_type - 200;
		data->packet_type = 100;
	}
	else return 0; //unknonwn packet type
	
	int param_id = pack[pp++];
	int pb1 = pack[pp++];
	int pb2 = pack[pp++];
	int pb3 = pack[pp++];
	data->batt = -1;
	data->steps = -1;
	data->version = -1;
	if(param_id == param_batt)
	{
		data->batt = pb1;
		data->version = pb2;
		uemg_batt_level = 2000 + data->batt*10 - 3200;
		uemg_batt_level /= 900;
		if(uemg_batt_level < 0) uemg_batt_level = 0;
		if(uemg_batt_level > 1) uemg_batt_level = 1;
	}
	if(param_id == param_imu_steps)
	{
		data->steps = (pb1<<8) | pb2;
	}
	data->data_id = pack[pp++];
	for(int n = 0; n < data->data_count; n++)
	{
		data->data_array[n] = (pack[pp]<<8) | pack[pp+1];
		pp += 2;
	}
//	if(data->packet_type == 200)
	{
		data->ax = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		data->ay = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		data->az = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		data->wx = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		data->wy = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		data->wz = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		int16_t qxr, qyr, qzr, qwr;
		qxr = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		qyr = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		qzr = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		qwr = (pack[pp]<<8) | pack[pp+1]; pp += 2;
		data->q_x = qxr;
		data->q_y = qyr;
		data->q_z = qzr;
		data->q_w = qwr;
		data->q_x /= 30000.0f;
		data->q_y /= 30000.0f;
		data->q_z /= 30000.0f;
		data->q_w /= 30000.0f;
		sQ Qsg;
		Qsg.x = data->q_x;
		Qsg.y = data->q_y;
		Qsg.z = data->q_z;
		Qsg.w = data->q_w;
		sV acc;
		acc.x = data->ax;
		acc.y = data->ay;
		acc.z = data->az;
		rotate_v(&Qsg, &acc);
//		printf("Qw %g A: %d %d %d Ar %g %g %g\n", data->q_w, data->ax, data->ay, data->az, acc.x, acc.y, acc.z);
//		data->wx = (pack[pp]<<8) | pack[pp+1]; pp += 2;
//		data->wy = (pack[pp]<<8) | pack[pp+1]; pp += 2;
//		data->wz = (pack[pp]<<8) | pack[pp+1]; pp += 2;
	}
	return 1;
}

void win_mouse_move(int mm_x, int mm_y)
{
	INPUT inp;
	inp.type = INPUT_MOUSE;
	inp.mi.dx = mm_x;
	inp.mi.dy = mm_y;
	inp.mi.mouseData = 0;
	inp.mi.dwFlags = MOUSEEVENTF_MOVE;
	inp.mi.time = 0;
	inp.mi.dwExtraInfo = 0;
	
	SendInput(1, &inp, sizeof(INPUT));
}
void win_mouse_press()
{
	INPUT inp;
	inp.type = INPUT_MOUSE;
	inp.mi.dx = 0;
	inp.mi.dy = 0;
	inp.mi.mouseData = 0;
	inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	inp.mi.time = 0;
	inp.mi.dwExtraInfo = 0;
	
	SendInput(1, &inp, sizeof(INPUT));
}
void win_mouse_release()
{
	INPUT inp;
	inp.type = INPUT_MOUSE;
	inp.mi.dx = 0;
	inp.mi.dy = 0;
	inp.mi.mouseData = 0;
	inp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	inp.mi.time = 0;
	inp.mi.dwExtraInfo = 0;
	
	SendInput(1, &inp, sizeof(INPUT));
}

float avg_spvs[4] = {10000,10000,10000,10000};

void process_imu(sUEMG_data_frame *data)
{
	sV acc;
	acc.x = data->ax;
	acc.y = data->ay;
	acc.z = data->az;
	sQ Qsg;
	Qsg.x = data->q_x;
	Qsg.y = data->q_y;
	Qsg.z = data->q_z;
	Qsg.w = data->q_w;
	sV gyro;
	gyro.x = data->wx;
	gyro.y = data->wy;
	gyro.z = data->wz;
	rotate_v(&Qsg, &gyro);
	
	rotate_v(&Qsg, &acc);
/*	world_speed.x += acc.x * 0.001;
	world_speed.y += acc.y * 0.001;
	world_speed.z += (acc.z - 8000.0) * 0.001;
	world_speed.x *= 0.999;
	world_speed.y *= 0.999;
	world_speed.z *= 0.999;*/
	
	float acc_ampl = sqrt(data->ax*data->ax + data->ay*data->ay + data->az*data->az);
	
	float dy = -data->wy;// gyro.y;
	float dz = -data->wz;// gyro.z;
	float sens = 0.15;
	int mm_x = -sens*dz;
	int mm_y = -sens*dy;
	
//	float dy = gyro.y;
//	float dz = gyro.z;
//	float sens = 0.03;
//	int mm_x = -sens*dz;
//	int mm_y = -sens*dy;
	

	float spvs[4];
	int need_click = 1;
	for(int n = 0; n < 4; n++)
	{
		spvs[n] = 0;
		for(int x = 2; x < 4; x++)
		{
			int vv = data->data_array[n*4+x];	
			spvs[n] += vv;
		}	
		if(spvs[n] < avg_spvs[n]*1.8) need_click = 0;
		avg_spvs[n] *= 0.99;
		avg_spvs[n] += 0.01*spvs[n];
	}
	
	static int in_click = 0;
	if(need_click && !in_click)
	{
		win_mouse_press();
		in_click = 3;
	}
	if(in_click && !need_click)
	{
		in_click--;
		if(in_click == 0) win_mouse_release();
	}
	if(mm_x || mm_y) win_mouse_move(mm_x, mm_y);
}

void device_parse_response(uint8_t *buf, int len)
//we should be prepared that bytestream has missed bytes - it happens too often to ignore,
//so each data transfer from base starts with 2 fixed prefix bytes. But it could happen that
//actual data contents occasionally matches these prefix bytes, so we can't just treat them
//as guaranteed packet start - so we assume it _could_ be a packet start and try to parse
//the result, and see if it makes sense
{
	if(!response_inited) //init buffer for storing bytestream data ("response")
	{
		response_inited = 1;
		response_buf = (uint8_t*)malloc(response_buf_len);
		response_pos = 0;
//		gettimeofday(&tm_zero, NULL);
		uemg_dat.data_id = -1;
	}
	//===========
	//buffer for storing bytestream, if buffer is overfilled - older half of the buffer is dropped
	if(len > response_buf_len/2) len = response_buf_len/2;
	if(response_pos > response_buf_len/2)
	{
		int dl = response_buf_len/4;
		memcpy(response_buf, response_buf+response_pos-dl, dl);
		response_pos = dl;
	}
	memcpy(response_buf+response_pos, buf, len);
	response_pos += len;
	//======= at this point, response_buf contains most recent unprocessed data, starting from 0
	int processed_pos = 0;
	for(int x = 0; x < response_pos-25; x++) //25 - always less than minimum valid packet length
	{
		if(response_buf[x] == uart_prefix[0] && response_buf[x+1] == uart_prefix[1])
		{//we detected possible start of the packet, trying to make sense of it
			uint8_t rssi_level = response_buf[x+2];
			uint8_t *pack = response_buf + x + 3;
			
			uint8_t message_length = pack[1];
			uint8_t packet_id = pack[0];

			if(x + 3 + message_length >= response_pos)
				break;

			processed_pos = x + message_length;
			
			if(packet_id == prev_pack_id)
			{
				printf("duplicate pack %d\n", packet_id);
				for(int x = 0; x < 16; x++)
					printf("%d ", pack[x]);
				printf("\n");
				continue;
			}
			
			int dpack = packet_id - prev_pack_id;
			prev_pack_id = packet_id;

			prev_data_id = uemg_dat.data_id;
			if(!uemg_packet_parse(&uemg_dat, pack+2)) continue;
			if(prev_data_id < 0) continue;
			if(uemg_dat.data_id < prev_data_id)
				uemg_data_id += 256;

			process_imu(&uemg_dat);
//			data_process_step(&uemg_dat);
		}
	}
	memcpy(response_buf, response_buf+processed_pos, response_pos-processed_pos);
	response_pos -= processed_pos;
}

int WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
)
{
	while(1)
	{
		while(!is_connected())
		{
			Sleep(1);
			device_open_port();
		}
		uint8_t buf[256];
		int data_len = device_poll(buf, 256);
		if(data_len > 0)
		{
			device_parse_response(buf, data_len);
		}
	}
	device_close_port();
	return 0;
}

#endif