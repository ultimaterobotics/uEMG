#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

//#ifndef SIMPLECHART__H
//#define SIMPLECHART__H

#include <stddef.h>
#include <stdint.h>
#include <math.h>

int strEq(const char *inputText1, const char *inputText2);

typedef struct SSimpleChart
{
//private:
	int type; // 0 - normal, 1 - xy
	int scroll_on; //1 - new values push old ones back, 0 - cyclic overwrite
	int cursor_pos;
	int dataSize;
	float *data;
	float *dataX; //for xy chart
	int dataPos;
	float scale;
	float mScale;
	float zeroV;
	float curMin;
	float curMax;
	int scalingType;
	int drawAxis;
	int color;
	int axisColor;
	int DX, DY;
	int SX, SY;
}SSimpleChart;
void sc_updateScaling(SSimpleChart *chart);
float sc_normVal(SSimpleChart *chart, float normPos);
//public:
void sc_create_simple_chart(SSimpleChart *chart, int length, int type);
//	SSimpleChart(int length);
//	~SSimpleChart();
void sc_setViewport(SSimpleChart *chart, int dx, int dy, int sizeX, int sizeY);
void sc_setParameter_cc(SSimpleChart *chart, const char *name, const char *value);
void sc_setParameter_cv(SSimpleChart *chart, const char *name, float value);
void sc_setParameter_cl(SSimpleChart *chart, const char *name, int r, int g, int b);
void sc_clear(SSimpleChart *chart);
void sc_addV(SSimpleChart *chart, float v);
float sc_getV(SSimpleChart *chart, int hist_depth);

void sc_draw(SSimpleChart *chart, GdkDrawable *drawable, GdkGC *gc, int w, int h);
void sc_draw_ln(SSimpleChart *chart, GdkDrawable *drawable, GdkGC *gc, int w, int h);
void sc_draw_buf(SSimpleChart *chart, uint8_t *draw_buf, int w, int h);
float sc_getMin(SSimpleChart *chart);
float sc_getMax(SSimpleChart *chart);
int sc_getX(SSimpleChart *chart);
int sc_getY(SSimpleChart *chart);
int sc_getSizeX(SSimpleChart *chart);
int sc_getSizeY(SSimpleChart *chart);
int sc_getColor(SSimpleChart *chart);
int sc_getDataSize(SSimpleChart *chart);
float sc_getMean(SSimpleChart *chart);
float sc_getSDV(SSimpleChart *chart);

//#endif