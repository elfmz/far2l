#!/bin/bash
#
# turn off F10 usage in gnome terminal
#
gsettings set org.gnome.Terminal.Legacy.Settings menu-accelerator-enabled false
#
# turn off some gnome wm-specific hot keys
#
gsettings set org.gnome.desktop.wm.keybindings panel-main-menu "[]" 		# Alt+F1
gsettings set org.gnome.desktop.wm.keybindings panel-run-dialog "[]" 		# Alt+F2
#gsettings set org.gnome.desktop.wm.keybindings close "[]" 					# Alt+F4
gsettings set org.gnome.desktop.wm.keybindings unmaximize "['']" 			# Alt+F5
gsettings set org.gnome.desktop.wm.keybindings begin-move "['disabled']" 	# Alt+F7
gsettings set org.gnome.desktop.wm.keybindings begin-resize "['disabled']" 	# Alt+F8
#gsettings set org.gnome.desktop.wm.keybindings toggle-maximized "['']" 	# Alt+F10
