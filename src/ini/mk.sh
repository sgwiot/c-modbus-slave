#!/bin/bash

#Start
x=6700
#End
End=6739
while [ $x -le "${End}" ] ; do
	cp tcp_6666.ini tcp_${x}.ini
	INIFILE=tcp_${x}.ini;  PORT=`basename $INIFILE | awk -F '_' '{print $2}' | awk -F '.' '{print $1 }' ` ; sed "s/6666/$PORT/"  -i  $INIFILE
	echo "$x times"
	x=$(( $x + 1 ))
done

