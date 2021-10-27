#include <math.h>
#include <stdlib.h>
#include <stdint.h> 
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h>
#include <unistd.h>
#include "nn_mlp.h"


//float LRELU = 0.25;

// For normalized online backprop

int* layerWeightStarts;
int* layerOutputStarts;

void mlp_init_weights(sMLP *mlp)
{
	for(int l = 0; l < mlp->layer_count-1; l++)
	{
		int w_cnt = (mlp->layer_structure[l]+1) * mlp->layer_structure[l+1];

		float scale = sqrt(2.0 / mlp->layer_structure[l]);

		for (int w = 0; w < w_cnt; w++)
		{
			float r = ((float)(rand()%10000)/10000.0 - .5 ) * 2.0;
			mlp->weights[l][w] = r * scale;
		}
	}
}

float AF(sMLP *mlp, float val) //activation function - can be replaced by any other
{
	float res = 0;
//	float aval = val;
//	if(aval < 0) aval = -val;
//	return val / (aval + 1.0);
//	float eval;
//	if(val < 5) eval = exp(val);
//	else val = exp(5);
//	res = eval / (eval + 1.0);
//	return res;

	if (val < 0)
		res = val * mlp->LRELU;
	else if(val < 1) 
		res = val;
	else res = 1 + val * mlp->LRELU;
	return res;

	// Leaky RELU activation
	if ( val < 0 )
		res = val * mlp->LRELU;
	else 
		res = val;
	return res;
}

float dAF(sMLP *mlp, float val) //derivative of activation function
{
	float res = 0;
//	float aval = val;
//	if(aval < 0) aval = -val;
//	return 1.0 / (aval + 1.0) * (aval + 1.0);
//	if(val < 5) eval = exp(val);
//	else val = exp(5);
//	res = eval / ((eval + 1.0)*(eval + 1.0));
//	return res;
	if(val > 1) return mlp->LRELU;
	else if(val > 0) return 1;
	else return mlp->LRELU;
	
	if(val > 0) return 1;
	else return mlp->LRELU;
}

float F(sMLP *mlp, int l, int n) //transfer function for neuron - activation + normalization if turned on
{ //if updateValues is true, would update average, variance for normalization
	float val = mlp->postsynapses[l][n]; //already calculated pre-transfer value
	return AF(mlp, val); //simple case - just apply activation function
}

float dF(sMLP *mlp, int l, int n) //derivative of transfer function, numerical calculation
{
	float val = mlp->postsynapses[l][n];
	return dAF(mlp, val);
}

// layerStructure contains the number of nodes per layer, zero layer marks end of array - C restriction
void mlp_init(sMLP *mlp, int* layer_struct)
{
	mlp->layer_count = 0;
	for(int x = 0; x < 123; x++)
	{
		if(layer_struct[x] == 0) break;
		mlp->layer_count++;
	}

	mlp->layer_structure = (int*)malloc(mlp->layer_count*sizeof(int));
	for(int x = 0; x < mlp->layer_count; x++)
		mlp->layer_structure[x] = layer_struct[x];
	
	mlp->weights = (float**)malloc(mlp->layer_count*sizeof(float*));
	mlp->postsynapses = (float**)malloc(mlp->layer_count*sizeof(float*));
	mlp->outputs = (float**)malloc(mlp->layer_count*sizeof(float*));
	mlp->dEdX = (float**)malloc(mlp->layer_count*sizeof(float*));
	for (int l = 0; l < mlp->layer_count; l++ )
	{
		if(l < mlp->layer_count-1)
		{
			int w_cnt = (mlp->layer_structure[l]+1) * mlp->layer_structure[l+1];
			mlp->weights[l] = (float*)malloc(w_cnt*sizeof(float));
		}
		mlp->postsynapses[l] = (float*)malloc(mlp->layer_structure[l]*sizeof(float)); 
		mlp->outputs[l] = (float*)malloc(mlp->layer_structure[l]*sizeof(float)); 
		mlp->dEdX[l] = (float*)malloc(mlp->layer_structure[l]*sizeof(float)); 
	}

	mlp_init_weights(mlp);

	mlp->output = mlp->outputs[mlp->layer_count-1];
	mlp->LRELU = 0.1;
	mlp->learn_rate = 0.01;
}

