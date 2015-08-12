#!/bin/bash

if [ $# -lt 1 ]; then
  echo $0 ifile1 ...
  exit 1
fi

# max size image we will generate from a table
MAXDIM=8192
# starting block factor
BL=8

hash fkeyprint 1>/dev/null 2>&1
if [ $? = 0 ]; then
  XEQ=ftools
else
  hash funimage 1>/dev/null 2>&1
  if [ $? = 0 ]; then
    XEQ=funtools
  else
    error "requires either ftools (fkeyprint) or funtools (fumimage)"
  fi
fi

while [ x"$1" != x ]; do
  # new file
  IFILE="$1"
  shift

  # get image dimensions
  case $XEQ in
      ftools)
	NAXIS1=`fkeyprint infile=$IFILE'[events][bin '$BL']' keynam=NAXIS1 | egrep = | awk -F= '{print $2}' | awk -F/ '{print $1}' | sed 's/^  *//g;s/  *$//g'`
	NAXIS2=`fkeyprint infile=$IFILE'[events][bin '$BL']' keynam=NAXIS2 | egrep = | awk -F= '{print $2}' | awk -F/ '{print $1}' | sed 's/^  *//g;s/  *$//g'`
	;;
      funtools)
	DIMS=`funimage $IFILE'[*,*,'$BL']' stdout bitpix=8 2>/dev/null | dd bs=80 count=10 2>/dev/null | sed -e 's/.\{80\}/&@/g' | tr '@' '\012' | egrep NAXIS'[1,2]' | sed 's# /.*##g;s# ##g'`
	eval $DIMS
	;;
  esac

  # get a reasonable block factor to make the image dimensions "finite"
  BL1=`echo "$NAXIS1 * $BL / $MAXDIM" | bc`
  BL2=`echo "$NAXIS2 * $BL / $MAXDIM" | bc`
  if [ $BL1 -gt $BL2 ]; then
    BL="$BL1"
  else
    BL="$BL2"
  fi

  # run regcnts using this block factor
  ./regcnts -b $BL $IFILE $*
done

