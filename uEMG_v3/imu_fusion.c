#include "imu_fusion.h"
#include "bmi160.h"
#include "fast_math.h"
#include "urf_timer.h"

float q_norm(sQ *q)
{
	return sqrt_f(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);
}

float v_norm(sV *v)
{
	return sqrt_f(v->x*v->x + v->y*v->y + v->z*v->z);
}

void q_renorm(sQ *q)
{
	float r = q_norm(q);
	if(r > 0)
	{
		float m = 1.0 / r;
		q->x *= m;	
		q->y *= m;	
		q->z *= m;	
		q->w *= m;	
	}
}

void v_renorm(sV *v)
{
	float r = v_norm(v);
	if(r > 0)
	{
		float m = 1.0 / r;
		v->x *= m;	
		v->y *= m;	
		v->z *= m;	
	}
}

void q_make_conj(sQ *q, sQ *qc)
{
	qc->x = -q->x;
	qc->y = -q->y;
	qc->z = -q->z;
	qc->w = q->w;
}

void q_mult(sQ *q1, sQ *q2, sQ *qr)
{
// x1  y1  z1
// x2  y2  z2
// x   y   z
	qr->w = q1->w*q2->w - (q1->x*q2->x + q1->y*q2->y + q1->z*q2->z);
	qr->x = q1->w*q2->x + q2->w*q1->x + q1->y*q2->z - q1->z*q2->y;
	qr->y = q1->w*q2->y + q2->w*q1->y + q1->z*q2->x - q1->x*q2->z;
	qr->z = q1->w*q2->z + q2->w*q1->z + q1->x*q2->y - q1->y*q2->x;
}

void rotate_v(sQ *q, sV *v)
{
	sQ r;
	r.w = 0;
	r.x = v->x;
	r.y = v->y;
	r.z = v->z;
	sQ qc, qq, rq;
	q_make_conj(q, &qc);
	q_mult(&r, &qc, &qq);
	q_mult(q, &qq, &rq);
	v->x = rq.x;
	v->y = rq.y;
	v->z = rq.z;
}

sQ qv_mult(sQ *q1, sQ *q2)
{
// x1  y1  z1
// x2  y2  z2
// x   y   z
	sQ q;
	q.w = 0;
	q.x = q1->y*q2->z - q1->z*q2->y;
	q.y = q1->z*q2->x - q1->x*q2->z;
	q.z = q1->x*q2->y - q1->y*q2->x;
	return q;
}

sV v_mult(sV *v1, sV *v2)
{
// x1  y1  z1
// x2  y2  z2
// x   y   z
	sV v;
	v.x = v1->y*v2->z - v1->z*v2->y;
	v.y = v1->z*v2->x - v1->x*v2->z;
	v.z = v1->x*v2->y - v1->y*v2->x;
	return v;
}
float v_dot(sV *v1, sV *v2)
{
	return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}

sQ Qsg; //quaternion from the sensor to ground reference frame
sQ cur_q;

void g_correction(sV *acceleration, float coeff) //rotate quaternion towards (0, 0, -1) acceleration with "strength" given by coeff variable
{
	sV acc;
	acc.x = acceleration->x;
	acc.y = acceleration->y;
	acc.z = acceleration->z;
	v_renorm(&acc);
	rotate_v(&Qsg, &acc); //rotate from sensor frame to ground frame
	v_renorm(&acc); //normalize: we want only direction, not value

	sV target_accel; //we want acceleration to be translated into (0, 0, 1)
	target_accel.x = 0;
	target_accel.y = 0;
	target_accel.z = 1;
	
	float vv = v_dot(&acc, &target_accel);
	if(vv > 0.999f) return; //close enough already, no correction required

	sV update_axis = v_mult(&acc, &target_accel);

	sQ update_q; //quaternion that defines updating rotation
	//approximation would work only for coeff ~= 0
	update_q.w = 1.0f - 0.5f*coeff*coeff;
	float s3 = coeff - 0.166667f*coeff*coeff*coeff;
	update_q.x = s3*update_axis.x;
	update_q.y = s3*update_axis.y;
	update_q.z = s3*update_axis.z;

//	update_q.w = cos_f(theta);
//	update_q.x = sin_f(theta)*update_axis.x;
//	update_q.y = sin_f(theta)*update_axis.y;
//	update_q.z = sin_f(theta)*update_axis.z;
	q_renorm(&update_q); //normalize before use! :)
	
//	sQ tQ = q_mult(&Qsg, &update_q);
	sQ tQ;
	q_mult(&update_q, &Qsg, &tQ);
	q_renorm(&tQ);
	Qsg.w = tQ.w;
	Qsg.x = tQ.x;
	Qsg.y = tQ.y;
	Qsg.z = tQ.z;
	return;
}

