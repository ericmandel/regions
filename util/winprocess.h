/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * winprocess.h - include file for the process handling
 *
 *
 */

#ifndef	__winprocess_h
#define	__winprocess_h

#include "xutil.h"

#if defined(__CYGWIN__) || defined(_WIN32)

int WinProcessOpen(char *cmd, void **ichan, void **ochan, void **pid);
void *WinProcessRead(void *fd, void *buf, int maxbytes, int *got);
int WinProcessWrite(void *fd, void *buf, int nbytes);
int WinProcessClose(void *pid, int *exit_status);
int WinProcessGetChan(void *pid, void **ichan, void **ochan);

#endif

#endif /* __winprocess.h */
