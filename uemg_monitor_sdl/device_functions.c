#pragma GCC diagnostic error "-Wimplicit-function-declaration"

#include "device_functions.h"
#include "serial_functions.h"
#include "drawing.h"

#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */

#include "fft.h"
#include "quat_math.h"

#include "nn_dataset.h"
#include "nn_mlp.h"
#include "save_processing.h"
#include "nn_adaptive_map.h"

#include "processing_kmeans.h"

//#include "data_processing.h"

#include "pca_processor.h"

#include "keyb_mouse_emu.h"

uint8_t *response_buf;
int response_pos = 0;
int response_buf_len = 4096;
int response_inited = 0;

sUEMG_saved_frame uemg_save_frame;

int save_file = -1;
int emulate_file = -1;
int emulate_on = 0;

uint8_t uart_prefix[2] = {79, 213};
uint8_t uart_suffix[2] = {76, 203};

sUEMG_data_frame uemg_dat;

float uemg_batt_level = 0.5;
float device_get_battery()
{
	return uemg_batt_level;
}

void device_save_on()
{
	if(save_file > 0) return;
	save_file = open("uemg_data.dat", O_WRONLY | O_CREAT, 0b110110110);
}

void device_save_off()
{
	if(save_file > 0)
	{
		close(save_file);
		save_file = -1;
	}	
}

void device_emulate_start(char *file_name)
{
	emulate_file = open(file_name, O_RDONLY);
	emulate_on = 1;
}
void device_emulate_end()
{
	if(emulate_file > 0)
		close(emulate_file);
	emulate_on = 0;
}

int record_to_dataset = 0;
sNN_dataset ds;
sMLP mlp;

void device_dataset_start()
{
	nn_dataset_init(&ds, 16, 10, 1, 100);
	int nn_layers[4] = {16, 30, 10, 0};
	mlp_init(&mlp, nn_layers);
	record_to_dataset = 1;
}

void device_mlp_train_step(int cycles)
{
	if(record_to_dataset)
	{
		record_to_dataset = 0;
		nn_dataset_shuffle_roles(&ds, 0.5, 0.5);
		for(int n = 0; n < 10000; n++)
		{
			sc_addV(acc_ch+0, 0);
			sc_addV(acc_ch+1, 0);
			sc_addV(acc_ch+2, 0);
			sc_addV(gyro_ch+0, 0);
			sc_addV(gyro_ch+1, 0);
			sc_addV(gyro_ch+2, 0);
		}
	}
	nn_dataset_train_MLP(&ds, &mlp, cycles);
	float cl_error[32];
	nn_dataset_test_MLP(&ds, &mlp, cl_error, NULL, 0);
	float tot_err = 1;
	float avg_err = 0;
	for(int c = 0; c < 10; c++)
	{
		tot_err *= (1.0 - cl_error[c]);
		avg_err += cl_error[c];
		printf("%g ", cl_error[c]);
	}
	printf("   W: %g\n", mlp_get_average_weight(&mlp));
	avg_err /= 10.0;
	sc_addV(acc_ch+0, tot_err);
	sc_addV(acc_ch+1, avg_err);
}

void device_save_mark(int mark)
{
	uemg_save_frame.mark = mark;
}

int device_get_mark()
{
	return uemg_save_frame.mark;
}

int prev_pack_id = 0;
float muscle_active[4] = {0,0,0,0};
float muscle_active_avg[4] = {1,1,1,1};
float adaptive_map_states[64];
float adaptive_map_data[128];
int adaptive_map_classes = 40;
int adaptive_map_size = 32;
int calc_fft_len = 32;
int raw_vals[4*32];
float prev_frame_sp[16];

sV world_pos = {0,0,0};
sV world_speed = {0,0,0};

int dc_inited = 0;
sDCdata dc_data;

int device_get_cur_class_res()
{
	return dc_data.res_id;
}


void dumb_classifier_init()
{
	dc_inited = 1;
	for(int c = 0; c < 4; c++)
		dc_data.avg_lvls[c] = 10000;
	for(int x = 0; x < 128; x++)
		dc_data.res_map[x] = 0;
}

