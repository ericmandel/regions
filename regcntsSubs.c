/*
 *	Copyright (c) 2015-2018 Smithsonian Astrophysical Observatory
 */

/*
 *
 * _regcnts: data manipulation algorithms for regcnts
 *
 */

#include "regcnts.h"

extern int optind;

#if __EMSCRIPTEN__
/* static pointers to the "real" structs so we can clean up on exit */
static Opts _opts=NULL;
static Data _src=NULL;
static Data _bkg=NULL;
static Res _res=NULL;
#endif

/* the ever-present */
void regcntsUsage (char *fname){
  fprintf(stderr, "usage: %s [switches] sname [sreg] [bname breg|breg|bcnts]\n",
	 fname);
  fprintf(stderr, "optional switches:\n");
  fprintf(stderr, "  -b [n]\t# bin factor for binary tables (make in-memory image smaller)\n");
  fprintf(stderr, "  -c [str]\t# data slice ('*:*:1428') or cube ('all') specification string\n");
  fprintf(stderr, "  -e [efile]\t# error filename (def: stderr)\n");
  fprintf(stderr, "  -g\t\t# output using nice g format\n");
  fprintf(stderr, "  -G\t\t# output using %%.14g format (maximum precision)\n");
  fprintf(stderr, "  -h\t\t# display this help\n");
  fprintf(stderr, "  -j\t\t# output using JSON format (def: tabular format)\n");
  fprintf(stderr, "  -m\t\t# match individual source and bkgd regions\n");
  fprintf(stderr, "  -o [ofile]\t# output filename (def: stdout)\n");
  fprintf(stderr, "  -p\t\t# output in pixels, even if wcs is present\n");
  fprintf(stderr, "  -r\t\t# output inner/outer radii (and angles) for annuli (and pandas)\n");
  fprintf(stderr, "  -s\t\t# output summed values\n");
  fprintf(stderr, "  -t\t\t# output in strict starbase/rdb table format\n");
  fprintf(stderr, "  -z\t\t# include regions with zero area in output\n");
  exit(1);
}

#ifdef USE_CFITSIO
void regcntsErrchk(Opts opts, int status) {
  FILE *fd;
  if(status){
    if( opts ){
      fd = opts->efd;
    } else {
      fd = stderr;
    }
    fits_report_error(fd, status);
    fflush(fd);
    /* use static pointers to clean up before exit */
#if __EMSCRIPTEN__
    regcntsExit();
#endif
    exit(status);
  }
}
#endif

/* allocate and initialize record structures */
void regcntsInitAlloc(Opts *opts, Data *src, Data *bkg, Res *res){
  /* allocate and initialize options and data records */
  *opts = xcalloc(1, sizeof(OptsRec));
  (*opts)->dobkgderr = 1;
  (*opts)->bktype = BKG_VAL;
  (*opts)->c = ' ';
  (*opts)->bin = 1;
  (*opts)->fd = stdout;
  (*opts)->otype = 0;
  (*opts)->efd = stderr;
  *src = xcalloc(1, sizeof(DataRec));
  (*src)->type = SRC;
  *bkg = xcalloc(1, sizeof(DataRec));
  (*bkg)->type = BKG;
  *res = xcalloc(1, sizeof(ResRec));
  (*res)->dppnorm = 1.0;
#if __EMSCRIPTEN__
  /* static pointers to the "real" structs so we can clean up on exit */
  _opts = *opts;
  _src = *src;
  _bkg = *bkg;
  _res = *res;
  setxerrorexit(regcntsExit);
#endif
}

