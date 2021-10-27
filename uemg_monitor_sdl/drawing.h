
#include <stdint.h>
#include "simplechart.h"
#include "spectrogram.h"

typedef struct sSprite
{
	int w;
	int h;
	int pos_x;
	int pos_y;
	uint8_t *img;
}sSprite;

SSpectrogramView emg_sp[4];
SSimpleChart raw_emg[4];
SSimpleChart emg_hpart[8];
SSimpleChart acc_ch[3];
SSimpleChart gyro_ch[3];

sSprite sp_gest_small[4];
sSprite sp_gest_large[4];

void draw_init(int ww, int hh);
void draw_loop(uint32_t *scr_buf);
void draw_set_mode(int use_raw_data);

void draw_set_recognized_gesture(int gesture);

void draw_fill_adaptive_map_state(int am_size, int am_N, float *am_states);
void draw_game_mode_select(int onoff);
void draw_game_mode_fill(int *gesture_queue, int queue_size);
void draw_game_mode_update(int cur_time, int highlight_gesture);
void draw_game_mode_gesture(int gesture);
void draw_game_mode_train_gesture(int gesture);

void draw_set_kmeans(float *data, float *clust_dist, int K);
void draw_set_mlp_out(float *mlp_out, int N);
