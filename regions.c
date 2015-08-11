/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * regions.c -- hi-level management of regions and region masks
 *
 */
#include "regionsP.h"

/*
 *
 * private routines
 *
 */

/*
 *
 * RegionsClip -- save the regions without the enclosing brackets
 *
 */
static char *_RegionsClip(char *regstr){
  char *tbuf;
  char *tptr;
  char *oregstr;
  int len;
  int i;

  /* check for the obvious */
  if( !regstr || !*regstr ){
    return NULL;
  }
  /* save regions, but without the enclosing brackets */
  tbuf = xstrdup(regstr);
  nowhite(tbuf, tbuf);
  tptr = tbuf;
  if( *tbuf == '[' ){
    tptr++;
    len = strlen(tptr);
    for(i=len-1; i>=0; i-- ){
      if( tptr[i] == ']' ){
	tptr[i] = '\0';
	break;
      }
    }
  }
  oregstr = xcalloc(strlen(tptr)+1, sizeof(char));
  nowhite(tptr, oregstr);
  xfree(tbuf);
  return oregstr;
}

/*
 *
 * semi-public routines, used by other modules
 *
 */

/* special null region returned if by OpenRegions if there are no regions */
Regions _null_region;

/*
 *
 * public routines
 *
 */

/*
 *
 * OpenRegions -- open regions for masking
 *
 */
