/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * regprog_c.c -- support for region filters using the C compiler
 *
 */

#include "regionsP.h"
#include "imregions_h.h"
#include "imregions_c.h"
#include "imfilter_c.h"

/*
 *
 * Private Routines
 *
 */

/* from Harbison&Steele by way of GNU configure ...
   returns 1 for big endian, 0 for little endian */
static int bigendian (void){
  union {
    long l;
    char c[sizeof (long)];
  } u;
  u.l = 1;
  return(u.c[sizeof (long) - 1] == 1);
}

/*
 *
 * RegionsProgOpen_C -- return filter program as a file for writing
 *
 */
static int regions_num=0;
static int RegionsProgOpen_C(Regions reg){
  int fd;
  char *ccstr=NULL;
  char prefix[SZ_LINE];
  char tbuf[SZ_LINE];
  char *tmpdir=NULL;
  char *rpath=NULL;
#ifdef __CYGWIN__
  char *s;
#endif
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  rpath = (char *)getenv("PATH");
  /* see if the user specified a compiler explicitly */
  if( !(ccstr = getenv("REGIONS_CC"))				&&
      !(ccstr = getenv("CC")) 					&&
      !(ccstr = REGIONS_CC)					){
    ccstr = "gcc";
  }
  /* make sure we have a compiler */
  if( !(reg->cc = Access(ccstr, "x"))				&&
      !(reg->cc = Find(ccstr, "x", NULL, rpath))		&&
      !(reg->cc = Find("gcc", "x", NULL, rpath))		&&
      !(reg->cc = Find("cc",  "x", NULL, rpath))		&&
      !(reg->cc = Find("cc",  "x", NULL, "."))  		&&
      !(reg->cc = Find("cc",  "x", NULL, CC_PATH))		){
    xerror(stderr, "no compiler found for filter compilation\n");
    return 0;
  }
  /* determine whether communication is via Unix pipes or Win32 pipes */
  reg->pipeos = PIPE_UNIX;
#ifdef __MINGW32__
  reg->pipeos = PIPE_WIN32;
#endif
#ifdef __CYGWIN__
  if( (s=strrchr(reg->cc, '/')) ){
    s++;
  }
  else if( (s=strrchr(reg->cc, '\\')) ){
    s++;
  } else {
    s = reg->cc;
  }
  if( !strcasecmp(s, "tcc") || !strcasecmp(s, "tcc.exe") ||
      !strcasecmp(s, "pcc") || !strcasecmp(s, "pcc.exe") ){
    reg->pipeos = PIPE_WIN32;
  }
#endif
  /* final checks on ptype:
     if we wanted dynamic but have no support, use process
     if we wanted dynamic but have no gcc, use process */
  if( reg->ptype == PTYPE_DYNAMIC ){
#if HAVE_DL
    if( !strstr(reg->cc, "gcc") ){
      reg->ptype = PTYPE_PROCESS;
    }
#else
    reg->ptype = PTYPE_PROCESS;
#endif
  }
  /* get default file names */
  snprintf(tbuf, SZ_LINE, "$REGIONS_OBJDIR:$REGIONS_LIBDIR:%s:$HOME/.regions:$SAORD_ROOT/lib:/usr/lib:/usr/local/lib:/opt/local/lib", OBJPATH);
  /* look for region library to link against */
  reg->objs = Find("libregions.a", "r", NULL, tbuf);
  /* if we wanted to use a process, but have no library, use self-contained */
  if( (reg->ptype == PTYPE_PROCESS) && !reg->objs ){
    reg->ptype = PTYPE_CONTAINED;
  }
  /* allow extra stuff on the command line */
  if( !(reg->cflags = xstrdup(getenv("REGIONS_CFLAGS")))	&&
      !(reg->cflags = xstrdup(REGIONS_CFLAGS))		){
    reg->cflags = xstrdup(" ");
  }
  if( !(reg->extra = xstrdup(getenv("REGIONS_EXTRA"))) ){
    reg->extra = xstrdup(" ");
  }
  /* set shared switches for gcc */
  if( strstr(reg->cc, "gcc") ){
    reg->shflags = xstrdup(GCC_SHARED_FLAGS);
  }
  /* get prefix for filter source and program */
  if( !(tmpdir = (char *)getenv("REGIONS_TMPDIR")) &&
      !(tmpdir = (char *)getenv("TMPDIR"))        &&
      !(tmpdir = (char *)getenv("TMP"))            ){
    tmpdir = DEFAULT_REGIONS_TMPDIR;
  }
  if( !*tmpdir ){
    tmpdir = ".";
  }
#ifdef __MINGW32__
  snprintf(prefix, SZ_LINE, "%s\\f", tmpdir);
#else
  snprintf(prefix, SZ_LINE, "%s/f", tmpdir);
#endif
  /* make up the routine name when we dynamically load */
  snprintf(tbuf, SZ_LINE, "Regions%d%d", (int)getpid(), regions_num++);
  reg->pname = xstrdup(tbuf);
  /* make up name of C source file we will generate */
  if( reg->debug >= 2 ){
    reg->fp = stdout;
    return 1;
  } else {
    if( (fd=mkrtemp(prefix, ".c", tbuf, SZ_LINE, 1)) < 0 ){
      xerror(stderr, "could not generate C filter source name: %s\n", prefix);
      return 0;
    }
    reg->code = xstrdup(tbuf);
    if( !(reg->fp = fdopen(fd, "w+b")) ){
      xerror(stderr, "could not open C filter source file: %s\n", tbuf);
      return 0;
    }
  }
  /* make up the name of the program we will compile into.
     we make this different from the .c file name to make interception
     by an intruder harder */
  if( mkrtemp(prefix, NULL, tbuf, SZ_LINE, 0) < 0 ){
    xerror(stderr, "could not generate C filter program name: %s\n", prefix);
    return 0;
  }
#ifdef __MINGW32__
  strcat(tbuf, ".exe");
#endif
  reg->prog = xstrdup(tbuf);
  return 1;
}