void mlp_clear(sMLP *mlp)
{	
	for (int l = 0; l < mlp->layer_count; l++ )
	{
		if(l < mlp->layer_count-1)
			free(mlp->weights[l]);
		free(mlp->postsynapses[l]);
		free(mlp->outputs[l]);
		free(mlp->dEdX[l]);
	}
	free(mlp->weights);
	free(mlp->postsynapses);
	free(mlp->outputs);
	free(mlp->dEdX);

	free(mlp->layer_structure);
}

float mlp_get_average_weight(sMLP *mlp) //for debug purposes - when weights constantly increase, something is wrong
{
	float w_sum = 0, w_tot_cnt = 0;
	for (int l = 0; l < mlp->layer_count-1; l++ )
	{
		int w_cnt = (mlp->layer_structure[l]+1) * mlp->layer_structure[l+1];
		for(int w = 0; w < w_cnt; w++)
		{
			w_sum += mlp->weights[l][w]*mlp->weights[l][w];
			w_tot_cnt++;
		}
	}
	return sqrt(w_sum / w_tot_cnt);
}

void mlp_calculate_output(sMLP *mlp, float* input)
{
	// The first layer - initial outputs are just the input
	for(int n = 0; n < mlp->layer_structure[0]; n++)
	{
		mlp->outputs[0][n] = input[n];
		mlp->postsynapses[0][n] = input[n];
	}

	// Determine outputs by moving forward layer by layer
	for(int l = 0; l < mlp->layer_count-1; l++ )
	{
		int in_cnt = mlp->layer_structure[l];
		int out_cnt = mlp->layer_structure[l+1];

		int widx = 0;
		for(int n = 0; n < out_cnt; n++)
		{
			float postsynapse = 0;
			for(int n1 = 0; n1 < in_cnt; n1++ )
			{
				postsynapse += mlp->outputs[l][n1] * mlp->weights[l][widx++];
			}
 
			// Bias
			postsynapse += 1 * mlp->weights[l][widx++];

			mlp->postsynapses[l+1][n] = postsynapse;
			mlp->outputs[l+1][n] = F(mlp, l+1, n); //F in general form, all cases are handled inside F
		}
	}
}

int mlp_get_class(sMLP *mlp)
{
	float max_out = mlp->output[0];
	int max_idx = 0;
	for(int x = 0; x < mlp->layer_structure[mlp->layer_count-1]; x++)
	{
		if(mlp->output[x] > max_out)
		{
			max_out = mlp->output[x];
			max_idx = x;
		}
	}
	return max_idx;
}

// Updates weights0 and also updates weights so that it doesn't need to be
// copied from weights0 later
void mlp_train(sMLP *mlp, float* input, float* target)
{
	// dE = change in error
	// dX = change in activated output (includes normalization)
	// dP = change in postsynapse
	// dW = change in weight

	for(int l = 0; l < mlp->layer_count; l++ )
		for(int n = 0; n < mlp->layer_structure[l]; n++ )
			mlp->dEdX[l][n] = 0;

	mlp_calculate_output(mlp, input);

	// Determine the sample error at the last layer
	int ll = mlp->layer_count-1;
	for(int n = 0; n < mlp->layer_structure[ll]; n++ )
	{
		mlp->dEdX[ll][n] = mlp->outputs[ll][n] - target[n];
	}

	// Move backwards through the network
	for(int l = mlp->layer_count - 1; l >= 1; l--)
	{
		int cur_cnt = mlp->layer_structure[l];
		int prev_cnt = mlp->layer_structure[l-1];
		
		for(int np = 0; np < prev_cnt; np++)
		{
			float dEdX1 = 0; //dE / dX_previous layer
			for(int nc = 0; nc < cur_cnt; nc++ )
			{
				float dF_val = dF(mlp, l, nc);
				int weight_idx = nc*(prev_cnt+1) + np;
				dEdX1 += mlp->dEdX[l][nc] * dF_val * mlp->weights[l-1][weight_idx]; 
//				if(isnan(dEdX1)) printf("nan dEdX1: %d %d %d %g %g %g %d\n", l, nc, np, dF_val, mlp->dEdX[l][nc], mlp->weights[l-1][weight_idx], weight_idx);
				
				float dXdW = dF_val * mlp->outputs[l-1][np];
				float dEdW = mlp->dEdX[l][nc] * dXdW;
//				if(isnan(dXdW)) printf("nan dXdW: %d %d %d %g\n", l, nc, np, dF_val);
//				if(isnan(dEdW)) printf("nan dEdW: %d %d %d %g\n", l, nc, np, dXdW);
				mlp->weights[l-1][weight_idx] -= dEdW * mlp->learn_rate;
					
				//bias element
				if(np == prev_cnt-1)
				{
					weight_idx = nc*(prev_cnt+1) + prev_cnt;
					float dXdW = dF_val;
					float dEdW = mlp->dEdX[l][nc] * dXdW;
//					if(isnan(dXdW)) printf("nan dXdWz: %d %d %d %g\n", l, nc, np, dF_val);
//					if(isnan(dEdW)) printf("nan dEdWz: %d %d %d %g\n", l, nc, np, dXdW);
					mlp->weights[l-1][weight_idx] -= dEdW * mlp->learn_rate;
				}
			}
			mlp->dEdX[l-1][np] = dEdX1;
		}
	}
}

