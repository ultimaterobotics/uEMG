#include "pca_processor.h"
#include "qr_factorize.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

//All matrices with M columns, N rows are located in memory as following:
// row1_1...row1_M row2_1...row2_M ... rowN_1...rowN_M
//therefore, Aij = A[i*M + j]

//data: X_1_1...X_1_vect_size-1 X_2_1...X_2_vect_size-1 ... X_vect_count-1_1 ... X_vect_count-1_vect_size-1
void pca_process_data(float *data, int vect_size, int vect_count, sPCA_data* pca_res)
{
	float *U = (float*)malloc(vect_size*sizeof(float)); //average values
	float *B = (float*)malloc(vect_size*vect_count*sizeof(float)); //after removing averages

	float *bias = (float*)malloc(vect_size*sizeof(float));
	float *norm_coeff = (float*)malloc(vect_size*sizeof(float));

	for(int row = 0; row < vect_size; row++)
	{
		float min_v = data[row];
		float max_v = data[row];
		for(int col = 0; col < vect_count; col++)
		{
			if(data[col*vect_size + row] < min_v) min_v = data[col*vect_size + row]; //min value
			if(data[col*vect_size + row] > max_v) max_v = data[col*vect_size + row]; //max value
		}
		bias[row] = -min_v;
		norm_coeff[row] = 1.0 / (max_v - min_v + 0.0001);
	}
	
	for(int row = 0; row < vect_size; row++)
	{
		float avg = 0;
		for(int col = 0; col < vect_count; col++)
			avg += (data[col*vect_size + row]-bias[row])*norm_coeff[row];
		avg /= vect_count;
		U[row] = avg;
		for(int col = 0; col < vect_count; col++)
			B[col*vect_size + row] = (data[col*vect_size + row]-bias[row])*norm_coeff[row] - avg;
	}
	float *C = (float*)malloc(vect_size*vect_size*sizeof(float)); //covariance
	for(int i = 0; i < vect_size; i++)
	{
		for(int j = 0; j < vect_size; j++)
		{
			float mult = 0;
			for(int x = 0; x < vect_count; x++)
				mult += B[i*vect_size + x] * B[j*vect_size + x];
			mult /= (vect_count-1);
			C[i*vect_size + j] = mult;
		}
	}
	printf("PCA C:\n");
	for(int i = 0; i < vect_size; i++)
	{
		for(int j = 0; j < vect_size; j++)
			printf("%g ", C[i*vect_size + j]);
		printf("\n");
	}
	
	float *EV = (float*)malloc(vect_size*vect_size*sizeof(float));
	float *evals = (float*)malloc(vect_size*sizeof(float));
	
	calc_eigenvectors(C, EV, evals, vect_size);

	float *D1 = (float*)malloc(vect_size*vect_size*sizeof(float));
	float *D = (float*)malloc(vect_size*vect_size*sizeof(float));
	for(int i = 0; i < vect_size; i++)
	{
		for(int j = 0; j < vect_size; j++)
		{
			float m = 0;
			for(int x = 0; x < vect_size; x++)
				m += C[i*vect_size+x] * EV[x*vect_size + j];
			D1[i*vect_size+j] = m;
		}
	}
	for(int i = 0; i < vect_size; i++)
	{
		for(int j = 0; j < vect_size; j++)
		{
			float m = 0;
			for(int x = 0; x < vect_size; x++)
				m += EV[x*vect_size+i] * D1[x*vect_size + j];
			D[i*vect_size+j] = m;
		}
	}
	printf("PCA D:\n");
	for(int i = 0; i < vect_size; i++)
	{
		for(int j = 0; j < vect_size; j++)
			printf("%g ", D[i*vect_size + j]);
		printf("\n");
	}
	free(D1);
	free(D);
	
	int *ev_idx = (int*)malloc(vect_size*sizeof(int));
	for(int x = 0; x < vect_size; x++)
		ev_idx[x] = x;
	for(int v1 = 0; v1 < vect_size; v1++)
	{
		for(int v2 = v1+1; v2 < vect_size; v2++)
		{
			if(fabs(evals[ev_idx[v2]]) > fabs(evals[ev_idx[v1]]))
			{
				int ti = ev_idx[v1];
				ev_idx[v1] = ev_idx[v2];
				ev_idx[v2] = ti;
			}
		}
	}
	pca_res->U = U;
	pca_res->V = EV;
	pca_res->bias = bias;
	pca_res->norm_coeff = norm_coeff;
	pca_res->ev = evals;
	pca_res->ev_sorted_idx = ev_idx;
	pca_res->size = vect_size;

	free(B);
	free(C);
}

void pca_get_components(float *in_vect, float *pca_components, int N, sPCA_data* pca)
{
	for(int v = 0; v < N; v++)
	{
		float m = 0;
		int vi = pca->ev_sorted_idx[v];
		for(int x = 0; x < pca->size; x++)
			m += ((in_vect[x]-pca->bias[x])*pca->norm_coeff[x] - pca->U[x])*pca->V[x*pca->size + vi];
		pca_components[v] = m;
	}
	return;
}
