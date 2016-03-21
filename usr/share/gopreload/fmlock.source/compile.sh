#!/bin/bash
gcc ./fmlock.c -o fmlock.gopreload
strip -s fmlock.gopreload