Regions OpenRegions(char *cards, char *regstr, char *mode){
  int lexonly=0;
  int got=0;
  char *s=NULL, *t=NULL;
  char tbuf[SZ_LINE];
  Regions reg;

  /* return if we have no regions */
  if( (regstr == NULL) || (*regstr == '\0') ||
	!strcmp(regstr, "[]") || !strcmp(regstr, "\n") ){
    return NOREGIONS;
  }
  /* more checks for an essentially blank region string */
  for(got=0, s=regstr; *s; s++){
    if( (*s != ' ') && (*s != '\t') && (*s != '\n') && (*s != ';') ){
      got++;
      break;
    }
  }
  if( !got ){
    return NOREGIONS;
  }
  /* allocate a new regions record */
  if( (reg = (Regions)xcalloc(1, sizeof(RegionsRec))) == NULL ){
    return NULL;
  }
  /* save cards */
  reg->cards = xstrdup(cards);
  /* clean up regions */
  s = _RegionsClip(regstr);
  /* if its not a file, use string directly */
  if( *s != '@' ){
    reg->regstr = xstrdup(s);
  } else if( (t = FileContents(&(s[1]), 0, NULL)) ){
    /* replace ASCII file with contents, if possible */
    reg->regstr = xstrdup(t);
    xfree(t);
  }
  xfree(s);
  /* return if we have no regions */
  if( !reg->regstr || (*reg->regstr == '\0') ){
    return NOREGIONS;
  }
  /* save mode */
  reg->mode=xstrdup(mode);
  /* process the mode string */
  if( (s = xstrdup(reg->mode)) ){
    /* debugging flag */
    if( keyword(s, "lexonly", tbuf, SZ_LINE) ){
      lexonly = 1;
    }
    xfree(s);
  }
  /* determine which method of regions processing we will use:
     currently we support only the C method */
  reg->method = DEFAULT_REGIONS_METHOD;
  *tbuf = '\0';
  if( (s=(char *)getenv("REGIONS_METHOD")) ){
    strcpy(tbuf, s);
  }
  if( !*tbuf && (s=xstrdup(reg->mode)) ){
    keyword(s, "method", tbuf, SZ_LINE);
    xfree(s);
  }
  if( *tbuf ){
    if( !strcasecmp(tbuf, "c") ){
      reg->method = METHOD_C;
    }
  }
  /* determine which type of process execution we do */
  switch(reg->method){
  case METHOD_C:
    reg->ptype = DEFAULT_REGIONS_PTYPE;
    *tbuf = '\0';
    if( (s=(char *)getenv("REGIONS_PTYPE")) ){
      strcpy(tbuf, s);
    }
    if( !*tbuf && (s=xstrdup(reg->mode)) ){
      keyword(s, "ptype", tbuf, SZ_LINE);
      xfree(s);
    }
    if( *tbuf ){
      if( *tbuf == 'p' ){
	reg->ptype = PTYPE_PROCESS;
      } else if( *tbuf == 'c' ){
	reg->ptype = PTYPE_CONTAINED;
      } else if( *tbuf == 'd' ){
	reg->ptype = PTYPE_DYNAMIC;
      }
    }
    break;
  default:
    reg->ptype = PTYPE_PROCESS;
    break;
  }
  /* determine region paint mode */
  *tbuf = '\0';
  if( (s=(char *)getenv("REGIONS_PAINT")) ){
    strcpy(tbuf, s);
  }
  if( !*tbuf && (s=xstrdup(reg->mode)) ){
    keyword(s, "paint", tbuf, SZ_LINE);
    xfree(s);
  }
  if( !*tbuf ){
    strcpy(tbuf, DEFAULT_PAINT_MODE);
  }
  if( *tbuf ){
    if( istrue(tbuf) ){
      reg->paint = 1;
    } else {
      reg->paint = 0;
    }
  }
  /* debugging mode */
  *tbuf = '\0';
  if( (s=(char *)getenv("REGIONS_DEBUG")) ){
    strcpy(tbuf, s);
  }
  if( !*tbuf && (s=xstrdup(reg->mode)) ){
    keyword(s, "debug", tbuf, SZ_LINE);
    xfree(s);
  }
  if( *tbuf ){
    if( isdigit((int)*tbuf) ){
      reg->debug = atoi(tbuf);
    } else if( istrue(tbuf) ){
      reg->debug = 1;
    } else if( isfalse(tbuf) ){
      reg->debug = 0;
    }
  }
  /* init regions to ensure dynamic load */
  initimregions();
  // parse regions and check results
  switch( regparse(reg) ){
  case -1:
    goto error;
  case 0:
    CloseRegions(reg);
    return NOREGIONS;
  default:
    break;
  }
  /* lexonly means we only wanted to run the lexer */
  if( lexonly ){
    goto done;
  }
  /* open the filter program file that we will compile from the regions */
  if( !RegionsProgOpen(reg) ){
    goto error;
  }
  /* prepend to the filter program as needed */
  if( !RegionsProgPrepend(reg) ){
    goto error;
  }
  /* write the header to the filter program */
  if( !RegionsProgWrite(reg) ){
    goto error;
  }
  /* append the program body to filter program */
  if( !RegionsProgAppend(reg) ){
    goto error;
  }
  /* that is all we need to output */
  if( !RegionsProgClose(reg) ){
    goto error;
  }
  /* compile the filter program */
  if( !RegionsProgCompile(reg) ){
    goto error;
  }
  if( reg->debug >= 2 ){
    goto done;
  }
  // open the filter process or load the dynamic object
  switch(reg->method){
  case METHOD_C:
    switch(reg->ptype){
    case PTYPE_PROCESS:
    case PTYPE_CONTAINED:
      switch(reg->pipeos){
      case PIPE_WIN32:
#if defined(__CYGWIN__) || defined(__MINGW32__)
	if( !WinProcessOpen(reg->prog, 
	     &(reg->ihandle), &(reg->ohandle), &(reg->process)) ){
	  goto error;
	}
#else
	xerror(stderr, "internal error: no WinProcess without Windows");
	goto error;
#endif
	break;
      default:
	if( !ProcessOpen(reg->prog, 
	     &(reg->ichan), &(reg->ochan), &(reg->pid)) ){
	  goto error;
	}
	break;
      }
      break;
    case PTYPE_DYNAMIC:
      if( !(reg->dl=DLOpen(reg->prog)) ){
	goto error;
      }
      break;
    default:
	goto error;
    }
    break;
  default:
    goto error;
  }
done:
  /* return the good news */
  return reg;
error:
  /* clean up the mess and return */
  CloseRegions(reg);
  return NULL;
}

