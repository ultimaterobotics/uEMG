#include <stdint.h>

typedef struct sUEMG_data_frame
{
	uint32_t unit_id;
	int packet_type;
	int data_count;
	int batt;
	int version;
	int steps;
	int data_id;
	int16_t data_array[64]; //in case of further changes, right now 16 data points
	int16_t ax;
	int16_t ay;
	int16_t az;
	int16_t wx;
	int16_t wy;
	int16_t wz;
	float q_w;
	float q_x;
	float q_y;
	float q_z;
}sUEMG_data_frame;

typedef struct sDCdata
{
	float avg_lvls[4];
	int res_id;
	float res_map[128];
}sDCdata;


enum param_sends
{
	param_batt = 0,
	param_imu_steps,
	param_end
};

int uemg_packet_parse(sUEMG_data_frame *data, uint8_t *pack);
void uemg_data_get_spectrum(sUEMG_data_frame *data, float *spectr);
void uemg_pack_spectrum(float *spect, float *packed_data);
int uemg_process_ml(float *dat, int train_mode, int train_idx);
void uemg_train_ml(int train_length);
void uemg_test_ml();

int uemg_combined_process(sUEMG_data_frame *data, int train_mode, int train_idx);
void uemg_data_clear();

float data_get_battery();
void data_get_cur_data(float *data);
void data_get_prev_data(float *data, int T);
