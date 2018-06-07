/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * regdisp: display regions mask(s)
 *
 */
#include "regcnts.h"

#define BORDER 2
#define LARGENUM 1024000000

/* the ever-present */
void regdispUsage (char *fname){
  fprintf(stderr, "usage: %s [switches] fitsfile [reg1 reg2 ...]\n", fname);
  fprintf(stderr, "optional switches:\n");
  fprintf(stderr, "  -h\t\t# display this help\n");
  fprintf(stderr, "  -l\t\t# display list of segments\n");
  fprintf(stderr, "  -m\t\t# display mask (default)\n");
  fprintf(stderr, "  -s\t\t# image section as 'x0 x1 y0 x1 block' (def: whole image)\n");
  fprintf(stderr, "If regions are not specified on the command line, they will be read from stdin.\n");
  exit(1);
}

void doreg(char *cards, char *regstr,
	   int x0, int x1, int y0, int y1, int block, int mode){
  int i, c, x, y, nreg, nmask;
  int xmin=LARGENUM, xmax=0, xdim=0;
  int ymin=LARGENUM, ymax=0, ydim=0;
  char **lines=NULL;
  Regions reg=NULL;
  RegionsMask mask=NULL;

  reg = OpenRegions(cards, regstr, NULL);
  nmask = FilterRegions(reg, x0, x1, y0, y1, block, &mask, &nreg);
  fprintf(stdout, "regions: %s\n", regstr);
  switch(mode ){
  case 0:
    for(i=0; i<nmask; i++){
      fprintf(stdout, "#%d: region=%d y=%d, x=%d,%d\n", 
	      i, mask[i].region, mask[i].y, mask[i].xstart, mask[i].xstop);
    }
    break;
  case 1:
    for(i=0; i<nmask; i++){
      // get mask limits
      if( mask[i].xstart < xmin ){
	xmin = mask[i].xstart;
      }
      if( mask[i].xstop > xmax ){
	xmax = mask[i].xstop;
      }
      if( mask[i].y < ymin ){
	ymin = mask[i].y;
      }
      if( mask[i].y > ymax ){
	ymax = mask[i].y;
      }
    }
    // show a border around the mask
    xmin = MAX(1, xmin - BORDER);
    xmax = MIN(x1-x0+1, xmax + BORDER);
    ymin = MAX(1, ymin - BORDER);
    ymax = MIN(y1-y0+1, ymax + BORDER);
    xdim = xmax - xmin + 1;
    ydim = ymax - ymin + 1;
    // allocate enough lines to display mask
    lines = calloc(ydim, sizeof(char *));
    // allocate char buffers for each line and see with "."
    for(i=0; i<ydim; i++){
      lines[i] = calloc(xdim + 1, sizeof(char));
      memset(lines[i], '.', xdim);
    }
    // fill the mask with region ids
    for(i=0; i<nmask; i++){
      y = mask[i].y - ymin;
      for(x=mask[i].xstart-xmin; x<=mask[i].xstop-xmin; x++){
	/* regions start with '1' */
	c = mask[i].region + 48;
	/* but wrap around to 0 after ~ */
	while( c > 126 ){
	  c -= 79;
	}
	lines[y][x] = c;
      }
    }
    // display mask (but reverse order)
    for(i=ydim-1; i>=0; i--){
      fprintf(stdout, "%s", lines[i]);
      fprintf(stdout, "\n");
    }
    fprintf(stdout, "LL corner: %d,%d\n", x0 + xmin - 1, y0 + ymin - 1);
    fprintf(stdout, "UR corner: %d,%d\n", x0 + xmax - 1, y0 + ymax - 1);
    break;
  }
  fflush(stdout);
  /* clean up */
  CloseRegions(reg);
  xfree(mask);
  for(i=0; i<ydim; i++){
    xfree(lines[i]);
  }
  xfree(lines);
}

#if __EMSCRIPTEN__
int regdisp (int argc, char **argv){
#else
int main(int argc, char **argv){
#endif
  int i, c, args, hdutype, ncard;
  int status=0;
  int mode=1;
  int x0=1, x1=0, y0=1, y1=0, block=1;
  long naxes[2];
  char *cards=NULL;
  char regstr[SZ_LINE];
  fitsfile *fptr=NULL, *nfptr=NULL;

  /* we want the args in the same order in which they arrived, and
     gnu getopt sometimes changes things without this */
  putenv("POSIXLY_CORRECT=true");
  /* process switch arguments */
  while ((c = getopt(argc, argv, "hlms:")) != -1){
    switch(c){
    case 'h':
      regdispUsage(argv[0]);
      break;
    case 'l':
      mode = 0;
      break;
    case 'm':
      mode = 1;
      break;
    case 's':
      if( sscanf(optarg, "%d %d %d %d %d", &x0, &x1, &y0, &y1, &block) != 5 ){
	xerror(stderr, "for sections: -s 'x0 x1 y0 y1 block'\n");
      }
      break;
    }
  }
  args = argc - optind;
  if( args <= 0 ){
    regdispUsage(argv[0]);
  }
  fptr = openFITSFile(argv[optind+0], READONLY, EXTLIST, &hdutype, &status);
  regcntsErrchk(NULL, status);
  if( (x1 == 0) || (y1 == 0) ){
    switch(hdutype){
    case IMAGE_HDU:
      break;
    default:
      nfptr = filterTableToImage(fptr, NULL, NULL, NULL, NULL, 1, &status);
      regcntsErrchk(NULL, status);
      closeFITSFile(fptr, &status);
      regcntsErrchk(NULL, status);
      fptr = nfptr;
      break;
    }
    fits_get_img_size(fptr, 2, naxes, &status);
    regcntsErrchk(NULL, status);
    x1 = naxes[0];
    y1 = naxes[1];
  }
  getHeaderToString(fptr, &cards, &ncard, &status);
  regcntsErrchk(NULL, status);
  if( args > 1 ){
    for(i=1; i<args; i++){
      doreg(cards, argv[optind+i], x0, x1, y0, y1, block, mode);
    }
  } else {
    while( fgets(regstr, SZ_LINE, stdin) ){
      doreg(cards, regstr, x0, x1, y0, y1, block, mode);
    }
  }
  closeFITSFile(fptr, &status);
  regcntsErrchk(NULL, status);
  xfree(cards);
  return 0;
}
