#!/bin/bash

# * this file is part of GOPreload
# * Programmed by Antonio Orefice
# * Released under the GNU General Public License (GPL) version 2.

source /etc/gopreload.conf

export IFS=$'\n'
ARCH=`uname -m`


function CheckList
{
	for list in $(ls $INSTALLDIR/enabled/*.openfiles-*MB.txt* ); do
		BaseListName=`basename $list`
		for item in $(cat $list); do
			ls $item 1>/dev/null 2>> /tmp/misses.$BaseListName
		done
		MISSED=$(cat /tmp/misses.$BaseListName|wc -l)
		TOTAL=$(cat $list|wc -l)
		echo -e '\t'$MISSED'/'$TOTAL files could not be preloaded because they were not found \($BaseListName\)
	done
}



MAXKB=`expr $MAXMB \* 1024`

rm /tmp/misses.*.openfiles-*MB.txt* /tmp/enabled.txt /tmp/enabled.prec.txt /tmp/preloadlist /tmp/preloadlist_maxmb.txt /tmp/listpreload.txt &>/dev/null

echo "0/4 - Checking list consistency..."
CheckList
echo


echo "1/4 - Computing preload list..."

while [ 1 ];
do

  #Do the Main loop only if enabled list has changed:
  cp /tmp/enabled.txt /tmp/enabled.prec.txt &> /dev/null
  ls -la $INSTALLDIR/enabled/ > /tmp/enabled.txt
  if ! diff /tmp/enabled.prec.txt /tmp/enabled.txt &>/dev/null;
  then
	cat $INSTALLDIR/enabled/*.openfiles-*MB.txt*|sort|uniq >/tmp/preloadlist.txt

	TOTSIZE=0
	SIZE=0
	COUNT=0

	echo "/tmp/preloadlist.txt" > /tmp/listpreload.txt
	LISTAFILE=`cat /tmp/preloadlist.txt`

	if [ "$1" = "debug" ]; then
		TIME=time
		echo "2/4 - Computing total MB..."
		for file in $LISTAFILE;
		do
			 SIZE=0`ls -1sd /$file 2>/dev/null| cut -d "/" -f 1|grep [0-9]|cut -d " " -f 1`
			 TOTSIZE=`expr $TOTSIZE + $SIZE`
			 COUNT=`expr $COUNT + 1`
		done
		killall mapandlock.$ARCH >/dev/null 2>/dev/null
		$INSTALLDIR/bin/mapandlock.$ARCH /tmp/listpreload.txt &
		echo "3/4 - Starting preload cycle at `date` for `expr  $TOTSIZE / 1024` MB in $COUNT files used by:"
		ls $INSTALLDIR/enabled/
		echo
		echo "4/4 - Sleeping $LONG_DELAY sec."
		sleep $LONG_DELAY
	else
		echo "2/4 - Computing total MB... skipped"
		killall -HUP mapandlock.$ARCH >/dev/null 2>/dev/null
		$INSTALLDIR/bin/mapandlock.$ARCH /tmp/listpreload.txt 2>/dev/null >/dev/null &
		echo "3/4 - Starting preload cycle at `date`"
		echo "4/4 - Sleeping $LONG_DELAY sec."
		sleep $LONG_DELAY
	fi
  else
	#echo "Nothing changed, Sleeping $LONG_DELAY sec."
	sleep $LONG_DELAY
	killall mapandlock.$ARCH >/dev/null 2>/dev/null
	$INSTALLDIR/bin/mapandlock.$ARCH /tmp/listpreload.txt &
  fi

done
