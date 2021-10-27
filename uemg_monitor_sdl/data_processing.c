#pragma GCC diagnostic error "-Wimplicit-function-declaration"

#include "data_processing.h"
#include "fft.h"
#include "quat_math.h"
#include "nn_dataset.h"
#include "nn_mlp.h"

float batt_level = 0;

float data_get_battery()
{
	return batt_level;
}

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
		batt_level = 2000 + data->batt*10 - 3200;
		batt_level /= 900;
		if(batt_level < 0) batt_level = 0;
		if(batt_level > 1) batt_level = 1;
		
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
	}
	return 1;
}


void uemg_data_get_spectrum(sUEMG_data_frame *data, float *spectr)
{
	for(int n = 0; n < 4; n++)
	{
		if(data->packet_type == 200)
		{
			for(int x = 0; x < 4; x++)
			{
				int vv = data->data_array[n*4+x];
				if(vv < 0) vv = 0xFFFF + vv;
				spectr[n*4+x] = vv;
			}
		}
		if(data->packet_type == 220)
		{
			float sp_buf[64];
			complex_f tmp[32];
			for(int n = 0; n < 4; n++)
			{
				for(int x = 0; x < 8; x++)
				{
					tmp[x].im = 0;
					tmp[x].re = data->data_array[n*8+x];
				}
				ffti_f(tmp, 3, 0);
				for(int x = 0; x < 4; x++)
					spectr[n*4+x] = sqrt(tmp[x].re*tmp[x].re + tmp[x].im*tmp[x].im);
			}
		}
	}	
}

float long_sp_avg[8] = {0,0,0,0,0,0,0,0};
float short_sp_avg[8] = {0,0,0,0,0,0,0,0};
float packet_data_buf[64];

void uemg_pack_spectrum(float *spect, float *packed_data)
{
	for(int N = 2; N >= 0; N--)
		for(int n = 0; n < 8; n++)
		{
			packed_data[(N+1)*8 + n] = packed_data[N*8+n];
			packet_data_buf[(N+1)*8 + n] = packet_data_buf[N*8+n];
		}
	for(int n = 0; n < 4; n++)
	{
		packed_data[n*2] = spect[n*4 + 2];
		packed_data[n*2+1] = spect[n*4 + 3];
	}
	for(int n = 0; n < 8; n++)
	{
		long_sp_avg[n] *= 0.95;
		long_sp_avg[n] += 0.05*packed_data[n];
		short_sp_avg[n] *= 0.9;
		short_sp_avg[n] += 0.1*packed_data[n];
		packed_data[n] = short_sp_avg[n];// / (1.0 + long_sp_avg[n]);
		packet_data_buf[n] = short_sp_avg[n];
//		printf("%g ", packed_data[n]);
	}
//	printf("\n");
}

void data_get_cur_data(float *data)
{
	for(int x = 0; x < 8; x ++)
		data[x] = packet_data_buf[x];
}
void data_get_prev_data(float *data, int T)
{
	int tt = T;
	if(tt > 3) tt = 3;
	if(tt < 0) tt = 0;
	for(int x = 0; x < 8; x ++)
		data[x] = packet_data_buf[x + tt*8];
}

