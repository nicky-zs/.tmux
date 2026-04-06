#!/bin/sh

set -eu

dir_name=$(dirname "$0")
. "$dir_name/helpers.sh"

# Use session ID (e.g., $0, $1) which never changes, unlike session name
# #{session_id} returns the immutable session ID assigned at session creation
SESSION_ID=$(tmux display-message -p '#{session_id}')
# Remove '$' prefix for filename safety
SESSION_ID="${SESSION_ID#\$}"

if [ "$(get_width)" -gt 200 ]; then
	$dir_name/../bin/resource_usage "$SESSION_ID"
else
	$dir_name/../bin/resource_usage "$SESSION_ID" narrow
fi
