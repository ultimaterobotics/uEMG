#include "nn_dataset.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void nn_dataset_init(sNN_dataset *ds, int in_size, int out_size, int tgt_is_class, int size_init)
{
	ds->in_size = in_size;
	ds->tgt_size = out_size;
	ds->tgt_is_class = tgt_is_class;
	ds->mem_buf_step = 1000;
	ds->use_pca = 0;
	ds->pca_input = NULL;
	if(size_init > 0) ds->mem_size = size_init;
	else ds->mem_size = ds->mem_buf_step;
	
	ds->inputs = (float*)malloc(ds->mem_size*ds->in_size*sizeof(float));
	if(ds->tgt_is_class)
	{
		ds->classes = (int*)malloc(ds->mem_size*sizeof(int));
		ds->class_sorted_list = (int*)malloc(ds->mem_size*sizeof(int));
		ds->targets = (float*)malloc(ds->tgt_size*sizeof(float));
	}
	else 
		ds->targets = (float*)malloc(ds->mem_size*ds->tgt_size*sizeof(float));
	ds->cur_tgt = (float*)malloc(ds->tgt_size*sizeof(float));
	ds->roles = (int*)malloc(ds->mem_size*sizeof(int));
	for(int c = 0; c < 256; c++) ds->class_ignore[c] = 0;
}
void nn_dataset_clear(sNN_dataset *ds)
{
	free(ds->inputs);
	if(ds->tgt_is_class)
	{
		free(ds->classes);
		free(ds->class_sorted_list);
	}
	free(ds->targets);
	free(ds->cur_tgt);
	free(ds->roles);
}
void nn_dataset_expand_memory(sNN_dataset *ds)
{
	ds->mem_size += ds->mem_buf_step;
	ds->inputs = (float*)realloc(ds->inputs, ds->mem_size*ds->in_size*sizeof(float));
	if(ds->tgt_is_class)
	{
		ds->classes = (int*)realloc(ds->classes, ds->mem_size*sizeof(int));
		ds->class_sorted_list = (int*)realloc(ds->class_sorted_list, ds->mem_size*sizeof(int));
	}
	else
		ds->targets = (float*)realloc(ds->targets, ds->mem_size*ds->tgt_size*sizeof(float));
	ds->roles = (int*)realloc(ds->roles, ds->mem_size*sizeof(int));
}

void nn_dataset_add(sNN_dataset *ds, float *input, float *target, int tgt_class, int role)
{
	if(ds->image_cnt + 2 >= ds->mem_size) nn_dataset_expand_memory(ds);
	memcpy(ds->inputs + ds->image_cnt*ds->in_size, input, ds->in_size*sizeof(float));
	if(ds->tgt_is_class)
		ds->classes[ds->image_cnt] = tgt_class;
	else
		memcpy(ds->targets + ds->image_cnt*ds->tgt_size, target, ds->tgt_size*sizeof(float));
	ds->roles[ds->image_cnt] = role;
	ds->image_cnt++;
}
void nn_dataset_calc_classes(sNN_dataset *ds)
{
	ds->class_max = 0;
	for(int n = 0; n < ds->image_cnt; n++)
		if(ds->classes[n] > ds->class_max) ds->class_max = ds->classes[n];
	ds->class_max++; //we want class count, not last class ID
	if(ds->class_max > 255) ds->class_max = 255; 
	for(int c = 0; c < ds->class_max; c++)
		ds->cnt_per_class[c] = 0;
	for(int n = 0; n < ds->image_cnt; n++)
	{
		int cl = ds->classes[n];
		if(cl < 0) cl = 0;
		if(cl > 255) cl = 255;
		ds->cnt_per_class[cl]++;
	}
}
void nn_dataset_update_index(sNN_dataset *ds)
{
	nn_dataset_calc_classes(ds);
	ds->class_start_idx[0] = 0;
	for(int c = 1; c < ds->class_max; c++)
	{
		ds->class_start_idx[c] = ds->class_start_idx[c-1] + ds->cnt_per_class[c-1];
	}
	int filled_idx[256];
	for(int c = 0; c < ds->class_max; c++)
		filled_idx[c] = 0;
	
	for(int r = 0; r < roles_cnt; r++) //fill by roles
	{
		for(int n = 0; n < ds->image_cnt; n++)
		{
			if(ds->roles[n] != r) continue;
			int cl = ds->classes[n];
			if(cl < 0 || cl > 255) continue;
			
			int ci = ds->class_start_idx[cl] + filled_idx[cl];
			if(ci < 0 || ci >= ds->image_cnt)
			{
				printf("ci %d : cl %d, cs %d, fi %d\n", ci, cl, ds->class_start_idx[cl], filled_idx[cl]);
				return;
			}
			ds->class_sorted_list[ci] = n;
			filled_idx[cl]++;
		}
	}
}
void nn_dataset_shuffle_roles(sNN_dataset *ds, float perc_train, float perc_test)
{
	if(!ds->tgt_is_class) //simple case
	{
		for(int n = 0; n < ds->image_cnt; n++)
		{
			if(rand()%100000 < perc_train*100000) ds->roles[n] = role_train;
			else ds->roles[n] = role_test;
		}
		return;
	}
	nn_dataset_calc_classes(ds);
	printf("nn_dataset_calc_classes: %d\n", ds->class_max);
	for(int n = 0; n < ds->image_cnt; n++)
		ds->roles[n] = role_auto;
	for(int c = 0; c < ds->class_max; c++)
		ds->class_train_cnt[c] = 0;

	int cl_to_fill = ds->class_max;
	while(cl_to_fill)
	{
		int n = rand()%ds->image_cnt;
		if(ds->roles[n] != role_auto) continue;
		int cl = ds->classes[n];
		if(cl < 0 || cl > 255) continue;
		if(ds->class_train_cnt[cl] >= perc_train*ds->cnt_per_class[cl]) continue;
		ds->roles[n] = role_train;
		ds->class_train_cnt[cl]++;
		if(ds->class_train_cnt[cl] >= perc_train*ds->cnt_per_class[cl]) cl_to_fill--;
	}
	for(int n = 0; n < ds->image_cnt; n++)
		if(ds->roles[n] == role_auto) ds->roles[n] = role_test;

	nn_dataset_update_index(ds);
}

