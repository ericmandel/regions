/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * mkrtemp.h -- declarations for mkrtemp, a fancy version of mkstemp
 *
 */

#ifndef	__mkrtemp_h
#define	__mkrtemp_h

#include "xutil.h"
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include "xalloc.h"
#include "word.h"

#ifdef __APPLE__
#define lrand48  random
#define srand48 srandom
#endif

#ifdef __MINGW32__
#define lrand48  rand
#define srand48 srand
#define lstat stat
#endif

int mkrtemp(char *prefix, char *suffix, char *path, int len, int doopen);

#endif
