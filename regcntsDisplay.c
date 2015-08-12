/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * _regcntsDisplay: formatted output for regcnts
 *
 */

#include "regcnts.h"

/* display results header */
void regcntsDisplayHeader(Opts opts, Data src, Data bkg, Res res){
  char *s;
  /* display source header information */
  fprintf(stdout, "# source\n");
  fprintf(stdout, "#   data_file:\t\t%s\n", src->name);
  if( opts->bin != 1 ){
    fprintf(stdout, "#   auto_block:\t\t%d\n", opts->bin);
  }
  if( src->dpp > 0.0 ){
    fprintf(stdout, "#   arcsec/pixel:\t%g\n", src->dpp*ASEC_DEG);
  }
  /* display bkgd header information */
  fprintf(stdout, "# background\n");
  if( bkg->name ){
    fprintf(stdout, "#   data_file:\t\t%s\n", bkg->name);
    if( bkg->dpp > 0.0 ){
      fprintf(stdout, "#   arcsec/pixel:\t%g\n", bkg->dpp*ASEC_DEG);
    }
    if( res->dppnorm != 1.0 ){
      fprintf(stdout, "# wcs area norm factor:\t%g/%g (source/bkgd))\n",
	      src->dpp,bkg->dpp);
    }
  } else if( opts->bktype != BKG_VAL ){
    fprintf(stdout, "#   data_file:\t\t%s\n", src->name);
  } else {
    fprintf(stdout, "#   constant_value:\t%.6f\n", res->bkgval);
  }
  /* dislay table information */
  if( (src->dpp > 0.0) && !opts->dopixels ){
    s = "arcsec";
  } else {
    s = "pixel";
  }
  fprintf(stdout, "# column units\n");
  fprintf(stdout, "#   area:\t\t%s**2\n", s);
  fprintf(stdout, "#   surf_bri:\t\tcnts/%s**2\n", s);
  fprintf(stdout, "#   surf_err:\t\tcnts/%s**2\n", s);
  if( opts->doradang ){
    fprintf(stdout, "#   radii:\t\t%ss\n", s);
    fprintf(stdout, "#   angles:\t\tdegrees\n");
  }
}

/* display main results info */
void regcntsDisplayMainInfo(Opts opts, Data src, Res res){
  int i, j;
  char *cradang=NULL;
  char *tradang=NULL;
  char *fmt=NULL;
  char tbuf[SZ_LINE];
  switch( opts->dosum ){
  case 1:
    fprintf(stdout, "\n");
    fprintf(stdout, "# summed background-subtracted results\n");
    fprintf(stdout, "upto%c  net_counts%c    error", opts->c, opts->c);
    break;
  case 2:
  default:
    if( opts->c == '\t' ){
      fprintf(stdout, "\f");
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "# background-subtracted results\n");
    fprintf(stdout, " reg%c  net_counts%c    error", opts->c, opts->c);
    break;
  }
  fprintf(stdout, "%c  background%c   berror", opts->c, opts->c);
  fprintf(stdout, "%c     area%c surf_bri%c surf_err",
	  opts->c, opts->c, opts->c);
  if( opts->doradang ){
    fprintf(stdout, "%c  radius1%c  radius2%c   angle1%c   angle2",
	    opts->c, opts->c, opts->c, opts->c);
  }
  fprintf(stdout, "\n");
  fprintf(stdout, "----%c------------%c---------", opts->c, opts->c);
  fprintf(stdout, "%c------------%c---------", opts->c, opts->c);
  fprintf(stdout, "%c---------%c---------%c---------", 
	  opts->c, opts->c, opts->c);
  if( opts->doradang ){
    fprintf(stdout, "%c---------%c---------%c---------%c---------",
	    opts->c, opts->c, opts->c, opts->c);
  }
  fprintf(stdout, "\n");
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
      fprintf(stdout, fmt,
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
	      fprintf(stdout, "%c%9.3f", opts->c, (dval*src->dpp*ASEC_DEG));
	    } else {
	      fprintf(stdout, "%c%9.3f", opts->c, dval);
	    }
	  } else {
	    fprintf(stdout, "%c%9.9s", opts->c, "NA");
	  }
	}
      }
      /* new-line at end  */
      fprintf(stdout, "\n");
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
      fprintf(stdout, fmt, 
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
	      fprintf(stdout, "%c%9.3f", opts->c, (dval*src->dpp*ASEC_DEG));
	    } else {
	      fprintf(stdout, "%c%9.3f", opts->c, dval);
	    }
	  } else {
	    fprintf(stdout, "%c%9.9s", opts->c, "NA");
	  }
	}
      }
      /* new-line at end  */
      fprintf(stdout, "\n");
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
  fprintf(stdout, "\n");
  fflush(stdout);
}

