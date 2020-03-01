#!/bin/bash

function showColors() {
	for i in {0..255}; do
		printf "\x1b[$1;5;${i}mcolor%-5i\x1b[0m" $i
		(( ($i + 1 ) % 8 )) || echo
	done
}

case "$1" in
	fg)
		showColors 38
		;;
	bg)
		showColors 48
		;;
	*)
		echo "Usage: $0 {fg|bg}"
		exit 1
esac
