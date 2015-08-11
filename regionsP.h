/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * regionsP.h - private include file for "compile on the fly" region filtering
 *
 */

#ifndef	__regionsP_h
#define	__regionsP_h

#if HAVE_CONFIG_H
#include "conf.h"
#endif

/* avoid use of system -- its not secure */
/* but funtools cannot use launch for the MinGW platform because the stdfiles
   support is missing in the launch_spawnvp() implementation of launch */
#ifndef USE_LAUNCH
#define USE_LAUNCH 1
#endif
#if HAVE_MINGW32
#undef USE_LAUNCH
#endif

#include "dl.h"
#include "util/xutil.h"
#include "util/file.h"
#include "util/find.h"
#include "util/macro.h"
#include "util/word.h"
#include "util/strtod.h"
#include "util/mkrtemp.h"
#include "util/xalloc.h"
#include "util/xerror.h"
#include "util/winprocess.h"
#include "util/zprocess.h"
#include "wcs/wcs.h"
#ifdef USE_LAUNCH
#include "util/xlaunch.h"
#endif

#if HAVE_CYGWIN||HAVE_MINGW32
#include <windows.h>
#endif

/* include file for the public */
#include "regions.h"

#ifndef OBJPATH
#define OBJPATH "."
#endif

#ifndef REGIONS_CC
#define REGIONS_CC NULL
#endif
     
#ifndef REGIONS_CFLAGS
#define REGIONS_CFLAGS NULL
#endif
     
/* define default wcs for regions */
#define DEFAULT_WCS "physical"

/* define methods of program generation */
#define METHOD_C	1

/* define types of regionsing process -- separate process, self-contained
   separate process, or dynamic load (if defined) */
#define PTYPE_PROCESS	1
#define PTYPE_CONTAINED	2
#define PTYPE_DYNAMIC	3

/* define how we connect the processes -- unix or windows pipes */
#define PIPE_UNIX	0
#define PIPE_WIN32	1

/* defaults which can be overridden by environment variables */
#define DEFAULT_REGIONS_METHOD METHOD_C
#ifdef USE_DL
#define DEFAULT_REGIONS_PTYPE  PTYPE_DYNAMIC
#else
#define DEFAULT_REGIONS_PTYPE  PTYPE_PROCESS
#endif
#define DEFAULT_PAINT_MODE    "false"
#define DEFAULT_REGIONS_TMPDIR "/tmp"

/* default cordinate system for regions */
#define DEFAULT_COORDSYS "physical"

/* if we have gcc, we can use dynamic loading instead of a separate process */
#define GCC_SHARED_FLAGS "-g -fPIC -shared"

/* places to look for the compiler other than user's path */
#define CC_PATH "/bin:/usr/bin:/usr/local/bin/:/opt/local/bin:/opt/SUNWspro/bin"

/* define non-WCS coordinate systems we handle specially */
#define LCX_IMAGE	1
#define LCX_PHYS	2

/* the following defines must match those in imregions.h */
/* define type of important tokens */
#define TOK_EREG	1
#define TOK_NREG	2
#define TOK_IREG	4
#define TOK_RTINE	8
#define TOK_NAME	16
#define TOK_ACCEL	32
#define TOK_VARARGS	64
#define TOK_REG		(TOK_EREG|TOK_NREG|TOK_IREG)
/* end of common defines */

typedef RegionsMask (*RegionsImageCall)(
  int txmin, int txmax, int tymin, int tymax, int tblock, int *got
);

/* reg.l */
int regparse(Regions reg);
int regparseFree(Regions reg);
int regwrap(void);

/* regprog.c */
int RegionsProgStart(Regions reg);
int RegionsProgOpen(Regions reg);
int RegionsProgPrepend(Regions reg);
int RegionsProgWrite(Regions reg);
int RegionsProgAppend(Regions reg);
int RegionsProgClose(Regions reg);
int RegionsProgCompile(Regions reg);
int RegionsProgEnd(Regions reg);
char *RegionsLexName(Regions reg, char *name);
char *RegionsLexRoutine1(Regions reg, char *name);
char *RegionsLexRoutine2(Regions reg, char *name);
char *RegionsLexRegion1(Regions reg, char *name);
char *RegionsLexRegion2(Regions reg, char *name);

/* regprog_c.c */
int RegionsProgLoad_C(Regions reg);

/* imregions.c */
void initimregions(void);

#endif /* __regionsP.h */
