#include "nn_adaptive_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int map_N = 0;
int map_vec_size = 0;
float *map_weights = NULL;
int map_active_vect = 0;
float map_threshold = 0.1;
float *map_fatigue_lvl;

void adatpive_map_init(int clusters, int size)
{
	printf("AM init\n");
	if(map_weights != NULL)
		free(map_weights);
	map_N = clusters;
	map_vec_size = size;
	map_weights = (float*)malloc(map_N*map_vec_size*sizeof(float));
	map_fatigue_lvl = (float*)malloc(map_N*sizeof(float));
	for(int x = 0; x < map_N*map_vec_size; x++)
		map_weights[x] = 0.1*(((x*x*3 + x*1455+151)%17) - 8.0);
	for(int x = 0; x < map_N; x++)
		map_fatigue_lvl[x] = 0;
}
float am_af(float v)
{
	if(v < 0)
		return v / (1.0 - v);
	return v / (1.0 + v);
}
float d_am_af(float v)
{
	float vv = 1.0 + v;
	if(v < 0) vv = 1.0 - v;
	return 1.0 / (vv*vv);
}

float am_train_coeff = 0.0005;

int adaptive_map_process(float *data, float *states)
{
	if(map_weights == NULL) return 0;
	float data_norm = 0.0001;
	for(int x = 0; x < map_vec_size; x++)
		data_norm += data[x]*data[x];
	data_norm = 1.0 / sqrt(data_norm);
	data_norm = 1.0f;
	float max_st = 1;// -12345678;
	int res_N = -1;
	for(int n = 0; n < map_active_vect; n++)
	{
		float sum = 0;
		for(int x = 0; x < map_vec_size; x++)
		{
			float v = data[x]*data_norm * map_weights[n*map_vec_size + x];
			sum += v;// v*v;//am_af(v);
		}
		sum *= 1.0f + map_fatigue_lvl[n];
		map_fatigue_lvl[n] *= 0.99f;
		states[n] = sum;
//		printf("sn %d %g\n", n, states[n]);
		if(sum > max_st || n == 0)
		{
			max_st = sum;
			res_N = n;
		}
	}
	if(max_st > map_threshold)
	{
		printf("AM: max st %g\n", max_st);
		if(map_active_vect < map_N)
		{
			int n = map_active_vect;
			for(int x = 0; x < map_vec_size; x++)
				map_weights[n*map_vec_size + x] = data[x]*data_norm;
			res_N = map_active_vect;
			map_active_vect++;
		}
	}
	if(res_N >= 0)
	{
		map_fatigue_lvl[res_N] += 0.03;
//		if(map_fatigue_lvl[res_N] > 3) map_fatigue_lvl[res_N] = 3;
		for(int n = 0; n < map_N; n++)
		{
			if(map_fatigue_lvl[n] < 0.00001)
			{
				for(int x = 0; x < map_vec_size; x++)
					map_weights[n*map_vec_size + x] = data[x]*data_norm;
				map_fatigue_lvl[n] = 1;
			}

			int dn = n - res_N;
			if(dn < 0) dn = -dn;
			if(dn > map_N/2) dn = map_N - dn;
			if(dn > 5) continue;
			float dn_cf = 1.0f / (1.0f + (float)dn*dn);
			float trc = am_train_coeff * dn_cf * 1.0 / (map_fatigue_lvl[n] + 0.1);
			float wgt_norm = 0;
			for(int x = 0; x < map_vec_size; x++)
			{
				map_weights[n*map_vec_size + x] += trc * data[x]*data_norm;
				float mw = map_weights[n*map_vec_size + x];
	//			printf("mw %d %g\n", x, mw);
				wgt_norm += mw*mw;
			}
			wgt_norm = 1.0 / sqrt(0.01 + wgt_norm);
	//		printf("%d wgtn %g\n", n, wgt_norm);
			for(int x = 0; x < map_vec_size; x++)
				map_weights[n*map_vec_size + x] *= wgt_norm;
		}
	}
	return res_N; 
}
