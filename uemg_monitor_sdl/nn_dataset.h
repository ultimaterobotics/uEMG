#include <stdint.h>
#include "nn_mlp.h"
#include "pca_processor.h"

typedef struct sNN_dataset
{
	int image_cnt;
	int tgt_is_class;
	int in_size;
	int tgt_size;
	
	float *inputs;
	float *targets;
	float *cur_tgt;
	float *pca_input;
	int *classes;
	int *roles;
	int *class_sorted_list;
	int class_start_idx[256];
	int class_train_cnt[256];
	
	int class_max;
	int class_ignore[256];
	int cnt_per_class[256]; //if more than 256 classes - well, too bad

	int mem_size;
	int mem_buf_step;
	
	int use_pca;
	int pca_N;
	sPCA_data *pca;
}sNN_dataset;

enum
{
	role_auto = 0,
	role_train,
	role_test,
	role_validate,
	roles_cnt
};

void nn_dataset_init(sNN_dataset *ds, int in_size, int out_size, int tgt_is_class, int size_init);
void nn_dataset_clear(sNN_dataset *ds);
void nn_dataset_add(sNN_dataset *ds, float *input, float *target, int tgt_class, int role);
void nn_dataset_normalize(sNN_dataset *ds);
void nn_dataset_calc_classes(sNN_dataset *ds);
void nn_dataset_shuffle_roles(sNN_dataset *ds, float perc_train, float perc_test);
void nn_dataset_update_index(sNN_dataset *ds);
void nn_dataset_train_MLP(sNN_dataset *ds, sMLP *mlp, int cycles_count);
void nn_dataset_test_MLP(sNN_dataset *ds, sMLP *mlp, float *res_class_error, int *cross_errors, int test_all);
void nn_dataset_set_pca(sNN_dataset *ds, sPCA_data *pca, int use_pca, int pca_N);
