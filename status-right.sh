#!/bin/bash

set -ue

dir_name=$(dirname $0)
. $dir_name/helpers.sh

if [[ $(get_width) -gt 200 ]]; then
	echo "#(~/.tmux/resources-usage.sh)"
else
	echo "#(~/.tmux/resources-usage.sh short)"
fi
