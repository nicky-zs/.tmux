#!/bin/sh

set -eu

dir_name=$(dirname "$0")
. "$dir_name/helpers.sh"

if [ "$(get_width)" -gt 200 ]; then
	echo " #H🔌:#S "
else
	echo "🔌#S "
fi
