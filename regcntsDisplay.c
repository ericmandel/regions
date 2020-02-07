/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * _regcntsDisplay: formatted output for regcnts
 *
 */

#include <time.h>
#include "regcnts.h"

/* display results header */
static void regcntsDisplayHeaderRDB(Opts opts, Data src, Data bkg, Res res){
  char *s;
  /* display source header information */
  fprintf(opts->fd, "# source\n");
  fprintf(opts->fd, "#   data_file:\t\t%s\n", src->name);
  /* slice information */
  if( opts->docube ){
    fprintf(opts->fd, "#   cube_slices:\t%d\n", src->maxslice);
  }
  if( opts->bin != 1 ){
    fprintf(opts->fd, "#   auto_block:\t\t%.2f\n", opts->bin);
  }
  if( src->dpp > 0.0 ){
    fprintf(opts->fd, "#   arcsec/pixel:\t%g\n", src->dpp*ASEC_DEG);
  }
  /* display bkgd header information */
  fprintf(opts->fd, "# background\n");
  if( bkg->name ){
    fprintf(opts->fd, "#   data_file:\t\t%s\n", bkg->name);
    if( bkg->dpp > 0.0 ){
      fprintf(opts->fd, "#   arcsec/pixel:\t%g\n", bkg->dpp*ASEC_DEG);
    }
    if( res->dppnorm != 1.0 ){
      fprintf(opts->fd, "# wcs area norm factor:\t%g/%g (source/bkgd))\n",
	      src->dpp,bkg->dpp);
    }
  } else if( opts->bktype != BKG_VAL ){
    fprintf(opts->fd, "#   data_file:\t\t%s\n", src->name);
  } else {
    fprintf(opts->fd, "#   constant_value:\t%.6f\n", res->bkgval);
  }
  /* dislay table information */
  if( (src->dpp > 0.0) && !opts->dopixels ){
    s = "arcsec";
  } else {
    s = "pixel";
  }
  fprintf(opts->fd, "# column units\n");
  fprintf(opts->fd, "#   area:\t\t%s**2\n", s);
  fprintf(opts->fd, "#   surf_bri:\t\tcnts/%s**2\n", s);
  fprintf(opts->fd, "#   surf_err:\t\tcnts/%s**2\n", s);
  if( opts->doradang ){
    fprintf(opts->fd, "#   radii:\t\t%ss\n", s);
    fprintf(opts->fd, "#   angles:\t\tdegrees\n");
  }
}

