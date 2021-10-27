#include <X11/Xlib.h>
#include "keyb_mouse_emu.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <X11/extensions/XTest.h>
Display *display;
Window root_window;

void emu_init()
{
	display = XOpenDisplay (NULL);
	root_window = DefaultRootWindow(display);
	if (display == NULL)
	{
		printf("Can't open display!\n");
		return ;
	}
}
 
void emu_mouse_move(int x, int y)
{
	XWarpPointer (display, root_window, None, 0, 0,0,0, x, y);
//	XWarpPointer (display, None, root_window, 0, 0,0,0, x, y);
	XFlush (display);
//	usleep (1);
}
void emu_mouse_click(int button)
{
	XTestFakeButtonEvent (display, 1, 1,  CurrentTime);
	XTestFakeButtonEvent (display, 1, 0,  CurrentTime);
	return;
  // Create and setting up the event
	XEvent event;
	memset (&event, 0, sizeof (event));
	event.xbutton.button = button;
	event.xbutton.same_screen = True;
	event.xbutton.subwindow = DefaultRootWindow (display);
	while (event.xbutton.subwindow)
	{
		event.xbutton.window = event.xbutton.subwindow;
		XQueryPointer (display, event.xbutton.window,
		     &event.xbutton.root, &event.xbutton.subwindow,
		     &event.xbutton.x_root, &event.xbutton.y_root,
		     &event.xbutton.x, &event.xbutton.y,
		     &event.xbutton.state);
	}
	// Press
	event.type = ButtonPress;
	if (XSendEvent (display, PointerWindow, True, ButtonPressMask, &event) == 0)
		printf ("Error to send the event!\n");
	XFlush(display);
	usleep(10);
  // Release
	event.type = ButtonRelease;
	if (XSendEvent (display, PointerWindow, True, ButtonReleaseMask, &event) == 0)
		printf ("Error to send the event!\n");
	XFlush (display);
	usleep(1);
}

void emu_mouse_wheel(int amount)
{
  // Create and setting up the event
	XEvent event;
	memset (&event, 0, sizeof (event));
	if(amount > 0)
		event.xbutton.button = 4;
	else 
		event.xbutton.button = 5;
	event.xbutton.same_screen = True;
	event.xbutton.subwindow = DefaultRootWindow (display);
	while (event.xbutton.subwindow)
	{
		event.xbutton.window = event.xbutton.subwindow;
		XQueryPointer (display, event.xbutton.window,
		     &event.xbutton.root, &event.xbutton.subwindow,
		     &event.xbutton.x_root, &event.xbutton.y_root,
		     &event.xbutton.x, &event.xbutton.y,
		     &event.xbutton.state);
	}
	// Press
	event.type = ButtonPress;
	if (XSendEvent (display, PointerWindow, True, ButtonPressMask, &event) == 0)
		printf ("Error to send the event!\n");
	XFlush(display);
	usleep(10);
  // Release
	event.type = ButtonRelease;
	if (XSendEvent (display, PointerWindow, True, ButtonReleaseMask, &event) == 0)
		printf ("Error to send the event!\n");
	XFlush (display);
	usleep(1);
}

// Get mouse coordinates
void emu_get_mouse_coords(int *x, int *y)
{
	XEvent event;
	XQueryPointer (display, DefaultRootWindow (display),
                 &event.xbutton.root, &event.xbutton.window,
                 &event.xbutton.x_root, &event.xbutton.y_root,
                 &event.xbutton.x, &event.xbutton.y,
                 &event.xbutton.state);
	*x = event.xbutton.x;
	*y = event.xbutton.y;
}