/*
 *
 * RegionsProgPrepend_C -- prepend the filter code
 *
 */
static int RegionsProgPrepend_C(Regions reg){
  char *s=NULL;
  char *contents=NULL;
  FILE *fd;

  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  /* make sure we are not in debug mode */
  if( reg->debug >= 2 ){
    return 1;
  }
  /* init temps */
  fd = reg->fp;
  /* initialize with process type */
  switch(reg->ptype){
  case PTYPE_CONTAINED:
    fprintf(fd, "#define REGIONS_PTYPE c\n");
    break;
  case PTYPE_DYNAMIC:
    fprintf(fd, "#define REGIONS_PTYPE d\n");
    break;
  case PTYPE_PROCESS:
    fprintf(fd, "#define REGIONS_PTYPE p\n");
    break;
  }
  /* for some compilers (e.g. pcc), we need to minimize use of #include */
  if( (s=strrchr(reg->cc, '/')) ){
    s++;
  } else {
    s = reg->cc;
  }
  if( !strcasecmp(s, "pcc") || !strcasecmp(s, "pcc.exe") ){
    fprintf(fd, "#define MINIMIZE_INCLUDES 1\n");
  }
  /* do we need windows pipes? */
  if( reg->pipeos == PIPE_WIN32 ){
    fprintf(fd, "#define USE_WIN32 1\n");
    fprintf(fd, "#include <windows.h>\n");
  }
  /* prepend the filter header */
  contents = IMREGIONS_H;
  if( (contents != NULL) && (*contents != '\0') ){
    fprintf(fd, "%s\n", contents);
  }
  /* these are implemented as aliases */
  fprintf(fd, "#define imvcirclei imvannulusi\n");
  fprintf(fd, "#define imncirclei imnannulusi\n");
  fprintf(fd, "#define imvcircle imvannulus\n");
  fprintf(fd, "#define imncircle imnannulus\n");
  fprintf(fd, "#define imcpandai impandai\n");
  fprintf(fd, "#define imcpanda impanda\n");
  fprintf(fd, "\n");
  /* add some math support */
  if( bigendian() ){
    fprintf(fd, "static unsigned char _nan[8]={0x7F,0xF0,1,1,1,1,1,1};\n");
  } else {
    fprintf(fd, "static unsigned char _nan[8]={1,1,1,1,1,1,0xF0,0x7F};\n");
  }
  fprintf(fd, "#define NaN *((double *)_nan)\n");
  fprintf(fd, "#define div(a,b) (feq(b,0)?(NaN):(a/b))\n");
  fprintf(fd, "\n");
  return 1;
}