void update_rotation_m(sV *w_rad_s, float dt)
{
	sQ update_q; //angular speed quaternion
	sV gyro_w;
	
//	wQ.w = 0;
	gyro_w.x = w_rad_s->x;
	gyro_w.y = w_rad_s->y;
	gyro_w.z = w_rad_s->z;
	rotate_v(&Qsg, &gyro_w);
	float w_norm = sqrt_f(gyro_w.x*gyro_w.x + gyro_w.y*gyro_w.y + gyro_w.z*gyro_w.z);
	float rot_speed = w_norm*dt;
	update_q.w = cos_small_f(rot_speed*0.5f);
	float sn = sin_small_f(rot_speed*0.5f);
	float w_norm1 = 1.0f / w_norm;
	update_q.x = sn * gyro_w.x * w_norm1;
	update_q.y = sn * gyro_w.y * w_norm1;
	update_q.z = sn * gyro_w.z * w_norm1;
	
	sQ Qsg1;
	q_mult(&update_q, &Qsg, &Qsg1);
	
	Qsg.w = Qsg1.w;
	Qsg.x = Qsg1.x;
	Qsg.y = Qsg1.y;
	Qsg.z = Qsg1.z;

	q_renorm(&Qsg);
	
	cur_q.x = Qsg.x;
	cur_q.y = Qsg.y;
	cur_q.z = Qsg.z;
	cur_q.w = Qsg.w;
}

void rotate_to_ground(sV *v)
{
	rotate_v(&Qsg, v);
}
void set_zero_position()
{
	Qsg.w = 1;
	Qsg.x = 0;
	Qsg.y = 0;
	Qsg.z = 0;
}

/*
float imu_get_roll()
{
	double sinr_cosp = +2.0 * (Qsg.w * Qsg.x + Qsg.y * Qsg.z);
	double cosr_cosp = +1.0 - 2.0 * (Qsg.x * Qsg.x + Qsg.y * Qsg.y);
	double roll = atan2(sinr_cosp, cosr_cosp);
	return roll;
}
float imu_get_pitch()
{
	// pitch (y-axis rotation)
	double sinp = +2.0 * (Qsg.w * Qsg.y - Qsg.z * Qsg.x);
	double pitch;
	if (fabs(sinp) >= 1)
		pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
	else
		pitch = asin(sinp);
	return pitch;
}
float imu_get_yaw()
{
	// yaw (z-axis rotation)
	double siny_cosp = +2.0 * (Qsg.w * Qsg.z + Qsg.x * Qsg.y);
	double cosy_cosp = +1.0 - 2.0 * (Qsg.y * Qsg.y + Qsg.z * Qsg.z);  
	double yaw = atan2(siny_cosp, cosy_cosp);
	return yaw;
}

*/

int imu_g_corrects = 1000;

void imu_fusion_init()
{
	set_zero_position();
}

static uint32_t prev_bmi_time = 0;
float zWx = 0, zWy = 0, zWz = 0;

void imu_fusion_process()
{
	if(bmi160_read())
	{
		uint32_t ms = millis();
		sBMI160 *bmi = bmi160_get_data();
		int interval = bmi->sensor_time - prev_bmi_time;
		if(interval < 0) interval += 0xFFFFFF;
		prev_bmi_time = bmi->sensor_time;
		float dt = interval;
		dt *= 0.000039f;
		if(dt > 0.1) dt = 0.1; //initial or random overflow
		sV acc;
		acc.x = bmi->aX;
		acc.y = bmi->aY;
		acc.z = bmi->aZ;
		sV gyro;
		gyro.x = bmi->wX - zWx;
		gyro.y = bmi->wY - zWy;
		gyro.z = bmi->wZ - zWz;

		float g_norm = v_norm(&acc);
		if(g_norm > 0.9f*9.8f && g_norm < 1.1f*9.8f)
		{
			if(imu_g_corrects > 0)
			{
				imu_g_corrects--;
				g_correction(&acc, 0.01); 
				zWx *= 0.99;
				zWx += 0.01*bmi->wX;
				zWy *= 0.99;
				zWy += 0.01*bmi->wY;
				zWz *= 0.99;
				zWz += 0.01*bmi->wZ;
			}
			else
				g_correction(&acc, 0.0003); 
		}
		update_rotation_m(&gyro, dt); //fixed 100 Hz
	}
}

sQ *imu_fusion_get_q()
{
	return &cur_q;
}