int dumb_classifier(float *spectrs, int splen)
{
	if(!dc_inited) dumb_classifier_init();
	float rel_vals[4];
	int rel_val_idx[4];
	for(int c = 0; c < 4; c++)
	{
		float cur_val = 0;
		for(int x = 0; x < splen; x++)
			cur_val += spectrs[c*splen + x] * (0.05f + 0.15f*(float)x);
		if(cur_val < dc_data.avg_lvls[c])
		{
			dc_data.avg_lvls[c] *= 0.98;
			dc_data.avg_lvls[c] += 0.02*cur_val;
		}
		else
		{
			dc_data.avg_lvls[c] *= 0.999;
			dc_data.avg_lvls[c] += 0.001*cur_val;
		}
		rel_vals[c] = cur_val / (0.1f + dc_data.avg_lvls[c]);
		rel_val_idx[c] = c;
	}
	
	int res_id = 0;
//	printf("DC: %g %g %g %g\n", rel_vals[0], rel_vals[1], rel_vals[2], rel_vals[3]);
	if(rel_vals[0] + rel_vals[2] > 10 && rel_vals[1] + rel_vals[3] < 7) res_id = 1;
	else if(rel_vals[0] + rel_vals[2] < 7 && rel_vals[1] + rel_vals[3] > 10) res_id = 2;
	else if(rel_vals[0] + rel_vals[1] + rel_vals[2] + rel_vals[3] > 15) res_id = 3;
/*	
	for(int c = 0; c < 4; c++)
		for(int c1 = c+1; c1 < 4; c1++)
		{
			if(rel_vals[c1] > rel_vals[c])
			{
				float t = rel_vals[c1];
				int ti = rel_val_idx[c1];
				rel_vals[c1] = rel_vals[c];
				rel_val_idx[c1] = rel_val_idx[c];
				rel_vals[c] = t;
				rel_val_idx[c] = ti;
			}
		}
	int comb_id = 0;
	int res_id = 0;
	for(int c0 = 0; c0 < 4; c0++)
		for(int c1 = 0; c1 < 4; c1++)
		{
			if(c1 == c0) continue;
			for(int c2 = 0; c2 < 4; c2++)
			{
				if(c2 == c1 || c2 == c0) continue;
				if(c0 == rel_val_idx[0] && c1 == rel_val_idx[1] && c2 == rel_val_idx[3]) res_id = comb_id;
				comb_id++;
			}
		}
	*/
	dc_data.res_id = res_id;//rel_val_idx[0] * 16 + rel_val_idx[1] * 4 + rel_val_idx[2];
	for(int x = 0; x < 128; x++)
		dc_data.res_map[x] *= 0.98;
	dc_data.res_map[res_id] += 0.5;
	
	return dc_data.res_id;
}

int in_game_mode = 0;
int game_mode_time = 0;
static struct timeval game_start_tm;

int train_gesture_count[5];
sMLP game_mlp;
sNN_dataset game_dataset;
int game_train_cnt = 0;
int game_cur_gesture = 0;
int game_images = 24;
int gest_queue[64];
int game_completed = 0;
int pca_nn_size = 10;

float ml_data_array[512]; //larger than required
float ml_data_tmp[512];
int ml_sp_size = 4; //spectral bins
int ml_input_size = 4*4*4+3;


void device_game_mode_switch()
{
	in_game_mode = !in_game_mode;
	if(in_game_mode)
	{
		if(game_completed)
			uemg_data_clear();

		gettimeofday(&game_start_tm, NULL);
		int g_time = 500 + (rand()%100);
		for(int g = 0; g < game_images; g++)
		{
			gest_queue[g] = g_time&0xFFFFFF;
			static int gst = 1;
			gest_queue[g] |= gst<<24;//(1 + (rand()%4000)/1000)<<24;
			gst++; if(gst > 4) gst = 1;
			g_time += 140 + (rand()%80);
		}
		draw_game_mode_fill(gest_queue, game_images);
		draw_game_mode_update(0, 0); 
	}
	draw_game_mode_select(in_game_mode);	
}