/* display main results info */
static void regcntsDisplayMainInfoRDB(Opts opts, Data src, Res res){
  int i, j;
  char *cradang=NULL;
  char *tradang=NULL;
  char *fmt=NULL;
  char tbuf[SZ_LINE];
  char s[SZ_LINE];
  // do we need to tag the slice?
  s[0] = '\0';
  if( opts->docube ){
    snprintf(s, SZ_LINE, " #%d", src->curslice+1);
  }
  switch( opts->dosum ){
  case 1:
    if( !opts->docube ){
      fprintf(opts->fd, "\n");
    }
    fprintf(opts->fd, "# summed background-subtracted results%s\n", s);
    fprintf(opts->fd, "upto%c  net_counts%c    error", opts->c, opts->c);
    break;
  case 2:
  default:
    if( opts->c == '\t' ){
      fprintf(opts->fd, "\f");
    }
    if( !opts->docube ){
      fprintf(opts->fd, "\n");
    }
    fprintf(opts->fd, "# background-subtracted results%s\n", s);
    fprintf(opts->fd, " reg%c  net_counts%c    error", opts->c, opts->c);
    break;
  }
  fprintf(opts->fd, "%c  background%c   berror", opts->c, opts->c);
  fprintf(opts->fd, "%c     area%c surf_bri%c surf_err",
	  opts->c, opts->c, opts->c);
  if( opts->doradang ){
    fprintf(opts->fd, "%c  radius1%c  radius2%c   angle1%c   angle2",
	    opts->c, opts->c, opts->c, opts->c);
  }
  fprintf(opts->fd, "\n");
  fprintf(opts->fd, "----%c------------%c---------", opts->c, opts->c);
  fprintf(opts->fd, "%c------------%c---------", opts->c, opts->c);
  fprintf(opts->fd, "%c---------%c---------%c---------",
	  opts->c, opts->c, opts->c);
  if( opts->doradang ){
    fprintf(opts->fd, "%c---------%c---------%c---------%c---------",
	    opts->c, opts->c, opts->c, opts->c);
  }
  fprintf(opts->fd, "\n");
  if( res->radang ){
    newdtable(",");
  }
  cradang = res->radang;
  for(i=0; i<src->nreg; i++){
    /* get next line from radii/angle string */
    if( cradang ){
      tradang = (char *)strchr(cradang, '\n');
      if( tradang ){
	*tradang = '\0';
      }
    }
    if( src->area[i] ){
      double cntsperarea;
      double errperarea;
      double areasq;
      cntsperarea = res->bscnts[i]/src->area[i];
      errperarea = res->bserr[i]/src->area[i];
      areasq = src->area[i];
      /* if we know how to convert to cnts/pix**2 to cnts/arcsec**2, do it */
      if( !opts->dopixels && (src->dpp > 0.0) ){
	cntsperarea = (cntsperarea / (src->dpp*src->dpp)) / ASEC_DEGSQ;
	errperarea  = (errperarea  / (src->dpp*src->dpp)) / ASEC_DEGSQ;
	areasq      = (areasq      * (src->dpp*src->dpp)) * ASEC_DEGSQ;
      }
      /* get correctly precisioned format statement */
      switch(opts->dog){
      case 0:
	fmt = "%4d%c%12.3f%c%9.3f%c%12.3f%c%9.3f%c%9.2f%c%9.3f%c%9.3f";
	break;
      case 1:
	fmt = "%4d%c%12.3g%c%9.3g%c%12.3g%c%9.3g%c%9.2g%c%9.3g%c%9.3g";
	break;
      case 2:
	fmt = "%4d%c%.14g%c%.14g%c%.14g%c%.14g%c%.14g%c%.14g%c%.14g";
	break;
      }
      fprintf(opts->fd, fmt,
	      i+1, opts->c,
	      res->bscnts[i], opts->c, res->bserr[i], opts->c,
	      res->bncnts[i], opts->c, res->bnerr[i], opts->c,
	      areasq, opts->c, cntsperarea, opts->c, errperarea);
      /* display values from this line of radii/angles */
      if( opts->doradang && cradang ){
	int ip=0;
	double dval;
	for(j=0; j<4; j++){
	  if( word(cradang, tbuf, &ip) && strcmp(tbuf, "NA") ){
	    dval = strtod(tbuf, NULL);
	    if( (j<2) && !opts->dopixels && (src->dpp>0.0) ){
	      fprintf(opts->fd, "%c%9.3f", opts->c, (dval*src->dpp*ASEC_DEG));
	    } else {
	      fprintf(opts->fd, "%c%9.3f", opts->c, dval);
	    }
	  } else {
	    fprintf(opts->fd, "%c%9.9s", opts->c, "NA");
	  }
	}
      }
      /* new-line at end  */
      fprintf(opts->fd, "\n");
    } else if( opts->dozero ){
    /* might have to display zero area pixels */
      /* get correctly precisioned format statement */
      switch(opts->dog){
      case 0:
	fmt = "%4d%c%12.3f%c%9.3f%c%12.3f%c%9.3f%c%9.2f%c%9.3f%c%9.3f";
	break;
      case 1:
	fmt = "%4d%c%12.3g%c%9.3g%c%12.3g%c%9.3g%c%9.2g%c%9.3g%c%9.3g";
	break;
      case 2:
	fmt = "%4d%c%.14g%c%.14g%c%.14g%c%.14g%c%.14g%c%.14g%c%.14g";
	break;
      }
      fprintf(opts->fd, fmt,
	      i+1, opts->c, 0.0, opts->c, 0.0, opts->c, 0.0, opts->c, 0.0,
	      opts->c, 0.0, opts->c, 0.0, opts->c, 0.0);
      /* add the correct radii and angle info, to make plotting easier */
      if( opts->doradang && cradang ){
	int ip=0;
	double dval;
	for(j=0; j<4; j++){
	  if( word(cradang, tbuf, &ip) && strcmp(tbuf, "NA") ){
	    dval = strtod(tbuf, NULL);
	    if( (j<2) && !opts->dopixels && (src->dpp>0.0) ){
	      fprintf(opts->fd, "%c%9.3f", opts->c, (dval*src->dpp*ASEC_DEG));
	    } else {
	      fprintf(opts->fd, "%c%9.3f", opts->c, dval);
	    }
	  } else {
	    fprintf(opts->fd, "%c%9.9s", opts->c, "NA");
	  }
	}
      }
      /* new-line at end  */
      fprintf(opts->fd, "\n");
    }
    /* bump to next line of radii/angles */
    if( tradang ){
      cradang = tradang+1;
      /* put back the cr in case we pass through again */
      *tradang = '\n';
    }
  }
  if( res->radang ){
    freedtable();
  }
  fprintf(opts->fd, "\n");
  fflush(opts->fd);
}

