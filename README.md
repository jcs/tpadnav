# tpadnav

OpenBSD's
[wmsouse (wstpad)](http://man.openbsd.org/wsmouse)
driver for touchpads recognizes two-finger scrolling left and right and
generates mouse button 6 and 7 events (as well as 4 and 5 for scrolling up and
down).

This program listens for those mouse button 6 and 7 events and then generates
fake `XF86XK_Back` or `XF86XK_Forward` keyboard events with the `XTest`
extension.
In at least Firefox, this effectively enables left and right two-finger swipe
gestures on a touchpad to navigate backward and forward.

Since these button events are generated many times along a two-finger swipe,
this program throttles its output and only generates one back or forward key
per 1 second (configurable with the `-i` option).

## License

ISC

## Compiling

Run `make` to compile, and `make install` to install to `/usr/local` by
default.

## Usage

Add `/usr/local/bin/tpadnav &` to `~/.xsession`.

If the 1 second throttling is generating more events than you want, try a
higher `-i` value (in seconds).

If it's not doing anything, run it with `-v` to see verbose output.