int train_calls_cnt = 0;
void nn_dataset_train_MLP(sNN_dataset *ds, sMLP *mlp, int cycles_count)
{
	train_calls_cnt++;
	for(int N = 0; N < cycles_count; N++)
	{
		for(int c = 0; c < ds->class_max; c++)
		{
			int tr_cnt = ds->class_train_cnt[c];
			int tr_num = rand()%tr_cnt;
			for(int x = 0; x < ds->tgt_size; x++)
				ds->cur_tgt[x] = (c == x);
			int tr_idx = ds->class_start_idx[c] + tr_num;
			if(tr_idx < 0 || tr_idx > ds->image_cnt)
			{
				printf("tr_idx err %d\n", tr_idx);
				return;
			}
			int in_idx = ds->class_sorted_list[tr_idx];
			if(in_idx < 0 || in_idx > ds->image_cnt)
			{
				printf("in_idx err %d, tr_idx %d\n", in_idx, tr_idx);
				return;
			}

			if(ds->use_pca)
			{
				pca_get_components(ds->inputs + ds->in_size * in_idx, ds->pca_input, ds->pca_N, ds->pca);
				mlp_train(mlp, ds->pca_input, ds->cur_tgt);
			}
			else
				mlp_train(mlp, ds->inputs + ds->in_size * in_idx, ds->cur_tgt);
		}
	}
}
//0.053654 0.141463 0.0875576 0.103448 0.204762 0.151111 
//0.052729 0.107317 0.0368664 0.108374 0.161905 0.137778 
void nn_dataset_test_MLP(sNN_dataset *ds, sMLP *mlp, float *res_class_error, int *cross_errors, int test_all)
{
	int err_count[256];
	int tot_count[256];
	int cross_err[4096];
	for(int c = 0; c < ds->class_max; c++)
	{
		err_count[c] = 0;
		tot_count[c] = 0;
	}
	for(int c = 0; c < ds->class_max*ds->class_max; c++)
		cross_err[c] = 0;
	for(int n = 0; n < ds->image_cnt; n++)
	{
		if(ds->roles[n] != role_test && !test_all) continue;
		if(ds->use_pca)
		{
			pca_get_components(ds->inputs + ds->in_size * n, ds->pca_input, ds->pca_N, ds->pca);
			mlp_calculate_output(mlp, ds->pca_input);
		}
		else
			mlp_calculate_output(mlp, ds->inputs + ds->in_size * n);
		int max_idx = 0;
		float max_v = mlp->output[0];
		for(int x = 0; x < ds->tgt_size; x++)
			if(mlp->output[x] > max_v) max_v = mlp->output[x], max_idx = x;
		int tgt_cl = ds->classes[n];
		if(max_idx != tgt_cl) err_count[tgt_cl]++;
		tot_count[tgt_cl]++;
		cross_err[tgt_cl*ds->class_max + max_idx]++;
	}
	for(int c = 0; c < ds->class_max; c++)
	{
		if(tot_count[c] > 0)
			res_class_error[c] = (float)err_count[c] / (float)tot_count[c];
		else 
			res_class_error[c] = 0;
	}
	if(cross_errors != NULL)
	{
		for(int c = 0; c < ds->class_max*ds->class_max; c++)
			cross_errors[c] = cross_err[c];
	}
}

void nn_dataset_normalize(sNN_dataset *ds)
{
	float min_vals[16384];
	float max_vals[16384];
	for(int x = 0; x < ds->in_size; x++)
	{
		min_vals[x] = ds->inputs[x]*2;
		max_vals[x] = ds->inputs[x]*0.5;
	}
	for(int i = 0; i < ds->image_cnt; i++)
	{
		float *vals = ds->inputs + ds->in_size*i;
		for(int x = 0; x < ds->in_size; x++)
		{
			if(vals[x] > max_vals[x]) max_vals[x] = 0.9*max_vals[x] + 0.1*vals[x];
			if(vals[x] < min_vals[x]) min_vals[x] = 0.9*min_vals[x] + 0.1*vals[x];
		}
	}
	for(int i = 0; i < ds->image_cnt; i++)
	{
		float *vals = ds->inputs + ds->in_size*i;
		for(int x = 0; x < ds->in_size; x++)
		{
			vals[x] = (vals[x] - min_vals[x]) / (max_vals[x] - min_vals[x] + 0.01);
		}
	}
}

void nn_dataset_set_pca(sNN_dataset *ds, sPCA_data *pca, int use_pca, int pca_N)
{
	ds->pca = pca;
	ds->use_pca = use_pca;
	ds->pca_N = pca_N;
	if(ds->pca_input != NULL) free(ds->pca_input);
	ds->pca_input = (float*)malloc(pca_N*sizeof(float));
}
