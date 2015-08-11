/* for TEST: gcc -DTEST -g -o foo imfilter.c imregions.o -lm */
#ifdef TEST
#include <stdio.h>
#include <math.h>
#include "imregions.h"
#define IMFILTRTN _FilterRegions
#define NSHAPE 2
#define FILTER ((imcircle(g,1,1,1,4,(double)x,(double)y,8.0,8.0,5.0)))&&(imcircle(g,2,2,0,1,(double)x,(double)y,8.0,8.0,3.0))
#define FILTSTR "((imcircle(g,1,1,1,4,(double)x,(double)y,8.0,8.0,5.0)))&&(imcircle(g,2,2,0,1,(double)x,(double)y,8.0,8.0,3.0))"
#define FINIT imcirclei(g,1,1,1,4,(double)x,(double)y,8.0,8.0,5.0);imcirclei(g,2,2,0,1,(double)x,(double)y,8.0,8.0,3.0);
#endif

/* global mask info */
#define MASKINC	10240
RegionsMask masks=NULL;		/* array valid region masks for one row */
int maxmask;			/* max masks allocated thus far */
int nmask;			/* number of mask segments */

/* increase the number of available masks */
void incnmask(void)
{
  int oldmax;
  if( ++nmask >= maxmask ){
    oldmax = maxmask;
    maxmask += MASKINC;
    masks = (RegionsMask)realloc(masks, maxmask*sizeof(RegionsMaskRec));
    memset(masks+oldmax, 0, (maxmask-oldmax)*sizeof(RegionsMaskRec));
  }
}

/* filter image and return valid region segments in global masks var */
RegionsMask IMFILTRTN(int txmin, int txmax, int tymin, int tymax, int tblock,
		      int *got){
  int i, j;
  int rlen;
  int nreg;
  int fieldonly;
  int x=0, y=0;
  int *rbuf;
  int *rptr;
  GReg g;
  Scan scan, tscan;

  /* start out pessimistically */
  nmask = 0;
  /* make sure we have something to process */
  if( NSHAPE <=0 ){
    if( got ){
      *got = 0;
    }
    return NULL;
  }
  /* optimization: see if we have only the field shape */
  fieldonly = (NSHAPE==1) && strstr(FILTSTR, "field");
  /* allocate space for the global rec, which is passed to region routines */
  g = (GReg)calloc(1, sizeof(GRegRec));
  /* allocate region records */
  g->nshape = NSHAPE;
  g->maxshape = (NSHAPE*(XSNO+1))+1;
  g->shapes = (Shape)calloc(g->maxshape, sizeof(ShapeRec));
  /* make sure we start at 1 */
  g->block= max(1,tblock);
  g->xmin = max(1,txmin); 
  g->xmax = txmax;
  g->ymin = max(1,tymin);
  g->ymax = tymax;
  /* get x and y limits on subsection */
  g->x0 = 1;
  g->y0 = 1;
  g->x1 = (g->xmax-g->xmin)/g->block+1;
  g->y1 = (g->ymax-g->ymin)/g->block+1;
  /* allocate a temp buffer for region ids */
  rlen = g->x1 - g->x0 + 1;
  rbuf = (int *)calloc(rlen+1, sizeof(int));
  /* allocate an array of masks, which will be written to master */
  maxmask = MASKINC;
  masks = (RegionsMask)calloc(maxmask, sizeof(RegionsMaskRec));
  masks[nmask].region = 0;
  /* init how many valid hits we had for this set of rows */
  nreg = 0;
  /* allocate a buffer for valid y row flags */
  g->ybuf = (int *)calloc(g->y1+1, sizeof(int));
  g->x0s = (int *)calloc(g->y1+1, sizeof(int));
  g->x1s = (int *)calloc(g->y1+1, sizeof(int));
  /* seed impossible values for x limits */
  for(i=0; i<=g->y1; i++){
    g->x0s[i]  = g->x1;
  }
  for(i=0; i<=g->y1; i++){
    g->x1s[i]  = g->x0;
  }
  /* initialize scan lines */
  FINIT;
  /* process all valid rows */
  for(y=g->y0; y<=g->y1; y++){
    if( fieldonly ){
      /* inc the mask count, (extend mask array, if necessary) */
      masks[nmask].region = 1;
      masks[nmask].y = y - g->y0 + 1;
      masks[nmask].xstart = 1;
      masks[nmask].xstop = (g->x1 - g->x0 + 1);
      incnmask();
      continue;
    }
    if( g->ybuf[y] ){
      /* to start this line, we make a seed mask with no region */
      if( masks[nmask].region ){
	/* inc the mask count, (extend mask array, if necessary) */
	incnmask();
	masks[nmask].region = 0;
      }
      /* process each pixel in this row where there is a region */
      for(x=g->x0s[y], rptr=&rbuf[1+(g->x0s[y]-g->x0)]; x<=g->x1s[y];
	  x++, rptr++){
	/* get filter result, which is the region id or 0 */
	g->rid = 0;
	if( FILTER ){
	  /* never change a region id to a -1 */
	  if( *rptr == 0 ){
	    nreg++;
	    *rptr = g->rid ? g->rid : -1;
	  }
	  /* but always overwrite a -1 */
	  else if( (*rptr == -1) && (g->rid >0) ){
	    *rptr = g->rid;
	  }
	}
      }
    }
    /* if we have processed a row, make up the valid segments in the mask */
    if( nreg ){
      for(i=1; i<=rlen; i++){
	if( rbuf[i] != masks[nmask].region ){
	  /* if previous was non-zero region, finish it and bump to next */
	  if( masks[nmask].region ){
	    masks[nmask].xstop = i - 1;
	    /* inc the mask count, (extend mask array, if necessary) */
	    incnmask();
	  }
	  masks[nmask].y = y - g->y0 + 1;
	  masks[nmask].region = rbuf[i];
	  masks[nmask].xstart = i;
	}
      }
      /* finish last non-zero segment, inc number of mask segs */
      if( masks[nmask].region ){
	masks[nmask].xstop = (g->x1 - g->x0 + 1);
	/* inc the mask count, (extend mask array, if necessary) */
	incnmask();
      }
      /* reset counters for next set of rows */
      (void)memset(rbuf, 0, (rlen+1)*sizeof(int));
      rptr = rbuf;
      nreg = 0;
    }
  }
  /* free buffers */
  if( rbuf) free(rbuf);
  /* free region information */
  if( g ){
    for(i=0; i<g->maxshape; i++){
      if( g->shapes[i].scanlist ){
	for(j=0; j<=g->y1; j++){
	  if( g->shapes[i].scanlist[j] ){
	    for(scan=g->shapes[i].scanlist[j]; scan; ){
	      tscan = scan->next;
	      free(scan);
	      scan = tscan;
	    }
	  }
	}
	free(g->shapes[i].scanlist);
      }
      if( g->shapes[i].pts ) free(g->shapes[i].pts);
      if( g->shapes[i].xv ) free(g->shapes[i].xv);
    }
    if( g->shapes ) free(g->shapes);
    if( g->ybuf )   free(g->ybuf);
    if( g->x0s )    free(g->x0s);
    if( g->x1s )    free(g->x1s);
    if( g )         free(g);
  }
  /* return results */
  if( got ){
    *got = nmask;
  }
  return masks;
}