/* parse command line arguments and figure out what src/bkg types we have */
void regcntsParseArgs(int argc, char **argv,
		      Opts opts, Data src, Data bkg, Res res){
  int c;
  int args;
  char *s;

  /* process switch arguments */
  optind = 1;
  while ((c = getopt(argc, argv, "b:c:e:gGhjmo:prstz1")) != -1){
    switch(c){
    case 'b':
      opts->bin = atoi(optarg);
      break;
    case 'c':
      opts->cube = optarg;
      opts->dodata = 1;
      if( strstr(opts->cube, "all") ){
	opts->docube = 1;
      }
      break;
    case 'e':
      opts->efd = fopen(optarg, "w");
      if( opts->efd ){
	setxerrorfd(opts->efd);
      } else {
	xerror(stderr, "could not open error file for writing: %s\n", optarg);
      }
      break;
    case 'g':
      opts->dog = 1;
      break;
    case 'G':
      opts->dog = 2;
      break;
    case 'h':
      regcntsUsage(argv[0]);
      break;
    case 'j':
      opts->otype = 1;
      break;
    case 'm':
      opts->domatch = 1;
      break;
    case 'o':
      opts->fd = fopen(optarg, "w");
      if( !opts->fd ){
	xerror(stderr, "could not open output file for writing: %s\n", optarg);
      }
      break;
    case 'p':
      opts->dopixels = 1;
      break;
    case 'r':
      opts->doradang = 1;
      break;
    case 's':
      opts->dosum = 1;
      break;
    case 'z':
      opts->dozero = 1;
      break;
    case 'T':
      opts->c = '\t';
      break;
    case '1':
      opts->dodata = 1;
      break;
    }
  }
  /* check for required arguments */
  args = argc - optind;
  if( args < 1 ){
    regcntsUsage(argv[0]);
  }
  /* arg 1: source file name */
  src->name = xstrdup(argv[optind+0]);
  /* arg 2: source region */
  if( (args == 1) || !*(src->regstr = argv[optind+1]) ){
    src->regstr = "field()";
  }
  /* arg 3: background region */
  if( args >= 3 ){
    if( args == 3 ){
      bkg->name = NULL;
      bkg->regstr = argv[optind+2];
    } else {
      bkg->name = xstrdup(argv[optind+2]);
      bkg->regstr = argv[optind+3];
    }
    /* check for constant numeric value -- background counts */
    res->bkgval = strtod(bkg->regstr, &s);
    /* if we did not get a valid numeric constant, its a region */
    /* this might get changed to "each", once we know the number of regions */
    if( bkg->regstr && *bkg->regstr && (s == bkg->regstr) ){
      opts->bktype = BKG_ALL;
      res->bkgval = 0.0;
    } else {
      /* can't have background file and background value */
      if( args == 4 ){
	regcntsUsage(argv[0]);
      }
      opts->bktype = BKG_VAL;
      bkg->regstr = NULL;
    }
  }
}

