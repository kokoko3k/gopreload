#!/bin/bash
ARCH=`uname -m`
gcc mapandlock.c -o mapandlock.$ARCH -fno-stack-protector
strip -s mapandlock.$ARCH
