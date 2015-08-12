#!/bin/bash
# set -x

# look for required input file argument
for var in "$@"; do
    case $var in
	-*) continue;;
         *) IFILE="$var"; break;;
    esac
done
if [ x"$IFILE" = x  ]; then
    echo "usage: $0 [sw] ifile [sregion [bregion]]"
    exit 1
fi

# max size image we will generate when reading a binary table
MAXDIM=8192
# block factor to testing image dims
TBL=8

# for event tables: if binning syntax is not used, calculate a block factor
echo "$IFILE" | tr '[A-Z]' '[a-z]' |egrep '\[(events|stdevt)\]' 1>/dev/null 2>&1
if [ $? = 0 ]; then
  # it's an event file. is binning specified already?
  echo "$IFILE" | tr '[A-Z]' '[a-z]' | egrep '\]\[bin ' 1>/dev/null 2>&1
  if [ $? != 0 ]; then
    # no binning ... make sure we have the appropriate support tools
    hash fkeyprint 1>/dev/null 2>&1
    if [ $? = 0 ]; then
        XEQ=ftools
    else
        hash funimage 1>/dev/null 2>&1
        if [ $? = 0 ]; then
            XEQ=funtools
        else
            XEQ="NONE"
        fi
    fi
    # get image dimensions, if possible
    case $XEQ in
    ftools)
        # first try to bin an event file
        NAXIS1=`fkeyprint infile=$IFILE'[bin '$TBL']' keynam=NAXIS1 2>/dev/null | egrep = | awk -F= '{print $2}' | awk -F/ '{print $1}' | sed 's/^  *//g;s/  *$//g'`
        if [ x$NAXIS1 != x ]; then
            NAXIS2=`fkeyprint infile=$IFILE'[bin '$TBL']' keynam=NAXIS2 2>/dev/null | egrep = | awk -F= '{print $2}' | awk -F/ '{print $1}' | sed 's/^  *//g;s/  *$//g'`
        fi
	;;
    funtools)
	XIFILE=`echo $IFILE | sed 's#\[.*##'`
        DIMS=`funimage $XIFILE'[*,*,'$TBL']' stdout bitpix=8 2>/dev/null | dd bs=80 count=10 2>/dev/null | sed -e 's/.\{80\}/&@/g' | tr '@' '\012' | egrep NAXIS'[1,2]' | sed 's# /.*##g;s# ##g'`
        if [ x"$DIMS" != x ]; then
            eval $DIMS
        fi
        ;;
    esac
    # calculate block factor, if possible
    if [ x"$NAXIS1" != x -a x"$NAXIS2" != x ]; then
        if [ $NAXIS1 -ge $NAXIS2 ]; then
            DIM=$NAXIS1
        else
            DIM=$NAXIS2
        fi
        # calculate a block factor to make the image dimensions reasonable
        TBL=`echo "(($DIM * $TBL) + ($MAXDIM - 1)) / $MAXDIM" | bc`
        if [ $TBL -gt 1 ]; then
            BL="-b $TBL"
        fi
    fi
  fi
fi

# run regcnts, possibly using a calculated block factor
regcnts $BL "$@"
