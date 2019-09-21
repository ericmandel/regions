option caseless
%array

%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "regionsP.h"

/* avoid clang warning */
#define YY_NO_INPUT

/* must match imregions.h */
/* 2**53 can be represented exactly in IEEE */
#define PSTOP		9007199254740992.0


#define MAX_INCLUDE_DEPTH 100
#define SEGINC 1024
#define MASKINC 1024
#define LARGE_NUMBER 65536

/* stolen from fitsy/xos.h */
#define X__PI	3.14159265358979323846
#define X_2PI	( 2 * X__PI )
#define X_R2D	(X_2PI / 360.0)
#define X_R2H	(X_2PI /  24.0)
#define X_H2D	(360.0 /  24.0)

#define r2h(r)	( (r) / X_R2H )
#define h2r(d)	( (d) * X_R2H )
#define r2d(r)	( (r) / X_R2D )
#define d2r(d)	( (d) * X_R2D )
#define h2d(r)	( (r) * X_H2D )
#define d2h(d)	( (d) / X_H2D )

/* region types */
#define TY_ANNULUS   1
#define TY_BOX       2
#define TY_CIRCLE    3
#define TY_ELLIPSE   4
#define TY_FIELD     5
#define TY_LINE      6
#define TY_PANDA     7
#define TY_BPANDA    8
#define TY_CPANDA    9
#define TY_EPANDA    10
#define TY_PIE       11
#define TY_QTPIE     11
#define TY_POINT     12
#define TY_POLYGON   13

/* oft-used checks on argument and coord types */
#define XARG    ((narg % 2) == 1)
#define YARG    ((narg % 2) == 0)
#define POSARG  ((regid == TY_POLYGON) || (regid == TY_LINE) || (narg<=2))
#define ANGARG (((regid == TY_PANDA)   && (narg==3)) || \
                ((regid == TY_PANDA)   && (narg==4)) || \
                ((regid == TY_BPANDA)  && (narg==3)) || \
                ((regid == TY_BPANDA)  && (narg==4)) || \
                ((regid == TY_CPANDA)  && (narg==3)) || \
                ((regid == TY_CPANDA)  && (narg==4)) || \
                ((regid == TY_EPANDA)  && (narg==3)) || \
                ((regid == TY_EPANDA)  && (narg==4)) || \
                ((regid == TY_PIE)     && (narg>=3)) )

#define NUMARG (((regid == TY_PANDA)   && (narg==5))  || \
                ((regid == TY_PANDA)   && (narg==8))  || \
		((regid == TY_BPANDA)  && (narg==5))  || \
                ((regid == TY_BPANDA)  && (narg==10)) || \
		((regid == TY_CPANDA)  && (narg==5))  || \
                ((regid == TY_CPANDA)  && (narg==8))  || \
		((regid == TY_EPANDA)  && (narg==5))  || \
                ((regid == TY_EPANDA)  && (narg==10)) )

#define SAVEANG ((regid == TY_BOX) || (regid == TY_ELLIPSE))
                
#define USEWCS (wcssys && strcmp(wcssys, "image") && strcmp(wcssys, "physical"))

#define IMFLIP	(USEWCS && wcs->imflip)

#define WCSSIZE (iswcs(wcs) && (cdelt1 != 0) && (cdelt2 != 0))

/* format statements */
#define DFMT1 ",%f"
#define DFMT2 ",%f,%f"
#define DFMT4 ",%f,%f,%f,%f"
#define SFMT1 ",%s"
#define IFMT1 ",%d"

typedef struct segmentrec {
  int n;
  int type;
  int ops;
  int len;
  int flag;
  int nregion;
  char *s;
  int s0;
  char *regions;
  int nr;
  char *shapes;
  int ns;
  int regval;
  char *radang;
  int ralen;
} *Segment, SegmentRec;

static YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
static int include_stack_ptr = 0;
static int parse_error = 0;

static int filterlen;
static int radanglen;
static int regtype;
static int regid;
static int laststart;
static int maxseg;
static int setargs;
static int inexpr;
static int macropass;
static int narg;
static int nparen;
static int nsparen;
static int nrparen;
static int nseg;
static int nroutine;
static int nshape;
static int nregion;
static int offscl;
static int xtype, ytype;
static int brlev;

static double lastd1;
static double lastd2;
static double xpos, ypos;
static double xpix, ypix;
static double cdelt1, cdelt2, crot;
static double dval;
static double lcxm[3][2];

static char lastarg[SZ_LINE];
static char regname[SZ_LINE];
static char macrobuf[SZ_LINE];
static char sbuf[SZ_LINE];
static char saveang[SZ_LINE];

static char *wcssys=NULL;
static char *filter=NULL;
static char *radang=NULL;

static struct WorldCoor *wcs=NULL;
static Regions xreg=NULL;
static Segment *seg=NULL;

/* these are read-only */
static char *rnames[] = {"annulus", "box", "circle", "ellipse", 
                         "line", "panda", "pie", "qtpie", "point",
			 "polygon", "field", "bpanda", "cpanda", "epanda", 
			 NULL};
static int rargs[] =     {4, 5, 3, 5,
                          4, 8, 4, 4, 2,
                          6, 0, 11, 8, 11,
                          0};
static int vargs[] =     {'y', 'o', 'y', 'o', 
                          'n', 'n', 'y', 'n', 'e',
                          'e', 'n',  'n',  'n',  'n', 
                          '\0'};

static void bin2num(char *d, char *s, int maxlen);
static int _regTypeCmp(const void *s1, const void *s2);
static int _regAccel(int n);
static int _regRegionPanda(void);
static int _regLCX(void);
static int _regArgCheck(char *name, int args);
static int _regNeedOp(int i, char *s);
static int _regRegions(int type);
static void _regStart();
static void _regEnd(void);
static void _regNew(void);
static void _regCat(char *s);
static void _regOp(char *s);
static void _regTypeSet(int type);
static void _regParen(char *s);
static void _regRegionVal(char *s);
static void _regArgSave(double dval);
static void _regParseEnd(void);
static void _regNum(int lcx);
static void _regMake(void);
static void _regInitWCS(void);
static void _regSetWCS(char *s);
static char *_regGetWCS(void);
static void _regScanString(char *s);
static int _regerror(char *msg);

int regparse(Regions reg);
%}