/* open a data file and grab array of image data */
void regcntsGetData(Opts opts, Data d){
#ifdef USE_CFITSIO
  int ncard;
  int hdutype;
  int naxis;
  int start[IMDIM];
  int stop[IMDIM];
  long naxes[2];
  int status = 0;   /*  CFITSIO status value MUST be initialized to zero!  */
  char *cube=NULL;
  fitsfile *nfptr=NULL;
  /* open the source FITS file */
  d->fptr = openFITSFile(d->name, READONLY, EXTLIST, &hdutype, &status);
  regcntsErrchk(opts, status);
  // process based on hdu type
  switch(hdutype){
  case IMAGE_HDU:
    // we can pny handle 2D images
    fits_get_img_dim(d->fptr, &naxis, &status);
    switch(naxis){
    case 2:
      break;
    case 3:
      if( !opts->cube ){
	opts->cube = "all";
	opts->dodata = 1;
	opts->docube = 1;
      }
      break;
    default:
      xerror(stderr, "2D or 3D images only (this image has %d)\n", naxis);
      break;
    }
    // get cards as a string
    getHeaderToString(d->fptr, &d->cards, &ncard, &status);
    regcntsErrchk(opts, status);
    // get image array
    if( opts->dodata ){
      if( d->type == SRC || ((d->type == BKG) && d->fromsrc) ){
	cube = opts->cube;
      }
      d->data = getImageToArray(d->fptr, NULL, NULL, 1, 0, cube,
				 start, stop, &d->bitpix, &status);
      regcntsErrchk(opts, status);
    }
    d->dim1 = stop[0] - start[0] + 1;
    d->dim2 = stop[1] - start[1] + 1;
    if( opts->docube ){
      if( d->type == SRC || ((d->type == BKG) && d->fromsrc) ){
	d->curslice = 0;
	d->maxslice = stop[2] - start[2] + 1;
	d->szslice = d->dim1 * d->dim2 * abs(d->bitpix) / 8;
	d->data0 = d->data;
      }
    }
    break;
  default:
    // image from table
    nfptr = filterTableToImage(d->fptr, 
			       NULL, NULL, NULL, NULL, opts->bin, &status);
    regcntsErrchk(opts, status);
    // get cards as a string
    getHeaderToString(nfptr, &d->cards, &ncard, &status);
    regcntsErrchk(opts, status);
    if( opts->dodata ){
    // get image array
      d->data = getImageToArray(nfptr, NULL, NULL, 1, 0, NULL,
				start, stop, &d->bitpix, &status);
      regcntsErrchk(opts, status);
      d->dim1 = stop[0] - start[0] + 1;
      d->dim2 = stop[1] - start[1] + 1;
      closeFITSFile(nfptr, &status);
    } else {
      closeFITSFile(d->fptr, &status);
      regcntsErrchk(opts, status);
      d->fptr = nfptr;
    }
    break;
  }
  /* get wcs, if possible */
  hlength(d->cards, 0);
  d->wcs = wcsinit(d->cards);
  /* use entire image for the section */
  if( opts->dodata ){
    d->x0 = 1;
    d->x1 = d->dim1;
    d->y0 = 1;
    d->y1 = d->dim2;
    d->block = 1;
  } else {
    fits_get_img_size(d->fptr, 2, naxes, &status);
    regcntsErrchk(opts, status);
    d->x0 = 1;
    d->x1 = naxes[0];
    d->y0 = 1;
    d->y1 = naxes[1];
    d->block = 1;
  }
#elif USE_FUNTOOLS
  /* open the source FITS file */
  if( !(d->fun = FunOpen(d->name, "r", NULL)) ){
    xerror(stderr, "can't FunOpen file (or find extension): %s\n", d->name);
  }
  /* get image data in double format*/
  if( !(d->data = FunImageGet(d->fun, NULL, NULL)) ){
    xerror(stderr, "can't FunImageGet: %s\n", d->name);
  }
  /* get required information from funtools structure */
  FunInfoGet(d->fun,
	     FUN_HEADER, &d->header,
	     FUN_SECT_X0, &d->x0, FUN_SECT_X1, &d->x1,
	     FUN_SECT_Y0, &d->y0, FUN_SECT_Y1, &d->y1,
	     FUN_SECT_DIM1, &d->dim1, FUN_SECT_BLOCK, &d->block,
	     FUN_BITPIX, &d->bitpix, FUN_WCS, &d->wcs,
	     0);
  d->cards = ft_cards(d->header);
#endif
  /* now see if we can get degrees/pixel from wcs */
  if( d->wcs && iswcs(d->wcs) ){
    if( !d->wcs->coorflip ){
      d->dpp = ABS(d->wcs->cdelt[0]) * d->block;
    } else {
      d->dpp = ABS(d->wcs->cdelt[1]) * d->block;
    }
  }
}

/* if no bkg file is specified, use the src */
void regcntsBkgDataFromSrc(Data src, Data bkg){
  bkg->x0 = src->x0;
  bkg->x1 = src->x1; 
  bkg->y0 = src->y0;
  bkg->y1 = src->y1; 
  bkg->dim1 = src->dim1;
  bkg->bitpix = src->bitpix;
  bkg->block = src->block;
  bkg->cards = src->cards;
  bkg->data = src->data;
  bkg->fromsrc = 1;
#if USE_CFITSIO
  bkg->fptr = src->fptr;
#elif USE_FUNTOOLS
  bkg->fun = src->fun;
  bkg->header = src->header;
#endif
}

/* open and run the region filter, returning mask of valid segments */
int regcntsOpenRegions(Data d){
  char *u, *t;
  d->reg = OpenRegions(d->cards, d->regstr, NULL);
  if( d->reg && (d->reg != NOREGIONS) ){
    /* retrieve region mask segments */
    d->nmask = FilterRegions(d->reg, d->x0, d->x1, d->y0, d->y1, d->block, 
			     &d->masks, &d->nreg);
    /* for display, we have to add # comment chars after each \n */
    t = d->reg->regstr;
    d->filtstr = xcalloc(strlen(t)*3, sizeof(char));
    for(u=d->filtstr; *t; t++){
      *u++ = *t;
      if( *t == '\n' ){
	*u++ = '#';
	*u++ = ' ';
      }
    }
    d->filtstr = xrealloc(d->filtstr, strlen(d->filtstr)+1);
  }
  /* allocate space for results */
  if( d->nreg ){
    d->cnts = (double *)xcalloc(d->nreg, sizeof(double));
    d->savecnts = (double *)xcalloc(d->nreg, sizeof(double));
    d->area = (int *)xcalloc(d->nreg, sizeof(int));
    d->savearea = (int *)xcalloc(d->nreg, sizeof(int));
  }
  return d->nreg;
}