/*
 *
 * RegionsProgWrite_C -- write the symbols for filtering
 *
 */
static int RegionsProgWrite_C(Regions reg){
  char *ibuf;
  char *contents=NULL;
  FILE *fd;

  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  /* make sure we are init'ed */
  if( reg->fp == NULL ){
    xerror(stderr, "no output file for parser\n");
    return 0;
  }
  /* get the filter string */
  if( !reg->filter || !strcmp(reg->filter, "()") ){
    return 0;
  }
  /* init temps */
  fd= reg->fp;
  /* ptype-specific processing */
  switch(reg->ptype){
  case PTYPE_CONTAINED:
    /* Write code to output file -- must be done BEFORE we write the
       region symbols, to avoid unintentional redefinitions */
    if( reg->debug < 2 ){
      /* region routines if not linking against previously compiled code */
      /* image are filtered by image pixels */
      contents = IMREGIONS_C;
      if( (contents != NULL) && (*contents != '\0') ){
	/* use fprintf so that we handle \n correctly in TEMPLATE */
	fprintf(fd, "%s\n", contents);
      } else {
	xerror(stderr, "could not write filter subroutines\n");
	return 0;
      }
    }
    break;
  }
  /* always need the swap routines (they're part of the filter) */
  /* name of routine to call */
  fprintf(fd, "#define IMFILTRTN %s\n", reg->pname);
  /* output counts of shapes */
  fprintf(fd, "#define NSHAPE %d\n", reg->nshape);
  /* output the filter itself */
  fprintf(fd, "#define FILTER %s\n", reg->filter);
  /* string version of the filter -- used to check for FIELD optimization */
  fprintf(fd, "#define FILTSTR \"%s\"\n", reg->filter);
  /* output the initialization string */
  ibuf = _RegionsInitString(reg);
  fprintf(fd, "#define FINIT %s\n", ibuf);
  xfree(ibuf);
  /* write it now */
  fflush(fd);
  return 1;
}

/*
 *
 * RegionsProgAppend_C -- append the filter program body
 *
 */
static int RegionsProgAppend_C(Regions reg){
  char *contents=NULL;

  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  /* make sure we are not in debug mode */
  if( reg->debug >= 2 ){
    return 1;
  }
  /* get body of filter program */
  contents = IMFILTER_C;
  if( (contents != NULL) && (*contents != '\0') ){
    /* use fprintf so that we handle \n correctly in TEMPLATE */
    fprintf(reg->fp, "%s\n", contents);
    return 1;
  } else {
    xerror(stderr, "could not write body of filter program\n");
    return 0;
  }
}

/*
 *
 * RegionsProgClose_C -- close the filter program file
 *
 */
static int RegionsProgClose_C(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  /* close file if we are not in debug mode */
  if( (reg->debug < 2) && reg->fp ){
    fclose(reg->fp);
    reg->fp = NULL;
  }
  return 1;
}

/*
 *
 * RegionsProgCompile_C -- compile the filter program
 *
 */