void mlp_train_layers(sMLP *mlp, float* input, float* target, int *layers_to_train)
{
	// dE = change in error
	// dX = change in activated output (includes normalization)
	// dP = change in postsynapse
	// dW = change in weight

	for(int l = 0; l < mlp->layer_count; l++ )
		for(int n = 0; n < mlp->layer_structure[l]; n++ )
			mlp->dEdX[l][n] = 0;

	mlp_calculate_output(mlp, input);

	// Determine the sample error at the last layer
	int ll = mlp->layer_count-1;
	for(int n = 0; n < mlp->layer_structure[ll]; n++ )
	{
		mlp->dEdX[ll][n] = mlp->outputs[ll][n] - target[n];
	}

	// Move backwards through the network
	for(int l = mlp->layer_count - 1; l >= 1; l--)
	{
		int cur_cnt = mlp->layer_structure[l];
		int prev_cnt = mlp->layer_structure[l-1];
		if(!layers_to_train[l]) break;
		
		for(int np = 0; np < prev_cnt; np++)
		{
			float dEdX1 = 0; //dE / dX_previous layer
			for(int nc = 0; nc < cur_cnt; nc++ )
			{
				float dF_val = dF(mlp, l, nc);
				int weight_idx = nc*(prev_cnt+1) + np;
				dEdX1 += mlp->dEdX[l][nc] * dF_val * mlp->weights[l-1][weight_idx]; 
//				if(isnan(dEdX1)) printf("nan dEdX1: %d %d %d %g %g %g %d\n", l, nc, np, dF_val, mlp->dEdX[l][nc], mlp->weights[l-1][weight_idx], weight_idx);
				
				float dXdW = dF_val * mlp->outputs[l-1][np];
				float dEdW = mlp->dEdX[l][nc] * dXdW;
//				if(isnan(dXdW)) printf("nan dXdW: %d %d %d %g\n", l, nc, np, dF_val);
//				if(isnan(dEdW)) printf("nan dEdW: %d %d %d %g\n", l, nc, np, dXdW);
				if(layers_to_train[l])
					mlp->weights[l-1][weight_idx] -= dEdW * mlp->learn_rate;
					
				//bias element
				if(np == prev_cnt-1)
				{
					weight_idx = nc*(prev_cnt+1) + prev_cnt;
					float dXdW = dF_val;
					float dEdW = mlp->dEdX[l][nc] * dXdW;
//					if(isnan(dXdW)) printf("nan dXdWz: %d %d %d %g\n", l, nc, np, dF_val);
//					if(isnan(dEdW)) printf("nan dEdWz: %d %d %d %g\n", l, nc, np, dXdW);
					if(layers_to_train[l])
						mlp->weights[l-1][weight_idx] -= dEdW * mlp->learn_rate;
				}
			}
			mlp->dEdX[l-1][np] = dEdX1;
		}
	}
}

