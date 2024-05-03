#!/bin/sh
############################################################
#This script used by FAR to show processes list
############################################################
############################################################
#For per user customization - create:
#~/.config/far2l/ps.sh
############################################################

if [ -x ~/.config/far2l/ps.sh ]; then
. ~/.config/far2l/ps.sh
fi

if command -v htop >/dev/null 2>&1; then #GNOME
	htop

else
	if [ ! -f ~/.config/far2l/ps.warned ] ; then
		B=$(printf '\e[1;91m')
		N=$(printf '\e[0m')
		echo "It is recommended to install <${B}htop${N}> utility."
		echo "Without <${B}htop${N}> installed far2l will use <${B}top${N}>."
		echo "Press any key to continue..."
		read -r -n 1 k <&1
		touch ~/.config/far2l/ps.warned
	fi
	top
fi