void device_game_mode_switch_old()
{
	in_game_mode = !in_game_mode;
	if(in_game_mode)
	{
		if(game_completed)
		{
			mlp_clear(&game_mlp);
			nn_dataset_clear(&game_dataset);
		}
		gettimeofday(&game_start_tm, NULL);
		int g_time = 500 + (rand()%100);
		for(int g = 0; g < game_images; g++)
		{
			gest_queue[g] = g_time&0xFFFFFF;
			static int gst = 1;
			gest_queue[g] |= gst<<24;//(1 + (rand()%4000)/1000)<<24;
			gst++; if(gst > 4) gst = 1;
			g_time += 140 + (rand()%80);
		}
		draw_game_mode_fill(gest_queue, game_images);
		draw_game_mode_update(0, 0); 
		for(int x = 0; x < 5; x++)
			train_gesture_count[x] = 0;

		int layers[5] = {35, 20, 15, 5, 0};
		layers[0] = pca_nn_size;
		mlp_init(&game_mlp, layers);
		game_train_cnt = 0;
		nn_dataset_init(&game_dataset, ml_input_size, 5, 1, 1000);
	}
	draw_game_mode_select(in_game_mode);
}

sPCA_data pca;

void pack_data_for_ML(sUEMG_data_frame *data)
{
	float cur_sp[16];

	if(data->packet_type == 200)
	{
		for(int x = 0; x < 16; x++)
		{
			int vv = data->data_array[x];
			if(vv < 0) vv = 0xFFFF + vv;
			cur_sp[x] = vv;
		}
	}
	else if(data->packet_type == 220)
	{
		float sp_buf[64];
		complex_f tmp[32];
		for(int n = 0; n < 4; n++)
		{
			for(int x = 0; x < 8; x++)
			{
				int vv = data->data_array[n*8+x];
				if(vv > 32767) vv = -65536+vv;				
				tmp[x].im = 0;
				tmp[x].re = vv;
			}
			ffti_f(tmp, 3, 0);
			for(int x = 0; x < 4; x++)
			{
				cur_sp[x] = sqrt(tmp[x].re*tmp[x].re + tmp[x].im*tmp[x].im);
			}
		}
	}
	else return;
	
	
	for(int n = 3; n > 0; n--)
	{
		for(int x = 0; x < 4*ml_sp_size; x++)
			ml_data_tmp[n*4*ml_sp_size + x] = ml_data_tmp[(n-1)*4*ml_sp_size + x];
	}
	for(int x = 0; x < 4*ml_sp_size; x++)
		ml_data_tmp[x] = cur_sp[x];

	float sp_vals[16];
	int sort_idx[4];
	for(int n = 0; n < 4; n++)
	{
		sort_idx[n] = n;
		sp_vals[n] = 0;
		for(int x = 0; x < 4*ml_sp_size; x++)
			sp_vals[n] += ml_data_tmp[n*4*ml_sp_size + x]*ml_data_tmp[n*4*ml_sp_size + x];
	}
	for(int n1 = 0; n1 < 4; n1++)
		for(int n2 = n1+1; n2 < 4; n2++)
		{
			if(sp_vals[sort_idx[n2]] > sp_vals[sort_idx[n1]])
			{
				int tt = sort_idx[n2];
				sort_idx[n2] = sort_idx[n1];
				sort_idx[n1] = tt;
			}
		}
	for(int n = 0; n < 4; n++)
	{
		int idx = sort_idx[n];
		for(int x = 0; x < 4*ml_sp_size; x++)
			ml_data_array[n*4*ml_sp_size + x] = ml_data_tmp[idx*4*ml_sp_size + x];
	}
	ml_data_array[4*4*ml_sp_size+0] = data->wx;
	ml_data_array[4*4*ml_sp_size+1] = data->wy;
	ml_data_array[4*4*ml_sp_size+2] = data->wz;
}

int filter_hist[10];
int filter_rec_class(int cl)
{
	for(int x = 0; x < 9; x++)
		filter_hist[x] = filter_hist[x+1];
	filter_hist[9] = cl;
	int outs[5] = {0,0,0,0,0};
	for(int x = 5; x < 10; x++)
		if(filter_hist[x] >= 0 && filter_hist[x] < 5)
			outs[filter_hist[x]]++;
	int max_out = 0;
	int max_idx = 0;
	for(int x = 0; x < 5; x++)
		if(outs[x] > max_out) max_out = outs[x], max_idx = x;
	return max_idx;
}

