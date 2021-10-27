#include "drawing.h"
#include "device_functions.h"
#include "data_processing.h"

#include "font_a.h"
#include "image_loader.h"

int draw_inited = 0;
GdkGC *gc;
int WX, WY;

uint8_t *draw_buf;
int draw_raw_data = 0;

void load_sprite(char *file_name, sSprite *sprite)
{
	sprite->img = img_load_png(file_name, &(sprite->w), &(sprite->h));
	sprite->pos_x = 0;
	sprite->pos_y = 0;
}

void draw_set_mode(int use_raw_data)
{
	draw_raw_data = use_raw_data;
}

void draw_init(int ww, int hh)
{
	draw_inited = 1;
	int width = ww;
	int height = hh;

	WX = width;
	WY = height;

	int cw = width*0.8;
	int ch = (height*0.9)/8;
	
	for(int n = 0; n < 8; n++)
	{
		sc_create_simple_chart(emg_hpart+n, 800, 0);
		sc_setViewport(emg_hpart+n, 10, ch*n/2+10+2*n, cw, ch);
		sc_setParameter_cc(emg_hpart+n, "scaling", "auto");
//		sc_setParameter_cc(emg_hpart+n, "scaling", "manual");
		sc_setParameter_cv(emg_hpart+n, "scale", 1);
		sc_setParameter_cl(emg_hpart+n, "color", 50, 250, 100);
		sc_setParameter_cl(emg_hpart+n, "axis color", 255, 255, 0);			
		sc_setParameter_cc(emg_hpart+n, "draw axis", "yes");			
	} 
	
	
	for(int n = 0; n < 4; n++)
	{
		spg_init(emg_sp+n, 200, 4);
		spg_set_viewport(emg_sp+n, 10, ch*n+10+2*n, cw, ch);
		spg_set_parameter_float(emg_sp+n, "color scale", 1000);

		sc_create_simple_chart(raw_emg+n, 800, 0);
		sc_setViewport(raw_emg+n, 10, ch*n+10+2*n, cw, ch);
		sc_setParameter_cc(raw_emg+n, "scaling", "auto");
//		sc_setParameter_cc(raw_emg+n, "scaling", "manual");
		sc_setParameter_cv(raw_emg+n, "scale", 1);
		sc_setParameter_cl(raw_emg+n, "color", 50, 250, 100);
		sc_setParameter_cl(raw_emg+n, "axis color", 255, 255, 0);			
		sc_setParameter_cc(raw_emg+n, "draw axis", "yes");			
	} 
	for(int n = 0; n < 3; n++)
	{
		sc_create_simple_chart(acc_ch+n, 200, 0);
		sc_setViewport(acc_ch+n, 10, ch*(4+n)+30, cw/2-10, ch); 
		sc_setParameter_cc(acc_ch+n, "scaling", "auto");
//		sc_setParameter_cc(acc_ch+n, "scaling", "manual");
		sc_setParameter_cv(acc_ch+n, "scale", 10000.0);
		int r, g, b;
		if(n == 0) r = 0, g = 255, b = 0;
		if(n == 1) r = 255, g = 255, b = 0;
		if(n == 2) r = 255, g = 0, b = 255;
		
		sc_setParameter_cl(acc_ch+n, "color", r, g, b);
		sc_setParameter_cl(acc_ch+n, "axis color", 127, 127, 127);			
	}
	for(int n = 0; n < 3; n++)
	{
		sc_create_simple_chart(gyro_ch+n, 600, 0);
		sc_setViewport(gyro_ch+n, 10+cw/2, ch*(4+n)+30, cw/2-10, ch); 
		sc_setParameter_cc(gyro_ch+n, "scaling", "auto");
		int r, g, b;
		if(n == 0) r = 0, g = 255, b = 0;
		if(n == 1) r = 255, g = 255, b = 0;
		if(n == 2) r = 255, g = 0, b = 255;
		
		sc_setParameter_cl(gyro_ch+n, "color", r, g, b);
		sc_setParameter_cl(gyro_ch+n, "axis color", 127, 127, 127);			
	}

	draw_buf = (uint8_t*)malloc(WX*WY*3);
}

