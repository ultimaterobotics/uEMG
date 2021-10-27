#include "qr_factorize.h"
#include <stdlib.h>
#include <math.h>

//All matrices with M columns, N rows are located in memory as following:
// row1_1...row1_M row2_1...row2_M ... rowN_1...rowN_M
//therefore, Aij = A[i*M + j]

void qr_factorize(float *A, float *Utmp, float *Etmp, float *Qres, float *Rres, int size) //tested, OK
{
	//using Qres, Rres as temporary arrays until everything is finished
	for(int k = 0; k < size; k++)
	{
		for(int x = 0; x < size; x++)
			Utmp[x*size+k] = A[x*size+k]; //columns, not rows

		for(int i = 0; i < k; i++)
		{
			float AkEi = 0;
			for(int x = 0; x < size; x++)
				AkEi += A[x*size+k]*Etmp[x*size+i];

			for(int x = 0; x < size; x++)
				Utmp[x*size+k] -= AkEi*Etmp[x*size+i];
		}
		float norm = 0;
		for(int x = 0; x < size; x++)
			norm += Utmp[x*size+k]*Utmp[x*size+k];
		norm = sqrt(norm);
		if(norm < 0.000001) norm = 0.000001;
		for(int x = 0; x < size; x++)
			Etmp[x*size+k] = Utmp[x*size+k] / norm;
	}
	for(int x = 0; x < size*size; x++) Qres[x] = Etmp[x];
	for(int i = 0; i < size; i++)
		for(int j = 0; j < size; j++)
		{
			Rres[i*size+j] = 0;
			if(j<i) continue;
			for(int x = 0; x < size; x++)
			{
				Rres[i*size+j] += A[x*size+j] * Etmp[x*size+i];
			}
		}
	return;
}

void calc_eigenvectors(float *A, float *V, float *evals, int size)
{
	float *Ak = (float*)malloc(size*size*sizeof(float));
	float *Uk = (float*)malloc(size*size*sizeof(float));
	float *Qk = (float*)malloc(size*size*sizeof(float));
	float *Rk = (float*)malloc(size*size*sizeof(float));

	float *Utmp = (float*)malloc(size*size*sizeof(float));
	float *Etmp = (float*)malloc(size*size*sizeof(float));
	
	for(int i = 0; i < size; i++)
		for(int j = 0; j < size; j++)
	{
		Ak[i*size+j] = A[i*size+j];
		Uk[i*size+j] = i==j;
	}
	for(int N = 0; N < 300; N++)
	{
		qr_factorize(Ak, Utmp, Etmp, Qk, Rk, size);
		float a_diff = 0;
		for(int i = 0; i < size; i++)
			for(int j = 0; j < size; j++)
		{
			float m = 0;
			for(int x = 0; x < size; x++)
				m += Rk[i*size+x] * Qk[x*size+j];
			a_diff += (Ak[i*size+j] - m)*(Ak[i*size+j] - m);
			Ak[i*size+j] = m;
			m = 0;
			for(int x = 0; x < size; x++)
				m += Uk[i*size+x] * Qk[x*size+j];
			Utmp[i*size+j] = m;
		}
		float u_diff = 0;
		for(int x = 0; x < size*size; x++)
		{
			u_diff += (Uk[x] - Utmp[x])*(Uk[x] - Utmp[x]);
			Uk[x] = Utmp[x];
		}
		if(a_diff < 0.000001) break;
	}
	for(int x = 0; x < size*size; x++) V[x] = Uk[x];
	for(int x = 0; x < size; x++) evals[x] = Ak[x*size+x];

	free(Ak);
	free(Uk);
	free(Rk);
	free(Qk);
	free(Utmp);
	free(Etmp);
}
