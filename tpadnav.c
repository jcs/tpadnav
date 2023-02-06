/*
 * Copyright (c) 2023 joshua stein <jcs@jcs.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XTest.h>

#define BUTTON_SWIPE_LEFT	6
#define BUTTON_SWIPE_RIGHT	7

/* one swipe per this many seconds will be recognized, overridable with -i */
#define MIN_SWIPE_SECS		1

extern char *__progname;

static int button_press_type = -1;
static int button_release_type = -1;
static int verbose = 0;
#define VPRINTF(format, ...) { if (verbose) { printf(format, ##__VA_ARGS__); }};

int swallow_xerror(Display *dpy, XErrorEvent *e);
void usage(void);

int
swallow_xerror(Display *dpy, XErrorEvent *e)
{
	return 0;
}

void
usage(void)
{
	fprintf(stderr, "usage: %s %s\n", __progname,
		"[-d display] [-i min swipe interval] [-v]");
	exit(1);
}

int
main(int argc, char* argv[])
{
	Display *dpy;
	Window root;
	XDeviceInfo *devices;
    	XDevice *device;
	XInputClassInfo	*xici;
	XEventClass tevent, *all_events;
	XEvent event;
	XDeviceButtonEvent *bevent;
	KeyCode code_back, code_forward;
	struct timeval now, last_swipe = { 0 };
	char *display = NULL;
	unsigned long screen, delta_msec;
	int ch, send_key, ndevices, n, j, nevents = 0, was_nevents;
	int min_swipe_secs = MIN_SWIPE_SECS;

	while ((ch = getopt(argc, argv, "d:i:v")) != -1) {
		switch (ch) {
		case 'd':
			display = optarg;
			break;
		case 'i':
			min_swipe_secs = atoi(optarg);
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	dpy = XOpenDisplay(display);
	if (!dpy)
		errx(1, "can't open display %s", XDisplayName(display));

#ifdef __OpenBSD__
	if (unveil("/", "") == -1 || unveil(NULL, NULL) == -1)
		err(1, "unveil");
	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");
#endif

        devices = XListInputDevices(dpy, &ndevices);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XSetErrorHandler(swallow_xerror);

	code_back = XKeysymToKeycode(dpy, XF86XK_Back);
	if (!code_back)
		errx(1, "XKeysymToKeycode(XF86XK_Back)");

	code_forward = XKeysymToKeycode(dpy, XF86XK_Forward);
	if (!code_forward)
		errx(1, "XKeysymToKeycode(XF86XK_Forward)");

	all_events = malloc(sizeof(XEventClass) * ndevices * 2);
	if (all_events == NULL)
		err(1, "malloc");
	memset(all_events, 0, ndevices * sizeof(XEventClass));

	nevents = 0;

	for (n = 0; n < ndevices; n++) {
		VPRINTF("opening device %s (%ld)\n", devices[n].name,
		    devices[n].id);
		device = XOpenDevice(dpy, devices[n].id);
		if (!device) {
			VPRINTF("unable to open device\n");
			continue;
		}

		was_nevents = nevents;
		for (xici = device->classes, j = 0;
		    j < devices[n].num_classes;
		    xici++, j++) {
			if (xici->input_class != ButtonClass)
				continue;

			DeviceButtonPress(device, button_press_type, tevent);
			all_events[nevents++] = tevent;

			DeviceButtonRelease(device, button_release_type,
			    tevent);
			all_events[nevents++] = tevent;
		}

		if (was_nevents == nevents) {
			VPRINTF("no button events on this device, closing\n");
			XCloseDevice(dpy, device);
			continue;
		}
	}

	XSetErrorHandler(NULL);

	VPRINTF("selecting %d events\n", nevents);
	if (XSelectExtensionEvent(dpy, root, all_events, nevents) != 0)
		err(1, "error selecting events");

	VPRINTF("listening for buttons...\n");

	for (;;) {
		XNextEvent(dpy, &event);

		if (event.type != button_release_type)
			continue;

		bevent = (XDeviceButtonEvent *)&event;
		VPRINTF("got release for button %d\n", bevent->button);

		if (bevent->button != BUTTON_SWIPE_LEFT &&
		    bevent->button != BUTTON_SWIPE_RIGHT)
			continue;

		gettimeofday(&now, NULL);

		if (last_swipe.tv_sec == 0)
			delta_msec = min_swipe_secs * 2000;
		else {
			delta_msec = (now.tv_sec - last_swipe.tv_sec) * 1000;
			delta_msec += (now.tv_usec - last_swipe.tv_usec) / 1000;
		}

		if (delta_msec < (min_swipe_secs * 1000)) {
			VPRINTF("swipe too soon (%ld msec), ignoring\n",
			    delta_msec);
			continue;
		}

		last_swipe = now;

		switch (bevent->button) {
		case BUTTON_SWIPE_LEFT:
			send_key = code_back;
			VPRINTF("swiped left, sending back\n");
			break;
		case BUTTON_SWIPE_RIGHT:
			send_key = code_forward;
			VPRINTF("swiped right, sending forward\n");
			break;
		default:
			continue;
		}

		XTestFakeKeyEvent(dpy, send_key, True, CurrentTime);
		XFlush(dpy);
		XTestFakeKeyEvent(dpy, send_key, False, CurrentTime);
		XFlush(dpy);
	}
}