void draw_h_line(uint8_t *draw_buf, int w, int h, int zx, int zy, int len, int color)
{
	for(int x = 0; x < len; x++)
	{
		int xx = zx + x;
		int yy = zy;
		if(xx < 0 || xx >= w || yy < 0 || yy >= h) continue;
		int idx = yy*w + xx;
		idx *= 4;
		draw_buf[idx+3] = 0xFF;
		draw_buf[idx+0] = color&0xFF;
		draw_buf[idx+1] = (color>>8)&0xFF;
		draw_buf[idx+2] = (color>>16)&0xFF;
	}
}

void draw_v_line(uint8_t *draw_buf, int w, int h, int zx, int zy, int len, int color)
{
	for(int y = 0; y < len; y++)
	{
		int xx = zx;
		int yy = zy+y;
		if(xx < 0 || xx >= w || yy < 0 || yy >= h) continue;
		int idx = yy*w + xx;
		idx *= 4;
		draw_buf[idx+3] = 0xFF;
		draw_buf[idx+0] = color&0xFF;
		draw_buf[idx+1] = (color>>8)&0xFF;
		draw_buf[idx+2] = (color>>16)&0xFF;
	}
}
void draw_line_low(uint8_t *draw_buf, int w, int h, int x0, int y0, int x1, int y1, uint32_t color)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int yi = 1;
    if(dy < 0)
	{
		yi = -1;
		dy = -dy;
	}
    int D = 2*dy - dx;
    int y = y0;

    for(int x = x0; x <= x1; x++)
	{
		if(x >= 0 && x < w && y >= 0 && y < h)
		{
			int idx = y*w + x;
			idx *= 4;
			draw_buf[idx+3] = 0xFF;
			draw_buf[idx+0] = color&0xFF;
			draw_buf[idx+1] = (color>>8)&0xFF;
			draw_buf[idx+2] = (color>>16)&0xFF;
		}
        if(D > 0)
		{
            y = y + yi;
            D = D + 2 * (dy - dx);
		}
        else
            D = D + 2*dy;
	}
}
void draw_line_high(uint8_t *draw_buf, int w, int h, int x0, int y0, int x1, int y1, uint32_t color)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int xi = 1;
    if(dx < 0)
	{
		xi = -1;
        dx = -dx;
	}
    int D = (2 * dx) - dy;
    int x = x0;

    for(int y = y0; y <= y1; y++)
	{
		if(x >= 0 && x < w && y >= 0 && y < h)
		{
			int idx = y*w + x;
			idx *= 4;
			draw_buf[idx+3] = 0xFF;
			draw_buf[idx+0] = color&0xFF;
			draw_buf[idx+1] = (color>>8)&0xFF;
			draw_buf[idx+2] = (color>>16)&0xFF;
		}
        if(D > 0)
		{
            x = x + xi;
            D = D + 2*(dx - dy);
		}
        else
            D = D + 2*dx;
	}
}
void draw_line(uint8_t *draw_buf, int w, int h, int x0, int y0, int x1, int y1, uint32_t color)
{
	int ady = y1 - y0;
	int adx = x1 - x0;
	if(ady < 0) ady = -ady;
	if(adx < 0) adx = -adx;
    if(ady < adx)
	{
        if(x0 > x1)
            draw_line_low(draw_buf, w, h, x1, y1, x0, y0, color);
        else
            draw_line_low(draw_buf, w, h, x0, y0, x1, y1, color);
	}
    else
	{
        if(y0 > y1)
            draw_line_high(draw_buf, w, h, x1, y1, x0, y0, color);
        else
            draw_line_high(draw_buf, w, h, x0, y0, x1, y1, color);
	}
}

float fPI = 3.1415926; 

float sin_f(float x)
{

/*********************************************************
 * high precision sine/cosine
 *********************************************************/
	float res = 0;
	while (x < -3.14159265) x += 6.28318531;
	while(x >  3.14159265) x -= 6.28318531;

	if (x < 0)
	{
		res = 1.27323954 * x + .405284735 * x * x;

		if (res < 0)
			res = .225 * (res *-res - res) + res;
		else
			res = .225 * (res * res - res) + res;
	}
	else
	{
		res = 1.27323954 * x - 0.405284735 * x * x;

		if (res < 0)
			res = .225 * (res *-res - res) + res;
		else
			res = .225 * (res * res - res) + res;
	}
	return res;
}

float cos_f(float x)
{
	return sin_f(x + 1.57079632);
}

