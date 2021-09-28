#!/bin/bash

set -ue

dir_name=$(dirname $0)
. $dir_name/helpers.sh

if [[ $(get_width) -gt 200 ]]; then
	echo "$(~/.tmux/bin/resource_usage)#[bg=colour6] $(date +'%a %Y/%m/%d %H:%M:%S') "
else
	echo "$(~/.tmux/bin/resource_usage narrow)#[bg=colour6] $(date +'%m/%d %H:%M:%S') "
fi