DIG	[0-9]
DIG2	[0-9a-fA-F]
BINARY  0[bB][01]+
INT1	[-+]?{DIG}+L?
INT2	[-+]?0[xX]{DIG2}+L?
INT	({INT1}|{INT2})
FLOAT1  [-+]?{DIG}+\.?([eE][-+]?{DIG}+)?
FLOAT2  [-+]?{DIG}*\.{DIG}+([eE][-+]?{DIG}+)?
FLOAT   ({FLOAT1}|{FLOAT2})
NUM	({INT}|{FLOAT}|{BINARY})
HMS	{INT}{SP}[:hH]{SP}{INT}{SP}[:mM]{SP}{NUM}{SP}[s]?
DMS	{INT}{SP}[:dD]{SP}{INT}{SP}[:mM]{SP}{NUM}{SP}[s]?
WCS	{HMS}|{DMS}
DEG	{NUM}d
RAD	{NUM}r
IPIX	{NUM}i
PPIX	{NUM}p
ASEC	{NUM}\"
AMIN	{NUM}'
COORSYS	(j2000|fk5|icrs|b1950|fk4|galactic|ecliptic|linear|image|physical|amplifier|detector)
NAME	[A-Za-z_][0-9A-Za-z~_]*(\[[^\[]*\])?
FILE	@[0-9A-Za-z~_/\-\.]*
REGION	[\!+-]?(ann|annulus|box|cir|circle|ell|ellipse|lin|line|pan|panda|pie|poi|qtpie|qtp|point|pol|polygon|fie|field|bpanda|bpa|cpanda|cpa|epanda|epa)
REGVAL	{SP}\{{INT}\}
LOP	(&&|\|\||[&\|])
OP	(&&|\|\||==|!=|<=|>=|<<|>>|[&\|~=<>+\-*/%^])
STR	(\"[^\"]*\"|'[^']*')
SP	[ \t]*
EOL	([;\n]|\\n)
SEP	(,|{EOL})
COM	#
US	(unsigned{SP})?
II	(int{SP})?
DTYPE	(\({US}char\)|\({US}short{II}\)|\({US}int\)|\({US}long{II}\)|\(float\)|\(double\))

%s RTINE
%s REG
%x COMM
%x JSON

%%

{DTYPE} { 
  _regCat(yytext);
}

{COORSYS}[\n;,]* {
  _regSetWCS(yytext);
}

global  { BEGIN COMM; }
tile    { BEGIN COMM; }
compass { BEGIN COMM; }

{COM} { 
  /* comment acts like EOL */
  if( YY_START == REG ){
    _regEnd();
    _regNew(); 
  } else {
    _regNew(); 
  }
  BEGIN COMM;
}
<COMM>\n  { BEGIN INITIAL; }
<COMM>\\n { BEGIN INITIAL; }
<COMM>.   { /* ignore comments up to eol */ ; }

"{" {
  /* json object finished a region */
  if( YY_START == REG ){
    _regEnd();
    _regNew(); 
  } else {
    _regNew(); 
  }
  brlev++;
  BEGIN JSON;
}
<JSON>"{" {
  brlev++;
}
<JSON>"}" {
  if( --brlev == 0 ){
    BEGIN INITIAL;
  }
}
<JSON>. { /* ignore json */ ;}

[+-]?text([ \t]+|{SP}\({SP})[^;\n]*{EOL}  	{ ; /* ignore */ }
[+-]?vector([ \t]+|{SP}\({SP})[^;\n]*{EOL}  	{ ; /* ignore */ }
[+-]?ruler([ \t]+|{SP}\({SP})[^;\n]*{EOL} 	{ ; /* ignore */ }
[+-]?projection([ \t]+|{SP}\({SP})[^;\n]*{EOL}	{ ; /* ignore */ }

[+-]?field([ \t]+|{SP}\({SP})?    |
{REGION}([ \t]+|{SP}\({SP})/{NUM} {
  /* finish off previous */
  if( YY_START == REG ){
    _regEnd();
    _regNew();
  }
  /* Found a region */
  BEGIN REG;
  _regStart();
}
{REGION}([ \t]+|{SP}\({SP})/{NAME} {
  /* can't happen */
  _regerror("invalid characters following the region name");
}
<REG>{NUM} {
  narg++;
  /* process this pure number in the current coord system */
  _regNum(_regLCX());
}
<REG>{IPIX} {
  narg++;
  /* remove trailing unit character */
  yytext[strlen(yytext)-1] = '\0';
  /* process this  as a pure number in image coord sys */
  _regNum(1);
}
<REG>{PPIX} {
  narg++;
  /* remove trailing unit character */
  yytext[strlen(yytext)-1] = '\0';
  /* process this  as a pure number in physical coord sys */
  _regNum(LCX_PHYS);
}
<REG>{WCS} {
  char *csys;
  if( nowcs(wcs) ){
    _regerror("no WCS information in file");
  }
  narg++;
  /* save x value globally for processing with y next time */
  if( XARG ){
    xpos = SAOstrtod(yytext,NULL);
    xtype = SAOdtype;
    if( (xtype != 0) && (xtype != '.') && (xtype != 'd') ){
      csys = _regGetWCS();
      /* arg1 coords are hms, but ecliptic, galactic are deg */
      if(!csys || (strncasecmp(csys, "ecl", 3) && strncasecmp(csys, "gal", 3))){
	xpos = h2d(xpos);
      }
    }
  } else {
    ypos = SAOstrtod(yytext,NULL);
    ytype = SAOdtype;
    /* convert wcs to image pixels */
    wcsc2pix(wcs, xpos, ypos, _regGetWCS(), &xpix, &ypix, &offscl);
    snprintf(sbuf, SZ_LINE, DFMT2, xpix, ypix);
    _regCat(sbuf);
  }
}
<REG>{DEG} {
  narg++;
  /* handle position arguments */
  if( POSARG ){
    if( nowcs(wcs) ){
      _regerror("no WCS information in file");
    }
    /* save x value globally for processing with y next time */
    if( XARG ){
      xpos = strtod(yytext,NULL);
    } else {
      ypos = strtod(yytext,NULL);
      /* convert wcs to image pixels */
      wcsc2pix(wcs, xpos, ypos, _regGetWCS(), &xpix, &ypix, &offscl);
      snprintf(sbuf, SZ_LINE, DFMT2, xpix, ypix);
      _regCat(sbuf);
    }
  } else if( ANGARG ){
    /* angle arguments are just passed along, with updated crot */
    dval = strtod(yytext,NULL);
    if( USEWCS ){
      dval += crot;
    }
    if( IMFLIP ){
      dval = -dval;
    }
    snprintf(sbuf, SZ_LINE, DFMT1, dval);
    _regCat(sbuf);
  } else if( NUMARG ){
    /* panda numeric args are just passed along */
    dval = strtod(yytext,NULL);
    snprintf(sbuf, SZ_LINE, DFMT1, dval);
    _regCat(sbuf);
  } else {
    /* handle size arguments */
    if( !WCSSIZE ){
      _regerror("no WCS (or CDELT) information in file");
    }
    if( SAVEANG ){
      dval = strtod(yytext,NULL);
      if( USEWCS ){
	dval += crot;
      }
      if( IMFLIP ){
	dval = -dval;
      }
      snprintf(saveang, SZ_LINE, DFMT1, dval);
    }
    /* arg 1 is ra, arg2 is dec */
    if( XARG ){
      dval = ABS(strtod(yytext,NULL)/cdelt1);
    } else {
      dval = ABS(strtod(yytext,NULL)/cdelt2);
    }
    snprintf(sbuf, SZ_LINE, DFMT1, dval);
    _regCat(sbuf);
    _regArgSave(dval);
  }
}
<REG>{RAD} {
  narg++;
  /* handle position arguments */
  if( POSARG ){
    if( nowcs(wcs) ){
      _regerror("no WCS information in file");
    }
    /* save x value for processing with y next time */
    if( XARG ){
      xpos = r2d(strtod(yytext,NULL));
    } else {
      ypos = r2d(strtod(yytext,NULL));
      /* convert wcs to image pixels */
      wcsc2pix(wcs, xpos, ypos, _regGetWCS(), &xpix, &ypix, &offscl);
      snprintf(sbuf, SZ_LINE, DFMT2, xpix, ypix);
      _regCat(sbuf);
    }
  } else if( ANGARG ){
    /* angle arguments are just passed along, with updated crot */
    dval = r2d(strtod(yytext,NULL));
    if( USEWCS ){
      dval += crot;
    }
    if( IMFLIP ){
      dval = -dval;
    }
    snprintf(sbuf, SZ_LINE, DFMT1, dval);
    _regCat(sbuf);
  } else if( NUMARG ){
    /* panda numeric args are just passed along */
    dval = strtod(yytext,NULL);
    snprintf(sbuf, SZ_LINE, DFMT1, dval);
    _regCat(sbuf);
  } else {
    /* handle size arguments */
    if( !WCSSIZE ){
      _regerror("no WCS (or CDELT) information in file");
    }
    if( SAVEANG ){
      dval = r2d(strtod(yytext,NULL));
      if( USEWCS ){
	dval += crot;
      }
      if( IMFLIP ){
	dval = -dval;
      }
      snprintf(saveang, SZ_LINE, DFMT1, dval);
    }
    /* arg 1 is ra, arg2 is dec */
    if( XARG ){
      dval = r2d(ABS(strtod(yytext,NULL)/cdelt1));
    } else {
      dval = r2d(ABS(strtod(yytext,NULL)/cdelt2));
    }
    snprintf(sbuf, SZ_LINE, DFMT1, dval);
    _regCat(sbuf);
    _regArgSave(dval);
  }
}
<REG>{AMIN} {
  if( !WCSSIZE ){
    _regerror("no WCS (or CDELT) information in file");
  }
  narg++;
  /* arg 1 is ra, arg2 is dec */
  if( XARG ){
    dval = ABS(strtod(yytext,NULL)/(cdelt1*60.0));
  } else {
    dval = ABS(strtod(yytext,NULL)/(cdelt2*60.0));
  }
  snprintf(sbuf, SZ_LINE, DFMT1, dval);
  _regCat(sbuf);
  _regArgSave(dval);
}
<REG>{ASEC} {
  if( !WCSSIZE ){
    _regerror("no WCS (or CDELT) information in file");
  }
  narg++;
  /* arg 1 is ra, arg2 is dec */
  if( XARG ){
    dval = ABS(strtod(yytext,NULL)/(cdelt1*3600.0));
  } else {
    dval = ABS(strtod(yytext,NULL)/(cdelt2*3600.0));
  }
  snprintf(sbuf, SZ_LINE, DFMT1, dval);
  _regCat(sbuf);
  _regArgSave(dval);
}
<REG>[a-z]{SP}={SP}{NUM}{SP} {
  int n;
  n = strtol(strchr(yytext, '=')+1, NULL, 10);
  if( n <= 0 ){
    _regerror("invalid region accelerator");
  } else {
    if( !_regAccel(n) ){
      _regerror("invalid region accelerator");
    }
  }
}
<REG>{NUM}/{OP} {
  if( _regArgCheck(regname, narg) == 0 ){
    /* new expression without an expression separator ... sigh */
    _regEnd();
    _regNew();
    BEGIN INITIAL;
    yyless(0);
  } else {
    narg++;
    /* process this pure number in the current coord system */
    _regNum(_regLCX());
  }
}
<REG>({NAME}{OP}|{FILE}) {
  /* new expression without an expression separator ... sigh */
  _regEnd();
  _regNew();
  BEGIN INITIAL;
  yyless(0);
}
<REG>, {
  /* explicit rule avoids the _regNew of general rule */
  ;
}
<REG>{REGVAL} {
  _regRegionVal(yytext);
  if( !nsparen ){
    _regEnd();
    BEGIN INITIAL;
  }
}
<REG>\(	{ 
  nsparen++;
}
<REG>\){REGVAL}	{
	_regRegionVal(yytext+1);
	if( nsparen ){
	  nsparen--;
	  if( !nsparen ){
	    _regEnd();
	    BEGIN INITIAL;
	  }
	} else {
	  _regEnd();
	  BEGIN INITIAL;
	  /* REJECT; */
	  // explicit execution of general rule instead of REJECT
	  nparen--;
	  _regNew();
	  _regCat(")");
	  _regNew();
	}
}
<REG>\)	{
  if( nsparen ){
    nsparen--;
    if( !nsparen ){
      _regEnd();
      BEGIN INITIAL;
    }
  } else {
    _regEnd();
    BEGIN INITIAL;
    /* explicit execution of general rule instead of REJECT */
    nparen--;
    _regNew();
    _regCat(")");
    _regNew();
  }
}
<REG>{OP} { 
  _regEnd();
  _regOp(yytext);
  BEGIN INITIAL;
}
<REG>{EOL} { 
  _regEnd();
  _regNew();
  BEGIN INITIAL;
}

{NAME}{SP}/\( { 
  /* Found a routine */
  laststart = YY_START;
  BEGIN RTINE;
  narg = 0;
  nrparen = 0;
  _regTypeSet(TOK_RTINE);
  _regCat(RegionsLexRoutine1(xreg, yytext));
  nroutine++;
}
<RTINE>, {
  /* explicit rule avoids the _regNew of general rule */
  _regCat(",");
}
<RTINE>\( {
  nrparen++;
  _regCat("(");
}
<RTINE>\) {
  nrparen--;
  if( nrparen ){
    _regCat(")");
  } else {
    /* end of routine */
    BEGIN laststart;
    _regCat(RegionsLexRoutine2(xreg, ")"));
  }
}

{FILE} {
  /* Found an include file */
  char *s=NULL;
  if( YY_START == REG ){
    _regEnd();
    _regNew();
  }
  if ( include_stack_ptr >= MAX_INCLUDE_DEPTH ) {
    _regerror("include files are nested too deeply");
  }
  /* get contents of include file */
  if( !(s = (char *)FileContents(yytext+1, 0, NULL)) ){
    _regerror("include file empty or inaccessible");
  }
  /* set up to scan include file contents */
  if( s && *s ){
    include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
    _regNew();
    _regParen("(");
    _regScanString(s);
    xfree(s);
  } else {
    xfree(s);
    _regerror("include file is empty");
  }
}

{SP} { ; /* ignore white space */ }

{STR} { narg++; _regCat(yytext); }

{NUM} {
  char tbuf[SZ_LINE];
  bin2num(tbuf, yytext, SZ_LINE-1);
  narg++;
  _regCat(tbuf);
}

{OP} {
  _regOp(yytext);
  if( YY_START != RTINE ){ BEGIN INITIAL; }
}

\( {
  nparen++;
  _regNew();
  _regCat(yytext);
  _regNew();
}
\) {
  nparen--;
  _regNew();
  _regCat(yytext);
  _regNew();
}
{SEP} {       
  _regNew();
  BEGIN INITIAL;
}

. { 
  _regCat(yytext);
}

<<EOF>>	{
  if( --include_stack_ptr < 0 ){
    yy_delete_buffer( YY_CURRENT_BUFFER );
    yyterminate();
  } else {
    _regParen(")");
    _regNew();
    yy_delete_buffer( YY_CURRENT_BUFFER );
    yy_switch_to_buffer(include_stack[include_stack_ptr] );
  }
}

%%

/*
 *
 * Private Routines 
 *
 *
 */

static void floatprefix(void){
  if( xreg->method == METHOD_C ){
    _regCat("(double)");
  }
}

static void floatsuffix(void){
  return;
}

static void bin2num(char *d, char *s, int maxlen) {
  unsigned long bits;
  char *p=NULL;

  if( strlen(s) < 2 ){
    if( d != s ){
      strncpy(d, s, maxlen-1);
    }
  } else {
    if( (*s=='0') && (*(s+1)=='b') ){
      bits = strtoul(s+2, &p, 2);
      if( *p != '\0' ){
	_regerror("invalid binary value");
      }
      snprintf(d, maxlen, "%lx", bits);
      if( (strlen(d) <2) || (*d!='0') || ((*(d+1)!='x')&&((*(d+1)!='X'))) ){
        snprintf(d, maxlen, "0x%lx", bits);
      }
    } else {
      if( d != s ){
	strncpy(d, s, maxlen-1);
      }
    }
  }
}

/* init lcx: look for logical coord system (LTV/LTM) keywords */
static void lcx_init(char *cards, double lcxm[3][2]){
  if( !cards || !hgetr8(cards, "LTM1_1", &lcxm[0][0]) ){
    lcxm[0][0] = 1.0;
  }
  if( !cards || !hgetr8(cards, "LTM2_1", &lcxm[1][0]) ){
    lcxm[1][0] = 0.0;
  }
  if( !cards || !hgetr8(cards, "LTM1_2", &lcxm[0][1]) ){
    lcxm[0][1] = 0.0;
  }
  if( !cards || !hgetr8(cards, "LTM2_2", &lcxm[1][1]) ){
    lcxm[1][1] = 1.0;
  }
  if( !cards || !hgetr8(cards, "LTV1", &lcxm[2][0]) ){
    lcxm[2][0] = 0.0;
  }
  if( !cards || !hgetr8(cards, "LTV2", &lcxm[2][1]) ){
    lcxm[2][1] = 0.0;
  }
#if FUNTOOLS_COMPATIBILITY
  if( (lcxm[0][0] == 1.0) && (lcxm[0][1] == 0.0) && (lcxm[2][0] == 0.5) ){
    lcxm[2][0] = 0.0;
  }
  if( (lcxm[0][1] == 0.0) && (lcxm[1][1] == 1.0) && (lcxm[2][1] == 0.5) ){
    lcxm[2][1] = 0.0;
  }
#endif
}

/* lcx: convert physical to image position */
static void lcx_phys2img(double lcxm[3][2], double x0, double y0, 
                         double *x1, double *y1){
  *x1 = x0 * lcxm[0][0] + y0 * lcxm[1][0] + lcxm[2][0];
  *y1 = x0 * lcxm[0][1] + y0 * lcxm[1][1] + lcxm[2][1];
}

/* lcx: convert physical to image size */
static void lcx_phys2img_size(double lcxm[3][2], double x0, double y0, 
                              double *x1, double *y1){
  *x1 = x0 * lcxm[0][0];
  *y1 = y0 * lcxm[1][1];
}

/*
 *
 * _regNew -- finish last start and set up new filter segment
 *
 */
static void _regNew(void) {
  int i;
  int last;
  
  if( nseg >= maxseg ){
    last = maxseg;
    if( maxseg == 0 ){
      maxseg = SEGINC;
      seg = (Segment *)xmalloc(maxseg * sizeof(Segment));
    } else {
      maxseg += SEGINC;
      seg = (Segment *)xrealloc(seg, maxseg * sizeof(Segment));
    }
    /* clear the unused records  */
    for(i=last; i<maxseg; i++){
      seg[i] = NULL;
    }
  }
  if( !nseg || (seg[nseg-1] && seg[nseg-1]->s) ){
    seg[nseg] = (Segment)xcalloc(1, sizeof(SegmentRec));
    nseg++;
  }
  seg[nseg-1]->n = nseg-1;
  seg[nseg-1]->nregion = 0;
}

/*
 *
 * _regParen -- add a paren to the filter string buffer
 *
 */
static void _regParen(char *str) {
  /* _regNew(); */
  _regCat(str);
  /* _regNew(); */
}

/*
 *
 * __regCat -- add a string to the filter string buffer
 *
 */
static void __regCat(char *str, char **ostr, int *olen) {
  int blen;
  int slen;
  
  if( (str == NULL) || (*str == '\0') ){
    return;
  } else {
    slen = strlen(str) + 1;
  }
  if( (*ostr == NULL) || (**ostr == '\0') ){
    blen = 0;
  } else {
    blen = strlen(*ostr);
  }
  while( (blen + slen) >= *olen ){
    *olen += SEGINC;
  }
  if( blen == 0 ){
    *ostr = (char *)xcalloc(*olen, sizeof(char));
  } else {
    *ostr = (char *)xrealloc(*ostr, *olen);
  }
  strcat(*ostr, str);
}

/*
 *
 * _regCat -- add a string to the filter string buffer
 *
 */
static void _regCat(char *str) {
  char *s;
  if( !str || !*str ){
    return;
  }
  if( nseg == 0 ){
    _regNew();
  }
  s = xmalloc(strlen(str)+1);
  nowhite(str, s);
  __regCat(s, &(seg[nseg-1]->s), &(seg[nseg-1]->len));
  xfree(s);
}

/*
 *
 * _regArgSave -- save a radius or angle value to the temp string:
 * this is a special hack for regcnts
 *
 */
static void _regArgSave(double dval) {
  char tbuf[SZ_LINE];

  if( narg < 3 ){
    return;
  }
  snprintf(tbuf, SZ_LINE, DFMT1, dval);
  if( narg == 3 ){ 
    *lastarg = '\0';
    lastd1 = 0.0;
    lastd2 = 0.0;
    if( seg[nseg-1]->radang ){
      xfree(seg[nseg-1]->radang);
      seg[nseg-1]->radang = NULL;
      seg[nseg-1]->ralen = 0;
    }
  }
  switch(regid){
  case TY_ANNULUS:
  case TY_CIRCLE:
    if( *lastarg ){
      __regCat(lastarg, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
    }
    __regCat(tbuf, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
    if( narg > 3 ){
      __regCat(",NA,NA\n", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
      strcpy(lastarg, tbuf);
    }
    lastd1 = lastd2;
    lastd2 = dval;
    break;

  case TY_BOX:
  case TY_ELLIPSE:
    if( (narg%2) == 1 ){
      if( *lastarg ){
        __regCat(lastarg, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
      }
      __regCat(tbuf, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
      if( narg > 3 ){
        __regCat(",NA,NA\n", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
        strcpy(lastarg, tbuf);
      }
      lastd1 = lastd2;
      lastd2 = dval;
    }
    break;

  case TY_PIE:
    if( narg == 3 ){
      __regCat("NA,NA", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
    } else if( *lastarg ){
      __regCat("NA,NA,", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
      __regCat(lastarg, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
    }
    __regCat(tbuf, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
    if( narg > 3 ){
      __regCat("\n", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
      strcpy(lastarg, tbuf);
    }
    lastd1 = lastd2;
    lastd2 = dval;
    break;

  case TY_POINT:
    /* we have to process this here if its is a varargs */
    // so, for the 3rd arg, we add a line for initial x, y pair
    if( narg == 3 ){ 
      __regCat("NA,NA,NA,NA\n", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
    } else if( (narg % 2) == 0 ){ 
      /* and for every even arg, add a line for the preceeding x,y pair */
      __regCat("NA,NA,NA,NA\n", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
    }
    break;
  default:
    return;
  }      
}

/*
 *
 * _regAccel -- apply acceleration to a segment
 *
 */
static int _regAccel(int n) {
  int i;
  char tbuf[SZ_LINE];
  char *s;
  double dinc;

  /* check region type */
  switch(regid){
    case TY_ANNULUS:
      break;
    case TY_BOX:
      narg -= 2;
      break;
    case TY_CIRCLE:
      narg -= 1;
      break;
    case TY_ELLIPSE:
      narg -= 2;
      break;
    case TY_PIE:
      break;
    default:
      _regerror("annulus, box, circle, ellipse, pie accelerators only - ");
      break;
  }
  /* add the argument */
  snprintf(tbuf, SZ_LINE, IFMT1, n);
  _regCat(tbuf);
  /* change the name of the routine we will call */
  if( seg[nseg-1]->s0 ){
    if( (s = xstrdup(&seg[nseg-1]->s[seg[nseg-1]->s0])) ){
      seg[nseg-1]->s[seg[nseg-1]->s0] = '\0';
      _regCat("n");
      _regCat(s);
      snprintf(tbuf, SZ_LINE, "%d ", n);
      __regCat(tbuf, &(seg[nseg-1]->regions), &(seg[nseg-1]->nr));
      __regCat(tbuf, &(seg[nseg-1]->shapes), &(seg[nseg-1]->ns));
      setargs = 1;
      free (s);
    }
    if( seg[nseg-1]->radang ){
      xfree(seg[nseg-1]->radang);
      seg[nseg-1]->radang = NULL;
      seg[nseg-1]->ralen = 0;
    }
    switch(regid){
      case TY_ANNULUS:
      case TY_BOX:
      case TY_CIRCLE:
      case TY_ELLIPSE:
        dinc = (lastd2 - lastd1)/n;
        for(i=1; i<=n; i++){
          snprintf(tbuf, SZ_LINE, DFMT2, lastd1+((i-1)*dinc), lastd1+(i*dinc));
          __regCat(tbuf, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
          __regCat(",NA,NA", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
	  __regCat("\n", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
        }
        break;
      case TY_PIE:
        dinc = (lastd2 - lastd1)/n;
        for(i=1; i<=n; i++){
          snprintf(tbuf, SZ_LINE, DFMT2, lastd1+((i-1)*dinc), lastd1+(i*dinc));
          __regCat("NA,NA", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
          __regCat(tbuf, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
	  __regCat("\n", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
        }
        break;
      default:
        _regerror("annulus, box, circle, ellipse, pie accelerators only - ");
        break;
    }
    _regTypeSet(TOK_ACCEL);
  } else {
    _regerror("misplaced accelerator");
  }
  return(n);
}

/*
 *
 * _regTypeSet -- add type info to the filter segment record
 *
 */
static void _regTypeSet(int type) {
  if( nseg == 0 ){
    _regNew();
  }
  seg[nseg-1]->type |= type;
}

/*
 *
 * _regOp -- add an operator to the filter segment record
 *
 */
static void _regOp(char *op) {
  _regCat(op);
  seg[nseg-1]->ops += 1;
}

/*
 *
 * regStart == start a new region
 *
 */
static void _regStart() {
  char *s, *t;
  char tbuf[SZ_LINE];

  /* initialize region */
  narg = 0;
  setargs = 0;
  s = xmalloc(strlen(yytext)+1);
  nowhite(yytext, s);
  /* look for include/exclude */
  if( *s == '+' ){
    regtype = TOK_IREG;
    strcpy(regname, s+1);
  } else if( *s == '-' ){
    regtype = TOK_EREG;
    strcpy(regname, s+1);
  } else if( *s == '!' ){
    regtype = TOK_NREG;
    strcpy(regname, s+1);
  } else {
    regtype = TOK_IREG;
    strcpy(regname, s);
  }
  _regTypeSet(regtype);
  if( !(t=strchr(regname, '(')) ){
    nsparen = 0;
  } else {
  /* remove the paren from the regname string -- we will explicitly add it */
    *t = '\0';
    nsparen=1;
  }  
  /* convert to lower case, remove white space */
  culc(regname);
  nowhite(regname, s);
  strcpy(regname, s);
  /* convert abbrev to region */
  if( !strncmp(regname, "ann", 3) ){
    strcpy(regname, "annulus");
    regid = TY_ANNULUS;
  } else if( !strncmp(regname, "box", 3) ){
    strcpy(regname, "box");
    regid = TY_BOX;
  } else if( !strncmp(regname, "cir", 3) ){
    strcpy(regname, "circle");
    regid = TY_CIRCLE;
  } else if( !strncmp(regname, "ell", 3) ){
    strcpy(regname, "ellipse");
    regid = TY_ELLIPSE;
  } else if( !strncmp(regname, "fie", 3) ){
    strcpy(regname, "field");
    regid = TY_FIELD;
  } else if( !strncmp(regname, "lin", 3) ){
    strcpy(regname, "line");
    regid = TY_LINE;
  } else if( !strncmp(regname, "pan", 3) ){
    strcpy(regname, "panda");
    regid = TY_PANDA;
  } else if( !strncmp(regname, "bpa", 3) ){
    strcpy(regname, "bpanda");
    regid = TY_BPANDA;
  } else if( !strncmp(regname, "cpa", 3) ){
    strcpy(regname, "cpanda");
    regid = TY_CPANDA;
  } else if( !strncmp(regname, "epa", 3) ){
    strcpy(regname, "epanda");
    regid = TY_EPANDA;
  } else if( !strncmp(regname, "pie", 3) ){
    strcpy(regname, "pie");
    regid = TY_PIE;
  } else if( !strncmp(regname, "qtp", 3) ){
    strcpy(regname, "qtpie");
    regid = TY_PIE;
  } else if( !strncmp(regname, "poi", 3) ){
    strcpy(regname, "point");
    regid = TY_POINT;
  } else if( !strncmp(regname, "pol", 3) ){
    strcpy(regname, "polygon");
    regid = TY_POLYGON;
  }
  /* image are filtered by image pixels */
  if( xreg->reg_prefix ){
    _regCat(xreg->reg_prefix);
  }
  _regCat("im");
  /* no region value to start */
  seg[nseg-1]->regval = 0;
  /* save size of current string so we can get back to the region string */
  seg[nseg-1]->s0 = strlen(seg[nseg-1]->s);
  /* append region */
  _regCat(RegionsLexRegion1(xreg,regname));
  /* append standard args */
  _regCat("(g,$RS,");
  /* add include/exclude and whether we mark y lines for this region
     (we don't mark y on excludes as an optimization) */
  switch(regtype){
  case TOK_IREG:
    snprintf(tbuf, SZ_LINE, "1,%d", TOK_IREG);
    _regCat(tbuf);
    break;
  case TOK_NREG:
    snprintf(tbuf, SZ_LINE, "0,%d", TOK_NREG);
    _regCat(tbuf);
    break;
  case TOK_EREG:
    snprintf(tbuf, SZ_LINE, "0,%d", TOK_EREG);
    _regCat(tbuf);
    break;
  }
  _regCat(",");
  floatprefix();
  _regCat("x");
  floatsuffix();
  _regCat(",");
  floatprefix();
  _regCat("y");
  floatsuffix();
  xfree(s);
}

static void _regRegionVal(char *s) {
  char *t=NULL;
  if( (t=strchr(s, '{')) ){
    t++;
  } else {
    t = s;
  }
  seg[nseg-1]->regval = atoi(t);
}

static void _regEnd(void) {
  int n;
  char *s;
  char ebuf[SZ_LINE];
  char tbuf[SZ_LINE];

  /* add final arguments as required */
  switch( regid ){
  case TY_BOX:
    if( (narg%2) == 0 ){
      _regCat(",0.0"); narg++;
    } else {
      /* if we saved an angle arg, we need to replace the last position arg */
      if( *saveang ){
        if( (s=strrchr(seg[nseg-1]->s, ',')) ){
          *s = '\0';
          _regCat(saveang);
        }
      }
    }
    break;
  case TY_ELLIPSE:
    if( (narg%2) == 0 ){
      _regCat(",0.0"); narg++;
    } else {
      /* if we saved an angle arg, we need to replace the last position arg */
      if( *saveang ){
        if( (s=strrchr(seg[nseg-1]->s, ',')) ){
          *s = '\0';
          _regCat(saveang);
        }
      }
    }
    break;
  case TY_BPANDA:
  case TY_EPANDA:
    if( (narg%2) == 0 ){
      _regCat(",0.0"); narg++;
    }
    /* drop through here to process panda */
  case TY_PANDA:
  case TY_CPANDA:
    _regRegionPanda();
    setargs = 1;
    break;
  default:
    break;
  }
  /* argument check */
  if( (n=_regArgCheck(regname, narg)) > 0 ){
    /* don't bother printing where we are in region */
    *yytext = '\0';
    switch( vargs[n-1] ){
    case 'n':
      snprintf(ebuf, SZ_LINE, "'%s' requires %d args (%d found)", 
	      rnames[n-1], rargs[n-1], narg);
      break;
    case 'e':
      snprintf(ebuf, SZ_LINE,
 	       "'%s' requires at least %d (even) args (%d found)", 
 	       rnames[n-1], rargs[n-1], narg);
      break;
    case 'o':
      snprintf(ebuf, SZ_LINE,
               "'%s' requires at least %d (odd) args (%d found)", 
 	       rnames[n-1], rargs[n-1], narg);
      break;
    case 'y':
    default:
      snprintf(ebuf, SZ_LINE, "'%s' requires %d args (%d found)", 
	      rnames[n-1], rargs[n-1], narg);
      break;
    }
    _regerror(ebuf);
  }
  /* process varargs */
  if( (n < 0) || (regid == TY_POLYGON) ){
    if( seg[nseg-1]->type & TOK_ACCEL ){
      _regerror("n= and variable arglist cannot be combined -");
    }
    /* if its a varargs, we need to end with PSTOP values */
    snprintf(sbuf, SZ_LINE, DFMT2, PSTOP, PSTOP);
    _regCat(sbuf);
    /* change the name of the routine we will call */
    if( regid != TY_POLYGON ){
      if( seg[nseg-1]->s0 ){
        if((s = xstrdup(&seg[nseg-1]->s[seg[nseg-1]->s0]))){
          seg[nseg-1]->s[seg[nseg-1]->s0] = '\0';
          _regCat("v");
          _regCat(s);
          switch(regid){
          case TY_ANNULUS:
            n = narg - 3;
            break;
          case TY_BOX:
            n = (narg - 5)/2;
            break;
          case TY_CIRCLE:
            n = narg - 3;
            break;
          case TY_ELLIPSE:
            n = (narg - 5)/2;
            break;
          case TY_PIE:
            n = narg - 3;
            break;
          case TY_POINT:
            n = narg/2;
            break;
          default:
            _regerror("varargs: annulus, box, circle, ellipse, pie, point;");
            break;
          }
          snprintf(tbuf, SZ_LINE, "%d ", n);
          __regCat(tbuf, &(seg[nseg-1]->regions), &(seg[nseg-1]->nr));
          __regCat(tbuf, &(seg[nseg-1]->shapes), &(seg[nseg-1]->ns));
          setargs = 1;
          free (s);
        }
        _regTypeSet(TOK_VARARGS);
      } else {
        _regerror("invalid varargs");
      }
    }
  }
  /* set number of regions and shapes, if not already done */
  if( !setargs ){
    snprintf(tbuf, SZ_LINE, "%d ", 1);
    __regCat(tbuf, &(seg[nseg-1]->regions), &(seg[nseg-1]->nr));
    __regCat(tbuf, &(seg[nseg-1]->shapes), &(seg[nseg-1]->ns));
  }
  /* finish off region string */
  _regCat(RegionsLexRegion2(xreg,")"));
  /* arguments are not set */
  setargs = 0;
  /* save angle not set */
  *saveang = 0;
}

static int _regRegionPanda(void) {
  int i;
  int n = 0;
  int got = 0;
  int a, r;
  int ahi = 0, rhi = 0;
  int hi = 0;
  double d[11];
  double ainc = 0.0, rinc = 0.0;
  char tbuf[SZ_LINE];
  char *s=NULL;
  char *t, *u;

  switch(regid){
  case TY_PANDA:
  case TY_CPANDA:
    hi = 7;
    break;
  case TY_BPANDA:
  case TY_EPANDA:
    hi = 10;
    break;
  default:
    _regerror("unknown panda type -- contact saord@cfa.harvard.edu");
    break;
  }
  s = xstrdup(seg[nseg-1]->s);
  for(i=hi; i>=0; i--){
    /* look for last comma */
    t = strrchr(s, ',');
    if( !t ){
      goto done;
    }
    /* null it out from base string */
    *t = '\0';
    /* point past comma and get double value */
    t++;
    d[i] = strtod(t, &u);
    if( t == u ){
      goto done;
    }
  }
  /* which type of panda? */
  switch(regid){
  case TY_PANDA:
  case TY_CPANDA:
    ainc = (d[3] - d[2])/d[4];
    ahi = (int)d[4];
    rinc = (d[6] - d[5])/d[7];
    rhi = (int)d[7];
    break;
  case TY_BPANDA:
  case TY_EPANDA:
    ainc = (d[3] - d[2])/d[4];
    ahi = (int)d[4];
    rinc = (d[7] - d[5])/d[9];
    rhi = (int)d[9];
    break;
  default:
    _regerror("unknown panda type -- contact saord@cfa.harvard.edu");
    break;
  }
  /* process limits arguments */
  for(a=1; a<=ahi; a++){
    for(r=1; r<=rhi; r++){
      snprintf(tbuf, SZ_LINE, DFMT4, 
               d[5]+((r-1)*rinc), d[5]+(r*rinc),
               d[2]+((a-1)*ainc), d[2]+(a*ainc));
      __regCat(tbuf, &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
      __regCat("\n", &(seg[nseg-1]->radang), &(seg[nseg-1]->ralen));
      n++;
    }
  }
  /* adjust region and shape count */
  snprintf(tbuf, SZ_LINE, "%d ", n);
  __regCat(tbuf, &(seg[nseg-1]->regions), &(seg[nseg-1]->nr));
  snprintf(tbuf, SZ_LINE, "%d ", 2*n); 
  __regCat(tbuf, &(seg[nseg-1]->shapes), &(seg[nseg-1]->ns));
  _regTypeSet(TOK_ACCEL);
  /* success */
  got = 1;
done:
  xfree(s);
  return got;
}

/*
 *
 * _regTypeCmp -- compare routine for qsort
 *
 */
static int _regTypeCmp(const void *s1, const void *s2) {
  Segment *f1 = (Segment *)s1;
  Segment *f2 = (Segment *)s2;
  
  /* check for broken qsort */
  if( !*f1 || !*f2 ){
    _regerror("qsort is broken -- contact saord@cfa.harvard.edu");
    /* will not happen */
    return 0;
  }
  /* make sure null string is at the end */
  if( !(*f1)->s && !(*f2)->s ){
    return 0;
  } else if( !(*f1)->s ){
    return 1;
  } else if( !(*f2)->s ){
    return -1;
  }
  /* paint: reverse order and make excludes local */
  if( xreg->paint ){
    /* if both are regions, reverse order */
    if( ((*f1)->type & TOK_REG) && ((*f2)->type & TOK_REG) ){
      if( (*f1)->n < (*f2)->n ){
	return 1;
      } else if( (*f1)->n > (*f2)->n ){ 
	return -1;
      } else {
	return 0;
      }
    } else {
      /* keep same order */
      if( (*f1)->n < (*f2)->n ){
	return -1;
      } else if( (*f1)->n > (*f2)->n ){
	return 1;
      } else {
	return 0;
      }
    }
  } else {
    /* if both or neither are eregions, keep them in the same order */
    if( ((*f1)->type & TOK_EREG) == ((*f2)->type & TOK_EREG) ){
      if( (*f1)->n < (*f2)->n ){
	return -1;
      } else if( (*f1)->n > (*f2)->n ){
	return 1;
      } else {
	return 0;
      }
    } else {
       /* exactly one is an eregion -- and we want eregions last */
       if( (*f1)->type & TOK_EREG ){
	 return 1;
       } else if( (*f2)->type & TOK_EREG ){
	 return -1;
       } else {
	 /* should not happen */
	 return 0;
       }
    }
  }
}

static int _regArgCheck(char *name, int args) {
  int i;

  for(i=0; rnames[i]; i++){
    if( !strcmp(name, rnames[i]) ){
      if( args == rargs[i] ){
        return(0);
      } else {
        /* less than required min */
        if( args < rargs[i] ){
          return(i+1);
	} else {
	  /* more than min args -- could be varargs */
          // check args allowed in varargs
          switch(vargs[i]){
          case 'e':
            if( (args%2) == 0 ){
              return(-1);
	    } else {
              return(i+1);
	    }
          case 'o':
            if( (args%2) == 1 ){
              return(-1);
	    } else {
              return(i+1);
	    }
	  case 'y':
	    return(-1);
          default:
            return(i+1);
          }
        }
      }
    }
  }
  return(0);
}

static void _regSetWCS(char *name) {
  char tbuf[SZ_LINE];
  int ip = 0;
  newdtable(",=;");
  /* get new global wcssys */
  if( word(name, tbuf, &ip) ){
    xfree(wcssys);
    wcssys = xstrdup(tbuf);
    culc(wcssys);
  }
  /* restore old delim table */
  freedtable();
}

static char *_regCB(char *name, void *client_data) {
  Segment f=(Segment)client_data;
  int ip = 0;
  int nr = 0;
  int ns = 0;
  char *s=NULL;
  char tbuf[SZ_LINE];

  if( !strcmp(name, "RS") ){
    /* exclude regions get no region value first time around */
    if( (macropass == 0) && (f->type & TOK_EREG) ){
      return NULL;
    }
    /* get next region value and remove from string */
    if( word(f->regions, tbuf, &ip) ){
      nr = atoi(tbuf);
      s = xstrdup(&(f->regions[ip]));
      xfree(f->regions);
      f->regions = s;    
    } else {
      _regerror("internal error: no region string specified");
      return(NULL);
    }
    if( !inexpr || (f->type & (TOK_ACCEL|TOK_VARARGS)) ){
      if( f->regval ){
        snprintf(macrobuf, SZ_LINE, "%d,", f->regval);
      } else {
        snprintf(macrobuf, SZ_LINE, "%d,", nregion+1);
      }
      nregion += nr;
      f->nregion += nr;
      inexpr = 1;
    } else {
      if( f->regval ){
        snprintf(macrobuf, SZ_LINE, "%d,", f->regval);
      } else {
        snprintf(macrobuf, SZ_LINE, "%d,", nregion);
      }
    }
    /* get next shape value and remove from string */
    ip = 0;
    if( word(f->shapes, tbuf, &ip) ){
      ns = atoi(tbuf);
      s = xstrdup(&(f->shapes[ip]));
      xfree(f->shapes);
      f->shapes = s;    
    } else {
      _regerror("internal error: no shape string specified");
      return(NULL);
    }
    /* concat shape value */
    nshape++;
    snprintf(tbuf, SZ_LINE, "%d", nshape);
    strcat(macrobuf, tbuf);
    /* set new shape value */
    nshape += (ns-1);

    /* return macro value */
    return(macrobuf);
  } else {
    return NULL;
  }
}

/*
 *
 * _regNeedOp -- determine if we add a connecting operator for this segment
 *
 */
static int _regNeedOp(int i, char *s) {
  int c;
  /* check for operator in current string */
  if( s && *s ){
    c = *s;
    switch(c){
    case '&':
    case '|':
    case '=':
    case '!':
    case '<':
    case '>':
      return 0;
    default:
      break;
    }
  }
  /* not in this string check for operator in previous */
  for(i=i-1; i>=0; i--){
    if( !seg[i] || !seg[i]->s ){
      continue;
    }
    if( strcmp(seg[i]->s, "(") && strcmp(seg[i]->s, ")") ){
      break;
    }
  }
  if( i >= 0 ){
    c = seg[i]->s[strlen(seg[i]->s)-1];
    switch(c){
    case '&':
    case '|':
    case '=':
    case '!':
    case '<':
    case '>':
      return 0;
    default:
      return 1;
    }
  } else {
    return 0;
  }
}

/*
 *
 * _regMake -- generate filter from region segments
 *
 */
static void _regMake(void) {
  int i, j, k, l;
  int x, xtot;
  int type = 0;
  int ltype = 0;
  int opar = 0;
  char *s;

  /* debugging */
  if( xreg->debug ){
    for(i=0; i<nseg; i++){
      if( seg[i]->s ){
        fprintf(stderr, "unsorted #%d %s\n", seg[i]->n, seg[i]->s);
      } else {
        fprintf(stderr, "unsorted #%d %d is empty\n", i, seg[i]->n);
      }
    }
  }
  /* look for bad region expressions and determine how many regions we have */
  for(i=0, j=0, k=0, l=0; i<nseg; i++){
    if( !seg[i] || !seg[i]->s ){
      continue;
    }
    type = seg[i]->type;
    /* look for bad combinations */
    if( (type & TOK_NREG) && !(type & TOK_IREG) ){
      _regerror("!region must be combined with another region (e.g., field()&&!region)");
    }
    if( (type & TOK_EREG) && (type & TOK_IREG) ){
      _regerror("-region must not be combined with another region (use !region instead)");
    }
    if( (type & TOK_EREG) && (type & TOK_RTINE) ){
      _regerror("-region must not be combined with routines (use !region instead)");
    }
    /* count region types */
    if( seg[i]->type & TOK_IREG ){ j++; }
    if( seg[i]->type & TOK_NREG ){ k++; }
    if( seg[i]->type & TOK_EREG ){ l++; }
  }
  /* if we have only exclude regions, add field */
  if( !j && !k && l ){
    strcpy(yytext, "field");
    _regStart();
    _regEnd();
  }
  /* reset region and shape counts */
  nregion = 0;
  nshape = 0;
  /* expand macro before reordering, to maintain region numbers in sequence */
  for(i=0; i<nseg; i++){
    if( seg[i] && seg[i]->s ){
      if( _regNeedOp(i, NULL) ){ inexpr = 0; }
      macropass = 0;
      s = ExpandMacro(seg[i]->s, NULL, NULL, 0, _regCB, seg[i]);
      xfree(seg[i]->s);
      seg[i]->s = s;
    }
  }
  /* we want descending order so that eregions are last in the expression */
  qsort(seg, nseg, sizeof(Segment), _regTypeCmp);
  /* more debugging */
  if( xreg->debug ){
    for(i=0; i<nseg; i++){
      if( seg[i]->s ){
        fprintf(stderr, "sorted #%d %s\n", seg[i]->n, seg[i]->s);
      } else {
        fprintf(stderr, "sorted #%d %d is empty\n", i, seg[i]->n);
      }
    }
  }
  /* add open paren and mark that we did it */
  __regCat("(", &filter, &filterlen);
  opar = 1;
  /* run through segments, inserting proper boolean connectors */
  ltype = 0;
  for(i=0; i<nseg; ){
    if( seg[i] && seg[i]->s ){
      /* no type probably means its a paren, so just add it and jump out */
      if( !seg[i]->type ){
        __regCat(seg[i]->s, &filter, &filterlen);
	/* remove empty parens, if we just make them */
	x = strlen(filter);
	if( (filter[x-2] == '(') && (filter[x-1] == ')') ){
	  filter[x-2] = '\0';
	}
	i++;
	continue;
      }
      type = seg[i]->type;
      if( _regNeedOp(i, NULL) ){ inexpr = 0; }
      macropass=1;
      s = ExpandMacro(seg[i]->s, NULL, NULL, 0, _regCB, seg[i]);
      if( _regNeedOp(i, s) ){
        /* remove trailing open parens from prev. segment (add back later) */
	for(xtot=0, x=strlen(filter)-1; x>=0; x--){
	  if( filter[x] != '(' ){
            break;
          };
	  filter[x] = '\0';
	  xtot++;
        }
        if( (ltype & TOK_IREG) && (type & TOK_IREG) ){
          __regCat("||", &filter, &filterlen);
        } else if( (ltype & TOK_IREG) && (type & TOK_EREG) ){
          if( opar > 0 ){
            __regCat(")", &filter, &filterlen);
            opar--;
          }
	  if( xreg->paint ){ 
            if( opar > 0 ){
              __regCat(")", &filter, &filterlen);
              opar --;
            }
            __regCat("||", &filter, &filterlen);
	  } else {
            __regCat("&&", &filter, &filterlen);
	  }
          __regCat("(", &filter, &filterlen);
          opar++;
        } else if( (ltype & TOK_EREG) && (type & TOK_EREG) ){
          __regCat("&&", &filter, &filterlen);
        } else if( (ltype & TOK_EREG) && (type & TOK_IREG) ){
          if( xreg->paint && (opar > 0) ){
            __regCat(")", &filter, &filterlen);
            opar --;
          }
          __regCat("&&", &filter, &filterlen);
          __regCat("(", &filter, &filterlen);
          opar ++;
          if( xreg->paint ){
            __regCat("(", &filter, &filterlen);
            opar++;
          }
        } else {
          /* otherwise implied operator is AND */
          __regCat("&&", &filter, &filterlen);
        }
	/* add back parens */
	for(x=0; x<xtot; x++){
	  __regCat("(", &filter, &filterlen);
        }
      } else {
        if( xreg->paint && (i == 0) ){
	  __regCat("((", &filter, &filterlen);
	  opar +=2;
	}
      }
      __regCat(s, &filter, &filterlen);
      xfree(s);
      if( seg[i]->radang ){
        if( *seg[i]->radang == ',' ){
          __regCat(seg[i]->radang+1, &radang, &radanglen);
        } else {
          __regCat(seg[i]->radang, &radang, &radanglen);
        }
      } else {
        __regCat("NA,NA,NA,NA\n", &radang, &radanglen);
      }
      i++;
      for(; (i<nseg) && (seg[i]->type==type); ){
        if( _regNeedOp(i, NULL) ){ inexpr = 0; }
        s = ExpandMacro(seg[i]->s, NULL, NULL, 0, _regCB, seg[i]);
	if( _regNeedOp(i, s) ){
	  for(xtot=0, x=strlen(filter)-1; x>=0; x--){
	    if( filter[x] != '(' ){ break; }
	    filter[x] = '\0';
	    xtot++;
          }
          /* if lhs and rhs are regions, implied operator is OR */
	  if( type & TOK_IREG ){
	    __regCat("||", &filter, &filterlen);
	  } else {
	    /* otherwise implied operator is AND */
	    __regCat("&&", &filter, &filterlen);
	  }
	  for(x=0; x<xtot; x++){
	    __regCat("(", &filter, &filterlen);
          }
        }
        __regCat(s, &filter, &filterlen);
	xfree(s);
        if( seg[i]->radang ){
          if( *seg[i]->radang == ',' ){
            __regCat(seg[i]->radang+1, &radang, &radanglen);
          } else {
            __regCat(seg[i]->radang, &radang, &radanglen);
	  }
        } else {
          __regCat("NA,NA,NA,NA\n", &radang, &radanglen);
        }
	i++;
      }
    } else {
      /* ignore invalid arguments */
      i++;
    }
    /* save last type */
    ltype = type;
  }
  /* if last segment was an exclude and we are in paint mode, ignore it */
  if( xreg->paint && (ltype & TOK_EREG) ){
    __regCat("&&0", &filter, &filterlen);
  }
  /* close off open parens (lots could be needed in paint mode) */
  while( opar-- ){
    __regCat(")", &filter, &filterlen);
  }
  /* check for even number of parens */
  l = strlen(filter);
  for(i=0, j=0; i<l; i++){
    if( filter[i] == '(' ){ j++; }
    if( filter[i] == ')' ){ j--; }
  }
  if( j != 0 ){
    if( xreg->debug ){
      fprintf(stderr, "paren error %d: %s\n", j, filter);
    }
    _regerror("unbalanced parentheses");
  }
  /* final check for empty filter */
  if( filter && !strcmp(filter, "()") ){
    xfree(filter);
    filter = NULL;
  }
  /* debugging */
  if( xreg->debug ){
    fprintf(stderr, "filter: %s\n", filter);
  }
}

/*
 *
 * _regMake -- generate filter from region segments
 *
 */
static int _regRegions(int type) {
  int i, j;
  for(i=0, j=0; i<nseg; i++){
    if( seg[i]->type & type ){
      j += seg[i]->nregion;
    }
  }
  return(j);
}

/*
 *
 * _regEnd -- cleanup from parsing regions
 *
 */
static void _regParseEnd(void) {
  int i;
  /* reset state variables */
  brlev = 0;
  include_stack_ptr = 0;
  nroutine = 0;
  nshape = 0;
  nparen = 0;
  parse_error = 0;
  filterlen = 0;
  radanglen = 0;
  regid = 0;
  regtype = 0;
  inexpr = 0;
  *regname = '\0';
  *saveang = 0;
  /* free up previous filter */
  xfree(filter);
  filter = NULL;
  /* free up previous radang string */
  xfree(radang);
  radang = NULL;
  /* free up wcs */
  wcsfree(wcs);
  wcs = NULL;   
  xfree(wcssys);
  wcssys = NULL;
  /* no segments */
  if( seg ){
    for(i=0; i<nseg; i++){
      if( seg[i] ){
        xfree(seg[i]->s);
        xfree(seg[i]->regions);
        xfree(seg[i]->shapes);
        xfree(seg[i]->radang);
        xfree(seg[i]);
      }
    }
    xfree(seg);
    seg = NULL;
  }
  maxseg = 0;
  nseg = 0;
  cdelt1 = 0.0;
  cdelt2 = 0.0;
  crot   = 0.0;
}

/*
 *
 * _regGetWCS -- return current WCS coord system to use for wcs translation
 *
 */
static char *_regGetWCS(void) {
  if( wcssys && strcmp(wcssys, "image") && strcmp(wcssys, "physical")){
    return wcssys;
  } else {
    return NULL;
  }
}

/*
 *
 * _regInitWCS -- initialize WCS from supplied FITS cards
 *
 */
static void _regInitWCS() {
  /* default coord sys */
  wcssys = xstrdup(DEFAULT_WCS);
  /* make up logical matrix (doesn't need any cards) */
  lcx_init(xreg->cards, lcxm);
  /* make sure we have something to process (from global regions struct) */
  if( !xreg || !xreg->cards || !*xreg->cards ){
    return;
  }
  /* free up old wcs */
  wcsfree(wcs);
  /* init new wcs, if possible */
  hlength(xreg->cards, 256000);
  wcs = wcsinit(xreg->cards);
  /* set some extra wcs parameters */
  if( wcs && iswcs(wcs) ){
    if( !wcs->coorflip ){
      cdelt1 = wcs->cdelt[0];
      cdelt2 = wcs->cdelt[1];
    } else {
      cdelt1 = wcs->cdelt[1];
      cdelt2 = wcs->cdelt[0];
    }
    if( wcs->imflip ) {
	crot = -wcs->rot;
    } else {
	crot =  wcs->rot;
    }

  } else {
    cdelt1 = 0.0;
    cdelt2 = 0.0;
    crot   = 0.0;
  }
}

/*
 *
 * _regLCX -- determine if we have a linear-type WCS (i.e. image coords)
 *
 */
static int _regLCX(void) {
  if( !wcssys || !*wcssys ){
    return 0;
  } else if( !strcasecmp(wcssys, "image") ){
    return LCX_IMAGE;
  } else if( !strcasecmp(wcssys, "physical")  ){
    return LCX_PHYS;
  } else {
    return 0;
  }
}

static void _regNum(int lcx) {
  char tbuf[SZ_LINE];

  /* process binary number, if necessary */
  bin2num(tbuf, yytext, SZ_LINE);
  /* are we in a linear coordinate system? */
  if( lcx ){
      /* positional arguments are image coords */
      if( POSARG ){
	/* save x value globally for processing with y next time */
	if( XARG ){
	  xpos = SAOstrtod(tbuf,NULL);
	} else {
	  ypos = SAOstrtod(tbuf,NULL);
	  switch(lcx){
	  case LCX_IMAGE:
	    xpix = xpos;
            ypix = ypos;
            break;
	  case LCX_PHYS:
	    lcx_phys2img(lcxm, xpos, ypos, &xpix, &ypix);
	    break;
	  }
	  snprintf(sbuf, SZ_LINE, DFMT2, xpix, ypix);
	  _regCat(sbuf);
	}
      } else if( ANGARG ){
      /* angle arguments are just passed along, with updated crot */
	dval = strtod(tbuf,NULL);
        if( USEWCS ){
	  dval += crot;
	}
	if( IMFLIP ){
	  dval = -dval;
	}
	snprintf(sbuf, SZ_LINE, DFMT1, dval);
	_regCat(sbuf);
      } else if( NUMARG ){
	/* panda numeric args are just passed along */
        dval = strtod(tbuf, NULL);
        snprintf(sbuf, SZ_LINE, DFMT1, dval);
        _regCat(sbuf);
      } else {
	/* convert size args, which may or may not be in pairs */
        if( SAVEANG ){
	  dval = strtod(tbuf,NULL);
          if( USEWCS ){
	    dval += crot;
	  }
	  if( IMFLIP ){
	    dval = -dval;
	  }
	  snprintf(saveang, SZ_LINE, DFMT1, dval);
        }
	/* arg 1 is associated with ra, arg2 with dec */
	if( XARG ){
	  xpos = ABS(strtod(tbuf,NULL));
	  ypos = 0.0;
	  switch(lcx){
	  case LCX_IMAGE:
            xpix = xpos;
            break;
	  case LCX_PHYS:
	    lcx_phys2img_size(lcxm, xpos, ypos, &xpix, &ypix);
	    break;
	  }
	  snprintf(sbuf, SZ_LINE, DFMT1, xpix);
          _regArgSave(xpix);
	} else {
	  xpos = 0.0;
	  ypos = ABS(strtod(tbuf,NULL));
	  switch(lcx){
	  case LCX_IMAGE:
            ypix = ypos;
            break;
	  case LCX_PHYS:
	    lcx_phys2img_size(lcxm, xpos, ypos, &xpix, &ypix);
	    break;
	  }
	  snprintf(sbuf, SZ_LINE, DFMT1, ypix);
          _regArgSave(ypix);
	}
	_regCat(sbuf);
    }
  } else if( ANGARG ){
    /* not image coords: angle args are just passed along with updated crot */
    dval = strtod(tbuf,NULL);
    if( USEWCS ){
      dval += crot;
    }
    if( IMFLIP ){
      dval = -dval;
    }
    snprintf(sbuf, SZ_LINE, DFMT1, dval);
    _regCat(sbuf);
  } else if( NUMARG ){
    /* panda numeric args are just passed along */
    dval = strtod(tbuf,NULL);
    snprintf(sbuf, SZ_LINE, DFMT1, dval);
    _regCat(sbuf);
  } else {
    /* everything else is assumed to be in units of "degrees"
     in the default system. add 'd' and put back for reparsing */
    int i;
    char *yycopy = xstrdup(tbuf);
    unput('d');
    for(i=yyleng-1; i>=0; --i)
      unput(yycopy[i]);
    xfree(yycopy);
    narg--;
  }
}

/* set up to parse this string */
void _regScanString(char *s){
  int i;
  char *t=NULL;
  /* make sure there is an explicit delim at end so we are done before
     lex finds EOF -- otherwise panda shapes don't get processed
     also, we extend the buffer because flex has problems in filt_init_buffer 
     if its smaller than sizeof(FILE *) */
  i = strlen(s);
  t = xcalloc(strlen(s)+SZ_LINE, sizeof(char));
  nowhite(s, t);
  if( (t[i-1] != ';') && (t[i-1] != '\n') ){
    strcat(t, "\n");
  }
  yy_scan_string(t);
  xfree(t);
}

/* yyerror -- renamed to _regerror because flex -P does not change this */
int _regerror(char *msg) {
  if( *yytext ){
    xerror(stderr, "%s while parsing: %s\n", msg ? msg : "", yytext);
  } else {
    xerror(stderr, "%s while parsing regions\n", msg ? msg : "");
  }
  YY_FLUSH_BUFFER;
  parse_error = 1;  
  yyterminate();
}

/*
 *
 * Semi-public Routines 
 *
 *
 */

/* main entry point: used by regions.c */
int regparse(Regions reg){
  /* save reg struct */
  xreg = reg;
  /* make sure we are cleaned up */
  _regParseEnd();
  /* initialize wcs for this image */
  _regInitWCS();
  /* reset parser */
  BEGIN INITIAL;
  laststart = INITIAL;
  /* scan new string */
  _regScanString(xreg->regstr);
  /* parse regions */
  regionslex();
  /* check for error */
  if( parse_error ){
    return -1;
  }
  /* generate the region string */
  _regMake();
  /* fill in information used internally */
  reg->filter = xstrdup(filter);
  reg->radang = xstrdup(radang);
  reg->nroutine = nroutine;
  reg->nshape = nshape;
  reg->nreg = _regRegions(TOK_IREG);
  /* clean up parser */
  _regParseEnd();
  regionslex_destroy();
  /* return number of mask segments */
  return reg->nshape;
}

int yywrap(void) {
  return 1;
}
