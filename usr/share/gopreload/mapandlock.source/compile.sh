#!/bin/bash
ARCH=`uname -m`
#gcc mapandlock.c -o mapandlock.$ARCH -fno-stack-protector
gcc mapandlock.c -o mapandlock.$ARCH
strip -s mapandlock.$ARCH
