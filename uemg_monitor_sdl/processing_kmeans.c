#pragma GCC diagnostic error "-Wimplicit-function-declaration"

#include "processing_kmeans.h" 

#include <math.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* read, write, close */

#include "quat_math.h"

#include "nn_dataset.h"
#include "nn_mlp.h"
#include "drawing.h"


float *kmeans_data = NULL;
int *kmeans_class_idx;
int kmeans_max_images = 1200; //~10 seconds of data
int kmeans_cur_img = 0;

float clusters[8*32];
float clust_z[16];
int clust_fix[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int kmeans_first_run = 1;
float mean_clust[8] = {0,0,0,0,0,0,0,0};
float sdv_clust[8] = {0,0,0,0,0,0,0,0};

void kmeans_prepare_distribution(int K)
{
	for(int k = 0; k < K; k++)
		for(int x = 0; x < 8; x++)
			mean_clust[x] += clusters[8*k + x];
	for(int x = 0; x < 8; x++)
		mean_clust[x] /= 8;
	for(int k = 0; k < K; k++)
		for(int x = 0; x < 8; x++)
		{
			float dd = clusters[8*k + x] - mean_clust[x];
			sdv_clust[x] += dd*dd;
		}
	for(int x = 0; x < 8; x++)
	{
		sdv_clust[x] /= 8;
		sdv_clust[x] = sqrt(sdv_clust[x]);
	}
}

float kmeans_dist(float *v1, float *v2)
{
	float vv1[8], vv2[8];
	for(int x = 0; x < 8; x++)
	{
		vv1[x] = 1 + (v1[x] - mean_clust[x]) / (1.0 + sdv_clust[x]*3.0);
		vv2[x] = 1 + (v2[x] - mean_clust[x]) / (1.0 + sdv_clust[x]*3.0);
	}
	
	float avg1 = 0.001, avg2 = 0.001;
	float ddist = 0;
	for(int n = 0; n < 8; n++)
	{
		avg1 += v1[n];
		avg2 += v2[n];
		float dd = v1[n] - v2[n];
		if(dd < 0) dd = -dd;
		ddist += dd;
	}
	avg1 /= 8.0;
	avg2 /= 8.0;

	float scale = 1.0 / 800.0;
	float size = 5;
	
	float X1[4], Y1[4];
	float X2[4], Y2[4];
	for(int x = 0; x < 4; x++)
	{
		X1[x] = vv1[x*2];
		Y1[x] = vv1[x*2+1];
		X2[x] = vv2[x*2];
		Y2[x] = vv2[x*2+1];
		if(x == 1 || x == 2)
		{
			X1[x] += size;
			X2[x] += size;
		}
		if(x == 2 || x == 3)
		{
			Y1[x] += size;
			Y2[x] += size;
		}
	}
	float SX1[4], SY1[4];
	float SX2[4], SY2[4];
	for(int x = 0; x < 4; x++)
	{
		SX1[x] = X1[(x+1)%4] - X1[x];
		SY1[x] = Y1[(x+1)%4] - Y1[x];
		SX2[x] = X2[(x+1)%4] - X2[x];
		SY2[x] = Y2[(x+1)%4] - Y2[x];
	}
	float A1[4], A2[4];
	float da = 0;
	for(int x = 0; x < 4; x++)
	{
//		float dx1 = SX1[x] - SX1[(x+1)%4];
//		float dy1 = SY1[x] - SY1[(x+1)%4];
//		float dx2 = SX2[x] - SX2[(x+1)%4];
//		float dy2 = SY2[x] - SY2[(x+1)%4];
//		float rr1 = sqrt(dx1*dx1 + dy1*dy1) + 0.01;
//		float rr2 = sqrt(dx2*dx2 + dy2*dy2) + 0.01;

		float rr1_1 = sqrt(SX1[x]*SX1[x] + SY1[x]*SY1[x]) + 0.01;
		float rr1_2 = sqrt(SX1[(x+1)%4]*SX1[(x+1)%4] + SY1[(x+1)%4]*SY1[(x+1)%4]) + 0.01;
		float rr2_1 = sqrt(SX2[x]*SX2[x] + SY2[x]*SY2[x]) + 0.01;
		float rr2_2 = sqrt(SX2[(x+1)%4]*SX2[(x+1)%4] + SY2[(x+1)%4]*SY2[(x+1)%4]) + 0.01;
		A1[x] = SX1[x]*SY1[(x+1)%4] + SY1[x]*SX1[(x+1)%4];
		A2[x] = SX2[x]*SY2[(x+1)%4] + SY2[x]*SX2[(x+1)%4];
		A1[x] /= (rr1_1*rr1_2);
		A2[x] /= (rr2_1*rr2_2);
//		A1[x] /= (rr1*rr2);
//		A2[x] /= (rr1*rr2);
		da += fabs(A2[x] - A1[x]);
	}
	da += ddist*scale;// fabs(avg1 - avg2);
	return da; 
	
	
	float k12 = avg1/avg2;
	float diff = 0;
	for(int n = 0; n < 8; n++)
	{
		float dd = v1[n] - v2[n]*k12;
		if(dd < 0) dd = -dd;
		diff += dd;
	}
	if(k12 < 1) k12 = 1.0 / k12;
	return diff + 0.1*ddist;
}

void kmeans_fix(int k)
{
	clust_fix[k] = !clust_fix[k];
}
void kmeans_make()
{
	int K = 9;
	int N = 2; //iterations
	if(kmeans_first_run)
	{
		for(int k = 0; k < K; k++)
		{
			for(int n = 0; n < 8; n++)
				clusters[k*8+n] = kmeans_data[k*10*8 + n];
		}
		kmeans_first_run = 0;
	}
	else
	{
		for(int k = 0; k < K; k++)
		{
			if(clust_z[k] < 25)
			{
				for(int n = 0; n < 8; n++)
					clusters[k*8+n] = kmeans_data[k*100*8 + n];
			}
		}
		
	}
	
	kmeans_prepare_distribution(K);
	for(int n = 0; n < N; n++)
	{
		for(int x = 0; x < kmeans_max_images; x++)
		{
			float *vect = kmeans_data + x*8;
			float min_dist = 123456789;
			int min_k = 0;
			for(int k = 0; k < K; k++)
			{
				float *clust = clusters + k*8;
				float dist = kmeans_dist(clust, vect);
/*				for(int d = 0; d < 8; d++)
					if(clust[d] - vect[d] < 0)
						dist += vect[d] - clust[d];
					else
						dist += clust[d] - vect[d];*/
				if(dist < min_dist)
				{
					min_dist = dist;
					min_k = k;
				}
			}
			kmeans_class_idx[x] = min_k;
		}

		for(int k = 0; k < K; k++)
		{
			if(clust_fix[k]) continue;
			clust_z[k] = 0.0001;
			for(int x = 0; x < 8; x++)
				clusters[k*8+x] = 0;
		}
		
		for(int x = 0; x < kmeans_max_images; x++)
		{
			int k = kmeans_class_idx[x];
			if(clust_fix[k]) continue;
			float *vect = kmeans_data + x*8;
			clust_z[k]++;
			for(int x = 0; x < 8; x++)
				clusters[k*8+x] += vect[x];
		}

		printf("=============iter %d=============\n", n);
		for(int k = 0; k < K; k++)
		{
			if(!clust_fix[k])
				for(int x = 0; x < 8; x++)
					clusters[k*8+x] /= clust_z[k];
				
			printf("class %d: %d\n", k, (int)clust_z[k]);
			for(int x = 0; x < 8; x++)
				printf("%g ", clusters[k*8+x]);
			printf("\n");
		}
	}
	float c_dist[32];
	float *vect = kmeans_data + kmeans_cur_img*8;
	kmeans_prepare_distribution(K);	
	for(int k = 0; k < K; k++)
	{
		float *clust = clusters + k*8;
		c_dist[k] = kmeans_dist(clust, vect);
	}
	
//	printf("setting clusters: %X\n", clusters);
	draw_set_kmeans(clusters, c_dist, K);
//	printf("clusters set: %X\n", clusters);
}

sMLP mlp_n;
int mlp_n_inited = 0;
float *mlp_n_images;
float *mlp_n_outputs;
int mlp_n_img_cnt = 0;
int mlp_n_max_img = 4096;

void init_fingers_mlp()
{
	int layers[5] = {8+9, 15, 10, 6, 0};
	mlp_init(&mlp_n, layers);
	mlp_n_images = (float*)malloc(mlp_n_max_img*(8+9)*sizeof(float));
	mlp_n_outputs = (float*)malloc(mlp_n_max_img*(6)*sizeof(float));
}

float cur_mlp_inp[32];

void prepare_fingers_mlp_input(float *cur_sp)
{		
	float vv1[8];
	for(int x = 0; x < 8; x++)
		vv1[x] = 1 + (cur_sp[x] - mean_clust[x]) / (1.0 + sdv_clust[x]*3.0);
	
	float size = 5;
	
	float X1[4], Y1[4];
	for(int x = 0; x < 4; x++)
	{
		X1[x] = vv1[x*2];
		Y1[x] = vv1[x*2+1];
		if(x == 1 || x == 2)
			X1[x] += size;
		if(x == 2 || x == 3)
			Y1[x] += size;
	}
	float SX1[4], SY1[4];
	for(int x = 0; x < 4; x++)
	{
		SX1[x] = X1[(x+1)%4] - X1[x];
		SY1[x] = Y1[(x+1)%4] - Y1[x];
	}
	float A1[4];
	for(int x = 0; x < 4; x++)
	{
		float rr1 = sqrt(SX1[x]*SX1[x] + SY1[x]*SY1[x]) + 0.01;
		float rr2 = sqrt(SX1[(x+1)%4]*SX1[(x+1)%4] + SY1[(x+1)%4]*SY1[(x+1)%4]) + 0.01;
		A1[x] = SX1[x]*SY1[(x+1)%4] + SY1[x]*SX1[(x+1)%4];
		A1[x] /= (rr1*rr2);
	}

	float scale = 1.0 / 10000.0;
	for(int x = 0; x < 4; x++)
	{
		cur_mlp_inp[x] = A1[x];
		cur_mlp_inp[x+4] = cur_sp[x*2]*scale;
	}
		
	for(int k = 0; k < 9; k++)
	{
		float *clust = clusters + k*8;
		cur_mlp_inp[8+k] = kmeans_dist(clust, cur_sp);
	}
}

void device_train_ml(int cur_img)
{
	if(cur_img == -2)
	{
		mlp_n_img_cnt = 0;
		return;
	}
	if(cur_img >= 0)
	{
		mlp_init_weights(&mlp_n);
		if(mlp_n_img_cnt < mlp_n_max_img)
		{
			for(int x = 0; x < 8+9; x++)
				mlp_n_images[(8+9)*mlp_n_img_cnt + x] = cur_mlp_inp[x];
			for(int x = 0; x < 6; x++)
				mlp_n_outputs[6*mlp_n_img_cnt + x] = (x==cur_img);
			mlp_n_img_cnt++;
		}
	}
	else
	{
		mlp_n.learn_rate = 0.003;
		for(int n = 0; n < 3000; n++)
		{
			for(int v = 0; v < mlp_n_img_cnt; v++)
				mlp_train(&mlp_n, mlp_n_images + (8+9)*v, mlp_n_outputs + 6*v);
		}
	}
}

int servo_out = 0;

void init_servo_out()
{
//	if(servo_out <= 0)
//		servo_out = open_servo_device();
}

void send_servo_data(uint8_t *svals)
{
	if(servo_out <= 0) return;
	
	uint8_t sndbuf[64];
	int pp = 0;
	sndbuf[pp++] = 255;
	for(int x = 0; x < 5; x++)
		sndbuf[pp++] = svals[x];
		
	write(servo_out, sndbuf, pp);
}

int send_servo_out = 0;

void kmeans_toggle_hand_control()
{
	send_servo_out = !send_servo_out;
}
struct timeval prev_send_tm;

int fd_fifo = -1;

void init_freecad_fifo()
{
    char *fifo = "/tmp/freecad_fifo_cmd";

    mkfifo(fifo, 0666);
    fd_fifo = open(fifo, O_RDWR | O_NONBLOCK);
}

sQ freecad_cam;

void send_data_to_freecad(sUEMG_data_frame *uemg_dat, int cmd_class)
{
	if(fd_fifo < 0)
	{
		init_freecad_fifo();
		freecad_cam.w = 1;
		freecad_cam.x = 0;
		freecad_cam.y = 0;
		freecad_cam.z = 0;
//		prev_q.x = uemg_dat->q_x;
//		prev_q.y = uemg_dat->q_y;
//		prev_q.z = uemg_dat->q_z;
//		prev_q.w = uemg_dat->q_w;
		return;
	}
	static int skip_f = 0;
	static int prev_cmd = 0;
	static int mode_cam = 0;
	static int mode_cam_change_request = 0;
	static float cam_zoom = 10;
	
	if(cmd_class == 1 && mode_cam_change_request < 5)
		mode_cam_change_request++;
	if(cmd_class != 1)
	{
		if(mode_cam_change_request > 6)
			mode_cam_change_request--;
		else mode_cam_change_request = 0;
	}

	if(mode_cam_change_request == 5)
	{
		mode_cam_change_request = 10;
		if(mode_cam) mode_cam = 0;
		else mode_cam = 1;
	}
	if(cmd_class == 1) mode_cam = 1;
	else mode_cam = 0;
	
	static int selection_state = 0;
	static int select_request = 0;
	int select_changed = 0;
	
	if(cmd_class == 3)
	{
		select_request++;
	}
	else select_request = 0;
	
	float gg = uemg_dat->ax*uemg_dat->ax + uemg_dat->ay*uemg_dat->ay + uemg_dat->az*uemg_dat->az;
	printf("gg: %g\n", gg);
	if(gg > 1.8*8192*8192)
	{
		select_request = 0;
		selection_state = 0;
		select_changed = 1;
	}
	
	if(select_request == 20)
	{
		selection_state++;
		if(selection_state > 2) selection_state = 0;
		select_changed = 1;
	}
	
	if(cmd_class == 2)
	{
		if(uemg_dat->wz > 500) cam_zoom += 0.1;
		if(uemg_dat->wz < -500) cam_zoom -= 0.1;
		if(cam_zoom < 5) cam_zoom = 5;
		if(cam_zoom > 20) cam_zoom = 20;
	}
	skip_f++;
	if(skip_f < 3 && !select_changed) return;
	skip_f = 0;
	float x,y,z,w;
	int cmd_code = 1;
	uint8_t out[128];
	static float a = 0;
	a += 0.01;
	if(a > 2*3.1415926) a = 0;
	
	if(selection_state == 0 && mode_cam)
	{
		sQ wq;
		wq.x = uemg_dat->wx;
		wq.y = uemg_dat->wy; 
		wq.z = uemg_dat->wz;
		
//		q_renorm(&wq);
		wq.w = 20000;
		q_renorm(&wq);
		sQ tmp;
		q_mult(&freecad_cam, &wq, &tmp);
		freecad_cam.w = tmp.w;
		freecad_cam.x = tmp.x;
		freecad_cam.y = tmp.y;
		freecad_cam.z = tmp.z;
		q_renorm(&freecad_cam);
	}
	
	x = -freecad_cam.y;
	y = -freecad_cam.z;
	z = -freecad_cam.x;
	w = acos(freecad_cam.w);
	int len = 0;
	if(select_changed)
		len = sprintf(out, "%d 0 0 0 0\n", 5 - selection_state);
	else if(selection_state != 0)
	{
		if(mode_cam)
			len = sprintf(out, "%d %g %g %g %g\n", 10 + selection_state, 0.00005*uemg_dat->wx, 0.00005*uemg_dat->wy, 0.00005*uemg_dat->wz, w);
	}
	else if(mode_cam)
		len = sprintf(out, "1 %g %g %g %g\n", x, y, z, w);
	else
		len = sprintf(out, "2 %g %g %g %g\n", cam_zoom, 0, 0, 0);
	write(fd_fifo, out, len);
}

void kmeans_run(float *cur_sp, sUEMG_data_frame *uemg_frame)
{
	if(!mlp_n_inited)
	{
		gettimeofday(&prev_send_tm, NULL);
		init_fingers_mlp();
		mlp_n_inited = 1;
	}
	prepare_fingers_mlp_input(cur_sp);
	mlp_calculate_output(&mlp_n, cur_mlp_inp);
	int out_cl = mlp_get_class(&mlp_n);
	draw_set_mlp_out(mlp_n.output, 6);
	
	init_servo_out();
	uint8_t servo_states[5];
	for(int s = 0; s < 5; s++) servo_states[s] = 30;
	servo_states[3] = 3;
	float max_out = mlp_n.output[0];
	int max_id = 0;
	for(int s = 0; s < 6; s++)
	{
		if(mlp_n.output[s] > max_out) max_out = mlp_n.output[s], max_id = s;
	}
	if(max_id > 0)
	{
		if(0)for(int s = 0; s < 5; s++)
		{
			int vv = mlp_n.output[s] * 250;
			if(vv < 3) vv = 3;
			if(vv > 250) vv = 250;
			servo_states[s] = vv;
			
		}
		servo_states[max_id-1] = 250;
			
	}
//	servo_states[4] = 250;
	struct timeval cur_tm;
	gettimeofday(&cur_tm, NULL);
		
	int send_dt = (cur_tm.tv_sec - prev_send_tm.tv_sec) * 1000 + (cur_tm.tv_usec - prev_send_tm.tv_usec) / 1000;
	if(send_dt > 40 && send_servo_out)
	{
		send_servo_data(servo_states);
		prev_send_tm.tv_sec = cur_tm.tv_sec;
		prev_send_tm.tv_usec = cur_tm.tv_usec;
	}
	send_data_to_freecad(uemg_frame, max_id);
}

void kmeans_push_vector(float *data)
{
	if(kmeans_data == NULL)
	{
		kmeans_data = (float*)malloc(kmeans_max_images*8*sizeof(float));
		kmeans_class_idx = (int*)malloc(kmeans_max_images*sizeof(int));
		kmeans_cur_img = 0;
	}
	for(int n = 0; n < 8; n++)
	{
		kmeans_data[kmeans_cur_img*8+n] = data[n];
	}
	kmeans_make();

	kmeans_cur_img++;
	if(kmeans_cur_img >= kmeans_max_images) kmeans_cur_img = 0;
}

void kmeans_save_state()
{
	mlp_save_to_file(&mlp_n, "mlp_n_state.dat");
	int kmeans_file = open("kmeans_state.dat", O_WRONLY | O_CREAT, 0b110110110);
	write(kmeans_file, clusters, 32*8*sizeof(float));
	close(kmeans_file);
}
void kmeans_load_state()
{
	mlp_read_from_file(&mlp_n, "mlp_n_state.dat");
	int kmeans_file = open("kmeans_state.dat", O_RDONLY);
	read(kmeans_file, clusters, 32*8*sizeof(float));
	close(kmeans_file);
	
	for(int x = 0; x < 16; x++)
		clust_fix[x] = 1;	
		
	kmeans_make();
}
