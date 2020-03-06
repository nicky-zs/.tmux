#!/bin/bash

set -ue

dir_name=$(dirname $0)
. $dir_name/helpers.sh

if [[ $(get_width) -gt 200 ]]; then
	echo " #H:#S #I:#P "
else
	echo " #S "
fi