/* knowing region count, allocate result data in the res structs */
void regcntsInitResults(Opts opts, Data src, Data bkg, Res res){
  /* look for degrees/pixel in source and background files -- we will
     use these to normalize background area, if both are present */
  if( bkg->name ){ 
    /* note that BOTH must be present or we do not do this normalization */
    if( (src->dpp > 0.0) && (bkg->dpp > 0.0) ){
      res->dppnorm = (src->dpp/bkg->dpp) * (src->dpp/bkg->dpp);
    }
  } else {
    /* get degrees/pixel for source; not used in norm, but users want to know */
    if( src->wcs && iswcs(src->wcs) ){
      if( !src->wcs->coorflip ){
	src->dpp = ABS(src->wcs->cdelt[0]) * src->block;
      } else {
	src->dpp = ABS(src->wcs->cdelt[1]) * src->block;
      }
    }
  }
  /* get radii and angle string, if needed */
  if( opts->doradang && src->reg ){
    res->radang = xstrdup(src->reg->radang);
  }
  /* allocate space for results */
  res->bncnts = (double *)xcalloc(src->nreg, sizeof(double));
  res->bnerr = (double *)xcalloc(src->nreg, sizeof(double));
  res->bscnts = (double *)xcalloc(src->nreg, sizeof(double));
  res->bserr = (double *)xcalloc(src->nreg, sizeof(double));
}

/* get counts in each region */
void regcntsCountsInRegions(Opts opts, Data d){
  int i, j, y, yoff;
  int lasty=-1;
#ifdef USE_CFITSIO
  int status = 0;
  long fpixel[4];
#endif
  char *cbuf;
  short *sbuf;
  unsigned short *usbuf;
  int *ibuf;
  long long *lbuf;
  float *fbuf;
  double *dbuf;
  if( d->data ){
    /* loop through each mask, adding up counts and area */
    for(i=0; i<d->nmask; i++){
      y = d->masks[i].y;
      yoff = (y - 1) * d->dim1;
      d->area[d->masks[i].region-1] += d->masks[i].xstop - d->masks[i].xstart+1;
      switch(d->bitpix){
      case 8:
	cbuf = (char *)d->data;
	for(j=d->masks[i].xstart-1; j<=d->masks[i].xstop-1; j++){
	  d->cnts[d->masks[i].region-1] += cbuf[yoff+j];
	}
	break;
      case 16:
	sbuf = (short *)d->data;
	for(j=d->masks[i].xstart-1; j<=d->masks[i].xstop-1; j++){
	  d->cnts[d->masks[i].region-1] += sbuf[yoff+j];
	}
	break;
      case -16:
	usbuf = (unsigned short *)d->data;
	for(j=d->masks[i].xstart-1; j<=d->masks[i].xstop-1; j++){
	  d->cnts[d->masks[i].region-1] += usbuf[yoff+j];
	}
	break;
      case 32:
	ibuf = (int *)d->data;
	for(j=d->masks[i].xstart-1; j<=d->masks[i].xstop-1; j++){
	  d->cnts[d->masks[i].region-1] += ibuf[yoff+j];
	}
	break;
      case 64:
	lbuf = (long long *)d->data;
	for(j=d->masks[i].xstart-1; j<=d->masks[i].xstop-1; j++){
	  d->cnts[d->masks[i].region-1] += lbuf[yoff+j];
	}
	break;
      case -32:
	fbuf = (float *)d->data;
	for(j=d->masks[i].xstart-1; j<=d->masks[i].xstop-1; j++){
	  d->cnts[d->masks[i].region-1] +=
	    (isnan(fbuf[yoff+j]) ? 0 : fbuf[yoff+j]);
	}
	break;
      case -64:
	dbuf = (double *)d->data;
	for(j=d->masks[i].xstart-1; j<=d->masks[i].xstop-1; j++){
	  d->cnts[d->masks[i].region-1] +=
	    (isnan(dbuf[yoff+j]) ? 0 : dbuf[yoff+j]);
	}
	break;
      default:
	break;
      }
    }
  } else {
    /* allocate a row buffer */
    dbuf = xmalloc((d->x1 - d->x0 + 1) * sizeof(double));
    for(i=0; i<d->nmask; i++){
      y = d->masks[i].y;
      if( y != lasty ){
#ifdef USE_CFITSIO
	fpixel[0] = 1;
	fpixel[1] = y;
	fpixel[2] = 1;
	fpixel[3] = 1;
	fits_read_pix(d->fptr, TDOUBLE, fpixel, d->x1, 0, dbuf, 0, &status);
	regcntsErrchk(opts, status);
#elif USE_FUNTOOLS
	if( !FunImageRowGet(d->fun, dbuf, y, y, "bitpix=-64") ){
	  xerror(stderr, "can't FunImageRowGet: %d %s\n", y, d->name);
	}
#endif
	lasty = y;
      }
      d->area[d->masks[i].region-1] += 
	d->masks[i].xstop - d->masks[i].xstart + 1;
      for(j=d->masks[i].xstart-1; j<=d->masks[i].xstop-1; j++){
	d->cnts[d->masks[i].region-1] += (isnan(dbuf[j]) ? 0 : dbuf[j]);
      }
    }
    /* free temp buffer */
    xfree(dbuf);
  }
}