void draw_radio_q(uint8_t *draw_buf, int w, int h, int zx, int zy, int size, int radio_q)
{
	int color = 0x00FFFF;
	float rad1 = size/12;
	float rads[3];
	rads[0] = size/6;
	rads[1] = size/4;
	rads[2] = size/3;
	
	if(radio_q < 70) color = 0xFFFF00;
	if(radio_q < 40) color = 0xFF0000;
	
	for(float a = 0; a < fPI*2; a += 0.01)
	{
		float ss = sin_f(a);
		float cc = cos_f(a);
		for(float rr = 0; rr < rad1; rr += 0.5)
		{
			int xx = rr*ss + zx + size/4;
			int yy = rr*cc + zy + size/2 - rad1;
			if(xx < 0 || xx >= w || yy < 0 || yy >= h) continue;
			int idx = yy*w + xx;
			idx *= 4;
			draw_buf[idx+3] = 0xFF;
			draw_buf[idx+0] = color&0xFF;
			draw_buf[idx+1] = (color>>8)&0xFF;
			draw_buf[idx+2] = (color>>16)&0xFF;
			
		}
	}
	
	float rcoeff = 1.2;
	int color0 = color;
	for(float a = -0.7; a < 0.7; a += 0.01)
	{
		float ss = sin_f(a);
		float cc = cos_f(a);
		for(int N = 0; N < 3; N++)
		{
			color = color0;
			if(radio_q < 20 + N * 30) color = 0x888888;
			for(float rr = rads[N]*1; rr < rads[N]*rcoeff; rr += 0.5)
			{
				int xx = rr*ss + zx + size/4;
				int yy = -rr*cc + zy + size/2 - rad1;
				if(xx < 0 || xx >= w || yy < 0 || yy >= h) continue;
				int idx = yy*w + xx;
				idx *= 4;
				draw_buf[idx+3] = 0xFF;
				draw_buf[idx+0] = color&0xFF;
				draw_buf[idx+1] = (color>>8)&0xFF;
				draw_buf[idx+2] = (color>>16)&0xFF;
			}
		}
	}
}

void draw_empty_rect(uint8_t *draw_buf, int w, int h, int zx, int zy, int width, int height, int color)
{
	for(int N = 0; N < 2; N++)
	{
		int x = zx;
		if(N == 1) x = zx+width-1;
		for(int y = zy; y < zy+height; y++)
		{
			if(x < 0 || x >= w) continue;
			if(y < 0 || y >= h) continue;
			int idx = (y*w + x)*4;
			draw_buf[idx+3] = 0xFF;
			draw_buf[idx+0] = color&0xFF;
			draw_buf[idx+1] = (color>>8)&0xFF;
			draw_buf[idx+2] = (color>>16)&0xFF;
		}
	}
	for(int N = 0; N < 2; N++)
	{
		int y = zy;
		if(N == 1) y = zy+height-1;
		for(int x = zx; x < zx+width; x++)
		{
			if(x < 0 || x >= w) continue;
			if(y < 0 || y >= h) continue;
			int idx = (y*w + x)*4;
			draw_buf[idx+3] = 0xFF;
			draw_buf[idx+0] = color&0xFF;
			draw_buf[idx+1] = (color>>8)&0xFF;
			draw_buf[idx+2] = (color>>16)&0xFF;
		}
	}
}

void draw_rect(uint8_t *draw_buf, int w, int h, int zx, int zy, int width, int height, int color)
{
	for(int x = zx; x < zx+width; x++)
		for(int y = zy; y < zy+height; y++)
		{
			if(x < 0 || x >= w) continue;
			if(y < 0 || y >= h) continue;
			int idx = (y*w + x)*4;
			draw_buf[idx+3] = 0xFF;
			draw_buf[idx+0] = color&0xFF;
			draw_buf[idx+1] = (color>>8)&0xFF;
			draw_buf[idx+2] = (color>>16)&0xFF;
		}
}