/* slave routine gets a new section from the master and returns a region mask */
int main(int argc, char **argv)
{
  int i, txmin, txmax, tymin, tymax, tblock;
  char tbuf[SZ_LINE];
  char *s=NULL, *t=NULL, *u=NULL;
#ifndef TEST
  int get, obytes;
#endif
#ifndef __MINGW32__
  int pipes[4];
#endif
#ifdef _WIN32
  HANDLE hStdin, hStdout; 
  DWORD dwRead, dwWritten; 
#endif

  /* Launch() sometimes rearranges passed pipes to be stdin/stdout */
#ifndef __MINGW32__
  if( (s=getenv("LAUNCH_PIPES")) ){
    t = (char *)strdup(s);
    for(i=0, u=(char *)strtok(t, ","); i<4 && u; 
	i++, u=(char *)strtok(NULL,",")){
      pipes[i] = atoi(u);
    }
    if( t ) free(t);
    if( i < 4 ) return(1);
    close(pipes[0]);
    close(pipes[3]);
    dup2(pipes[2], 0);  close(pipes[2]);
    dup2(pipes[1], 1);  close(pipes[1]);
  }
#endif
#ifdef _WIN32
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
  hStdin = GetStdHandle(STD_INPUT_HANDLE); 
  if( (hStdout == INVALID_HANDLE_VALUE) || (hStdin == INVALID_HANDLE_VALUE) ){
    unlink(argv[0]);
    return 0;
  }
#endif
#ifdef TEST
  /* read next section from stdin */
  while( fgets(tbuf, SZ_LINE, stdin) ){
#else  /* TEST */
#ifdef _WIN32
  /* read next section from master(windows version) */
  while((ReadFile(hStdin, &get, sizeof(int), &dwRead, NULL) != FALSE) && 
	(dwRead == sizeof(int)) ){
    if((ReadFile(hStdin, tbuf, get, &dwRead, NULL)==FALSE) || (dwRead != get)){
      break;
    }
#else
  /* read next section from master (unix version) */
  while( read(0, &get, sizeof(int)) == sizeof(int) ){
    if(read(0, tbuf, get) != get){
      break;
    }
#endif
#endif /* TEST */
    /* get section from input */
    if( sscanf(tbuf, "%d %d %d %d %d",
	       &txmin, &txmax, &tymin, &tymax, &tblock) != 5 ){
      break;
    }
    /* filter image section, returning results in globals: masks and nmask */
    IMFILTRTN(txmin, txmax, tymin, tymax, tblock, NULL);
#ifdef TEST
    /* display segments for debugging */
    fprintf(stdout, "nmask=%d\n", nmask);
    for(i=0; i<nmask; i++){
      fprintf(stdout, "region: %d\tx: (%d,%d)\ty: %d\n",
	      masks[i].region, masks[i].xstart, masks[i].xstop, masks[i].y);
    }
    fflush(stdout);
#else /* TEST */
    /* calculate size of data we will write to master */
    obytes =  nmask * sizeof(RegionsMaskRec);
#ifdef _WIN32
    /* write masks to master (windows version) */
    WriteFile(hStdout, &obytes, sizeof(int), &dwWritten, NULL); 
    WriteFile(hStdout, masks, obytes, &dwWritten, NULL); 
#else
    /* write masks to master (unix version) */
    write(1, &obytes, sizeof(int));
    write(1, masks, obytes);
#endif
#endif /* TEST */
    /* free mask records */
    if( masks ){
      free(masks);
    }
  }
#ifndef TEST
  unlink(argv[0]);
#endif /* TEST */
  return 0;
}