/* sum region counts and area if we are in 'sum' mode */
void regcntsSumCounts(Opts opts, Data src, Data bkg){
  int i;
  /* if we need to display sums, do the sum now, but save unsummed values,
     because we will have to display those as well */
  switch(opts->dosum){
  case 1:
    // save unsummed counts
    memcpy(src->savecnts, src->cnts, src->nreg*sizeof(double));
    memcpy(src->savearea, src->area, src->nreg*sizeof(int));
    for(i=1; i<src->nreg; i++){
      src->cnts[i] += src->cnts[i-1];
      src->area[i] += src->area[i-1];
    }
    // sum counts
    if( opts->bktype != BKG_VAL ){
      memcpy(bkg->savecnts, bkg->cnts, bkg->nreg*sizeof(double));
      memcpy(bkg->savearea, bkg->area, bkg->nreg*sizeof(int));
      for(i=1; i<bkg->nreg; i++){
	bkg->cnts[i] += bkg->cnts[i-1];
	bkg->area[i] += bkg->area[i-1];
      }
    }
    break;
  case 2:
    // restore unsummed counts
    memcpy(src->cnts, src->savecnts, src->nreg*sizeof(double));
    memcpy(src->area, src->savearea, src->nreg*sizeof(int));
    if( opts->bktype != BKG_VAL ){
      memcpy(bkg->cnts, bkg->savecnts, bkg->nreg*sizeof(double));
      memcpy(bkg->area, bkg->savearea, bkg->nreg*sizeof(int));
    }
    break;
  default:
    break;
  }
}

/* process the background data */
void regcntsSubtractBkg(Opts opts, Data src, Data bkg, Res res){
  int i;
  double tempnorm=0.0;
  /* process the background */
  switch(opts->bktype){
  /* constant background is counts/pixel */
  case BKG_VAL:
    for(i=0; i<src->nreg; i++){
      res->bncnts[i] = (res->bkgval * src->area[i]);
      res->bscnts[i] = src->cnts[i] - res->bncnts[i];
      res->bserr[i] = sqrt(src->cnts[i]);
      res->bnerr[i] = 0.0;
    }
    break;
  case BKG_ALL:
    switch( opts->dosum ){
    case 1:
      res->bkgval =  bkg->cnts[bkg->nreg-1];
      res->bkgarea = bkg->area[bkg->nreg-1];
      break;
    case 2:
    default:
      res->bkgval = 0.0;
      res->bkgarea = 0;
      /* get total background and background area */
      for(i=0; i<bkg->nreg; i++){
	res->bkgval +=  bkg->cnts[i];
	res->bkgarea += bkg->area[i];
      }
      break;
    }
    /* now check for a valid background area */
    if( !res->bkgarea ){
      xerror(stderr, "background has zero area\n");
    }
    /* subtract entire normalized background from each source region */
    for(i=0; i<src->nreg; i++){
      tempnorm = (double)src->area[i] / res->bkgarea;
      /* add normalization due to different pixels sizes in src and bkg */
      tempnorm *= res->dppnorm;
      res->bncnts[i] = (res->bkgval * tempnorm);
      res->bscnts[i] = src->cnts[i] - res->bncnts[i];
      res->bserr[i] = sqrt(src->cnts[i] + (tempnorm*tempnorm*res->bkgval));
      if( opts->dobkgderr ){
	res->bnerr[i] = sqrt(res->bkgval) * tempnorm;
      } else {
	res->bnerr[i] = 0.0;
      }
    }
    break;
  case BKG_EACH:
    /* subtract each background from each source region */
    for(i=0; i<src->nreg; i++){
      if( !bkg->area[i] ){
	xerror(stderr, "background region %d has zero area\n", i);
      }
      tempnorm = (double)src->area[i] / (double)bkg->area[i];
      /* add normalization due to different pixels sizes in src and bkg */
      tempnorm *= res->dppnorm;
      res->bncnts[i] = (bkg->cnts[i] * tempnorm);
      res->bscnts[i] = src->cnts[i] - res->bncnts[i];
      res->bserr[i] = sqrt(src->cnts[i] + (tempnorm * tempnorm * bkg->cnts[i]));
      if( opts->dobkgderr ){
	res->bnerr[i] = sqrt(bkg->cnts[i]) * tempnorm;
      } else {
	res->bnerr[i] = 0.0;
      }
    }
    break;
  }
}