/*
 *
 * FilterRegions -- filter an image by sending sections to a co-process,
 * which returns a mask of segments containing valid image data
 *
 */
int FilterRegions(Regions reg, int x0, int x1, int y0, int y1, int block,
		  RegionsMask *mask, int *nreg){
  int got;
  char tbuf[SZ_LINE];
  RegionsImageCall imrtn;

  /* make sure we have a valid regions */
  if( !reg || (reg == NOREGIONS) ){
    return 0;
  }
  /* process filter request */
  switch(reg->ptype){
  case PTYPE_PROCESS:
  case PTYPE_CONTAINED:
    switch(reg->pipeos){
    case PIPE_WIN32:
#if defined(__CYGWIN__) || defined(__MINGW32__)
      /* write command to process */
      snprintf(tbuf, SZ_LINE-1, "%d %d %d %d %d\n", x0, x1, y0, y1, block);
      WinProcessWrite(reg->ohandle, tbuf, (int)strlen(tbuf));
      /* read back mask records */
      *mask = WinProcessRead(reg->ihandle, NULL, -1, &got);
      got /=  sizeof(RegionsMaskRec);
#else
      xerror(stderr, "internal error: no WinProcess without Windows");
      return -1;
#endif
      break;
    default:
      /* write command to process */
      snprintf(tbuf, SZ_LINE-1, "%d %d %d %d %d\n", x0, x1, y0, y1, block);
      ProcessWrite(reg->ochan, tbuf, (int)strlen(tbuf));
      /* read back mask records */
      *mask = ProcessRead(reg->ichan, NULL, -1, &got);
      got /=  sizeof(RegionsMaskRec);
      break;
    }
    break;
  case PTYPE_DYNAMIC:
    if( (imrtn=(RegionsImageCall)DLSym(reg->dl, reg->pname)) ){
      *mask = (*imrtn)(x0, x1, y0, y1, block, &got);
    } else {
      return -1;
    }
    break;
  default:
    return -1;
  }
  /* how many include regions? */
  if( nreg ){
    *nreg = reg->nreg;
  }
  /* return the news */
  return got;
}

/*
 *
 * CloseRegions -- close a regions and free up memory
 *
 */
int CloseRegions(Regions reg){
  int status=0;

  if( !reg || (reg == NOREGIONS) ){
    return 0;
  }
  switch(reg->ptype){
  case PTYPE_PROCESS:
  case PTYPE_CONTAINED:
    /* close the regions process */
    switch(reg->pipeos){
    case PIPE_WIN32:
#if defined(__CYGWIN__) || defined(__MINGW32__)
      if( reg->process ){
	WinProcessClose(reg->process, &status);
      }
#else
      xerror(stderr, "internal error: no WinProcess without Windows");
#endif
      break;
    default:
      if( reg->pid > 0 ){
	ProcessClose(reg->pid, &status);
      }
      break;
    }
    /* delete program */
    if( reg->prog ){
      unlink(reg->prog);
    }
    break;
  case PTYPE_DYNAMIC:
    if( reg->dl ){
      status = DLClose(reg->dl);
    }
    break;
  default:
    break;
  }
  /* call any method-specific cleanup routines */
  RegionsProgClose(reg);
  RegionsProgEnd(reg);
  /* free alloc'ed memory */
  xfree(reg->pname);
  xfree(reg->mode);
  xfree(reg->cc);
  xfree(reg->cflags);
  xfree(reg->objs);
  xfree(reg->extra);
  xfree(reg->shflags);
  xfree(reg->code);
  xfree(reg->prog);
  xfree(reg->regstr);
  xfree(reg->cards);
  xfree(reg->radang);
  xfree(reg->filter);
  /* free main struct */
  xfree(reg);
  /* return the status from closing the sub-process */
  return status;
}
