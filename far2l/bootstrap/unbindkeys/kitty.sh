#!/bin/bash
#
# use this command to set kitty as default terminal emulator
# sudo update-alternatives --set x-terminal-emulator /usr/bin/kitty
#
# now lets free some kitty key combinations for far2l
#
kitty_config_dir="${HOME}/.config/kitty"
kitty_config_file="${kitty_config_dir}/kitty.conf"
if [ ! -d "${kitty_config_dir}" ]; then
  mkdir -p "${kitty_config_dir}"
fi
if [ ! -f "${kitty_config_file}" ]; then
  touch "${kitty_config_file}"
fi
if ! grep -q "map ctrl+shift+right no_op" "${kitty_config_file}"; then
  echo "map ctrl+shift+right no_op" >> "${kitty_config_file}"
fi
if ! grep -q "map ctrl+shift+left no_op" "${kitty_config_file}"; then
  echo "map ctrl+shift+left no_op" >> "${kitty_config_file}"
fi
if ! grep -q "map ctrl+shift+home no_op" "${kitty_config_file}"; then
  echo "map ctrl+shift+home no_op" >> "${kitty_config_file}"
fi
if ! grep -q "map ctrl+shift+end no_op" "${kitty_config_file}"; then
  echo "map ctrl+shift+end no_op" >> "${kitty_config_file}"
fi
if ! grep -q "enable_audio_bell no" "${kitty_config_file}"; then
  echo "enable_audio_bell no" >> "${kitty_config_file}"
fi
