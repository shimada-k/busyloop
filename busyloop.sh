#!/bin/bash

#./turbofreq --csv -nahalem -inteldoc &

sleep 5

./busyloop -c 0 -N 1 &
sleep 10
kill -s SIGTERM `cat ./busyloop.pid`

./busyloop -c 1 -N 1 &
sleep 10
kill -s SIGTERM `cat ./busyloop.pid`

./busyloop -c 2 -N 1 &
sleep 10
kill -s SIGTERM `cat ./busyloop.pid`

./busyloop -c 3 -N 1 &
sleep 10
kill -s SIGTERM `cat ./busyloop.pid`

