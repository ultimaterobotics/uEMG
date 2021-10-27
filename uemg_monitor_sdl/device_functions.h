#include <stdint.h>


int device_get_cur_class_res();

void device_parse_response(uint8_t *buf, int len);
void device_save_on();
void device_save_off();
void device_save_mark(int mark);
void device_emulate_start(char *file_name);
void device_emulate_end();
int device_get_mark();
float device_get_battery();

void device_game_mode_switch();
void device_make_kmeans();
void device_fix_kmeans(int k);
void device_train_ml(int cur_img);
void device_toggle_hand_control();

void device_save_ml_state();
void device_load_ml_state();

void device_emulate_step();