/* display raw source info */
void regcntsDisplaySrcInfo(Opts opts, Data src){
  int i;
  int tarea=0;
  double tcnts=0;
  char *fmt=NULL;
  /* display raw source counts */
  if( opts->dosum ){
    if( opts->c == '\t' ) fprintf(stdout, "\f");
    fprintf(stdout, "\n");
    /* display source info */
    if( src->filtstr ){
      fprintf(stdout, "# source_region(s):\n");
      fprintf(stdout, "# %s\n\n", src->filtstr);
    }
    fprintf(stdout, "# summed_source_data\n");
    fprintf(stdout, " reg%c      counts%c   pixels%c     sumcnts%c   sumpix\n",
	    opts->c, opts->c, opts->c, opts->c);
    fprintf(stdout,
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
      fprintf(stdout, fmt,
	      i+1, opts->c,
	      src->cnts[i], opts->c, src->area[i], opts->c,
	      tcnts, opts->c, tarea);
    }
  } else {
    if( opts->c == '\t' ){
      fprintf(stdout, "\f");
    }
    /* display source info */
    if( src->filtstr ){
      fprintf(stdout, "# source_region(s):\n");
      fprintf(stdout, "# %s\n\n", src->filtstr);
    }
    fprintf(stdout, "# source_data\n");
    fprintf(stdout, " reg%c      counts%c   pixels", opts->c, opts->c);
    fprintf(stdout, "\n");
    fprintf(stdout, "----%c------------%c---------", opts->c, opts->c);
    fprintf(stdout, "\n");
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
      fprintf(stdout, fmt, i+1, opts->c, src->cnts[i], opts->c, src->area[i]);
      fprintf(stdout, "\n");
    }
  }
  fprintf(stdout, "\n");
  fflush(stdout);
}

/* display raw background info */
void regcntsDisplayBkgInfo(Opts opts, Data bkg, Res res){
  int i;
  char *fmt=NULL;
  /* display raw background info */
  switch(opts->bktype){
  case BKG_VAL:
    break;
  case BKG_ALL:
    if( opts->c == '\t' ){
      fprintf(stdout, "\f");
    }
    if( bkg->filtstr ){
      fprintf(stdout, "# background_region(s)\n");
      fprintf(stdout, "# %s\n\n", bkg->filtstr);
    }
    fprintf(stdout, "# background_data\n");
    fprintf(stdout, " reg%c      counts%c   pixels", opts->c, opts->c);
    fprintf(stdout, "\n");
    fprintf(stdout, "----%c------------%c---------", opts->c, opts->c);
    fprintf(stdout, "\n");
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
    fprintf(stdout, fmt, "all ", opts->c, res->bkgval, opts->c, res->bkgarea);
    fprintf(stdout, "\n");
    break;
  case BKG_EACH:
    if( opts->dosum ){
      int tarea=0;
      double tcnts=0;
      if( opts->c == '\t' ){
	fprintf(stdout, "\f");
      }
      if( bkg->filtstr ){
	fprintf(stdout, "# background_region(s)\n");
	fprintf(stdout, "# %s\n\n", bkg->filtstr);
      }
      fprintf(stdout, "# summed_background_data\n");
      fprintf(stdout,
	      " reg%c      counts%c   pixels%c     sumcnts%c   sumpix\n",
	      opts->c, opts->c, opts->c, opts->c);
      fprintf(stdout, 
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
	fprintf(stdout, fmt, i+1, opts->c,
		bkg->cnts[i], opts->c, bkg->area[i], opts->c,
		tcnts, opts->c, tarea);
      }
    } else {
      if( opts->c == '\t' ) fprintf(stdout, "\f");
      if( bkg->filtstr ){
	fprintf(stdout, "# background_region(s)\n");
	fprintf(stdout, "# %s\n\n", bkg->filtstr);
      }
      fprintf(stdout, "# background_data\n");
      fprintf(stdout, " reg%c      counts%c   pixels", opts->c, opts->c);
      fprintf(stdout, "\n");
      fprintf(stdout, "----%c------------%c---------", opts->c, opts->c);
      fprintf(stdout, "\n");
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
	fprintf(stdout, fmt, i+1, opts->c, bkg->cnts[i], opts->c, bkg->area[i]);
	fprintf(stdout, "\n");
      }
    }
    break;
  }
  fprintf(stdout, "\n");
  fflush(stdout);
}
