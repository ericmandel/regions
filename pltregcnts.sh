#!/bin/bash

${AWK:-awk} '
  BEGIN{
    mode="header"
  }
  mode=="header"{
    if( $1 == "#" ){
      if( $2 == "data_file:" ){
        file = "("$3")"
      } else if( $2 == "radii:" ){
        xlabel = "("$3")"
      } else if( $2 == "surf_bri:" ){
        ylabel = "("$3")"
      }
    } else if( $1 == "----" ){
      if( NF == 12 ){
        printf "regcnts%s avg_radius%s surf_bri%s 3\n", file, xlabel, ylabel
        mode = "data"
        next
      } else {
         print "ERROR: use regcnts -rG with annulus or panda regions only"
         exit 1
      }
    }
  }
  mode=="data"{
    if( NF == 12 ){
      if( $10 != "NA" ){
        print ($9+$10)/2, $7, $8
      } else{
         print "ERROR: annulus or panda regions only"
	 exit 1
      }
    } else{
      mode = "done"
    }
  }
'
exit $?
