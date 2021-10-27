#pragma GCC diagnostic error "-Wimplicit-function-declaration"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "serial_functions.h"
#include "device_functions.h"

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <sys/time.h> 
#include <X11/Xlib.h>

#include "save_processing.h"
#include "drawing.h"

#include "qr_factorize.h"

int use_fullscreen = 0;

SDL_Surface *scr;

int W;
int H;

void get_scr_res(int *w, int *h)
{
    Display *display;
    Screen *screen;

    // open a display
    display = XOpenDisplay(NULL);

    // return the number of available screens
    int count_screens = ScreenCount(display);

	if(count_screens < 1) printf("no screen found!\n");
	screen = ScreenOfDisplay(display, 0);
	printf("screen %dX%d\n", screen->width, screen->height);
    // close the display
	*w = screen->width;
	*h = screen->height;
    XCloseDisplay(display);	
}

void prepareOut(int w, int h)
{	
	SDL_Init(SDL_INIT_VIDEO);

	W = w;
	H = h;
	int ok;
	if(use_fullscreen)
		ok = SDL_VideoModeOK(W, H, 32, SDL_DOUBLEBUF|SDL_HWSURFACE|SDL_FULLSCREEN);
	else
		ok = SDL_VideoModeOK(W, H, 32, SDL_DOUBLEBUF|SDL_HWSURFACE);
	printf("mode isok: %d\n", ok);
	if(use_fullscreen)
		scr = SDL_SetVideoMode(W, H, 32, SDL_DOUBLEBUF|SDL_HWSURFACE|SDL_FULLSCREEN);
	else
		scr = SDL_SetVideoMode(W, H, 32, SDL_DOUBLEBUF|SDL_HWSURFACE);
	if(scr == NULL)
		printf("can't create video out\n");
	else
		printf("scr: %d x %d, %d\n", scr->w, scr->h, scr->format->BitsPerPixel);
}

int outBuf = 0;


void drawFrameM(uint8_t *drawBuf)
{
	SDL_LockSurface(scr);
	uint8_t *impix = (uint8_t*)scr->pixels;
	memcpy(impix, drawBuf, W*H*4);
	SDL_UnlockSurface(scr);
}

void drawFrame(uint8_t *drawBuf)
{
	SDL_LockSurface(scr);
	uint8_t *impix = (uint8_t*)scr->pixels;
	memcpy(impix, drawBuf, W*H*4);
	SDL_UnlockSurface(scr);
	SDL_Flip(scr);
}

TTF_Font *font = NULL; 
TTF_Font *fontSS = NULL; 
TTF_Font *fontSM = NULL; 
TTF_Font *fontSL = NULL; 

enum
{
	mode_serial = 0,
	mode_telnet
};

int emulated_mode = 0;

void main_loop(uint32_t* scr_buf)
{
	draw_loop(scr_buf);
	if(emulated_mode)
		device_emulate_step();
	else
		serial_main_loop();
}
#include "pca_processor.h"

