#!/bin/bash

set -eu

if [[ ${1-""} == "short" ]]; then
	short=1
else
	short=0
fi

get_color() {
	local num=$1
	if [[ $(bc <<< "$num >= 80") == 1 ]]; then # high
		echo "#[bg=colour1,fg=colour255]"
	elif [[ $(bc <<< "$num >= 30") == 1 && $(bc <<< "$num < 80") == 1 ]]; then # middle
		echo "#[bg=colour3,fg=colour255]"
	else # low
		echo "#[bg=colour10,fg=colour255]"
	fi
}

reset_color() {
	echo "#[bg=colour10,fg=colour255]"
}

display_kb() {
	local units=(KB MB GB TB PB EB ZB YB) curUnit=0
	local size=$1
	while [[ $(bc <<< "$size >= 1000") == 1 ]]; do
		size=$(bc <<< "scale=3; $size / 1024")
		curUnit=$((curUnit + 1))
		if [[ $curUnit > 7 ]]; then
			echo "memory too large" >&2; exit -1
		fi
	done
	printf "%.1f%s" $size ${units[$curUnit]}
}

mem_usage() {
	eval $(cat /proc/meminfo | grep -E '^MemTotal:|^MemFree:|^MemAvailable:|^Buffers:|^Cached:' | tr ':' '=' | tr -d ' ' | sed 's/^/local /' | sed 's/kB$//')
	local usage
	if [[ -v MemAvailable ]]; then
		usage=$(( MemTotal - MemAvailable ))
	else
		usage=$(( MemTotal - MemFree - Buffers - Cached ))
	fi
	local rate=$(bc <<< "scale=3; 100.0 * $usage / $MemTotal")
	if [[ $short == 1 ]]; then
		printf "$(get_color $rate)%s" $(display_kb $usage)
	else
		printf "$(get_color $rate) %s/%s " $(display_kb $usage) $(display_kb $MemTotal)
	fi
}

cpu_usage() {
	local prev=$(cat '/tmp/tmux-cpu-stat-last')
	local last=$(grep '^cpu[0-9]* ' /proc/stat | awk '{s=0;for(i=2;i<=NF;i++){s+=$i};print $5,s}' | tee '/tmp/tmux-cpu-stat-last')
	local rate=$(paste <(echo "$prev") <(echo "$last") | awk '{print 100-(100.0*($3-$1)/($4-$2))}')

	local cpu=$(echo "$rate" | head -n 1)
	local cores=$(echo "$rate" | tail -n +2 | wc -l)
	if [[ $cores == 1 ]]; then
		if [[ $short == 1 ]]; then
			printf "$(get_color $cpu)%3.1f%%$(reset_color)|" $cpu
		else
			printf "$(get_color $cpu) CPU:%3.1f%% " $cpu
		fi
	else
		local core=$(echo "$rate" | tail -n +2 | sort -rn | head -n 1)
		if [[ $short == 1 ]]; then
			printf "$(get_color $cpu)%3.1f%%$(reset_color)|$(get_color $core)%3.1f%%$(reset_color)|" $cpu $core
		else
			printf "$(get_color $cpu) CPU:%3.1f%% $(get_color $core) Core:%3.1f%% " $cpu $core
		fi
	fi
}

echo "$(cpu_usage)$(mem_usage)"
