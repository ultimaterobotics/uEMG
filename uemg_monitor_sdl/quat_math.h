
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


float q_norm(sQ *q);
float v_norm(sV *v);
void q_renorm(sQ *q);
void v_renorm(sV *v);
void q_make_conj(sQ *q, sQ *qc);
void q_mult(sQ *q1, sQ *q2, sQ *qr);
void rotate_v(sQ *q, sV *v);
sQ qv_mult(sQ *q1, sQ *q2);
sV v_mult(sV *v1, sV *v2);
float v_dot(sV *v1, sV *v2);