void mlp_train_autoencode_layer(sMLP *mlp, float* input, int layer_to_ac)
{
	// dE = change in error
	// dX = change in activated output (includes normalization)
	// dP = change in postsynapse
	// dW = change in weight

	for(int l = 0; l < mlp->layer_count; l++ )
		for(int n = 0; n < mlp->layer_structure[l]; n++ )
			mlp->dEdX[l][n] = 0;

	mlp_calculate_output(mlp, input);

	// Determine the sample error at the last layer
	int ll = layer_to_ac;
	for(int n = 0; n < mlp->layer_structure[ll]; n++ )
	{
		mlp->dEdX[ll][n] = mlp->outputs[ll][n] - mlp->outputs[ll-2][n];
	}

	// Move backwards through the network
	for(int l = layer_to_ac; l >= 1; l--)
	{
		int cur_cnt = mlp->layer_structure[l];
		int prev_cnt = mlp->layer_structure[l-1];
		
		for(int np = 0; np < prev_cnt; np++)
		{
			float dEdX1 = 0; //dE / dX_previous layer
			for(int nc = 0; nc < cur_cnt; nc++ )
			{
				float dF_val = dF(mlp, l, nc);
				int weight_idx = nc*(prev_cnt+1) + np;
				dEdX1 += mlp->dEdX[l][nc] * dF_val * mlp->weights[l-1][weight_idx]; 
//				if(isnan(dEdX1)) printf("nan dEdX1: %d %d %d %g %g %g %d\n", l, nc, np, dF_val, mlp->dEdX[l][nc], mlp->weights[l-1][weight_idx], weight_idx);
				
				float dXdW = dF_val * mlp->outputs[l-1][np];
				float dEdW = mlp->dEdX[l][nc] * dXdW;
//				if(isnan(dXdW)) printf("nan dXdW: %d %d %d %g\n", l, nc, np, dF_val);
//				if(isnan(dEdW)) printf("nan dEdW: %d %d %d %g\n", l, nc, np, dXdW);
				mlp->weights[l-1][weight_idx] -= dEdW * mlp->learn_rate;
					
				//bias element
				if(np == prev_cnt-1)
				{
					weight_idx = nc*(prev_cnt+1) + prev_cnt;
					float dXdW = dF_val;
					float dEdW = mlp->dEdX[l][nc] * dXdW;
//					if(isnan(dXdW)) printf("nan dXdWz: %d %d %d %g\n", l, nc, np, dF_val);
//					if(isnan(dEdW)) printf("nan dEdWz: %d %d %d %g\n", l, nc, np, dXdW);
					mlp->weights[l-1][weight_idx] -= dEdW * mlp->learn_rate;
				}
			}
			mlp->dEdX[l-1][np] = dEdX1;
		}
	}
}

void mlp_save_to_file(sMLP *mlp, char *fname)
{
	int w_count = 0;
	for(int l = 0; l < mlp->layer_count-1; l++)
		w_count += (mlp->layer_structure[l]+1)*mlp->layer_structure[l+1];

	float *bin_dump = (float *)malloc((mlp->layer_count + w_count + 128)*sizeof(float));
	int bp = 0;
	bin_dump[bp++] = mlp->layer_count;
	for(int l = 0; l < mlp->layer_count; l++)
		bin_dump[bp++] = mlp->layer_structure[l];
	for(int l = 0; l < mlp->layer_count-1; l++)
	{
		int w_cnt = (mlp->layer_structure[l]+1) * mlp->layer_structure[l+1];

		for (int w = 0; w < w_cnt; w++)
		{
			bin_dump[bp++] = mlp->weights[l][w];
		}
	}

	int mlp_file = open(fname, O_WRONLY | O_CREAT, 0b110110110);
	write(mlp_file, bin_dump, bp*sizeof(float));
	close(mlp_file);
	
}

void mlp_read_from_file(sMLP *mlp, char *fname)
{
	int mlp_file = open(fname, O_RDONLY);
	lseek(mlp_file,0,0);
	int file_len = lseek(mlp_file,0,2);
	lseek(mlp_file,0,0);
	float *bin_dump = (float *)malloc(file_len+4);
	read(mlp_file, bin_dump, file_len);
	close(mlp_file);

//	float *bin_dump = (float *)malloc((mlp->layer_count + w_count + 128)*sizeof(float));
	int bp = 0;
	int layers[123];
	int lc = bin_dump[bp++];
	for(int l = 0; l < lc; l++)
		layers[l] = bin_dump[bp++];
	layers[lc] = 0;
	mlp_init(mlp, layers);
	
	for(int l = 0; l < mlp->layer_count-1; l++)
	{
		int w_cnt = (mlp->layer_structure[l]+1) * mlp->layer_structure[l+1];

		for (int w = 0; w < w_cnt; w++)
		{
			mlp->weights[l][w] = bin_dump[bp++];
		}
	}
}