/* display raw source info */
static void regcntsDisplaySrcInfoRDB(Opts opts, Data src){
  int i;
  int tarea=0;
  double tcnts=0;
  char *fmt=NULL;
  /* display raw source counts */
  if( opts->dosum ){
    if( opts->c == '\t' ){
      fprintf(opts->fd, "\f");
    }
    fprintf(opts->fd, "\n");
    /* display source info */
    if( src->filtstr ){
      fprintf(opts->fd, "# source_region(s):\n");
      fprintf(opts->fd, "# %s\n\n", src->filtstr);
    }
    fprintf(opts->fd, "# summed_source_data\n");
    fprintf(opts->fd, " reg%c      counts%c   pixels%c     sumcnts%c   sumpix\n",
	    opts->c, opts->c, opts->c, opts->c);
    fprintf(opts->fd,
	    "----%c------------%c---------%c------------%c---------\n",
	    opts->c, opts->c, opts->c, opts->c);
    for(i=0; i<src->nreg; i++){
      tcnts += src->cnts[i];
      tarea += src->area[i];
      /* get correctly precisioned format statement */
      switch(opts->dog){
      case 0:
	fmt = "%4d%c%12.3f%c%9d%c%12.3f%c%9d\n";
	break;
      case 1:
	fmt = "%4d%c%12.3g%c%9d%c%12.3g%c%9d\n";
	break;
      case 2:
	fmt = "%4d%c%.14g%c%9d%c%.14g%c%9d\n";
	break;
      }
      fprintf(opts->fd, fmt,
	      i+1, opts->c,
	      src->cnts[i], opts->c, src->area[i], opts->c,
	      tcnts, opts->c, tarea);
    }
  } else {
    if( opts->c == '\t' ){
      fprintf(opts->fd, "\f");
    }
    /* display source info */
    if( src->filtstr ){
      fprintf(opts->fd, "# source_region(s):\n");
      fprintf(opts->fd, "# %s\n\n", src->filtstr);
    }
    fprintf(opts->fd, "# source_data\n");
    fprintf(opts->fd, " reg%c      counts%c   pixels", opts->c, opts->c);
    fprintf(opts->fd, "\n");
    fprintf(opts->fd, "----%c------------%c---------", opts->c, opts->c);
    fprintf(opts->fd, "\n");
    for(i=0; i<src->nreg; i++){
      /* get correctly precisioned format statement */
      switch(opts->dog){
      case 0:
	fmt = "%4d%c%12.3f%c%9d";
	break;
      case 1:
	fmt = "%4d%c%12.3g%c%9d";
	break;
      case 2:
	fmt = "%4d%c%.14g%c%9d";
	break;
      }
      fprintf(opts->fd, fmt, i+1, opts->c, src->cnts[i], opts->c, src->area[i]);
      fprintf(opts->fd, "\n");
    }
  }
  fprintf(opts->fd, "\n");
  fflush(opts->fd);
}

/* display raw background info */
static void regcntsDisplayBkgInfoRDB(Opts opts, Data bkg, Res res){
  int i;
  char *fmt=NULL;
  /* display raw background info */
  switch(opts->bktype){
  case BKG_VAL:
    break;
  case BKG_ALL:
    if( opts->c == '\t' ){
      fprintf(opts->fd, "\f");
    }
    if( bkg->filtstr ){
      fprintf(opts->fd, "# background_region(s)\n");
      fprintf(opts->fd, "# %s\n\n", bkg->filtstr);
    }
    fprintf(opts->fd, "# background_data\n");
    fprintf(opts->fd, " reg%c      counts%c   pixels", opts->c, opts->c);
    fprintf(opts->fd, "\n");
    fprintf(opts->fd, "----%c------------%c---------", opts->c, opts->c);
    fprintf(opts->fd, "\n");
    /* get correctly precisioned format statement */
    switch(opts->dog){
    case 0:
      fmt = "%s%c%12.3f%c%9d";
      break;
    case 1:
      fmt = "%s%c%12.3g%c%9d";
      break;
    case 2:
      fmt = "%s%c%.14g%c%9d";
      break;
    }
    fprintf(opts->fd, fmt, "all ", opts->c, res->bkgval, opts->c, res->bkgarea);
    fprintf(opts->fd, "\n");
    fprintf(opts->fd, "\n");
    break;
  case BKG_EACH:
    if( opts->dosum ){
      int tarea=0;
      double tcnts=0;
      if( opts->c == '\t' ){
	fprintf(opts->fd, "\f");
      }
      if( bkg->filtstr ){
	fprintf(opts->fd, "# background_region(s)\n");
	fprintf(opts->fd, "# %s\n\n", bkg->filtstr);
      }
      fprintf(opts->fd, "# summed_background_data\n");
      fprintf(opts->fd,
	      " reg%c      counts%c   pixels%c     sumcnts%c   sumpix\n",
	      opts->c, opts->c, opts->c, opts->c);
      fprintf(opts->fd,
	      "----%c------------%c---------%c------------%c---------\n",
	      opts->c, opts->c, opts->c, opts->c);
      for(i=0; i<bkg->nreg; i++){
	tcnts += bkg->cnts[i];
	tarea += bkg->area[i];
	/* get correctly precisioned format statement */
	switch(opts->dog){
	case 0:
	  fmt = "%4d%c%12.3f%c%9d%c%12.3f%c%9d\n";
	  break;
	case 1:
	  fmt = "%4d%c%12.3g%c%9d%c%12.3g%c%9d\n";
	  break;
	case 2:
	  fmt = "%4d%c%.14g%c%9d%c%.14g%c%9d\n";
	  break;
	}
	fprintf(opts->fd, fmt, i+1, opts->c,
		bkg->cnts[i], opts->c, bkg->area[i], opts->c,
		tcnts, opts->c, tarea);
      }
    } else {
      if( opts->c == '\t' ) fprintf(opts->fd, "\f");
      if( bkg->filtstr ){
	fprintf(opts->fd, "# background_region(s)\n");
	fprintf(opts->fd, "# %s\n\n", bkg->filtstr);
      }
      fprintf(opts->fd, "# background_data\n");
      fprintf(opts->fd, " reg%c      counts%c   pixels", opts->c, opts->c);
      fprintf(opts->fd, "\n");
      fprintf(opts->fd, "----%c------------%c---------", opts->c, opts->c);
      fprintf(opts->fd, "\n");
      for(i=0; i<bkg->nreg; i++){
	/* get correctly precisioned format statement */
	switch(opts->dog){
	case 0:
	  fmt = "%4d%c%12.3f%c%9d";
	  break;
	case 1:
	  fmt = "%4d%c%12.3g%c%9d";
	  break;
	case 2:
	  fmt = "%4d%c%.14g%c%9d";
	  break;
	}
	fprintf(opts->fd, fmt, i+1, opts->c, bkg->cnts[i], opts->c, bkg->area[i]);
	fprintf(opts->fd, "\n");
      }
    }
    fprintf(opts->fd, "\n");
    break;
  }
  fflush(opts->fd);
}

