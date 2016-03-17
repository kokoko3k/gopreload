#!/bin/bash

# * this file is part of GOPreload
# * by Antonio Orefice
# * Released under the GNU General Public License (GPL) version 2.

source /etc/gopreload.conf

EXCLUDE_PATTERN_FILE=$INSTALLDIR/prepare_exclude.txt


if [ ! -d "$INSTALLDIR" ] ; then
  echo ; echo "Couldn't find $INSTALLDIR, exiting now." ; echo
  exit 1
fi

mkdir $INSTALLDIR/enabled 2>/dev/null
mkdir $INSTALLDIR/disabled 2>/dev/null

if [ ! -d "$INSTALLDIR/enabled" ] ; then
  echo ; echo "$INSTALLDIR/enabled missing!" 
  echo "Please create it and give your user permissions to write into." ; echo
  exit 1
fi

if [ ! -w "$INSTALLDIR/enabled" ] ; then
  echo ; echo "You have not write permissions to $INSTALLDIR/enabled,"
  echo "because gopreload defaults that way for regular users."
  echo "Please, manually change the permissions on $INSTALLDIR/enabled"
  echo "to make it writable by you." ; echo
  exit 1
fi

if [ ! -d "$INSTALLDIR/disabled" ] ; then
  echo ; echo "$INSTALLDIR/disabled missing!" 
  echo "Please create it and give your user permissions to write into." ; echo
  exit 1
fi

if [ ! -w "$INSTALLDIR/disabled" ] ; then
  echo ; echo "You have not write permissions to $INSTALLDIR/disabled,"
  echo "because gopreload defaults that way for regular users."
  echo "Please, manually change the permissions on $INSTALLDIR/disabled"
  echo "to make it writable by you." ; echo
  exit 1
fi

MAXKB=`expr $MAXMB \* 1024`
echo "Detecting opened files..."

rm /tmp/openlibs2.$EUID.txt /tmp/_LiNkS_.$EUID.txt 2>/dev/null

strace -f -F -e trace=open,openat,access $* 2>/tmp/out.$EUID.gopreload &
PIDOFSTRACE=$!

echo "Press [ENTER] when you've done"
read
kill $PIDOFSTRACE 2>/dev/null >/dev/null

echo "Detection completed, parsing file list..."
#grep '("/' /tmp/out.$EUID.gopreload | grep -vi -f $EXCLUDE_PATTERN_FILE | grep -vi $EXCLUDE_LIST | cut -f 2 -d '"' | sort -u >/tmp/openlibs2.$EUID.txt
grep \" /tmp/out.$EUID.gopreload |cut -d '"' -f 2 | grep -vi -f $EXCLUDE_PATTERN_FILE | grep -vi $EXCLUDE_LIST | sort -u >/tmp/openlibs2.$EUID.txt

#Remove symbolic links...
echo "Done with parsing, backtracing symbolic links..."
for file in $(cat /tmp/openlibs2.$EUID.txt); do
	readlink -m $file >>/tmp/_LiNkS_.$EUID.txt
done
sort -u /tmp/_LiNkS_.$EUID.txt > /tmp/openlibs2.$EUID.txt


TOTAL=`cat /tmp/openlibs2.$EUID.txt | wc -l`

BASEFILENAME=$(basename $1)

rm $INSTALLDIR/enabled/$BASEFILENAME.$EUID.openfiles-*MB.txt 2>/dev/null
rm $INSTALLDIR/disabled/$BASEFILENAME.$EUID.openfiles.txt 2>/dev/null
TOTSIZE=0
COUNTER=0
OUTFILE=$INSTALLDIR/enabled/$BASEFILENAME.$EUID.openfiles.txt
echo "cmd=$@" > $OUTFILE
for i in $(cat /tmp/openlibs2.$EUID.txt); do
        	COUNTER=`expr $COUNTER + 1`
                echo -ne "\r$COUNTER on $TOTAL done, will use `expr $TOTSIZE / 1024`MB to preload them"
		SIZE=0`ls -1sd /$i 2>/dev/null| cut -d "/" -f 1|grep [0-9]|cut -d " " -f 1`
		if [ $SIZE -lt $MAXKB ]; then
        	        file "$i"|grep -v directory |cut -f 1 -d ":" >> $OUTFILE
			TOTSIZE=`expr $TOTSIZE + $SIZE`
		else
			echo
			echo "Excluding $i because it's too large: $SIZE KB > $MAXKB KB based on user prefs."
		fi


done
rm /tmp/openlibs2.$EUID.txt /tmp/out.$EUID.gopreload /tmp/_LiNkS_.$EUID.txt 2>/dev/null

echo " "
mv $INSTALLDIR/enabled/$BASEFILENAME.$EUID.openfiles.txt $INSTALLDIR/enabled/$BASEFILENAME.$EUID.openfiles-`expr $TOTSIZE / 1024`MB.txt
echo "$INSTALLDIR/enabled/$BASEFILENAME.$EUID.openfiles-`expr $TOTSIZE / 1024`MB.txt compiled."

