get_width() {
	tmux lsw -F '#{window_active} #{window_width}' | grep '^1 ' | cut -d' ' -f2
}