void draw_battery(uint8_t *draw_buf, int w, int h, int zx, int zy, int size, float blevel)
{
	int color = 0x00FFAA;
	int hsize = size/5;
	draw_h_line(draw_buf, w, h, zx, zy, size, color);
	draw_h_line(draw_buf, w, h, zx, zy+hsize, size, color);
	draw_v_line(draw_buf, w, h, zx, zy, hsize, color);
	draw_v_line(draw_buf, w, h, zx+size, zy, hsize, color);

	if(blevel > 0.8) color = 0x00FF00;
	else if(blevel > 0.6) color = 0x00AAAA;
	else if(blevel > 0.4) color = 0xFFFF00;
	else color = 0xFF0000;
	
	if(blevel < 0.15)
	{
		blevel = 0.15;
		struct timeval cur_time;
		gettimeofday(&cur_time, NULL);
		if((cur_time.tv_usec%500000) > 250000) return;
	}
	
	for(int x = 1; x < size*blevel - 1; x++)
	{
		if(x == size*0.2 || x == size*0.2+1) continue;
		if(x == size*0.4 || x == size*0.4+1) continue;
		if(x == size*0.6 || x == size*0.6+1) continue;
		if(x == size*0.8 || x == size*0.8+1) continue;
		draw_v_line(draw_buf, w, h, zx+x, zy+1, hsize-2, color);
	}
}

void draw_text(uint8_t *draw_buf, int w, int h, int x, int y, int zoom, char *txt, int color)
{
	int p = 0;
	int out_x = x;
	int out_y = y;
	while(txt[p] != 0)
	{
		int n = txt[p];
		p++;
		if(n < 32 || n > 32+96) continue;
		n -= 32;
		for(int rw = 0; rw < 13; rw++)
		{
			uint8_t vv = font_a[n][rw];
			for(int ln = 0; ln < 8; ln++)
			{
				if((vv & (1<<ln)) > 0)
					for(int dx = 0; dx < zoom; dx++)
						for(int dy = 0; dy < zoom; dy++)
						{
							int xx = out_x + (7-ln)*zoom + dx;
							int yy= out_y + (12-rw)*zoom + dy;
							if(xx < 0 || xx >= w || yy < 0 || yy >= h) continue;
							int idx = yy*w + xx;
							idx *= 4;
							draw_buf[idx+3] = 0xFF;
							draw_buf[idx+0] = color&0xFF;
							draw_buf[idx+1] = (color>>8)&0xFF;
							draw_buf[idx+2] = (color>>16)&0xFF;
						}
			}
		}
		out_x += (8+2)*zoom;
	}
}

float am_states_buf[123];
int am_cur_size = 0;
int am_cur_N = 0;

void draw_fill_adaptive_map_state(int am_size, int am_N, float *am_states)
{
	for(int x = 0; x < am_size; x++)
		am_states_buf[x] = am_states[x];
	am_cur_size = am_size;
	am_cur_N = am_N;
}  

void draw_adaptive_map(uint8_t *draw_buf, int w, int h)
{
	if(am_cur_size == 0) return;
	int dx = 30;
	int sq_sz = 20;
	int dy = h/2;
	
	for(int n = 0; n < am_cur_size; n++)
	{
		if(n == am_cur_N)
			draw_rect(draw_buf, w, h, dx + sq_sz*n, dy, sq_sz, sq_sz, 0xFF0000);
		else
			draw_empty_rect(draw_buf, w, h, dx + sq_sz*n, dy, sq_sz, sq_sz, 0xFF0000);
	}
	for(int n = 0; n < am_cur_size; n++)
	{
		float st = am_states_buf[n];
		int r, g, b;
		if(st < 0.5)
		{
			g = 50;
			r = 0;
			b = (0.5 - st)*255;
			if(b > 255) b = 255;
		}
		else
		{
			b = 0;
			g = 50;
			r = (st - 0.5)*255;
			if(r > 255) r = 255;
		}
		int cl = (r<<16) | (g<<8) | b;
		draw_rect(draw_buf, w, h, dx + sq_sz*n, dy+sq_sz+5, sq_sz, sq_sz, cl);
	}
}

int game_mode_on = 0;

void draw_game_mode_select(int onoff)
{
	game_mode_on = onoff;
}

int game_gesture_queue[256];
int game_gestures_count = 0;
int game_time = 0;
int game_highlight = 0;

void draw_game_mode_fill(int *gesture_queue, int queue_size)
{
	game_gestures_count = queue_size;
	for(int x = 0; x < queue_size; x++)
		game_gesture_queue[x] = gesture_queue[x];
}
void draw_game_mode_update(int cur_time, int highlight_gesture)
{
	game_time = cur_time;
	game_highlight = highlight_gesture;
}