/* make (filter) string json-worthy */
static char *escjason(char *s){
  int i, j;
  int slen = 0;
  char *t = NULL;
  if( s && *s ){
    slen = strlen(s);
  }
  t = calloc(slen * 2 + 1, sizeof(char));
  for(i=0, j=0; i<slen; i++){
    if( s[i] == '"' ){
      t[j++] = '\\';
      t[j++] = s[i];
    } else if( s[i] == '\n' ){
      t[j++] = ';';
      if( s[i+1] == '#' ) i++;
    } else {
      t[j++] = s[i];
    }
  }
  t[j] = '\0';
  return t;
}

/* output a line of json, first removing NaN */
static void nonanjson(FILE *fd, char *s){
  int done = 0;
  char *nxt, *nptr;
  char *cur = s;
  while( !done ){
    if( (nptr = strstr(cur, "-nan")) || (nptr = strstr(cur, "-NaN")) ){
      nxt = nptr + 4;
    } else if( (nptr = strstr(cur, "nan")) || (nptr = strstr(cur, "NaN")) ){
      nxt = nptr + 3;
    } else {
      nxt = NULL;
    }
    // if we found a NaN
    if( nptr ){
      // replace with a NULL
      *nptr = '\0';
    }
    // output line up to NaN
    fprintf(fd, "%s", cur);
    if( nptr ){
      // output NaN replacement
      fprintf(fd, "\"NaN\"");
    }
    // skip past NaN or finish
    if( nxt ){
      cur = nxt;
    } else {
      done = 1;
    }
  }
}

