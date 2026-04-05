#!/bin/bash

set -eu

show_colors() {
    local mode=$1
    for i in {0..255}; do
        # 打印颜色块
        printf "\e[%d;5;%dmcolor%-5d\e[0m" "$mode" "$i" "$i"
        
        # 优雅的换行：利用算术运算和 && 逻辑
        (( (i + 1) % 8 == 0 )) && echo
    done
}

case "${1:-}" in
    fg) show_colors 38 ;;
    bg) show_colors 48 ;;
    
    # 匹配 0-255 的数字
    [0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])
        printf "\e[48;5;%dm%*s\e[0m\n" "$1" "$(tput cols)" ""
        ;;
    
    *)
        printf "Usage: %s {fg|bg|0-255}\n" "${0##*/}" >&2
        exit 1
        ;;
esac

