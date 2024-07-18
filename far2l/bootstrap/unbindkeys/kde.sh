#!/bin/bash

file_path="$HOME/.config/kglobalshortcutsrc"

if [ ! -f "$file_path" ]; then
    echo "$file_path not found."
    exit 1
fi

sed -i 's/Alt+F1/none/g' "$file_path"
sed -i 's/Alt+F2/none/g' "$file_path"
sed -i 's/Alt+F3/none/g' "$file_path"
#sed -i 's/Alt+F4/none/g' "$file_path"
sed -i 's/Alt+F5/none/g' "$file_path"
sed -i 's/Alt+F6/none/g' "$file_path"
sed -i 's/Alt+F7/none/g' "$file_path"
sed -i 's/Alt+F8/none/g' "$file_path"
sed -i 's/Alt+F9/none/g' "$file_path"
sed -i 's/Alt+F10/none/g' "$file_path"
sed -i 's/Alt+F11/none/g' "$file_path"
sed -i 's/Alt+F12/none/g' "$file_path"

sed -i 's/Ctrl+F1/none/g' "$file_path"
sed -i 's/Ctrl+F2/none/g' "$file_path"
sed -i 's/Ctrl+F3/none/g' "$file_path"
sed -i 's/Ctrl+F4/none/g' "$file_path"
sed -i 's/Ctrl+F5/none/g' "$file_path"
sed -i 's/Ctrl+F6/none/g' "$file_path"
sed -i 's/Ctrl+F7/none/g' "$file_path"
sed -i 's/Ctrl+F8/none/g' "$file_path"
sed -i 's/Ctrl+F9/none/g' "$file_path"
sed -i 's/Ctrl+F10/none/g' "$file_path"
sed -i 's/Ctrl+F11/none/g' "$file_path"
sed -i 's/Ctrl+F12/none/g' "$file_path"