float pca_sample_data[21*20] = {
1.355398384,1.855398384,1.194326925,1.694326925,1.245091803,1.269578406,1.769578406,1.008569164,1.508569164,1.270962641,1.770962641,1.305253314,1.377767271,1.877767271,0.362564971,0.862564971,0.018640427,0.518640427,0.141775135,0.31199387,0.81199387,
0.468017899,0.968017899,0.345197471,0.845197471,0.499484174,0.043460726,0.543460726,1.242070991,1.742070991,1.126649743,1.626649743,1.135932591,1.496907106,1.996907106,0.003283945,0.503283945,0.010236346,0.510236346,0.413119674,0.413971662,0.913971662,
1.104673551,1.604673551,1.234425148,1.734425148,1.195437475,1.396538386,1.896538386,1.036208694,1.536208694,1.275875717,1.775875717,1.326348737,1.018970062,1.518970062,1.389841833,1.889841833,1.284184248,1.784184248,1.35513169,1.222155152,1.722155152,
3.091759922,3.591759922,3.408942775,3.908942775,3.384487161,3.401118373,3.901118373,0.060393263,0.560393263,0.488165655,0.988165655,0.000948232,0.227534142,0.727534142,3.237208139,3.737208139,3.018610047,3.518610047,3.433719652,3.008308391,3.508308391,
7.423130337,7.923130337,7.088488859,7.588488859,7.100479331,7.102792547,7.602792547,0.104278574,0.604278574,0.003791937,0.503791937,0.273062809,0.240466944,0.740466944,7.068574544,7.568574544,7.475870625,7.975870625,7.079527441,7.285700592,7.785700592,
9.329782976,9.829782976,9.367496962,9.867496962,9.334956581,9.013695803,9.513695803,1.301912942,1.801912942,1.318178503,1.818178503,1.001845926,1.254130415,1.754130415,9.309425657,9.809425657,9.003999478,9.503999478,9.093795134,9.077164332,9.577164332,
6.31278568,6.81278568,6.441074326,6.941074326,6.335146084,6.006251892,6.506251892,0.287600108,0.787600108,0.055208525,0.555208525,0.4132655,0.002949054,0.502949054,6.499334685,6.999334685,6.412249224,6.912249224,6.407437411,6.354819526,6.854819526,
4.235015601,4.735015601,4.316022256,4.816022256,4.269757504,4.463448108,4.963448108,0.046026309,0.546026309,0.469750149,0.969750149,0.064891945,0.226734295,0.726734295,4.403094124,4.903094124,4.12558547,4.62558547,4.19736108,4.404335696,4.904335696,
2.462931835,2.962931835,2.030900886,2.530900886,2.16334131,2.233202566,2.733202566,1.350544234,1.850544234,1.056226315,1.556226315,1.447158433,1.227413409,1.727413409,2.299441732,2.799441732,2.454078377,2.954078377,2.102398006,2.352375702,2.852375702,
1.307054461,0.807054461,1.446211724,0.946211724,1.183136074,1.15878546,0.65878546,0.111203413,-0.388796587,0.070994723,-0.429005277,0.437361101,0.295931684,-0.204068316,1.352880482,0.852880482,1.489773306,0.989773306,1.432060834,1.339500869,0.839500869,
1.226089146,0.726089146,1.124622139,0.624622139,1.411909074,1.331447691,0.831447691,1.425398221,0.925398221,1.473740306,0.973740306,1.161498973,1.012090752,0.512090752,1.344583353,0.844583353,1.439887365,0.939887365,1.183983451,1.141140663,0.641140663,
1.082043723,0.582043723,1.024630356,0.524630356,1.097489804,1.317442588,0.817442588,1.282926151,0.782926151,1.41595788,0.91595788,1.139255031,1.350797765,0.850797765,1.363516012,0.863516012,1.394184564,0.894184564,1.499937914,1.214389008,0.714389008,
0.448164919,-0.051835081,0.467446984,-0.032553016,0.363884496,0.144373882,-0.355626118,3.026679345,2.526679345,3.420906766,2.920906766,3.189901972,3.370143135,2.870143135,3.010886552,2.510886552,3.344221508,2.844221508,3.045581418,3.172236588,2.672236588,
0.233519694,-0.266480306,0.267557938,-0.232442062,0.125943825,0.330681689,-0.169318311,7.048769479,6.548769479,7.250352321,6.750352321,7.221508761,7.359471686,6.859471686,7.368303694,6.868303694,7.440054536,6.940054536,7.483144438,7.03920846,6.53920846,
1.472016133,0.972016133,1.461439624,0.961439624,1.250262581,1.42089083,0.92089083,9.419352075,8.919352075,9.272820007,8.772820007,9.0686153,9.444377831,8.944377831,9.168785632,8.668785632,9.377813953,8.877813953,9.421838719,9.038914533,8.538914533,
0.302323597,-0.197676403,0.014860282,-0.485139718,0.119788777,0.077804543,-0.422195457,6.279904221,5.779904221,6.196824403,5.696824403,6.219543265,6.20375489,5.70375489,6.456495649,5.956495649,6.450269756,5.950269756,6.226056902,6.033578939,5.533578939,
0.084448342,-0.415551658,0.351836792,-0.148163208,0.213097435,0.361274506,-0.138725494,4.136559964,3.636559964,4.451892737,3.951892737,4.32793968,4.035681275,3.535681275,4.226005489,3.726005489,4.319795765,3.819795765,4.038685294,4.106379,3.606379,
1.367544348,0.867544348,1.234283513,0.734283513,1.434706684,1.294397904,0.794397904,2.466139903,1.966139903,2.442851444,1.942851444,2.313951478,2.177196548,1.677196548,2.339981275,1.839981275,2.134642229,1.634642229,2.427319676,2.340406034,1.840406034,
0.03992922,-0.46007078,0.228300337,-0.271699663,0.179047524,0.43154261,-0.06845739,1.067126871,0.567126871,1.167952922,0.667952922,1.366969851,1.006555733,0.506555733,1.474634118,0.974634118,1.060597694,0.560597694,1.111982827,1.25217034,0.75217034,
1.202544227,0.702544227,1.19691987,0.69691987,1.233856165,1.363911793,0.863911793,1.320005943,0.820005943,1.202208368,0.702208368,1.3201451,1.228369051,0.728369051,1.272236696,0.772236696,1.441172651,0.941172651,1.265180647,1.466939922,0.966939922
};