static int RegionsProgCompile_C(Regions reg){
  char *s;
  char *math;
  char tbuf[SZ_LINE];
  char pmode[SZ_LINE];
  char log[SZ_LINE];
  char *devnull;
#ifdef USE_LAUNCH
  char *stdfiles[3];
#else
  char tbuf2[SZ_LINE];
#endif
  int len;
  int got;
  int keep=0;

  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  /* make sure we are not in debug mode */
  if( !reg->cc || (reg->debug >= 2) ){
    return 1;
  }
  /* flag whether to keep compiler files around */
  if( !(s=getenv("REGIONS_KEEP")) || !istrue(s) ){
    keep = 0;
  } else {
    keep = 1;
  }
  /* add math library, if necessary */
  switch(reg->pipeos){
  case PIPE_WIN32:
    math = "";
    break;
  default:
    math = "-lm";
    break;
  }
  /* set up /dev/null */
#ifdef __MINGW32__
  devnull = "nul";
#else
  devnull = "/dev/null";
#endif
  /* get log file name */
  snprintf(log, SZ_LINE, "%s.log", reg->prog);
  /* delete old version */
  unlink(reg->prog);
  switch(reg->ptype){
  case PTYPE_PROCESS:
    /* make up the compile command */
    snprintf(tbuf, SZ_LINE, "%s %s -o %s %s %s %s %s", 
	     reg->cc, reg->cflags, reg->prog, reg->code, 
	     reg->objs ? reg->objs : " ",
	     reg->extra, math);
#ifndef USE_LAUNCH
    snprintf(tbuf2, SZ_LINE, " 1>%s 2>%s", devnull, log);
    strcat(tbuf, tbuf2);
#endif
    strcpy(pmode, "x");
    break;
  case PTYPE_CONTAINED:
    /* make up the compile command */
    snprintf(tbuf, SZ_LINE, "%s %s -o %s %s %s %s", 
	     reg->cc, reg->cflags, reg->prog, reg->code,
	     reg->extra, math);
#ifndef USE_LAUNCH
    snprintf(tbuf2, SZ_LINE, " 1>%s 2>%s", devnull, log);
    strcat(tbuf, tbuf2);
#endif
    strcpy(pmode, "x");
    break;
  case PTYPE_DYNAMIC:
#if HAVE_DL
    snprintf(tbuf, SZ_LINE, "%s %s %s %s -o %s %s %s", 
	     reg->cc, reg->cflags, reg->shflags, 
	     reg->objs ? reg->objs : " ", 
	     reg->prog, reg->code, reg->extra);
#ifndef USE_LAUNCH
    snprintf(tbuf2, SZ_LINE, " 1>%s 2>%s", devnull, log);
    strcat(tbuf, tbuf2);
#endif
    strcpy(pmode, "r");
#else
    xerror(stderr, "dynamic linking is not available on this host\n");
#endif
    break;
  default:
    return 0;
  }
  /* issue the shell command to compile the program */
#ifdef USE_LAUNCH
  stdfiles[0] = NULL;
  stdfiles[1] = devnull;
  stdfiles[2] = log;
  got = Launch(tbuf, 1, stdfiles, NULL);
#else
  got = system(tbuf);
#endif
  /* delete the filter program body in any case */
  if( !keep ){
    unlink(reg->code);
  }
  /* Sun cc can leave an extraneous .o around, which we don't want */
  strcpy(tbuf, reg->code);
  /* change .c to .o */
  tbuf[strlen(tbuf)-1] = 'o';
  unlink(tbuf);
  /* ... actually its usually left in the current directory */
  if( (s = strrchr(tbuf, '/')) ){
    unlink(s+1);
  }
  /* now we can see if we succeeded in issuing the command */
  if( got < 0 ){
    xerror(stderr, "could not run filter compilation\n");
    return 0;
  }
  /* if we have an executable program, we succeeded */
  if( (s=Find(reg->prog, pmode, NULL, NULL)) != NULL ){
    unlink(log);
    xfree(s);
  } else {
    s = FileContents(log, 0, &len);
    if( s && *s && len ){
      xerror(stderr, "Compilation error message:\n%s\n", s);
    }
    if( !keep ){
      unlink(log);
    }
    xfree(s);
    xerror(stderr, "filter compilation failed\n");
    return 0;
  }
  
  /* good news */
  return 1;
}

/*
 *
 * RegionsProgEnd_C -- end the filtering process
 *
 */
static int RegionsProgEnd_C(Regions reg){
  char *s;
  int status=0;

  /* make sure we have something to play with */
  if( !reg ){
    return 0;
  }
  /* delete the filter program */
  unlink(reg->prog);
  /* delete the filter program body, if necessary */
  if( !(s=getenv("REGIONS_KEEP")) || !istrue(s) ){
     unlink(reg->code);
  }
  return status;
}

/*
 *
 * Public Routines
 *
 */

/*
 *
 * RegionsProgLoad_C -- load the routines needed to support C filtering
 *
 */
int RegionsProgLoad_C(Regions reg){
  /* make sure we have something to work with */
  if( !reg ){
    return 0;
  }
  reg->reg_open = RegionsProgOpen_C;
  reg->reg_prepend = RegionsProgPrepend_C;
  reg->reg_write = RegionsProgWrite_C;
  reg->reg_append = RegionsProgAppend_C;
  reg->reg_close = RegionsProgClose_C;
  reg->reg_compile = RegionsProgCompile_C;
  reg->reg_end = RegionsProgEnd_C;
  return 1;
}
