#!/bin/sh
set -e
insmod $*
MAJOR=$(grep " blinker$" /proc/devices| sed -e "s/ blinker//")
mknod /dev/blinker c $MAJOR 0 || rmmod $1
