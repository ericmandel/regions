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

void *DLOpen(char *name){
    xerror(stderr, "dynamic linking not available\n");
    return NULL;
}

void *DLSym(void *dl, char *name){
    return NULL;
}

int DLClose(void *dl){
    return -1;
}

#endif
