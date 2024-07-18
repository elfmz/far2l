#!/bin/bash

file_path="$HOME/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-keyboard-shortcuts.xml"

if [ ! -f "$file_path" ]; then
    echo "$file_path not found."
    exit 1
fi

sed -i 's/&lt;Alt&gt;F1/\&lt;Alt\&gt;\&lt;Shift\&gt;F1/g' "$file_path"
sed -i 's/&lt;Alt&gt;F2/\&lt;Alt\&gt;\&lt;Shift\&gt;F2/g' "$file_path"
sed -i 's/&lt;Alt&gt;F3/\&lt;Alt\&gt;\&lt;Shift\&gt;F3/g' "$file_path"
#sed -i 's/&lt;Alt&gt;F4/\&lt;Alt\&gt;\&lt;Shift\&gt;F4/g' "$file_path"
sed -i 's/&lt;Alt&gt;F5/\&lt;Alt\&gt;\&lt;Shift\&gt;F5/g' "$file_path"
sed -i 's/&lt;Alt&gt;F6/\&lt;Alt\&gt;\&lt;Shift\&gt;F6/g' "$file_path"
sed -i 's/&lt;Alt&gt;F7/\&lt;Alt\&gt;\&lt;Shift\&gt;F7/g' "$file_path"
sed -i 's/&lt;Alt&gt;F8/\&lt;Alt\&gt;\&lt;Shift\&gt;F8/g' "$file_path"
sed -i 's/&lt;Alt&gt;F9/\&lt;Alt\&gt;\&lt;Shift\&gt;F9/g' "$file_path"

sed -i 's/&lt;Primary&gt;F1/\&lt;Primary\&gt;\&lt;Shift\&gt;F1/g' "$file_path"
sed -i 's/&lt;Primary&gt;F2/\&lt;Primary\&gt;\&lt;Shift\&gt;F2/g' "$file_path"
sed -i 's/&lt;Primary&gt;F3/\&lt;Primary\&gt;\&lt;Shift\&gt;F3/g' "$file_path"
sed -i 's/&lt;Primary&gt;F4/\&lt;Primary\&gt;\&lt;Shift\&gt;F4/g' "$file_path"
sed -i 's/&lt;Primary&gt;F5/\&lt;Primary\&gt;\&lt;Shift\&gt;F5/g' "$file_path"
sed -i 's/&lt;Primary&gt;F6/\&lt;Primary\&gt;\&lt;Shift\&gt;F6/g' "$file_path"
sed -i 's/&lt;Primary&gt;F7/\&lt;Primary\&gt;\&lt;Shift\&gt;F7/g' "$file_path"
sed -i 's/&lt;Primary&gt;F8/\&lt;Primary\&gt;\&lt;Shift\&gt;F8/g' "$file_path"
sed -i 's/&lt;Primary&gt;F9/\&lt;Primary\&gt;\&lt;Shift\&gt;F9/g' "$file_path"
