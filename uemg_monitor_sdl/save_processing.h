#include <stdint.h>

typedef struct sUEMG_saved_frame
{
	uint32_t data_id;
	int mark;
	int16_t sp_data[16];
	int16_t ax;
	int16_t ay;
	int16_t az;
	int16_t wx;
	int16_t wy;
	int16_t wz;	
}sUEMG_saved_frame;

void sp_open_file(char *filename);
void sp_prepare_dataset();
void sp_train_mlp(int cycles_count, float *cl_err, float *av_err);
void sp_set_train_speed(float speed);
