#!/bin/bash

die(){
	kill "$general"
	wait >/dev/null 2>&1
	exit "$status"
}
trap die 2

./general -c certs/localhost.pem &
general=$!

sleep 1

./test_https -f4 -l20 -n1000 -ph https://localhost:5443
status=$?

die