/* display results header */
static void regcntsDisplayHeaderJSON(Opts opts, Data src, Data bkg, Res res){
  char *s;
  /* display source header information */
  fprintf(opts->fd, "{\n");
  fprintf(opts->fd, "  \"source\": {\n");
  if( opts->bin != 1 ){
    fprintf(opts->fd, "    \"autoBlock\": %.2f,\n", opts->bin);
  }
  if( src->dpp > 0.0 ){
    fprintf(opts->fd, "    \"arcsecPerPixel\": %g,\n", src->dpp*ASEC_DEG);
  }
  if( opts->docube ){
    fprintf(opts->fd, "    \"cubeSlices\": %d,\n", src->maxslice);
  }
  fprintf(opts->fd, "    \"dataFile\": \"%s\"\n", src->name);
  fprintf(opts->fd, "  },\n");
  /* display bkgd header information */
  fprintf(opts->fd, "  \"background\": {\n");
  if( bkg->name ){
    if( bkg->dpp > 0.0 ){
      fprintf(opts->fd, "    \"arcsecPerPixel\": %g\n", bkg->dpp*ASEC_DEG);
    }
    if( res->dppnorm != 1.0 ){
      fprintf(opts->fd, "    \"wcsAreaNormFactor\": \"%g/%g\",\n",
	      src->dpp,bkg->dpp);
    }
    fprintf(opts->fd, "    \"dataFile\": \"%s\"\n", bkg->name);
  } else if( opts->bktype != BKG_VAL ){
    fprintf(opts->fd, "    \"dataFile\": \"%s\"\n", src->name);
  } else {
    fprintf(opts->fd, "    \"constantValue\": %.6f\n", res->bkgval);
  }
  fprintf(opts->fd, "  },\n");
  /* dislay table information */
  if( (src->dpp > 0.0) && !opts->dopixels ){
    s = "arcsec";
  } else {
    s = "pixel";
  }
  fprintf(opts->fd, "  \"columnUnits\": {\n");
  if( opts->doradang ){
    fprintf(opts->fd, "    \"radii\": \"%ss\",\n", s);
    fprintf(opts->fd, "    \"angles\": \"degrees\",\n");
  }
  fprintf(opts->fd, "    \"area\": \"%s**2\",\n", s);
  fprintf(opts->fd, "    \"surfBrightness\": \"cnts/%s**2\",\n", s);
  fprintf(opts->fd, "    \"surfError\": \"cnts/%s**2\"\n", s);
  fprintf(opts->fd, "  },\n");
}

