/*
 *	Copyright (c) 2015 Smithsonian Astrophysical Observatory
 */

/*
 *
 * xerror -- generalized error messages
 *
 */
#include "xerror.h"

/*
 *
 * Private Routines
 *
 */
#define XBUFSZ 32768

static char _xerrors[XBUFSZ];
static int _xerror = -1;

static char _xwarnings[XBUFSZ];
static int _xwarning = -1;

static FILE *_xfd=NULL;
static void (*_xexit)(void)=NULL;

/*
 *
 * xerrorstring -- return last exception string
 *
 */
char *xerrorstring(void){
  return(_xerrors);
}

/*
 *
 * setxerror -- set the error flag
 *
 */
int setxerror(int flag){
  int oflag;
  oflag = _xerror;
  _xerror = flag;
  return oflag;
}

/*
 *
 * setxerrorfd -- override the xerror fd globally
 *
 */
FILE *setxerrorfd(FILE *fd){
  FILE *ofd;
  ofd = _xfd;
  _xfd = fd;
  return ofd;
}

/*
 *
 * setxerrorexit -- set the routine to call before we exit
 * (used to clean up when exiting in Emscripten, which does not call atexit())
 *
 */
void setxerrorexit(void (*rtn)(void)){
  _xexit = rtn;
}

/*
 *
 * xerror - error message handler
 * actions based on value of error flag:
 *
 *  flag    action
 *  ----    ------
 *   0      store error message in _xerrors string
 *   1      store, print error message and continue
 *   2      print error message and exit
 *
 */
void xerror(FILE *fd, char *format, ...){
    char tbuf[SZ_LINE];
    va_list args;
    va_start(args, format);
    /* initialize level, if not already done */
    if( _xerror == -1 ){
      char *s;
      if( (s=getenv("XERROR")) ){
	_xerror = atoi(s);
      } else {
	_xerror = 2;
      }
    }
    snprintf(tbuf, SZ_LINE-1, "ERROR: %s", format);
    vsnprintf(_xerrors, SZ_LINE-1, tbuf, args);
    /* if global _xfd is set, use that instead of the passed value */
    if( _xfd != NULL ){
      fd = _xfd;
    } 
    /* if the error flag is positive, we output immediately */
    if( (fd != NULL) && _xerror ){
      fputs(_xerrors, fd);
      fflush(fd);
    }
    /* if the error flag is set high, we exit */
    if( _xerror >= 2 ){
      if( _xexit != NULL ){
	(_xexit)();
      }
      exit(_xerror);
    }
}

/*
 *
 * xwarningstring -- return last exception string
 *
 */
char *xwarningstring(void){
  return(_xwarnings);
}

/*
 *
 * setxwarning -- set the warning flag
 *
 */
int setxwarning(int flag){
  int oflag;
  oflag = _xwarning;
  _xwarning = flag;
  return oflag;
}

void xwarning(FILE *fd, char *format, ...){
    char tbuf[SZ_LINE];
    va_list args;
    va_start(args, format);
    /* initialize level, if not already done */
    if( _xwarning == -1 ){
      char *s;
      if( (s=getenv("XWARNING")) ){
	_xwarning = atoi(s);
      } else {
	_xwarning = 1;
      }
    }
    snprintf(tbuf, SZ_LINE-1, "WARNING: %s", format);
    vsnprintf(_xwarnings, SZ_LINE-1, tbuf, args);
    /* if global _xfd is set, use that instead of the passed value */
    if( _xfd != NULL ){
      fd = _xfd;
    } 
    /* if the warning flag is positive, we output immediately */
    if( (fd != NULL) && _xwarning ){
      fputs(_xwarnings, fd);
      fflush(fd);
    }
}
