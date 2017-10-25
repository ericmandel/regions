#include <stdio.h>
#include <xutil.h>
#include <file.h>

/* must match Module['rootdir'] in post.js! */
#define ROOTDIR "/"

#define MAX_ARGS 10
#define SZ_REGSTR 102400

static char rstr[SZ_REGSTR];

int _regcnts (int argc, char **argv);

char *regcnts(char *iname, char *sregion, char *bregion, char *cmdswitches){
  int i=0, j=0;
  char *targs=NULL, *targ=NULL;
  char *args[SZ_LINE];
  char tbufs[MAX_ARGS][SZ_LINE];
  char ipath[SZ_LINE];
  char opath[SZ_LINE];
  char *s=NULL;
  args[i++] = "_regcnts";
  *opath = '\0';
  if( cmdswitches && *cmdswitches ){
    targs = (char *)strdup(cmdswitches);
    for(targ=(char *)strtok(targs, " \t"); targ != NULL;
	targ=(char *)strtok(NULL," \t")){
      if( j < MAX_ARGS ){
	strncpy(tbufs[j], targ, SZ_LINE-1);
	if( !strcmp(tbufs[j], "-o") ){
	  strcpy(opath, "!NEXT!");
	} else if( !strcmp(opath, "!NEXT!") ){
	  snprintf(opath, SZ_LINE-1, "%s%s", ROOTDIR, tbufs[j]);
	  strncpy(tbufs[j], opath, SZ_LINE-1);
	}
	args[i++] = tbufs[j++];
      } else {
	break;
      }
    }
    if( targs ) free(targs);
  }
  /* make up output file, if none specified */
  if( opath[0] == '\0' ){
    args[i++] = "-o";
    snprintf(opath, SZ_LINE-1, "%s%s", ROOTDIR, "regcnts.json");
    args[i++] = opath;
  }
  /* input file */
  snprintf(ipath, SZ_LINE-1, "%s%s", ROOTDIR, iname);
  args[i++] = ipath;
  /* regions */
  args[i++] = sregion;
  args[i++] = bregion;
  /* make the low-level call */
  _regcnts(i, args);
  /* look for a return value */
  if( (s = FileContents(opath, 0, NULL)) != NULL ){
    strncpy(rstr, s, SZ_REGSTR);
  } else {
    strncpy(rstr, "ERROR: regcnts failed; no output file created", SZ_REGSTR);
  }
  if( s ) free(s);
  if( *opath ) unlink(opath);
  return rstr;
}