uint8_t encode_16_8(int val) //encodes signed 16 bit int into unsigned 8 bit with progressive scaling
{
	int vp = val;
	if(vp < 0) vp = -vp;
	if(vp > 32767) vp = 32767;
	vp>>=1;
	int pow4 = 0;
	while(vp >= (1<<(pow4*2))) pow4++;
	int div = 1<<(pow4*2);
	float res = (float)vp / (float)div; //between 0.25 and 1.0, stricly less than 1
	res -= 0.25; //now between 0 and 0.75
	if(res < 0) res = 0; //for zero case
	//and now we have 4 bits to encode res, 3 bits go for pow2 and one for sign
	int rr = res*20.0f + 0.5f;
	if(rr > 15) rr = 15; //precaution, in fact never should be
	if(val < 0) pow4 |= 0b1000;
	return (pow4<<4) | rr;
}
int decode_8_16(uint8_t val) //decodes 8-bit compressed value into signed 16 bit
{
	int pow4 = (val>>4)&0b111;
	int is_negative = val>>7;
	int rr = val&0xF;
	float r1 = 1<<(pow4*2);
	float r2 = rr;
	return 2*r1 * (0.25f + r2/20.0f) * (1 - 2*is_negative);
}
int main( int   argc, char *argv[] )
{
	float rel_diff_sum = 0;
	float rel_dz = 0;
	float max_diff_sum = 0;
	if(1)for(int tv = -32768; tv < 32768; tv++)
	{
		uint8_t v1 = encode_16_8(tv);
		int dv = decode_8_16(v1);
		float r1 = tv;
		float r2 = dv;
		float rel_diff = fabs(r1-r2) / (fabs(tv) + 1);
		if(rel_diff > max_diff_sum && (tv < -16 || tv > 16)) max_diff_sum = rel_diff;
		rel_diff_sum += rel_diff;
		rel_dz++;
		if(tv%15 == 0) printf("%d - %d\n", tv, dv);
	}
	rel_diff_sum /= rel_dz;
	printf("avg rel diff %g max %g\n", rel_diff_sum, max_diff_sum);
	uint8_t v2 = encode_16_8(150);
	uint8_t v3 = encode_16_8(1500);
	uint8_t v4 = encode_16_8(15000);
	uint8_t v5 = encode_16_8(32000);
	printf("v2 %d %d\n", v2, decode_8_16(v2));
	printf("v3 %d %d\n", v3, decode_8_16(v3));
	printf("v4 %d %d\n", v4, decode_8_16(v4));
	printf("v5 %d %d\n", v5, decode_8_16(v5));
	//PCA test
	int vsize = 2, vcount = 6;
	float A[12] = {-1, 1, -2, -1, -3, -2, 1, 1, 2, 1, 3, 2};
//	int vsize = 4, vcount = 5;
//	float A[4*5] = {-0.9, 1.03, -1.34, -1.31,
//				-1.14, -0.12, -1.34, -1.31, 
//				-1.38, 0.337, -1.4, -1.31,
//				-1.5, 0.106, -1.28, -1.31,
//				-1.02, 1.26, -1.34, -1.31 };
	sPCA_data pca_test;
	pca_process_data(A, vsize, vcount, &pca_test);
//	pca_process_data(pca_sample_data, 21, 20, &pca_test);

	
	printf("PCA l:");
	for(int i = 0; i < vsize; i++)
		 printf("%g ", pca_test.ev[pca_test.ev_sorted_idx[i]]);
	printf("\n");
	printf("V:\n");
	for(int i = 0; i < vsize; i++)
	{ 
		for(int j = 0; j < vsize; j++)
		{
			int e_idx = pca_test.ev_sorted_idx[j];
			printf("%g ", pca_test.V[i*vsize + e_idx]);
		}
		printf("\n");
	}
	float tr1[21];
	printf("\n");
	for(int x = 0; x < vcount; x++)
	{ 
//		pca_get_components(A+x*2, tr1, 2, &pca_test);
		pca_get_components(A+x*vsize, tr1, vsize, &pca_test);
		printf("x%d: ", x);
		for(int i = 0; i < vsize; i++)
			printf("%g ", tr1[i]);
		printf("\n");
	}
//	return 0;
/*  //QR test
	float A[9] = {1.1,1,0,1,0,1,0,1,1};
	float V[9];
	float vals[3];
	calc_eigenvectors(A, V, vals, 3);
	for(int j = 0; j < 3; j++)
		for(int i = 0; i < 3; i++)
			V[i*3+j] /= V[2*3+j];

	printf("V:\n");
	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
			printf("%g ", V[i*3+j]);
		printf("\n");
	}
	printf("ev: %g %g %g\n", vals[0], vals[1], vals[2]);
	return 0;*/
	int d;
	d = (1,2,3,4);
	printf("%d\n", d);
	
	serial_functions_init();
	serial_main_init();
	open_device();

	SDL_Surface *msg = NULL;
	
	int w = 1300;
	int h = 800;
	if(use_fullscreen)
		get_scr_res(&w, &h);

	int nn_mode = 0;
	int nn_auto = 0;

	prepareOut(w, h);
	printf("size %d x %d\n", w, h);
	uint8_t *draw_pix = (uint8_t*)malloc(w*h*5);
	draw_init(w, h);
	//text output
	SDL_Color textColor = { 255, 255, 255 }; 	
	TTF_Init();
	font = TTF_OpenFont( "../Ubuntu-R.ttf", 14 ); //filename, wanted font size
	fontSS = TTF_OpenFont( "../Ubuntu-R.ttf", h/24 );
	fontSM = TTF_OpenFont( "../Ubuntu-R.ttf", h/12 );
	fontSL = TTF_OpenFont( "../Ubuntu-R.ttf", h/8 );
	if(font == NULL ) 
	{
		font = TTF_OpenFont( "Ubuntu-R.ttf", 14 ); //filename, wanted font size
		fontSS = TTF_OpenFont( "Ubuntu-R.ttf", h/24 );
		fontSM = TTF_OpenFont( "Ubuntu-R.ttf", h/12 );
		fontSL = TTF_OpenFont( "Ubuntu-R.ttf", h/8 );
		if(font == NULL ) 
			printf("can't load font!\n");
	}
	struct timeval curTime, prevTime, zeroTime;
	gettimeofday(&prevTime, NULL);
	gettimeofday(&zeroTime, NULL);

	int mbtn_l = 0, mbtn_r = 0, mbtn_m = 0;
	
	int need_send = 0, done = 0;
	
	uint8_t prev_keys[512];
	for(int x = 0; x < 512; x++)
		prev_keys[x] = 0;
	
	float cl_err = 0, avg_err = 0;
	while( !done ) 
	{ 
//===========INPUT PROCESSING==================================
		int mouse_x = 0, mouse_y = 0;
		SDL_Event event;
		static float col_scale = 1000;
		while( SDL_PollEvent( &event ) ) 
		{ 
			if( event.type == SDL_QUIT ) 
			{
				done = 1; 
				printf("SDL quit message\n");
			} 
			if( event.type == SDL_KEYDOWN ) 
			{
				if(event.key.keysym.sym == SDLK_ESCAPE) 
//				if(event.key.keysym.sym == SDL_SCANCODE_ESCAPE) 
				{ 
					done = 1;
					printf("esc pressed\n");
				} 
				if(event.key.keysym.sym == SDLK_e) 
				{ 
					if(!emulated_mode)
					{
						device_emulate_start("../data/uemg_data_new_5_g_gr1.dat");
						emulated_mode = 1;
					}
					else
					{
						device_emulate_end();
						emulated_mode = 0;
					}
				} 
				if(event.key.keysym.sym == SDLK_n && !nn_mode) 
				{
					sp_open_file("../data/uemg_data_new_5_g_gr1.dat");
					sp_prepare_dataset();
					nn_mode = 1;
				}
				if(event.key.keysym.sym == SDLK_m && nn_mode) 
				{
					nn_auto = !nn_auto;
				}
				if(event.key.keysym.sym == SDLK_r) 
				{
					device_save_on();
				}
				if(event.key.keysym.sym == SDLK_s) 
				{
					device_save_off();
				}
				if(event.key.keysym.sym == SDLK_a) 
				{
					col_scale *= 1.1;
					for(int n = 0; n < 4; n++)
						spg_set_parameter_float(emg_sp+n, "color scale", col_scale);
				}
				if(event.key.keysym.sym == SDLK_z) 
				{
					col_scale *= 0.9;
					for(int n = 0; n < 4; n++)
						spg_set_parameter_float(emg_sp+n, "color scale", col_scale);
				}
				if(event.key.keysym.sym == SDLK_q) 
				{
					for(int n = 0; n < 4; n++)
						spg_set_parameter_str(emg_sp+n, "scaling", "normal");
				}
				if(event.key.keysym.sym == SDLK_w) 
				{
					for(int n = 0; n < 4; n++)
						spg_set_parameter_str(emg_sp+n, "scaling", "adaptive");
				}
				if(event.key.keysym.sym == SDLK_g) 
				{
					device_game_mode_switch();
				}

				if(event.key.keysym.sym == SDLK_1) 
				{
					device_train_ml(1);
				}
				if(event.key.keysym.sym == SDLK_2) 
				{
					device_train_ml(2);
				}
				if(event.key.keysym.sym == SDLK_3) 
				{
					device_train_ml(3);
				}
				if(event.key.keysym.sym == SDLK_4) 
				{
					device_train_ml(4);
				}
				if(event.key.keysym.sym == SDLK_5) 
				{
					device_train_ml(5);
				}
				if(event.key.keysym.sym == SDLK_6) 
				{
					device_train_ml(0);
				}
				if(event.key.keysym.sym == SDLK_7) 
				{
					device_train_ml(-1);
				}
				if(event.key.keysym.sym == SDLK_8) 
				{
					device_train_ml(-2);
				}
				
				if(event.key.keysym.sym == SDLK_0) 
				{
					for(int x = 0; x < 9; x++)
						device_fix_kmeans(x);
				}
				if(event.key.keysym.sym == SDLK_p) 
				{
					device_toggle_hand_control();
				}
				if(event.key.keysym.sym == SDLK_k) 
					device_save_ml_state();
				if(event.key.keysym.sym == SDLK_l) 
					device_load_ml_state(); 
				
			}
		} 

//==============MOUSE==========================================
		uint8_t mstate;
		mstate = SDL_GetMouseState(&mouse_x, &mouse_y);

		int lclk = 0;
		if (mstate & SDL_BUTTON (1))
		{
			if(mbtn_l == 0) lclk = 1;
			mbtn_l = 1;
		}
		else
		{
			mbtn_l = 0;
		}
		if (mstate & SDL_BUTTON (2)) mbtn_m = 1;
		if (mstate & SDL_BUTTON (3)) mbtn_r = 1;
		
		if(lclk)
		{
			;
		}
//==============MOUSE END======================================

//==============KEYBOARD==========================================
//		uint8_t *keys = (uint8_t *)SDL_GetKeyboardState( NULL );
		uint8_t *keys = (uint8_t *)SDL_GetKeyState( NULL );
		int mark_key = 0;
		for(int x = 0; x < 512; x++)
		{
			prev_keys[x] = keys[x];
		}
		if(keys[SDLK_1]) 
			mark_key = 1;
		if(keys[SDLK_2]) 
			mark_key = 2;
		if(keys[SDLK_3]) 
			mark_key = 3;
		if(keys[SDLK_4]) 
			mark_key = 4;
		if(keys[SDLK_5]) 
			mark_key = 5;
		if(keys[SDLK_6]) 
			mark_key = 6;
		if(keys[SDLK_7]) 
			mark_key = 7;
		if(keys[SDLK_8]) 
			mark_key = 8;
		if(keys[SDLK_9]) 
			mark_key = 9;
		if(keys[SDLK_0]) 
			mark_key = 10;
		if((keys[SDLK_m] || nn_auto) && nn_mode)
		{
			sp_train_mlp(1000, &cl_err, &avg_err);
		}
		
		if(nn_mode)
		{
			if(keys[SDLK_1]) 
				sp_set_train_speed(0.03);
			if(keys[SDLK_2]) 
				sp_set_train_speed(0.003);
			if(keys[SDLK_3]) 
				sp_set_train_speed(0.0003);
			if(keys[SDLK_4]) 
				sp_set_train_speed(0.00003);
		}
				
		device_save_mark(mark_key);
//		
//		if(keys[SDLK_ESCAPE])
////		if(keys[SDL_SCANCODE_ESCAPE])
//			done = 1;
//		if(keys[SDL_SCANCODE_X]) 

//==============KEYBOARD END======================================
		
//===========INPUT PROCESSING END==============================

		gettimeofday(&curTime, NULL);
		int dT = (curTime.tv_sec - prevTime.tv_sec) * 1000000 + (curTime.tv_usec - prevTime.tv_usec);
		prevTime = curTime;

		if(mbtn_l == 0)
		{
			memset(draw_pix, 0, w*h*4);
			main_loop((uint32_t*)draw_pix);
		}
//		draw_interface(draw_pix, w, h);


		drawFrameM(draw_pix);
		
//		SDL_RenderClear(renderer);
//		SDL_UpdateTexture(scrt, NULL, draw_pix, w * 4);
//		SDL_RenderCopy(renderer, scrt, NULL, NULL);
		
		char outstr[128];
		SDL_Rect mpos;

		int curY = 25, curDY = 15;

		static float fps = 0;

		float mDT = 1000000.0 / (float)dT;
		fps = fps*0.9 + 0.1*mDT;
		
		sprintf(outstr, "fps %g classif %d", fps, device_get_cur_class_res());
		msg = TTF_RenderText_Solid(font, outstr, textColor); 
		mpos.x = 5; mpos.y = curY; curY += curDY;
		mpos.w = msg->w; mpos.h = msg->h;
		SDL_BlitSurface(msg, NULL, scr, &mpos);
		SDL_FreeSurface(msg);

		if(emulated_mode)
		{
			sprintf(outstr, "%d", device_get_mark());
			msg = TTF_RenderText_Solid(fontSM, outstr, textColor); 
			mpos.x = 1200; mpos.y = 50;
			mpos.w = msg->w; mpos.h = msg->h;
			SDL_BlitSurface(msg, NULL, scr, &mpos);
			SDL_FreeSurface(msg);
		}
		if(nn_mode)
		{
			sprintf(outstr, "%.3f  %.3f", avg_err, cl_err);
			msg = TTF_RenderText_Solid(fontSS, outstr, textColor); 
			mpos.x = 1000; mpos.y = 50;
			mpos.w = msg->w; mpos.h = msg->h;
			SDL_BlitSurface(msg, NULL, scr, &mpos);
			SDL_FreeSurface(msg);
		}
		textColor.r = 0xFF;
		textColor.g = 0xBB;
		textColor.b = 0;
		
		SDL_Flip(scr);

//		SDL_RenderPresent(renderer);
	}
	free(draw_pix);
//	delete draw_pix;
//	TTF_CloseFont( font ); 
//	TTF_Quit();
	SDL_Quit();

//	device_close_log_file();
	close_device();
    return 0;
}