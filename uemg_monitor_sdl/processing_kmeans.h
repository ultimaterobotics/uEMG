#include "data_processing.h"

void kmeans_prepare_distribution(int K);
float kmeans_dist(float *v1, float *v2);
void kmeans_fix(int k);
void kmeans_make();
void kmeans_toggle_hand_control();
void kmeans_run(float *cur_sp, sUEMG_data_frame *uemg_frame);
void kmeans_push_vector(float *data);

void kmeans_save_state();
void kmeans_load_state();