/* display main results info */
static void regcntsDisplayMainInfoJSON(Opts opts, Data src, Res res){
  int i, j;
  char *cradang=NULL;
  char *tradang=NULL;
  char *fmt=NULL;
  char tbuf[SZ_LINE];
  char lbuf[SZ_LINE];
  char *header[SZ_LINE];
  char s[SZ_LINE];
  // do we need to tag the slice?
  s[0] = '\0';
  if( opts->docube ){
    snprintf(s, SZ_LINE, "%d", src->curslice+1);
  }
  switch( opts->dosum ){
  case 1:
    fprintf(opts->fd, "  \"summedBackgroundSubtractedResults%s\": [\n", s);
    header[0] = "upto";
    header[1] = "netCounts";
    header[2] = "error";
    break;
  case 2:
  default:
    fprintf(opts->fd, "  \"backgroundSubtractedResults%s\": [\n", s);
    header[0] = "reg";
    header[1] = "netCounts";
    header[2] = "error";
    break;
  }
  header[3] = "background";
  header[4] = "berror";
  header[5] = "area";
  header[6] = "surfBrightness";
  header[7] = "surfError";
  if( opts->doradang ){
    header[8] = "radius1";
    header[9] = "radius2";
    header[10] = "angle1";
    header[11] = "angle2";
  }
  if( res->radang ){
    newdtable(",");
  }
  cradang = res->radang;
  for(i=0; i<src->nreg; i++){
    /* get next line from radii/angle string */
    if( cradang ){
      tradang = (char *)strchr(cradang, '\n');
      if( tradang ){
	*tradang = '\0';
      }
    }
    if( src->area[i] ){
      double cntsperarea;
      double errperarea;
      double areasq;
      cntsperarea = res->bscnts[i]/src->area[i];
      errperarea = res->bserr[i]/src->area[i];
      areasq = src->area[i];
      /* if we know how to convert to cnts/pix**2 to cnts/arcsec**2, do it */
      if( !opts->dopixels && (src->dpp > 0.0) ){
	cntsperarea = (cntsperarea / (src->dpp*src->dpp)) / ASEC_DEGSQ;
	errperarea  = (errperarea  / (src->dpp*src->dpp)) / ASEC_DEGSQ;
	areasq      = (areasq      * (src->dpp*src->dpp)) * ASEC_DEGSQ;
      }
      /* get correctly precisioned format statement */
      switch(opts->dog){
      case 0:
	fmt = "    {\"%s\": %d, \"%s\": %.3f, \"%s\": %.3f, \"%s\": %.3f, \"%s\": %.3f, \"%s\": %.2f, \"%s\": %.3f, \"%s\": %.3f";
	break;
      case 1:
	fmt = "    {\"%s\": %4d, \"%s\": %.3g, \"%s\": %.3g, \"%s\": %.3g, \"%s\": %.3g, \"%s\": %.2g, \"%s\": %.3g, \"%s\": %.3g";
	break;
      case 2:
	fmt = "    {\"%s\": %d, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g";
	break;
      }
      snprintf(lbuf, SZ_LINE, fmt,
	      header[0], i+1,
	      header[1], res->bscnts[i],
	      header[2], res->bserr[i],
	      header[3], res->bncnts[i],
	      header[4], res->bnerr[i],
	      header[5], areasq,
	      header[6], cntsperarea,
	      header[7], errperarea);
      nonanjson(opts->fd, lbuf);
      /* display values from this line of radii/angles */
      if( opts->doradang && cradang ){
	int ip=0;
	double dval;
	fprintf(opts->fd, ", ");
	for(j=0; j<4; j++){
	  if( word(cradang, tbuf, &ip) && strcmp(tbuf, "NA") ){
	    dval = strtod(tbuf, NULL);
	    if( (j<2) && !opts->dopixels && (src->dpp>0.0) ){
	      snprintf(lbuf, SZ_LINE, " \"%s\": %.3f", header[8+j], (dval*src->dpp*ASEC_DEG));
	    } else {
	      snprintf(lbuf, SZ_LINE, " \"%s\": %.3f", header[8+j], dval);
	    }
	  } else {
	    snprintf(lbuf, SZ_LINE, " \"%s\": %.9s", header[8+j], "\"NA\"");
	  }
	  nonanjson(opts->fd, lbuf);
	  if( j != 3 ){
	    fprintf(opts->fd, ",");
	  }
	}
      }
      /* new-line at end  */
      fprintf(opts->fd, "}");
      if( i != (src->nreg-1) ){
	fprintf(opts->fd, ",");
      }
      /* new-line at end  */
      fprintf(opts->fd, "\n");
    } else if( opts->dozero ){
    /* might have to display zero area pixels */
      /* get correctly precisioned format statement */
      switch(opts->dog){
      case 0:
	fmt = "    {\"%s\": %4d, \"%s\": %12.3f, \"%s\": %9.3f, \"%s\": %12.3f, \"%s\": %9.3f, \"%s\": %9.2f, \"%s\": %9.3f, \"%s\": %9.3f";
	break;
      case 1:
	fmt = "    {\"%s\": %4d, \"%s\": %12.3g, \"%s\": %9.3g, \"%s\": %12.3g, \"%s\": %9.3g, \"%s\": %9.2g, \"%s\": %9.3g, \"%s\": %9.3g";
	break;
      case 2:
	fmt = "    {\"%s\": %4d, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g, \"%s\": %.14g";
	break;
      }
      fprintf(opts->fd, fmt,
	      header[0], i+1,
	      header[1], 0.0,
	      header[2], 0.0,
	      header[3], 0.0,
	      header[4], 0.0,
	      header[5], 0.0,
	      header[6], 0.0,
	      header[7], 0.0);
      /* add the correct radii and angle info, to make plotting easier */
      if( opts->doradang && cradang ){
	int ip=0;
	double dval;
	fprintf(opts->fd, ", ");
	for(j=0; j<4; j++){
	  if( word(cradang, tbuf, &ip) && strcmp(tbuf, "NA") ){
	    dval = strtod(tbuf, NULL);
	    if( (j<2) && !opts->dopixels && (src->dpp>0.0) ){
	      snprintf(lbuf, SZ_LINE, " \"%s\": %9.3f", header[8+j], (dval*src->dpp*ASEC_DEG));
	    } else {
	      snprintf(lbuf, SZ_LINE, " \"%s\": %9.3f", header[8+j], dval);
	    }
	  } else {
	    snprintf(lbuf, SZ_LINE, " \"%s\": %9.9s", header[8+j], "\"NA\"");
	  }
	  nonanjson(opts->fd, lbuf);
	  if( j != 3 ){
	    fprintf(opts->fd, ",");
	  }
	}
      }
      /* new-line at end  */
      fprintf(opts->fd, "}");
      if( i != (src->nreg-1) ){
	fprintf(opts->fd, ",");
      }
      /* new-line at end  */
      fprintf(opts->fd, "\n");
    }
    /* bump to next line of radii/angles */
    if( tradang ){
      cradang = tradang+1;
      /* put back the cr in case we pass through again */
      *tradang = '\n';
    }
  }
  if( res->radang ){
    freedtable();
  }
  fprintf(opts->fd, "  ],\n");
  fflush(opts->fd);
}

