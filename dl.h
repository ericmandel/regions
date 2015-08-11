#ifndef	__dl_h
#define	__dl_h

#if HAVE_CONFIG_H
#include "conf.h"
#endif

void *DLOpen(char *name);
void *DLSym(void *dl, char *name);
int DLClose(void *dl);

#endif