void process_mouse(sUEMG_data_frame *data, int rec_class)
{
	sV x0;
	x0.x = 1;
	x0.y = 0;
	x0.z = 0;
	sV z0;
	z0.x = 0;
	z0.y = 0;
	z0.z = 1;
	sQ Qsg;
	Qsg.w = data->q_w;
	Qsg.x = data->q_x;
	Qsg.y = data->q_y;
	Qsg.z = data->q_z;
	rotate_v(&Qsg, &x0);
	rotate_v(&Qsg, &z0);
//	printf("X: %g \t%g \t%g Z %g\t %g \t%g\n", x0.x, x0.y, x0.z, z0.x, z0.y, z0.z);
	float dy = -data->wy;// gyro.y;
	float dz = -data->wz;// gyro.z;
	float sens = 0.03;
	int mm_x = -sens*dz;
	int mm_y = -sens*dy;
	
	static int conseq1 = 0;
	static int conseq2 = 0;

	if(rec_class == 1) conseq1++;
	else conseq1 = 0;
	if(conseq1 > 5 && (conseq1%3)==1) emu_mouse_wheel(1);
	if(rec_class == 2) conseq2++;
	else conseq2 = 0;
	if(conseq2 > 5 && (conseq2%3)==1) emu_mouse_wheel(-1);
	
	static int prev_rec_class = 0;
	static int conseq_non4 = 0;

	if(rec_class == 4 && prev_rec_class != 4)
	{
		printf("============click==========\n");
		emu_mouse_click(1);
		prev_rec_class = 4;
	}
	else
		if(1)if(mm_x || mm_y) emu_mouse_move(mm_x, mm_y);	
	if(prev_rec_class == 4 && rec_class != 4) conseq_non4++;
	else conseq_non4 = 0;
	if(conseq_non4 > 5) prev_rec_class = rec_class;
}

void device_fix_kmeans(int k)
{
	kmeans_fix(k);
}

void device_make_kmeans()
{
	kmeans_make();
}

void device_toggle_hand_control()
{
	kmeans_toggle_hand_control();
}
void device_run_ml(float *cur_sp, sUEMG_data_frame *data)
{
	kmeans_run(cur_sp, data);
}

void device_save_ml_state()
{
	kmeans_save_state();
}

void device_load_ml_state()
{
	kmeans_load_state();
}

void data_process_step(sUEMG_data_frame *data)
{
	float sp_raw[64];
	float sp_packed[64];
	uemg_data_get_spectrum(data, sp_raw);
	uemg_pack_spectrum(sp_raw, sp_packed);
		
	draw_set_mode(0);
	for(int n = 0; n < 8; n++)
		sc_addV(emg_hpart + n, sp_packed[n]);

	kmeans_push_vector(sp_packed);
	
	device_run_ml(sp_packed, data);

	int need_train = 0;
	int train_class = 0;
	if(in_game_mode)
	{
		struct timeval cur_tm;
		gettimeofday(&cur_tm, NULL);
		
		int game_dt = (cur_tm.tv_sec - game_start_tm.tv_sec) * 1000 + (cur_tm.tv_usec - game_start_tm.tv_usec) / 1000;
		int game_time = game_dt*0.1;
		int gesture_match = 0;
		static int last_match_time = 0;
		for(int g = 0; g < game_images; g++)
		{
			int g_time = gest_queue[g]&0xFFFFFF;
			int gesture = gest_queue[g]>>24;
			int ypos = 150 + g_time - game_time;
			if(ypos > 150 - 10 && ypos < 150+10)
			{
				if(game_cur_gesture != gesture)
				{
					game_cur_gesture = gesture;
					game_train_cnt = 0;
					gesture_match = 1;
					last_match_time = game_time;
				}
			}
		}
		if(!gesture_match && game_cur_gesture != 0 && game_time - last_match_time > 85)
		{
			game_cur_gesture = 0;
			game_train_cnt = 0;
		}
		
		if(game_train_cnt < 5 + 1*(game_cur_gesture != 0))
		{
			game_train_cnt++;
			draw_game_mode_train_gesture(game_cur_gesture);
			need_train = 1;
			train_class = game_cur_gesture;
		}
		else
			draw_game_mode_train_gesture(-1);

		if(game_time > (gest_queue[game_images-1]&0xFFFFFF) + 150)
		{
			for(int N = 0; N < 100; N++)
			{
				uemg_train_ml(1000);
				uemg_test_ml();
			}
			in_game_mode = 0;	
			draw_game_mode_select(0);
		}
		draw_game_mode_update(game_time, 0);
	}
	int rec_class = uemg_process_ml(sp_packed, need_train, train_class);
	rec_class = filter_rec_class(rec_class);
	draw_set_recognized_gesture(rec_class);
//	process_mouse(data, rec_class);
}