/* display raw source info */
static void regcntsDisplaySrcInfoJSON(Opts opts, Data src){
  int i;
  int tarea=0;
  double tcnts=0;
  char *fmt=NULL;
  char *filtstr=NULL;
  char *header[SZ_LINE];
  char s[SZ_LINE];
  char lbuf[SZ_LINE];
  // do we need to tag the slice?
  s[0] = '\0';
  if( opts->docube ){
    snprintf(s, SZ_LINE, "%d", src->curslice+1);
  }
  /* make filter string json-compatible */
  if( src->filtstr ) filtstr = escjason(src->filtstr);
  /* display raw source counts */
  if( opts->dosum ){
    /* display source info */
    if( filtstr ){
      fprintf(opts->fd, "  \"sourceRegions%s\": ", s);
      fprintf(opts->fd, " \"%s\",\n", filtstr);
    }
    fprintf(opts->fd, "  \"summedSourceData%s\": [\n", s);
    header[0] = "reg";
    header[1] = "counts";
    header[2] = "pixels";
    header[3] = "sumcnts";
    header[4] = "sumpix";
    for(i=0; i<src->nreg; i++){
      tcnts += src->cnts[i];
      tarea += src->area[i];
      /* get correctly precisioned format statement */
      switch(opts->dog){
      case 0:
	fmt = "    {\"%s\": %d, \"%s\": %.3f, \"%s\": %9d, \"%s\": %.3f, \"%s\": %d}";
	break;
      case 1:
	fmt = "    {\"%s\": %d, \"%s\": %.3g, \"%s\": %d, \"%s\": %.3g, \"%s\": %d}";
	break;
      case 2:
	fmt = "    {\"%s\": %d, \"%s\": %.14g, \"%s\": %d, \"%s\": %.14g, \"%s\": %d}";
	break;
      }
      snprintf(lbuf, SZ_LINE, fmt,
	      header[0], i+1,
	      header[1], src->cnts[i],
	      header[2], src->area[i],
	      header[3], tcnts,
	      header[4], tarea);
      nonanjson(opts->fd, lbuf);
      if( i != (src->nreg-1) ){
	fprintf(opts->fd, ",");
      }
      fprintf(opts->fd, "\n");
    }
  } else {
    /* display source info */
    if( filtstr ){
      fprintf(opts->fd, "  \"sourceRegions%s\": ", s);
      fprintf(opts->fd, " \"%s\",\n", filtstr);
    }
    fprintf(opts->fd, "  \"sourceData%s\": [\n", s);
    header[0] = "reg";
    header[1] = "counts";
    header[2] = "pixels";
    for(i=0; i<src->nreg; i++){
      /* get correctly precisioned format statement */
      switch(opts->dog){
      case 0:
	fmt = "    {\"%s\": %d, \"%s\": %.3f, \"%s\": %d}";
	break;
      case 1:
	fmt = "    {\"%s\": %d, \"%s\": %.3g, \"%s\": %9d}";
	break;
      case 2:
	fmt = "    {\"%s\": %d, \"%s\": %.14g, \"%s\": %d}";
	break;
      }
      snprintf(lbuf, SZ_LINE, fmt,
	      header[0], i+1,
	      header[1], src->cnts[i],
	      header[2], src->area[i]);
      nonanjson(opts->fd, lbuf);
      if( i != (src->nreg-1) ){
	fprintf(opts->fd, ",");
      }
      fprintf(opts->fd, "\n");
    }
  }
  fprintf(opts->fd, "  ],\n");
  fflush(opts->fd);
  if( filtstr ) free(filtstr);
}

