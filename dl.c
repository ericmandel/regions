#include <stdio.h>
#include "util/xerror.h"

#if HAVE_DL

#include "dl.h"
#include <dlfcn.h>

void *DLOpen(char *name){
  void *d=NULL;
  if( !(d=dlopen(name, RTLD_LAZY)) ){
    xerror(stderr, "in DLOpen: %s\n", dlerror());
  }
  return d;
}

void *DLSym(void *dl, char *name){
  void *d=NULL;
  if( !(d=dlsym(dl, name)) ){
    xerror(stderr, "in DLSym: %s\n", dlerror());
  }
  return d;
}

int DLClose(void *dl){
  if( dl ){
    return dlclose(dl);
  } else{
    return -1;
  }
}

#else

/*
* http://stackoverflow.com/questions/3599160/unused-parameter-warnings-in-c-code
*/
#ifndef UNUSED
#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif
#endif

void *DLOpen(char *UNUSED(name)){
    xerror(stderr, "dynamic linking not available\n");
    return NULL;
}

void *DLSym(void *UNUSED(dl), char *UNUSED(name)){
    return NULL;
}

int DLClose(void *UNUSED(dl)){
    return -1;
}

#endif
