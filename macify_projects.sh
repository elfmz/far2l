sed -i -- 's/ -export-dynamic//g' far2l/far2l.project
sed -i -- 's/<Linker Options="-Wl,--whole-archive -lWinPort -Wl,--no-whole-archive/<Linker Options="-L\/usr\/local\/lib -Wl,-export_dynamic -force_load ..\/WinPort\/Debug\/libWinPort.a/g' far2l/far2l.project

sed -i -- 's/<Linker Options="/<Linker Options="-L\/usr\/local\/lib /g' farlng/farlng.project

for f in * ; do
	if [[ -f $f/$f.project ]]; then
		skipfiles=(far2l farlng _All)
		case "${skipfiles[@]}" in *"$f"*) continue ;; esac
		echo $f
		sed -i -- 's/<Linker Options="/<Linker Options="-flat_namespace -undefined suppress /g' $f/$f.project
	fi
done
