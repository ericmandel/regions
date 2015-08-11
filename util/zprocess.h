/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * zprocess.h - include file for the process handling
 *
 * (NB: not called process.h because of a name conflict with cygwin)
 *
 */

#ifndef	__zprocess_h
#define	__zprocess_h

#include "xutil.h"
#include "xlaunch.h"
#include "find.h"

int ProcessOpen (char *cmd, int *inchan, int *outchan, int *pid);
void *ProcessRead (int fd, void *buf, int maxbytes, int *got);
int ProcessWrite(int fd, void *buf, int nbytes);
int ProcessClose(int pid, int *exit_status);
int ProcessGetChan(int pid, int *inchan, int *outchan);

#endif /* __zprocess.h */