int uemg_square_recognition(float *data)
{
	static int rec_hist[7] = {0,0,0,0,0,0,0}; //output median filter
	int med_len = 7;
	
	//class 1: top left corner (N = 0) going inside
	//class 2: bottom right corner (N = 2) going outside
	//class 3: 1+2 simultaneously,
	//class 4: square as a whole moves a lot
	
	float scale = 1.0 / 800.0;
	
	float scaled_vars[4];
	float rel_vars[4];
	float avg_val = 0.1; //some shift vs zero
	float ref_val = 0.2;
	for(int N = 0; N < 4; N++)
	{
		if(data[N*2] > data[N*2+1])
			scaled_vars[N] = scale*data[N*2];
		else 
			scaled_vars[N] = scale*data[N*2+1];
		avg_val += scaled_vars[N];
	}
	avg_val *= 0.25;
//	printf("SRA: %g\t %g\t %g\t %g\n", scaled_vars[0], scaled_vars[1], scaled_vars[2], scaled_vars[3]);
	for(int N = 0; N < 4; N++)
		rel_vars[N] = scaled_vars[N] / (ref_val + avg_val);
//	printf("SRR: %g\t %g\t %g\t %g\n", rel_vars[0], rel_vars[1], rel_vars[2], rel_vars[3]);
	
	float scores[5];
	scores[0] = 1;
	scores[1] = 4.0*rel_vars[0] - rel_vars[1] - rel_vars[2] - rel_vars[3];
	scores[2] = 4.0*rel_vars[3] - rel_vars[0] - rel_vars[1] - rel_vars[2];
	scores[3] = 1.5*(rel_vars[0] + rel_vars[2]) - rel_vars[1] - rel_vars[3];
	scores[4] = 3.0*avg_val;// - rel_vars[0] - rel_vars[2] - rel_vars[1] - rel_vars[3];
//	printf("SR: %g\t %g\t %g\t %g\n", scores[1], scores[2], scores[3], scores[4]);
	float smax = scores[0];
	int rec_res = 0;
	for(int N = 1; N < 5; N++)
	{
		if(scores[N] > smax)
		{
			smax = scores[N]; 
			rec_res = N;
		}
	}
	if(rec_res == 3 && avg_val < ref_val*1.5) rec_res = 0; //some artifact
	if(rec_res == 4 && avg_val < ref_val) rec_res = 0; //some artifact
	for(int x = 0; x < med_len-1; x++)
	{
		rec_hist[x] = rec_hist[x+1];
	}
	rec_hist[med_len-1] = rec_res;
	int class_cnt[5] = {0,0,0,0,0};
	for(int x = 0; x < med_len; x++)
		class_cnt[rec_hist[x]]++;
	int med_filt_rec = 0;
	int max_cls = 0;
	for(int x = 0; x < 5; x++)
		if(class_cnt[x] > max_cls)
		{
			max_cls = class_cnt[x];
			med_filt_rec = x;
		}
	if(max_cls < 4) med_filt_rec = 0;
	printf("CLASS: %d\n", med_filt_rec);
	return med_filt_rec;
}

int data_storage_inited = 0;
sNN_dataset ds;
sMLP mlp;
const int mlp_input_size = 8;
const int ml_class_cnt = 5;

int uemg_process_mlp(float *dat, int train_mode, int train_idx)
{
	if(!data_storage_inited)
	{
		data_storage_inited = 1;
		nn_dataset_init(&ds, mlp_input_size, ml_class_cnt, 1, 100);
		int layers[6] = {mlp_input_size, 15, 10, ml_class_cnt, 0, 0};
		mlp_init(&mlp, layers);
	}
	if(train_mode) nn_dataset_add(&ds, dat, NULL, train_idx, 0);
	mlp_calculate_output(&mlp, dat);
	return mlp_get_class(&mlp);
}

int train_prepared = 0;

void uemg_train_mlp(int train_length)
{
	if(!train_prepared)
	{
		train_prepared = 1;
		nn_dataset_shuffle_roles(&ds, 1, 0);
		mlp.learn_rate = 0.0005;
	}
	nn_dataset_train_MLP(&ds, &mlp, train_length);
}

void uemg_test_mlp()
{
	float class_err[ml_class_cnt];
	int cross_err[ml_class_cnt*ml_class_cnt];
	nn_dataset_test_MLP(&ds, &mlp, class_err, cross_err, 1);
	printf("class err:\n");
	for(int x = 0; x < ml_class_cnt; x++)
	{
		printf("%g ", class_err[x]);
	}
	printf("\nerr matrix:\n");
	for(int x = 0; x < ml_class_cnt; x++)
	{
		for(int x2 = 0; x2 < ml_class_cnt; x2++)
			printf("%d ", cross_err[x*ml_class_cnt + x2]);
		printf("\n");
	}
}

int uemg_process_ml(float *dat, int train_mode, int train_idx)
{
	return uemg_square_recognition(dat);
//	return uemg_process_mlp(dat, train_mode, train_idx);
}

void uemg_train_ml(int train_length)
{
	uemg_train_mlp(train_length);
}

void uemg_test_ml()
{
	uemg_test_mlp();
}

int uemg_combined_process(sUEMG_data_frame *data, int train_mode, int train_idx)
{
	float sp_raw[64];
	uemg_data_get_spectrum(data, sp_raw);
	float sp_packed[64];
	uemg_pack_spectrum(sp_raw, sp_packed);
	return uemg_process_ml(sp_packed, train_mode, train_idx);
}


void uemg_data_clear()
{
	nn_dataset_clear(&ds);
	mlp_clear(&mlp);
	train_prepared = 0;
	data_storage_inited = 0;
}
