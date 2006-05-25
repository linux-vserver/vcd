#!/bin/sh
#
uname -r
echo
echo '#' cow -h
cow -h
echo
echo '#' cow -i -u -n 3
cow -i -u -n 3
echo
echo '#' cow -i -n 3
cow -i -n 3
echo
echo '#' cow -u -n 3
cow -u -n 3