/* routine to clean up on exit */
void regcntsExit(void){
#if __EMSCRIPTEN__
  regcntsCleanUp(_opts, _src, _bkg, _res);
#endif
  return;
}

/* clear arrays */
void regcntsClearArrays(Data d, Res res){
  if( d != NULL ){
    /* clear src or bkg arrays */
    memset((char *)d->cnts, 0, d->nreg * sizeof(double));
    memset((char *)d->savecnts, 0, d->nreg * sizeof(double));
    memset((char *)d->area, 0, d->nreg * sizeof(int));
    memset((char *)d->savearea, 0, d->nreg * sizeof(int));
    /* clear results arrays */
    if( (d->type == SRC) && (res != NULL) ){
      memset((char *)res->bncnts, 0, d->nreg * sizeof(double));
      memset((char *)res->bnerr, 0, d->nreg * sizeof(double));
      memset((char *)res->bscnts, 0, d->nreg * sizeof(double));
      memset((char *)res->bserr, 0, d->nreg * sizeof(double));
    }
  }
}

/* clean up */
void regcntsCleanUp(Opts opts, Data src, Data bkg, Res res){
#ifdef USE_CFITSIO
  int status = 0;
#endif
  /* opts */
  if( opts->efd != NULL ){
    fflush(opts->efd);
    if( opts->efd != stderr && opts->efd != stdout ){
      fclose(opts->efd);
    }
  }
  if( opts->fd != NULL ){
    fflush(opts->fd);
    if( opts->fd != stdout && opts->fd != stderr ){
      fclose(opts->fd);
    }
  }
  xfree(opts);
  /* background data */
  xfree(bkg->cnts);
  xfree(bkg->savecnts);
  xfree(bkg->area);
  xfree(bkg->savearea);
  xfree(bkg->masks);
  xfree(bkg->filtstr);
  xfree(bkg->name);
  if( bkg->data != src->data ){
    xfree(bkg->data);
  }
  if( bkg->cards != src->cards ){
    xfree(bkg->cards);
  }
  wcsfree(bkg->wcs);
  CloseRegions(bkg->reg);
#ifdef USE_CFITSIO
  if( bkg->name && bkg->fptr  ){
    closeFITSFile(bkg->fptr, &status);
  }
#elif USE_FUNTOOLS
  FunClose(bkg->fun); 
#endif
  xfree(bkg);
  /* source data */
  xfree(src->cnts);
  xfree(src->savecnts);
  xfree(src->area);
  xfree(src->savearea);
  xfree(src->masks);
  xfree(src->filtstr);
  xfree(src->name);
  xfree(src->cards);
  xfree(src->data);
  wcsfree(src->wcs);
  CloseRegions(src->reg);
#ifdef USE_CFITSIO
  if( src->name && src->fptr  ){
    closeFITSFile(src->fptr, &status);
  }
#elif USE_FUNTOOLS
  FunClose(src->fun); 
#endif
  xfree(src);
  /* results */
  xfree(res->bncnts);
  xfree(res->bnerr);
  xfree(res->bscnts);
  xfree(res->bserr);
  xfree(res->radang);
  xfree(res);
}
