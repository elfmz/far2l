# source this file in order to run far2l tty mode and make
# shell change directory to last active far2l directory when it exit
# Example: add to you bash.rc:
#  alias f2l="source $(which far2l-cd.sh)"
# and later just run f2l
# Note that there is no much sense to just run this file, so use 'source' Luke

if [ -d /tmp ]; then
	export FAR2L_CWD=/tmp/far2l-cwd-$$
elif [ -d /var/tmp ]; then
	export FAR2L_CWD=/var/tmp/far2l-cwd-$$
else
	export FAR2L_CWD=~/.far2l-cwd-$$
fi

if echo '' > "$FAR2L_CWD"; then
	chmod 0600 "$FAR2L_CWD"
else
	unset FAR2L_CWD
fi

# echo Running far2l --tty "$@"
far2l --tty "$@"

if [ "$FAR2L_CWD" != '' ]; then
	cwd="$(cat "$FAR2L_CWD")"
	unlink "$FAR2L_CWD"
	unset FAR2L_CWD

	if [ "$cwd" != '' ] && [ -d "$cwd" ]; then
#		echo "Changing workdir to $cwd"
		cd "$cwd"
	fi
fi