void data_process_step_old(sUEMG_data_frame *data)
{
	pack_data_for_ML(data);
	if(data->packet_type == 100)
	{
		printf("pack 100 ");
		
		for(int n = 0; n < 4; n++)
			for(int x = 0; x < 4; x++)
		{
			int vv = data->data_array[n*4+x];
			if(vv < 0) vv = 0xFFFF + vv;
			prev_frame_sp[n*4+x] = vv;
			printf("%d ", vv);
			float *spg = spg_get_spectr(emg_sp+n, 1);
			spg[x*2 + 1] = vv;
		}
		printf("\n");
		return;
	}
//	printf("pack %d\n", data->packet_type);
	
	for(int x = 0; x < adaptive_map_size; x++)
	{
		adaptive_map_data[adaptive_map_size*3+x] = adaptive_map_data[adaptive_map_size*2+x];
		adaptive_map_data[adaptive_map_size*2+x] = adaptive_map_data[adaptive_map_size+x];
		adaptive_map_data[adaptive_map_size+x] = adaptive_map_data[x];
	}
	for(int n = 0; n < 4; n++)
	{
		if(data->packet_type == 200)
		{
			draw_set_mode(0);
			float cur_sp[16];
			int vv0 = 1;
			for(int x = 0; x < 4; x++)
			{
				int vv = data->data_array[n*4+x];
				if(vv < 0) vv = 0xFFFF + vv;
				if(x == 0) vv0 = vv;
				cur_sp[x] = vv;
//				cur_sp[x*2+1] = prev_frame_sp[n*4+x];
//				if(x > 0) cur_sp[x] /= (float)vv0 / 10000.0;
//				printf("%d ", vv);
				adaptive_map_data[n*4 + x] = 0.0001*vv;
				if(x == 0) adaptive_map_data[n*4 + x] *= 0.01;
			}
			spg_add_spectr(emg_sp+n, cur_sp);
//			muscle_active[n] *= 0.7;
//			muscle_active[n] += 0.3*(cur_sp[1] + cur_sp[2] + cur_sp[3]) / (1000.0 + cur_sp[0]);
			muscle_active[n] = (cur_sp[1] + cur_sp[2] + cur_sp[3]) / (1000.0 + cur_sp[0]);
		}
		if(data->packet_type == 220)
		{
			draw_set_mode(1);
			for(int x = 0; x < 8; x++)
			{
				int vv = data->data_array[n*8+x];
				if(vv > 32767) vv = -65536+vv;
				raw_vals[n*32+x+8*3] = raw_vals[n*32+x+8*2];
				raw_vals[n*32+x+8*2] = raw_vals[n*32+x+8];
				raw_vals[n*32+x+8] = raw_vals[n*32+x];
				raw_vals[n*32+x] = vv;
//				printf("%d ", vv);
			}
		}
	}
	float mlp_input[67];
	static int pca_ready = 0;
	if(data->packet_type == 220)
	{
		draw_set_mode(0);
		float sp_buf[64];
		complex_f tmp[32];
		for(int n = 0; n < 4; n++)
		{
			for(int x = 0; x < calc_fft_len; x++)
			{
				tmp[x].im = 0;
				tmp[x].re = raw_vals[n*32+x];
			}
			ffti_f(tmp, 3, 0);
			float sp_calc[32];
			for(int x = 0; x < 4; x++)
			{
				sp_calc[x] = sqrt(tmp[x].re*tmp[x].re + tmp[x].im*tmp[x].im);
				if(x < 8)
				{
					sp_buf[n*8+x] = sp_calc[x];
					mlp_input[n*8+x] = sp_calc[x];
				}
			}
			mlp_input[32] = data->wx;
			mlp_input[33] = data->wy;
			mlp_input[34] = data->wz;
			if(!pca_ready)
				spg_add_spectr(emg_sp+n, sp_calc);
			for(int x = 0; x < 8; x++)
				adaptive_map_data[n*8+x] = sp_calc[x];
		}
//		mlp_input[0] = 15000;
//		float sp_sum = 0;
//		for(int x = 0; x < 33; x++)
//		{
//			sp_sum += mlp_input[x]*mlp_input[x];
//		}
//		sp_sum = sqrt(sp_sum);
//		for(int x = 0; x < 33; x++)
//			mlp_input[x] /= sp_sum;
		if(pca_ready)
		{
			draw_set_mode(0);
			float pca_v[35];
			for(int x = 0; x < 35; x++) pca_v[x] = 0;
			pca_get_components(mlp_input, pca_v, 8, &pca);
			spg_add_spectr(emg_sp, pca_v);
//			printf("PCA: ");
//			for(int x = 0; x < 8; x++) printf("%g ", pca_v[x]);
//			printf("\n");
		}
		else for(int x = 0; x < 8; x++)
		{
			sc_addV(raw_emg + 0, raw_vals[0*32+x]);// - raw_vals[3*8+x]);
			sc_addV(raw_emg + 1, raw_vals[1*32+x]);// - raw_vals[0*8+x]);
			sc_addV(raw_emg + 2, raw_vals[2*32+x]);// - raw_vals[1*8+x]);
			sc_addV(raw_emg + 3, raw_vals[3*32+x]);// - raw_vals[2*8+x]);
		}
	}
//	printf("\n");
	if(game_completed && pca_ready)
	{
		float pca_v[128];
		pca_get_components(ml_data_array, pca_v, pca_nn_size, &pca);
		mlp_calculate_output(&game_mlp, pca_v);
		printf("PCA: ");
		for(int x = 0; x < 8; x++) printf("%g ", pca_v[x]);
		printf("\n");
		
		float max_out = game_mlp.output[0];
		int rec_gest = 0;
		for(int x = 1; x < 5; x++)
			if(game_mlp.output[x] > max_out)
			{
				max_out = game_mlp.output[x];
				rec_gest = x;
			}
		for(int x = 0; x < 5; x++)
			dc_data.res_map[x] = game_mlp.output[x];
		draw_fill_adaptive_map_state(adaptive_map_classes, rec_gest, dc_data.res_map);
		
	}
//	int am_N = adaptive_map_process(adaptive_map_data, adaptive_map_states);
//	draw_fill_adaptive_map_state(adaptive_map_classes, am_N, adaptive_map_states);
//	draw_fill_adaptive_map_state(adaptive_map_classes, dc_data.res_id, dc_data.res_map);

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
	world_speed.x += acc.x * 0.001;
	world_speed.y += acc.y * 0.001;
	world_speed.z += (acc.z - 8000.0) * 0.001;
	world_speed.x *= 0.999;
	world_speed.y *= 0.999;
	world_speed.z *= 0.999;
	
	float acc_ampl = sqrt(data->ax*data->ax + data->ay*data->ay + data->az*data->az);
	
//	sc_addV(acc_ch+0, data->ax);
//	sc_addV(acc_ch+1, data->ay);
//	sc_addV(acc_ch+2, data->az);
	sc_addV(acc_ch+0, acc_ampl);
	sc_addV(acc_ch+1, world_speed.y);
	sc_addV(acc_ch+2, world_speed.z);

	sc_addV(gyro_ch+0, gyro.x);
	sc_addV(gyro_ch+1, gyro.y);
	sc_addV(gyro_ch+2, gyro.z);
	float dy = -data->wy;// gyro.y;
	float dz = -data->wz;// gyro.z;
	float sens = 0.03;
	int mm_x = -sens*dz;
	int mm_y = -sens*dy;


	static float avg_spvs[4] = {10000,10000,10000,10000};
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
		if(spvs[n] < avg_spvs[n]*1.7) need_click = 0;
		avg_spvs[n] *= 0.99;
		avg_spvs[n] += 0.01*spvs[n];		
	}

