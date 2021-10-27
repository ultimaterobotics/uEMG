#include "device_functions.h"
#include "serial_functions.h"

#include <poll.h>
#include <fcntl.h>
#include <sys/time.h>

//#include <linux/kernel.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
//#include <asm/ioctls.h>

#include "drawing.h"

struct timeval curTime, prevTime, zeroTime;
int device = 0;
double real_time = 0;
speed_t baudrate;

void serial_functions_init()
{
//	baudrate = B1000000;
	baudrate = B921600;
//	baudrate = B230400;
}

void open_device()
{
	uint8_t *dev = "/dev/ttyUSB0";//gtk_entry_get_text(GTK_ENTRY(serial_entry_device));
	struct termios newtio;
	device = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	bzero(&newtio, sizeof(newtio));

	newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = 0;//IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;//ICANON;

	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 1;

	tcflush(device, TCIOFLUSH);
	tcsetattr(device, TCSANOW, &newtio);
	char txt[128];
	sprintf(txt, "device port open result: %d\n", device);
	printf("%s", txt);
}

int open_servo_device()
{
	int srv_dev = 0;
	uint8_t *dev = "/dev/ttyACM0";//gtk_entry_get_text(GTK_ENTRY(serial_entry_device));
	struct termios newtio;
	srv_dev = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	bzero(&newtio, sizeof(newtio));

	newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = 0;//IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;//ICANON;

	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 1;

	tcflush(srv_dev, TCIOFLUSH);
	tcsetattr(srv_dev, TCSANOW, &newtio);
	char txt[128];
	sprintf(txt, "servo device port open result: %d\n", srv_dev);
	printf("%s", txt);
	return srv_dev;
}

void close_device()
{
	close(device);
	device = 0;
//	add_text_to_main_serial_log("device closed\n");	
}

uint8_t main_inited = 0;

void serial_main_init()
{
	gettimeofday(&prevTime, NULL);
	gettimeofday(&zeroTime, NULL);
}


int serial_main_loop()
{
	if(!main_inited)
	{
		serial_main_init();
		main_inited = 1;
	}
	gettimeofday(&curTime, NULL);
	int dT = (curTime.tv_sec - prevTime.tv_sec) * 1000000 + (curTime.tv_usec - prevTime.tv_usec);
	real_time += (double)dT * 0.000001;

	prevTime = curTime;

	if(device > 0)
	{
		struct pollfd pfdS;
		pfdS.fd = device;
		pfdS.events = POLLIN | POLLPRI;
		if(poll( &pfdS, 1, 1 ))
		{
			int lng = 0;
			uint8_t bbf[4096];
			uint8_t hex_bbf[16384];
			lng = read(device, bbf, 4096);
			if(lng > 0)
			{
				device_parse_response(bbf, lng);
			}
		}
	}
	return 1;
}

int write_to_device(uint8_t *buf, int len)
{
	if(device > 0)
		return write(device, buf, len);
	return -1;
}

void send_data(uint8_t *data, int len)
{
	if(device > 0)
	{
		uint8_t buf[1234];

		int pp = 0;
		buf[pp++] = 29;
		buf[pp++] = 115;
		buf[pp++] = len; //payload length
		for(int x = 0; x < len; x++)
			buf[x+pp] = data[x];
		buf[len+pp] = 0;
		write(device, buf, pp+len+1);
	}
}