/* display raw background info */
static void regcntsDisplayBkgInfoJSON(Opts opts, Data bkg, Res res){
  int i;
  char *fmt=NULL;
  char *filtstr=NULL;
  char *header[SZ_LINE];
  char s[SZ_LINE];
  char lbuf[SZ_LINE];
  // do we need to tag the slice?
  s[0] = '\0';
  if( opts->docube ){
    snprintf(s, SZ_LINE, "%d", bkg->curslice+1);
  }
  /* make filter string json-compatible */
  if( bkg->filtstr ) filtstr = escjason(bkg->filtstr);
  /* display raw background info */
  switch(opts->bktype){
  case BKG_VAL:
    break;
  case BKG_ALL:
    if( filtstr ){
      fprintf(opts->fd, "  \"backgroundRegions%s\": ", s);
      fprintf(opts->fd, " \"%s\",\n", filtstr);
    }
    fprintf(opts->fd, "  \"backgroundData%s\": [\n", s);
    header[0] = "reg";
    header[1] = "counts";
    header[2] = "pixels";
    /* get correctly precisioned format statement */
    switch(opts->dog){
    case 0:
      fmt = "    {\"%s\": %s, \"%s\": %.3f, \"%s\": %d}";
      break;
    case 1:
      fmt = "    {\"%s\": %s, \"%s\": %.3g, \"%s\": %d}";
      break;
    case 2:
      fmt = "    {\"%s\": %s, \"%s\": %.14g, \"%s\": %d}";
      break;
    }
    snprintf(lbuf, SZ_LINE, fmt,
	    header[0], "\"all\"",
	    header[1], res->bkgval,
	    header[2], res->bkgarea);
    nonanjson(opts->fd, lbuf);
    fprintf(opts->fd, "\n");
    fprintf(opts->fd, "  ],\n");
    break;
  case BKG_EACH:
    if( opts->dosum ){
      int tarea=0;
      double tcnts=0;
      if( filtstr ){
	fprintf(opts->fd, "  \"backgroundRegions%s\": ", s);
	fprintf(opts->fd, " \"%s\",\n", filtstr);
      }
      fprintf(opts->fd, "  \"summedBackgroundData%s\": [\n", s);
      header[0] = "reg";
      header[1] = "counts";
      header[2] = "pixels";
      header[3] = "sumcnts";
      header[4] = "sumpix";
      for(i=0; i<bkg->nreg; i++){
	tcnts += bkg->cnts[i];
	tarea += bkg->area[i];
	/* get correctly precisioned format statement */
	switch(opts->dog){
	case 0:
	  fmt = "    {\"%s\": %d, \"%s\": %.3f, \"%s\": %9d, \"%s\": %.3f, \"%s\": %d}";
	  break;
	case 1:
	  fmt = "    {\"%s\": %d, \"%s\": %.3g, \"%s\": %d, \"%s\": %.3g, \"%s\": %d}";
	  break;
	case 2:
	  fmt = "    {\"%s\": %d, \"%s\": %.14g, \"%s\": %d, \"%s\": %.14g, \"%s\": %d}";
	  break;
	}
	snprintf(lbuf, SZ_LINE, fmt,
		header[0], i+1,
		header[1], bkg->cnts[i],
		header[2], bkg->area[i],
		header[3], tcnts,
		header[4], tarea);
	nonanjson(opts->fd, lbuf);
	if( i != (bkg->nreg-1) ){
	  fprintf(opts->fd, ",");
	}
	fprintf(opts->fd, "\n");
      }
    } else {
      if( filtstr ){
	fprintf(opts->fd, "  \"backgroundRegions%s\": ", s);
	fprintf(opts->fd, " \"%s\",\n", filtstr);
      }
      fprintf(opts->fd, "  \"backgroundData%s\": [\n", s);
      header[0] = "reg";
      header[1] = "counts";
      header[2] = "pixels";
      for(i=0; i<bkg->nreg; i++){
	/* get correctly precisioned format statement */
	switch(opts->dog){
	case 0:
	  fmt = "    {\"%s\": %d, \"%s\": %.3f, \"%s\": %9d}";
	  break;
	case 1:
	  fmt = "    {\"%s\": %d, \"%s\": %.3g, \"%s\": %d}";
	  break;
	case 2:
	  fmt = "    {\"%s\": %d, \"%s\": %.14g, \"%s\": %d}";
	  break;
	}
	snprintf(lbuf, SZ_LINE, fmt,
		header[0], i+1,
		header[1], bkg->cnts[i],
		header[2], bkg->area[i]);
	nonanjson(opts->fd, lbuf);
	if( i != (bkg->nreg-1) ){
	  fprintf(opts->fd, ",");
	}
	fprintf(opts->fd, "\n");
      }
    }
    fprintf(opts->fd, "  ],\n");
    break;
  }
  fflush(opts->fd);
  if( filtstr ) free(filtstr);
}

void regcntsDisplayEndJSON(Opts opts){
  char tbuf[SZ_LINE];
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  strncpy(tbuf, asctime(tm), SZ_LINE-1);
  tbuf[strlen(tbuf)-1] = '\0';
  fprintf(opts->fd, "  \"date\": \"%s\"\n", tbuf);
  fprintf(opts->fd, "}\n");
  fflush(opts->fd);
}

/* display results header */
void regcntsDisplayHeader(Opts opts, Data src, Data bkg, Res res){
  switch(opts->otype){
  case 0:
    regcntsDisplayHeaderRDB(opts, src, bkg, res);
    break;
  case 1:
    regcntsDisplayHeaderJSON(opts, src, bkg, res);
    break;
  }
}

/* display main results info */
void regcntsDisplayMainInfo(Opts opts, Data src, Res res){
  switch(opts->otype){
  case 0:
    regcntsDisplayMainInfoRDB(opts, src, res);
    break;
  case 1:
    regcntsDisplayMainInfoJSON(opts, src, res);
    break;
  }
}

/* display raw source info */
void regcntsDisplaySrcInfo(Opts opts, Data src){
  switch(opts->otype){
  case 0:
    regcntsDisplaySrcInfoRDB(opts, src);
    break;
  case 1:
    regcntsDisplaySrcInfoJSON(opts, src);
    break;
  }
}

/* display raw background info */
void regcntsDisplayBkgInfo(Opts opts, Data bkg, Res res){
  switch(opts->otype){
  case 0:
    regcntsDisplayBkgInfoRDB(opts, bkg, res);
    break;
  case 1:
    regcntsDisplayBkgInfoJSON(opts, bkg, res);
    break;
  }
}

/* finish up display */
void regcntsDisplayEnd(Opts opts){
  switch(opts->otype){
  case 0:
    break;
  case 1:
    regcntsDisplayEndJSON(opts);
    break;
  }
}