//	printf("M: %d %d %g %g %g %g\n", mm_x, mm_y, spvs[0], spvs[1], spvs[2], spvs[3]);
	if(need_click) printf("click %d %d\n", mm_x, mm_y);
	if(1)if(mm_x || mm_y) emu_mouse_move(mm_x, mm_y);
	
	struct timeval cur_tm;
	gettimeofday(&cur_tm, NULL);

	if(in_game_mode)
	{
		game_completed = 1;
		int game_dt = (cur_tm.tv_sec - game_start_tm.tv_sec) * 1000 + (cur_tm.tv_usec - game_start_tm.tv_usec) / 1000;
		int game_time = game_dt*0.1;
		int gesture_match = 0;
		static int last_match_time = 0;
		for(int g = 0; g < game_images; g++)
		{
			int g_time = gest_queue[g]&0xFFFFFF;
			int gesture = gest_queue[g]>>24;
			int ypos = 150 + g_time - game_time;
			if(ypos > 150 - 10 && ypos < 150+10)
			{
				if(game_cur_gesture != gesture)
				{
					game_cur_gesture = gesture;
					game_train_cnt = 0;
					gesture_match = 1;
					last_match_time = game_time;
				}
			}
		}
		if(!gesture_match && game_cur_gesture != 0 && game_time - last_match_time > 85)
		{
			game_cur_gesture = 0;
			game_train_cnt = 0;
		}
		if(game_time > (gest_queue[game_images-1]&0xFFFFFF) + 150)
		{
			static int shfld = 0;
			if(!shfld)
			{
				nn_dataset_shuffle_roles(&game_dataset, 0.7, 0.3);
				shfld = 1; 

				pca_process_data(game_dataset.inputs, game_dataset.in_size, game_dataset.image_cnt, &pca);
				float pca_5c[5];
				pca_get_components(game_dataset.inputs, pca_5c, 5, &pca);
				printf("pca components: %g %g %g %g %g\n", pca_5c[0], pca_5c[1], pca_5c[2], pca_5c[3], pca_5c[4]);
				printf("pca EVs:\n");
				for(int n = 0; n < pca.size; n++)
					printf("%g ", pca.ev[pca.ev_sorted_idx[n]]);
				printf("\n");
				pca_ready = 1;
				nn_dataset_set_pca(&game_dataset, &pca, 1, pca_nn_size);
				
			}
			nn_dataset_train_MLP(&game_dataset, &game_mlp, 200);
			float res_err[5]; 
			int cross_errs[25];
			nn_dataset_test_MLP(&game_dataset, &game_mlp, res_err, cross_errs, 0);
			printf("NN: %g %g %g %g %g\n", res_err[0], res_err[1], res_err[2], res_err[3], res_err[4]);
		}
		float out_gest[5] = {0,0,0,0,0};
		out_gest[game_cur_gesture] = 1;
		game_train_cnt++;
		game_mlp.learn_rate = 0.0001;
		
		static int had_train_add = 0;
		if(game_train_cnt < 5 + 1*(game_cur_gesture != 0))
		{
			nn_dataset_add(&game_dataset, ml_data_array, out_gest, game_cur_gesture, 0);
//			for(int x = 0; x < 10; x++)
//				mlp_train(&game_mlp, mlp_input, out_gest);
			draw_game_mode_train_gesture(game_cur_gesture);
			had_train_add = 1;
		}
		else
		{
			draw_game_mode_train_gesture(-1);
			if(had_train_add)
			{
				had_train_add = 0;
/*				static int first_add = 1;
				if(!first_add)
				{
					free(pca.V);
					free(pca.ev);
					free(pca.U);
					free(pca.ev_sorted_idx);
				}
				first_add = 0;*/
			}
		}

		if(pca_ready)
		{
			float pca_v[35];
			for(int x = 0; x < 35; x++) pca_v[x] = 0;
			pca_get_components(mlp_input, pca_v, pca_nn_size, &pca);
			mlp_calculate_output(&game_mlp, pca_v);

	//		mlp_calculate_output(&game_mlp, mlp_input);
			float max_out = game_mlp.output[0];
			int rec_gest = 0;
			for(int x = 1; x < 5; x++)
				if(game_mlp.output[x] > max_out)
				{
					max_out = game_mlp.output[x];
					rec_gest = x;
				}
			static int med5g[5] = {0,0,0,0,0};
			for(int x = 1; x < 5; x++)
				med5g[x-1] = med5g[x];
			med5g[4] = rec_gest;
			
			int freqs[5] = {0,0,0,0,0};
			for(int x = 0; x < 5; x++) freqs[med5g[x]]++;
			int max_f = 0;
			for(int x = 0; x < 5; x++) if(freqs[x] > freqs[max_f]) max_f = x;
			
			draw_game_mode_gesture(max_f);
			draw_game_mode_update(game_time, max_f);
		}
		else
			draw_game_mode_update(game_time, 0);
	}

	if(0)if(dc_data.res_id > 0)
	{
		static struct timeval prev_tm;
		int dT = (cur_tm.tv_sec - prev_tm.tv_sec) * 1000000 + (cur_tm.tv_usec - prev_tm.tv_usec);
		prev_tm = cur_tm;
		if(dT > 100*1000)
		{
			printf("mouse click try\n");
			emu_mouse_click(1);
		}
		else
			printf("mouse click ignored %d\n", dT);
	}
	else
	{
		if(0)if(mm_x || mm_y) emu_mouse_move(mm_x, mm_y);
		printf("mm x %d y %d %g %g\n", mm_x, mm_y, dy, dz);
	}
		
	
