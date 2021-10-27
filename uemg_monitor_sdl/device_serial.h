#include <stdint.h>
#include <stdbool.h>

#define DEBUG_PRINTS 0

//#define BUILD_FOR_MACOS
#define BUILD_FOR_WINDOWS
//#define BUILD_FOR_LINUX

void device_get_port_name(char *name);
void device_open_port(void);
void device_close_port(void);
int device_poll(uint8_t *buffer, int max_buf_length);
bool is_connected(void);
char* get_connection_name(void);