void draw_gesture(uint8_t *draw_buf, int w, int h, int gesture, int xpos, int ypos, int highlight)
{
	uint32_t col;
	if(gesture == 1)
	{
		col = 0xFF00FF;
		if(highlight) col |= 0xFF00;
		draw_rect(draw_buf, w, h, xpos, ypos+10, 40, 20, col);
		draw_rect(draw_buf, w, h, xpos, ypos, 7, 20, col);
		draw_rect(draw_buf, w, h, xpos+10, ypos-5, 7, 20, col);
		draw_rect(draw_buf, w, h, xpos+18, ypos-5, 7, 20, col);
		draw_rect(draw_buf, w, h, xpos+26, ypos-3, 6, 20, col);
		draw_rect(draw_buf, w, h, xpos+34, ypos-1, 5, 20, col);
	}
	if(gesture == 2)
	{
		col = 0xFF0055;
		if(highlight) col |= 0xFF00;
		draw_rect(draw_buf, w, h, xpos, ypos+10, 30, 20, col);
		draw_line(draw_buf, w, h, xpos+10, ypos+10, xpos, ypos-20, col);
		draw_line(draw_buf, w, h, xpos+11, ypos+10, xpos+1, ypos-20, col);
		draw_line(draw_buf, w, h, xpos+10, ypos+10, xpos+20, ypos-20, col);
		draw_line(draw_buf, w, h, xpos+11, ypos+10, xpos+21, ypos-20, col);
	}
	if(gesture == 3)
	{
		col = 0x00AAFF;
		if(highlight) col |= 0xFF0000;
		draw_rect(draw_buf, w, h, xpos, ypos+10, 25, 30, col);
		draw_rect(draw_buf, w, h, xpos, ypos-10, 8, 20, col);
	}
	if(gesture == 4)
	{
		col = 0x55FF00;
		if(highlight) col |= 0xFF;
		draw_rect(draw_buf, w, h, xpos, ypos+10, 40, 20, col);
		draw_rect(draw_buf, w, h, xpos, ypos, 8, 20, col);
		draw_rect(draw_buf, w, h, xpos+10, ypos-10, 7, 20, col);
		draw_rect(draw_buf, w, h, xpos+18, ypos-10, 7, 20, col);
	}
}
int game_mode_show_gesture = 0;
int game_mode_train_gesture = 0;
void draw_game_mode_gesture(int gesture)
{
	game_mode_show_gesture = gesture;
}
void draw_game_mode_train_gesture(int gesture)
{
	game_mode_train_gesture = gesture;
}

void draw_game_state(uint8_t *draw_buf, int w, int h)
{
	int line_pos = 150;
	draw_h_line(draw_buf, WX, WY, 0, line_pos, 600, 0xFFAA77);
	for(int g = 0; g < game_gestures_count; g++)
	{
		int g_time = game_gesture_queue[g]&0xFFFFFF;
		int gesture = game_gesture_queue[g]>>24;
		int ypos = line_pos + g_time - game_time;
		if(ypos < 0 || ypos > WY) continue;
		int xpos = 0;
		xpos = 100*gesture;
		int need_highlight = 0;
		if(game_highlight == gesture && ypos > line_pos - 30 && ypos < line_pos + 30)
		{
			need_highlight = (game_time/8)%2;
		}
		draw_gesture(draw_buf, w, h, gesture, xpos, ypos, need_highlight);
	}
	if(game_mode_show_gesture > 0)
		draw_gesture(draw_buf, w, h, game_mode_show_gesture, 600, 200, 0);
	if(game_mode_train_gesture > 0)
		draw_gesture(draw_buf, w, h, game_mode_train_gesture, 600, 100, 1);
	if(game_mode_train_gesture == 0)
		draw_empty_rect(draw_buf, w, h, 600, 100, 30, 30, 0xFF00FF);
}

int recognized_gesture = 0;

void draw_set_recognized_gesture(int gesture)
{
	recognized_gesture = gesture;
}

float kmeans_clust[8*32];
float kmeans_clust_dist[32];
int kmeans_K = 0;

