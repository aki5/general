#!/bin/bash

numkeys=100
if [ $# -gt 0 ]; then
	numkeys=$1
fi

die(){
	kill "$general"
	wait >/dev/null 2>&1
	exit "$status"
}
trap die 2

./general -c certs/localhost.pem &
general=$!

sleep 1

./test_https -f5 -l10 -n$numkeys -ph https://localhost:5443
status=$?

die
