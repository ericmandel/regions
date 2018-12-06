/*
 *	Copyright (c) 2015-2018 Smithsonian Astrophysical Observatory
 */

/*
 *
 * regcnts.h - include file for "compile on the fly" counts in regions
 *
 */

#ifndef	__regcnts_h
#define	__regcnts_h

/* this allows us to check against old funtools code */
// #define USE_FUNTOOLS 1
/* but this is the one we want to use for production */
#define USE_CFITSIO 1

#include <math.h>
#ifdef USE_CFITSIO
#include "fitsio.h"
#include "jsfitsio.h"
#elif USE_FUNTOOLS
#include "funtools.h"
#else
#error "requires funtools or cfitsio"
#endif
#include "regionsP.h"

/* type of data we process */
#define SRC  0
#define BKG  1

/* max image dimensions we can handle */
#define IMDIM 4

/* types of background */
#define BKG_VAL	 1
#define BKG_ALL	 2
#define BKG_EACH 3

/* convenience */
#define ASEC_DEG   3600.0
#define ASEC_DEGSQ (ASEC_DEG*ASEC_DEG)

#ifdef USE_CFITSIO
#define EXTLIST "EVENTS STDEVT"
#endif

/* options record */
typedef struct optsRec {
  int bin;
  int bktype;
  int c;
  int dobkgderr;
  int docube;
  int dodata;
  int dog;
  int domatch;
  int dopixels;
  int doradang;
  int dosum;
  int dozero;
  int otype;
  char *cube;
  FILE *fd;
  FILE *efd;
} *Opts, OptsRec;

/* data record (src and bkg) */
typedef struct dataRec {
  int type;
  int x0, x1, y0, y1;
  int dim1, bitpix;
  int fromsrc;
#if USE_CFITSIO
  int dim2;
  int curslice;
  int maxslice;
  int szslice;
#endif
  char *regstr;
  char *filtstr;
  char *name;
  char *cards;
  void *data0;
  void *data;
  int block;
  int nmask;
  int nreg;
  int *area;
  int *savearea;
  double dpp;
  double *cnts;
  double *savecnts;
  RegionsMask masks;
  Regions reg;
  struct WorldCoor *wcs;
#ifdef USE_CFITSIO
  fitsfile *fptr;
#elif USE_FUNTOOLS
  Fun fun;
  FITSHead header;
#endif
} *Data, DataRec;

/* results record */
typedef struct resRec {
  char *radang;
  int bkgarea;
  double *bncnts;
  double *bnerr; 
  double *bscnts;
  double *bserr;
  double bkgval;
  double dppnorm;
} *Res, ResRec;

void regcntsInitAlloc(Opts *opts, Data *src, Data *bkg, Res *res);
void regcntsParseArgs(int argc, char **argv, 
		      Opts opts, Data src, Data bkg, Res res);
void regcntsGetData(Opts opts, Data d);
void regcntsBkgDataFromSrc(Data src, Data bkg);
int regcntsOpenRegions(Data d);
void regcntsInitResults(Opts opts, Data src, Data bkg, Res res);
void regcntsCountsInRegions(Opts opts, Data d);
void regcntsDisplayHeader(Opts opts, Data src, Data bkg, Res res);
void regcntsSumCounts(Opts opts, Data src, Data bkg);
void regcntsSubtractBkg(Opts opts, Data src, Data bkg, Res res);
void regcntsDisplayMainInfo(Opts opts, Data src, Res res);
void regcntsDisplaySrcInfo(Opts opts, Data src);
void regcntsDisplayBkgInfo(Opts opts, Data bkg, Res res);
void regcntsDisplayEnd(Opts opts);
void regcntsExit(void);
void regcntsClearArrays(Data d, Res res);
void regcntsCleanUp(Opts opts, Data src, Data bkg, Res res);
void regcntsErrchk(Opts opts, int status);

#endif