void draw_set_kmeans(float *data, float *clust_dist, int K)
{
	for(int x = 0; x < K*8; x++)
		kmeans_clust[x] = data[x];
	for(int x = 0; x < K; x++)
		kmeans_clust_dist[x] = clust_dist[x];
	kmeans_K = K;
}

void draw_multidim_data(uint8_t *draw_buf, int w, int h)
{
	float scale = 1.0 / 800.0;
	float multidim_data[8];
	float X[8];
	for(int N = 0; N < 1 + kmeans_K; N++)
	{
		if(N == 0)
			data_get_cur_data(multidim_data);
		else
		{
//			printf("filling clusters %d\n", N-1);

			for(int n = 0; n < 8; n++)
			{
//				printf("%d: %g \n", n, kmeans_clust[8*(N-1) + n]);
				multidim_data[n] = kmeans_clust[8*(N-1) + n];
			}
			
		}
			//data_get_prev_data(multidim_data, N);
		float intens = 0;
		float mean_clust[8] = {0,0,0,0,0,0,0,0};
		float sdv_clust[8] = {0,0,0,0,0,0,0,0};
		if(kmeans_K > 0)
		{
			for(int k = 0; k < kmeans_K; k++)
				for(int x = 0; x < 8; x++)
					mean_clust[x] += kmeans_clust[8*k + x];
			for(int x = 0; x < 8; x++)
				mean_clust[x] /= 8;
			for(int k = 0; k < kmeans_K; k++)
				for(int x = 0; x < 8; x++)
				{
					float dd = kmeans_clust[8*k + x] - mean_clust[x];
					sdv_clust[x] += dd*dd;
				}
			for(int x = 0; x < 8; x++)
			{
				sdv_clust[x] /= 8;
				sdv_clust[x] = sqrt(sdv_clust[x]);
			}
		}
		for(int x = 0; x < 8; x++)
		{
			X[x] = multidim_data[x]*scale;
			if(kmeans_K > 0)
				X[x] = 1 + (multidim_data[x] - mean_clust[x]) / (1.0 + sdv_clust[x]*3.0);
			
			if(X[x] < 0) X[x] = 0;
			if(X[x] > 2) X[x] = 2;
			intens += X[x];
		}
			
		int xpos = 200;
		int ypos = 200;
		int size = 200;
		int var_sz = 100;
		int col_b = 255;
		int col_r = 255;
		if(N > 0)
		{
			int id = N-1;
			
			xpos = 600 + (id%3)*150;
			ypos = 150 + (id/3)*200;
			size = 80;
		}
//		if(N > 0) col_r = 127, col_b = 127;
		uint32_t cl = (col_r<<16) | col_b;
		int xx[4];
		int yy[4];
		int xx0[4];
		int yy0[4];
		float avg_xx = 0;
		float avg_yy = 0;
		for(int n = 0; n < 4; n++)
		{
			xx[n] = xpos + (n==1 || n == 2) * size;
			yy[n] = ypos + (n==2 || n == 3) * size;
			xx0[n] = xx[n];
			yy0[n] = yy[n]; 
			
			xx[n] += var_sz * X[n*2];
			yy[n] += var_sz * X[n*2+1];
			avg_xx += xx[n];
			avg_yy += yy[n];
		}
		avg_xx *= 0.25;
		avg_yy *= 0.25;
		for(int n = 0; n < 4; n++)
		{
			xx0[n] += avg_xx - xpos - size/2;
			yy0[n] += avg_yy - ypos - size/2;
		}
		if(N > 0)
			cl = 0x00FFFF;
		for(int n = 0; n < 4; n++)
		{
//			printf("ln %d %d %d %d\n", xx[n], yy[n], xx[(n+1)%4], yy[(n+1)%4]);
			if(N == 0)
				draw_line(draw_buf, w, h, xx[n], yy[n], xx[(n+1)%4], yy[(n+1)%4], cl);
			else
				draw_line(draw_buf, w, h, xx[n] - avg_xx + xpos, yy[n] - avg_yy + ypos, xx[(n+1)%4] - avg_xx + xpos, yy[(n+1)%4] - avg_yy + ypos, cl);
		}
		if(N > 0)
		{
			float len = intens*20;
			float vx = xx[2] - xx[0];
			float vy = yy[2] - yy[0];
			float rr = sqrt(vx*vx + vy*vy) + 0.01;
			vx /= rr;
			vy /= rr;
			draw_line(draw_buf, w, h, xx[0] - avg_xx + xpos, yy[0] - avg_yy + ypos, xx[0] - avg_xx + xpos + vx*len, yy[0] - avg_yy + ypos + vy*len, cl);
			
			float max_d = kmeans_clust_dist[0] + 0.0001;
			for(int k = 0; k < kmeans_K; k++) if(kmeans_clust_dist[k] > max_d) max_d = kmeans_clust_dist[k];
//			float hgt = 10000.0 / (1.0 + kmeans_clust_dist[N-1]);
			float hgt = 200.0 * (1.0 - kmeans_clust_dist[N-1] / max_d);
			if(hgt > 200) hgt = 200;
			draw_rect(draw_buf, w, h, avg_xx, avg_yy-hgt, 20, hgt, cl);
		}
		cl = 0x00FF00;
//		if(N > 0) cl = 0x007F00;
		if(N == 0)for(int n = 0; n < 4; n++) 
			draw_line(draw_buf, w, h, xx0[n], yy0[n], xx0[(n+1)%4], yy0[(n+1)%4], cl);
	}
}

