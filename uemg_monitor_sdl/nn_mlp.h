#ifndef NN_MLP__H
#define NN_MLP__H

typedef struct sMLP
{
	int* layer_structure;
	int layer_count; //number of layers - more convenient than .length method
	
	float** weights; // weights to the previous layer and bias
	float** postsynapses; //weighted sum before applying any functions
	float** outputs;

	float* output; //final processed output
	
	float** dEdX; //error by output of each layer - after activation function
	
	float learn_rate;
	float LRELU; //leaky rely slope
}sMLP;

void mlp_init(sMLP *mlp, int* layerStruct);
void mlp_init_weights(sMLP *mlp);
void mlp_clear(sMLP *mlp);
float mlp_get_average_weight(sMLP *mlp);
void mlp_calculate_output(sMLP *mlp, float* input);
void mlp_train(sMLP *mlp, float* input, float* target);
int mlp_get_class(sMLP *mlp);

void mlp_train_layers(sMLP *mlp, float* input, float* target, int *layers_to_train);
void mlp_train_autoencode_layer(sMLP *mlp, float* input, int layer_to_ac);

void mlp_save_to_file(sMLP *mlp, char *fname);
void mlp_read_from_file(sMLP *mlp, char *fname);

#endif