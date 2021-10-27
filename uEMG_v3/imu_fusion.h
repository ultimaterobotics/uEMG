
typedef struct sQ
{
	float x;
	float y;
	float z;
	float w;
}sQ;

typedef struct sV
{
	float x;
	float y;
	float z;
}sV;

void imu_fusion_init();
void imu_fusion_process();
sQ *imu_fusion_get_q();