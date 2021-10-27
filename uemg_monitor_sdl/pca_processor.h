#ifndef PCA_PROCESSOR__H
#define PCA_PROCESSOR__H

typedef struct sPCA_data
{
	int size;
	float *V;
	float *ev;
	float *U;
	float *bias;
	float *norm_coeff;
	int normalize;
	int *ev_sorted_idx;
}sPCA_data;

void pca_process_data(float *data, int vect_size, int vect_count, sPCA_data* pca_res); //data: X_1_1...X_1_vect_size-1 X_2_1...X_2_vect_size-1 ... X_vect_count-1_1 ... X_vect_count-1_vect_size-1
void pca_get_components(float *in_vect, float *pca_components, int N, sPCA_data* pca);

#endif