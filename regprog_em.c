/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * regprog_c.c -- support for region filters using the emscripten compiler
 *
 */

#if __EMSCRIPTEN__

#include "regionsP.h"

/*
 *
 * Private Routines
 *
 */

/*
 *
 * RegionsProgOpen_EM -- return filter program as a file for writing
 *
 */
static int RegionsProgOpen_EM(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  return 1;
}

/*
 *
 * RegionsProgPrepend_EM -- prepend the filter code
 *
 */
static int RegionsProgPrepend_EM(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  return 1;
}

/*
 *
 * RegionsProgWrite_EM -- write the symbols for filtering
 *
 */
static int RegionsProgWrite_EM(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  /* get the filter string */
  if( !reg->filter || !strcmp(reg->filter, "()") ){
    return 0;
  }
  return 1;
}

/*
 *
 * RegionsProgAppend_EM -- append the filter program body
 *
 */
static int RegionsProgAppend_EM(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  return 1;
}

/*
 *
 * RegionsProgClose_EM -- close the filter program file
 *
 */
static int RegionsProgClose_EM(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  return 1;
}

/*
 *
 * RegionsProgCompile_EM -- compile the filter program
 *
 */
static int RegionsProgCompile_EM(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  return 1;
}

/*
 *
 * RegionsProgEnd_EM -- end the filtering process
 *
 */
static int RegionsProgEnd_EM(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  return 1;
}

/*
 *
 * Public Routines
 *
 */

/*
 *
 * RegionsProgLoad_EM -- load the routines needed to support C filtering
 *
 */
int RegionsProgLoad_EM(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  reg->reg_prefix = "Astroem.";
  reg->reg_open = RegionsProgOpen_EM;
  reg->reg_prepend = RegionsProgPrepend_EM;
  reg->reg_write = RegionsProgWrite_EM;
  reg->reg_append = RegionsProgAppend_EM;
  reg->reg_close = RegionsProgClose_EM;
  reg->reg_compile = RegionsProgCompile_EM;
  reg->reg_end = RegionsProgEnd_EM;
  return 1;
}

/*
 *
 * FilterRegions_EM -- region filtering call using Emscripten
 *
 */
RegionsMask FilterRegions_EM(Regions reg,
			     int txmin, int txmax, int tymin, int tymax,
			     int tblock, int *got){
  char *ibuf=NULL;
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  ibuf = _RegionsInitString(reg);
  EM_ASM({
      if( typeof window.Regions !== 'object' ){
         window.Regions = {};
      }
      window.Regions.NSHAPE = function(){return $0};
      window.Regions.FINIT =  new Function('g', 'x', 'y', Pointer_stringify($1));
      window.Regions.FILTER = new Function('g', 'x', 'y', 'return (' + Pointer_stringify($2) + ')');
  }, reg->nshape, ibuf, reg->filter);
  if( ibuf ) free(ibuf);
  // call the real filter routine
  return IMFILTRTN(txmin, txmax, tymin, tymax, tblock, got);
}

int have_emscripten = 1;

#else

int have_emscripten = 0;

#endif
