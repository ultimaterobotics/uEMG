#include "device_serial.h"

#ifdef BUILD_FOR_MACOS
#include <IOKit/serial/ioss.h>
#endif

#ifdef BUILD_FOR_WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#else
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>

#include <sys/ioctl.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> /* exit */
#include <string.h> /* memcpy, memset */

int device_port = -1;
char current_device_name[1024];
#ifdef BUILD_FOR_WINDOWS
HANDLE device_port_w = INVALID_HANDLE_VALUE;
OVERLAPPED ovp_var;
#endif

void device_get_port_name(char *name)
{
	//stub implementation, need to properly enumerate USB devices to find match
	//but it works if no other USB-serial stuff is plugged in
	sprintf(name, "");
#ifdef BUILD_FOR_LINUX	
	sprintf(name, "/dev/ttyUSB0");
	if(access(name, F_OK) != 0)
		sprintf(name, "");
#endif
#ifdef BUILD_FOR_MACOS
	sprintf(name, "/dev/tty.SLAB_USBtoUART");
	if(access(name, F_OK) != 0)
		sprintf(name, "");
#endif
#ifdef BUILD_FOR_WINDOWS 

	char lpTargetPath[5000]; // buffer to store the path of the COMPORTS

    for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
    {
		char str[256];
		sprintf(str, "COM%d", i);

		DWORD test = QueryDosDeviceA(str, lpTargetPath, 1000);

        // Test the return value and error if any
        if (test != 0) //QueryDosDevice returns zero if it didn't find an object
        {
			if (strstr(lpTargetPath, "Silab") != NULL)
			{
				sprintf(name, "%s", str);
			}
			printf("port %s - devname %s\n", str, lpTargetPath);
        }
    }
#endif
	sprintf(current_device_name, "%s", name);
	return;
}

char* get_connection_name()
{
	return current_device_name;
}

void device_open_port()
{
	printf("device open port attempt\n");
	char dev[1024];
	device_get_port_name(dev);
	if (strlen(dev) < 2) return;

#ifndef BUILD_FOR_WINDOWS

	struct termios newtio;
	device_port = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	bzero(&newtio, sizeof(newtio));

#ifdef BUILD_FOR_LINUX
	newtio.c_cflag = B921600 | CS8 | CLOCAL | CREAD;
#endif
#ifdef BUILD_FOR_MACOS
	newtio.c_cflag = CS8 | CLOCAL | CREAD;
#endif
	newtio.c_iflag = 0;//IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;//ICANON;

	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 1;

	tcflush(device_port, TCIOFLUSH);
	tcsetattr(device_port, TCSANOW, &newtio);

#ifdef BUILD_FOR_MACOS
	speed_t speed = 921600;
	ioctl(device_port, IOSSIOSPEED, &speed);
#endif

	char txt[128];
	sprintf(txt, "device port open result: %d\n", device_port);
#if DEBUG_PRINTS
	printf("%s", txt);
#endif
#else
	device_port_w = CreateFileA(dev, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (device_port_w == INVALID_HANDLE_VALUE)
	{
		printf("%s : can't open port\n", dev);
		return ;
	}
 
	// Flush away any bytes previously read or written.
	BOOL success = FlushFileBuffers(device_port_w);
	if (!success)
	{
		printf("Failed to flush serial port\n");
	}
 
	// Configure read and write operations to time out after 100 ms.
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = 0;// 100;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;// 100;
	timeouts.WriteTotalTimeoutMultiplier = 0;
 
	success = SetCommTimeouts(device_port_w, &timeouts);
	if (!success)
	{
		printf("Failed to set serial timeouts\n");
	}
 
	DCB state;
	state.DCBlength = sizeof(DCB);
	success = GetCommState(device_port_w, &state);
	if (!success)
	{
		printf("Failed to get serial settings\n");
	}
 
	state.BaudRate = 921600;
 
	success = SetCommState(device_port_w, &state);
	if (!success)
	{
		printf("Failed to set serial settings\n");
	}
 
	return ;
#endif
}

void device_close_port()
{
#ifndef BUILD_FOR_WINDOWS
	close(device_port);
#else
	CloseHandle(device_port_w);
	device_port_w = INVALID_HANDLE_VALUE;
#endif
	device_port = -1;
}

int device_poll(uint8_t *buffer, int max_buf_length)
{
	static int no_data_cycles = 0;
#ifndef BUILD_FOR_WINDOWS
	if(device_port > 0)
	{
		struct pollfd pfdS;
		pfdS.fd = device_port;
		pfdS.events = POLLIN | POLLPRI;
		no_data_cycles++;
		if(poll( &pfdS, 1, 1 ))
		{
			int lng = 0;
			lng = read(device_port, buffer, max_buf_length);
			if(lng > 0)
			{
				no_data_cycles = 0;
//				device_parse_response(buffer, lng);
				return lng;
			}
		}
		if(no_data_cycles > 100)
		{
			char dev[1024];
			device_get_port_name(dev);
			if(strlen(dev) < 2)
			{
				device_close_port();
			}
			no_data_cycles = 0; //don't want to check too often
		}
	}
	return 0;
#else
	DWORD ret_cnt;
	ReadFile(device_port_w, buffer, max_buf_length, &ret_cnt, NULL);
	if(ret_cnt < 1) no_data_cycles++;
	if (no_data_cycles > 1000)
	{
		char res[256];

		DWORD test = QueryDosDeviceA(current_device_name, res, 256);
		if (test == 0)
			device_close_port();
		else no_data_cycles = 0;
	}
	return ret_cnt;
#endif
}

bool is_connected()
{
#ifndef BUILD_FOR_WINDOWS
	if(device_port > 0) return 1;
#else
	if (device_port_w != INVALID_HANDLE_VALUE) return 1;
#endif
	return 0;
}
