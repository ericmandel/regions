/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * regprog.c -- hi level support for region filtering using different techniques
 *
 */

#include "regionsP.h"

/*
 *
 * RegionsProgStart -- start the region filtering process
 *
 */
int RegionsProgStart(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  if( reg->debug > 1 ){
    fprintf(stderr, "in RegionsProgStart\n");
  }
  /* call the technique-specific routine */
  if( reg->reg_start ){
    return (reg->reg_start)(reg);
  } else {
    return 0;
  }
}

/*
 *
 * RegionsProgOpen -- return region filter program as a file for writing
 *
 */
int RegionsProgOpen(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  if( reg == NULL ){
    return 0;
  }
  if( reg->debug > 1 ){
    fprintf(stderr, "in RegionsProgOpen\n");
  }
  /* call the technique-specific routine */
  if( reg->reg_open ){
    return (reg->reg_open)(reg);
  } else {
    return 1;
  }
}

/*
 *
 * RegionsProgPrepend -- prepend to the filter
 *
 */
int RegionsProgPrepend(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  if( reg->debug > 1 ){
    fprintf(stderr, "in RegionsProgPrepend\n");
  }
  /* call the technique-specific routine */
  if( reg->reg_prepend ){
    return (reg->reg_prepend)(reg);
  } else {
    return 1;
  }
}

/*
 *
 * RegionsProgWrite -- write the symbols
 *
 */
int RegionsProgWrite(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  if( reg->debug > 1 ){
    fprintf(stderr, "in RegionsProgWrite\n");
  }
  /* call the technique-specific routine */
  if( reg->reg_write ){
    return (reg->reg_write)(reg);
  } else {
    return 1;
  }
}

/*
 *
 * RegionsProgAppend -- append the filter program body
 *
 */
int RegionsProgAppend(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  if( reg->debug > 1 ){
    fprintf(stderr, "in RegionsProgAppend\n");
  }
  /* call the technique-specific routine */
  if( reg->reg_append ){
    return (reg->reg_append)(reg);
  } else {
    return 1;
  }
}

/*
 *
 * RegionsProgClose -- close the filter program file
 *
 */
int RegionsProgClose(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  if( reg->debug > 1 ){
    fprintf(stderr, "in RegionsProgClose\n");
  }
  /* debugging only */
  if( reg->debug >= 2 ){
    return 1;
  }
  /* call the technique-specific routine */
  if( reg->reg_close ){
    return (reg->reg_close)(reg);
  } else {
    return 1;
  }
}

/*
 *
 * RegionsProgCompile -- compile the filter program
 *
 */
int RegionsProgCompile(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  if( reg->debug > 1 ){
    fprintf(stderr, "in RegionsProgCompile\n");
  }
  /* debugging only */
  if( reg->debug >= 2 ){
    return 1;
  }
  /* call the technique-specific routine */
  if( reg->reg_compile ){
    return (reg->reg_compile)(reg);
  } else {
    return 1;
  }
}

/*
 *
 * RegionsProgEnd -- end the filter process
 *
 */
int RegionsProgEnd(Regions reg){
  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  if( reg->debug > 1 ){
    fprintf(stderr, "in RegionsProgEnd\n");
  }
  /* call the technique-specific routine */
  if( reg->reg_end ){
    return (reg->reg_end)(reg);
  } else {
    return 0;
  }
}

/*
 *
 * RegionsLexName -- return the "in-expression" name
 *
 */
char *RegionsLexName(Regions reg, char *name){
  /* make sure we have something to play with */
  if( !reg ){
    return NULL;
  }
  /* call the technique-specific routine */
  if( reg->reg_name ){
    return (reg->reg_name)(reg, name);
  } else {
    return name;
  }
}

/*
 *
 * RegionsLexRoutine1 -- return the beginning of the routine string
 *
 */
char *RegionsLexRoutine1(Regions reg, char *name){
  /* make sure we have something to play with */
  if( !reg ){
    return NULL;
  }
  /* call the technique-specific routine */
  if( reg->reg_routine1 ){
    return (reg->reg_routine1)(reg, name);
  } else {
    return name;
  }
}

/*
 *
 * RegionsLexRoutine2 -- return the beginning of the routine string
 *
 */
char *RegionsLexRoutine2(Regions reg, char *name){
  /* make sure we have something to play with */
  if( !reg ){
    return NULL;
  }
  /* call the technique-specific routine */
  if( reg->reg_routine2 ){
    return (reg->reg_routine2)(reg, name);
  } else {
    return name;
  }
}

/*
 *
 * RegionsLexRegion1 -- return the beginning of the region string
 *
 */
char *RegionsLexRegion1(Regions reg, char *name){
  /* make sure we have something to play with */
  if( !reg ){
    return NULL;
  }
  /* call the technique-specific routine */
  if( reg->reg_region1 ){
    return (reg->reg_region1)(reg, name);
  } else {
    return name;
  }
}

/*
 *
 * RegionsLexRegion2 -- return the end of the region string
 *
 */
char *RegionsLexRegion2(Regions reg, char *name){
  /* make sure we have something to play with */
  if( !reg ){
    return NULL;
  }
  /* call the technique-specific routine */
  if( reg->reg_region2 ){
    return (reg->reg_region2)(reg, name);
  } else {
    return name;
  }
}

