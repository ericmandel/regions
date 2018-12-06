/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

#include "regcnts.h"

#if __EMSCRIPTEN__
int _regcnts (int argc, char **argv){
#else
int main (int argc, char **argv){
#endif

  Opts opts=NULL;
  Data src=NULL;
  Data bkg=NULL;
  Res res=NULL;

  /* we want the args in the same order in which they arrived, and
     gnu getopt sometimes changes things without this */
  putenv("POSIXLY_CORRECT=true");
  /* allocate and initialize all structures */
  regcntsInitAlloc(&opts, &src, &bkg, &res);
  /* parse arguments and initialize opts and image records */
  regcntsParseArgs(argc, argv, opts, src, bkg, res);
  /* open the source file and get image data */
  regcntsGetData(opts, src);
  /* process background region, if necessary */
  if( bkg->regstr ){
    /* open the background FITS file and get image data */
    if( bkg->name ){
      regcntsGetData(opts, bkg);
    } else {
      /* else use the source file for the background */
      regcntsBkgDataFromSrc(src, bkg);
    }
  }
  /* open the source region filter */
  if( !regcntsOpenRegions(src) ){
    xerror(stderr, "no valid regions included in '%s'\n", src->regstr);
  }
  /* open the background region filter, if necessary */
  if( bkg->regstr ){
    regcntsOpenRegions(bkg);
    /* see if we want to, and can, match source and bkgd regions */
    if( opts->domatch && (src->nreg == bkg->nreg) ){
	opts->bktype = BKG_EACH;
    }
  }
  /* final checks and initialization of result buffers */
  regcntsInitResults(opts, src, bkg, res);
  /* display header */
  regcntsDisplayHeader(opts, src, bkg, res);
cubeagain:
  /* get the counts in each region */
  regcntsCountsInRegions(opts, src);
  /* process background data, if necessary */
  if( opts->bktype != BKG_VAL ){
    /* get the counts in each region */
    regcntsCountsInRegions(opts, bkg);
  }
  /* when summing, we first display the summed results, then the unsummed */
sumagain:
  /* sum counts between regions, if necessary */
  regcntsSumCounts(opts, src, bkg);
  /* perform appropriate background subtraction */
  regcntsSubtractBkg(opts, src, bkg, res);
  /* display the main output table */
  regcntsDisplayMainInfo(opts, src, res);
  /* after displaying summed, go back and display unsummed values */
  switch(opts->dosum){
  case 1:
    /* done with summed, go back and do unsummed */
    opts->dosum = 2;
    goto sumagain;
  case 2:
    /* all done: we've processed both summed and unsummed counts */
    break;
  default:
    break;
  }
  /* display raw source info */
  regcntsDisplaySrcInfo(opts, src);
  /* display raw background info */
  regcntsDisplayBkgInfo(opts, bkg, res);
  /* process next slice of cube, if necessary */
  if( opts->docube ){
    // next background slice
    if( bkg->maxslice ){
      bkg->curslice += 1;
      if( bkg->curslice < bkg->maxslice ){
	// point to next slice of data
	bkg->data = bkg->data0 + (bkg->curslice * bkg->szslice);
	// clear arrays
	regcntsClearArrays(bkg, NULL);
      } else {
	// restore data so we can free it properly
	bkg->data = bkg->data0;
      }
    }
    // next source slice
    if( src->maxslice ){
      src->curslice += 1;
      if( src->curslice < src->maxslice ){
	// point to next slice of data
	src->data = src->data0 + (src->curslice * src->szslice);
	// clear arrays
	regcntsClearArrays(src, res);
	// process next slice
	goto cubeagain;
      } else {
	// restore data so we can free it properly
	src->data = src->data0;
      }
    }
  }
  /* finish up display */
  regcntsDisplayEnd(opts);
  /* cleanup */
  regcntsCleanUp(opts, src, bkg, res);
  return 0;
}
