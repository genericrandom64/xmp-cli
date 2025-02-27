/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#if defined(_WIN32)
#include <windows.h>

void delay_ms(int msec) {
	Sleep(msec);
}

#elif defined(__OS2__)
#define INCL_DOSPROCESS
#include <os2.h>

void delay_ms(int msec) {
	DosSleep(msec);
}

#elif defined(_DOS)
#include <dos.h>

void delay_ms(int msec) {
	delay(msec); /* doesn't seem to use int 15h. */
}

#elif defined(HAVE_USLEEP)
#include <unistd.h>

void delay_ms(int msec) {
	usleep(msec * 1000);
}

#elif defined(HAVE_SELECT)

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#else
#  include <sys/time.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif
#include <stddef.h>

void delay_ms(int msec) {
	struct timeval tv;
	long usec;

	usec = msec * 1000;
	tv.tv_sec = usec / 1000000;
	tv.tv_usec = usec % 1000000;
	select(0, NULL, NULL, NULL, &tv);
}

#else
#error Missing implementation of delay_ms()
#endif