float mlp_draw_outs[64];
int mlp_draw_N = 0;

void draw_set_mlp_out(float *mlp_out, int N)
{
	for(int x = 0; x < N; x++)
		mlp_draw_outs[x] = mlp_out[x];
	mlp_draw_N = N;
}

void draw_mlp_outs(uint8_t *draw_buf, int w, int h)
{
	int DX = 100;
	int DY = 100;
	int bar_w = 30;
	int max_hgt = 80;
	
	float max_v = mlp_draw_outs[0];
	int max_id = 0;

	for(int n = 0; n < mlp_draw_N; n++)
		if(mlp_draw_outs[n] > max_v) max_v = mlp_draw_outs[n], max_id = n;
	
	for(int n = 0; n < mlp_draw_N; n++)
	{
		float val = mlp_draw_outs[n];
		int hgt = val*max_hgt;
		int cl = 0x00FF00;
		if(n == max_id) cl = 0xFFFF00;
		draw_rect(draw_buf, w, h, DX + (bar_w + 10)*n, DY-hgt, bar_w, hgt, cl);
	}
}

void draw_loop(uint32_t *scr_buf)
{
	if(!draw_inited) return;
	int clr = 0xFF222222;
	for(int x = 0; x < WX*WY; x++)
		scr_buf[x] = clr;

	if(1)
	{
		float bat_lvl = data_get_battery();
		draw_battery(scr_buf, WX, WY, WX-200, 50, 100, bat_lvl);
		draw_multidim_data(scr_buf, WX, WY);
		draw_mlp_outs(scr_buf, WX, WY);
		return;
	}
	if(game_mode_on)
	{
		float bat_lvl = data_get_battery();
		draw_battery(scr_buf, WX, WY, WX-200, 50, 100, bat_lvl);
		draw_game_state(scr_buf, WX, WY);
		return;
	}

	for(int N = 0; N < 8; N++)
		sc_draw_buf(emg_hpart+N, scr_buf, WX, WY);
	draw_gesture(scr_buf, WX, WY, recognized_gesture, WX-200, WY-300, 0);
		

	if(0)
	{
	for(int N = 0; N < 4; N++)
	{
		if(draw_raw_data)
			sc_draw_buf(raw_emg+N, scr_buf, WX, WY);
		else
			spg_draw(emg_sp + N, scr_buf, WX, WY);
	}
	draw_adaptive_map(scr_buf, WX, WY);
	}

	for(int N = 0; N < 3; N++)
	{
		sc_draw_buf(acc_ch+N, scr_buf, WX, WY);
		sc_draw_buf(gyro_ch+N, scr_buf, WX, WY);
	}

	float bat_lvl = device_get_battery();
//	bat_lvl = (bat_lvl / 4096.0)*3300.0*2.95;
//	printf("bat %g - %g\n", get_cut_bat(), bat_lvl);
//	bat_lvl = (bat_lvl - 3200.0) / 900.0;
//	if(bat_lvl < 0) bat_lvl = 0;
//	if(bat_lvl > 1) bat_lvl = 1;
	draw_battery(scr_buf, WX, WY, WX-200, 50, 100, bat_lvl);
}
