#!/bin/bash

# * this file is part of GOPreload
# * by Antonio Orefice
# * Released under the GNU General Public License (GPL) version 2.

IFS=$'\n'

echo 
echo "Please stand-by (don't open any window!)"
echo "Go make a coffee."
echo

source /etc/gopreload.conf
FILELISTS="$INSTALLDIR/enabled/*.$EUID.openfiles-*.txt"

for PROG in $(head -q -n 1 $FILELISTS|cut -d "=" -f 2-) ; do
    sh -c "$INSTALLDIR/bin/Prepare_unattended.sh $PROG"
done
