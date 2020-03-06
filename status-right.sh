#!/bin/bash

set -ue

dir_name=$(dirname $0)
. $dir_name/helpers.sh

if [[ $(get_width) -gt 200 ]]; then
	echo "#(~/.tmux/resources-usage.sh)#[bg=colour6] #(date +'%a %Y/%m/%d %H:%M:%S') "
else
	echo "#(~/.tmux/resources-usage.sh short)#[bg=colour6] #(date +'%a %m/%d %H:%M') "
fi