//	sc_addV(gyro_ch+0, data->q_x);
//	sc_addV(gyro_ch+1, data->q_y);
//	sc_addV(gyro_ch+2, data->q_z);
	
	if(record_to_dataset)
	{
		float in_arr[16];
		for(int x = 0; x < 16; x++)
			in_arr[x] = data->data_array[x];
		nn_dataset_add(&ds, in_arr, NULL, uemg_save_frame.mark, role_auto);
	}
}

uint32_t uemg_data_id = 0;
int prev_data_id = -1;

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
		adatpive_map_init(adaptive_map_classes, adaptive_map_size*4);
		emu_init();
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

			data_process_step(&uemg_dat);

			if(save_file > 0)
			{
				uemg_save_frame.data_id = uemg_data_id + uemg_dat.data_id;
				for(int x = 0; x < 16; x++)
					uemg_save_frame.sp_data[x] = uemg_dat.data_array[x];
				uemg_save_frame.ax = uemg_dat.ax;
				uemg_save_frame.ay = uemg_dat.ay;
				uemg_save_frame.az = uemg_dat.az;
				uemg_save_frame.wx = uemg_dat.wx;
				uemg_save_frame.wy = uemg_dat.wy;
				uemg_save_frame.wz = uemg_dat.wz;
				write(save_file, &uemg_save_frame, sizeof(uemg_save_frame));
			}
		}
	}
	memcpy(response_buf, response_buf+processed_pos, response_pos-processed_pos);
	response_pos -= processed_pos;
}

void device_emulate_step()
{
	if(!emulate_on) return;
	if(emulate_file <= 0) return;
	int bytes_read = read(emulate_file, &uemg_save_frame, sizeof(uemg_save_frame));
	if(bytes_read < sizeof(uemg_save_frame))
	{
		device_emulate_end();
		return;
	}
	for(int x = 0; x < 16; x++)
		uemg_dat.data_array[x] = uemg_save_frame.sp_data[x];
	uemg_dat.ax = uemg_save_frame.ax;
	uemg_dat.ay = uemg_save_frame.ay;
	uemg_dat.az = uemg_save_frame.az;
	uemg_dat.wx = uemg_save_frame.wx;
	uemg_dat.wy = uemg_save_frame.wy;
	uemg_dat.wz = uemg_save_frame.wz;
	
	data_process_step(&uemg_dat);
}