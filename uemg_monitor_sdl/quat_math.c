#include "quat_math.h"
#include <math.h>

float q_norm(sQ *q)
{
	return sqrt(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);
}

float v_norm(sV *v)
{
	return sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
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
