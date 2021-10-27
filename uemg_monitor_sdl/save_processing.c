#include "save_processing.h"
#include "nn_dataset.h"
#include "nn_mlp.h"
#include "drawing.h"
#include <fcntl.h>


int data_file = -1;
sNN_dataset ds;
sMLP mlp;

int cl_count = 5;


void sp_open_file(char *filename)
{
	data_file = open(filename, O_RDONLY);
	printf("data file open: %d\n", data_file);
}

void sp_prepare_dataset()
{
	if(data_file < 1) return;
	nn_dataset_init(&ds, 16*4, 10, 1, 0);

	sUEMG_saved_frame uemg_save_frame;

	int added_cnt = 0;
	float in_dat[128];
	int no_sw_time = 0;
	int prev_mark = 0;
	while(1)
	{
		int bytes_read = read(data_file, &uemg_save_frame, sizeof(uemg_save_frame));
		if(bytes_read < sizeof(uemg_save_frame))
		{
			break;
		}
		for(int n = 0; n < 16*3; n++)
			in_dat[16+n] = in_dat[n];
		for(int x = 0; x < 16; x++)
		{
			in_dat[x] = uemg_save_frame.sp_data[x];
			in_dat[x] *= 0.0001;
		}
		if(uemg_save_frame.mark != prev_mark)
		{
			prev_mark = uemg_save_frame.mark;
			no_sw_time = 0;
			if(ds.image_cnt > 4)
				ds.image_cnt -= 4;
		}
		no_sw_time++; 
		if(uemg_save_frame.mark < cl_count)
		{
			if(added_cnt > 3)
			{
				nn_dataset_add(&ds, in_dat, NULL, uemg_save_frame.mark, role_auto);
			}
			added_cnt++; 
		}
	} 
	printf("added %d records\n", added_cnt);
	close(data_file);

//	nn_dataset_normalize(&ds);
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
	 
	int nn_layers[16] = {16*3, 50, 30, 10, cl_count, 0};
	mlp_init(&mlp, nn_layers);
	mlp.learn_rate = 0.003;
}
void sp_train_mlp(int cycles_count, float *cl_err, float *av_err)
{
	if(data_file < 1) return;
	nn_dataset_train_MLP(&ds, &mlp, cycles_count);
	printf("W: %g\n", mlp_get_average_weight(&mlp));
	float cl_error[32];
	int cross_err[4096];
	nn_dataset_test_MLP(&ds, &mlp, cl_error, cross_err, 0);
	float tot_err = 1;
	float avg_err = 0;
	for(int c = 0; c < cl_count; c++)
	{
		tot_err *= (1.0 - cl_error[c]);
		avg_err += cl_error[c];
		printf("%g ", cl_error[c]);
	}
	printf("\n");
	for(int c1 = 0; c1 < cl_count; c1++)
	{
		for(int c2 = 0; c2 < cl_count; c2++)
			printf("%d ", cross_err[c1*cl_count + c2]);
		printf("\n");
	}
	avg_err /= cl_count;
	tot_err = pow(tot_err, 1.0/(float)cl_count);
	sc_addV(acc_ch+0, tot_err);
	sc_addV(acc_ch+1, avg_err); 
	*av_err = avg_err;
	*cl_err = tot_err;
}
//6 classes, x4 input: 0.0520231 0.0731707 0.045977 0.0555556 0.11976 0.105556 
//6 classes, x1 input: 0.0531792 0.189024 0.103448 0.0802469 0.179641 0.211111
//5 classes, x4 input: 0.0294798 0.0731707 0.103448 0.0493827 0.0778443  
//5 classes, x1 input: 0.0508671 0.121951 0.0977011 0.0432099 0.125749 
//5 classes, x1 input 20-10-15-5: 0.0601295 0.190244 0.179724 0.0788177 0.119048 
//5 classes, x1 input 50-5: 0.0656799 0.160976 0.138249 0.0689655 0.138095

void sp_set_train_speed(float speed)
{
	mlp.learn_rate = speed;
}
