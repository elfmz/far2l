VER=`cat ../version`

echo $VER: building changelog...
( cd ../.. ; packaging/suse/git-rpm-changelog.sh --since '2022-01-01' | python3 packaging/suse/filter-changelog.py ) > changelog

echo $VER: building...

cd ../..
CWD=`pwd`
cd ..

[ -f far2l-$VER.tar.gz ] && rm -f far2l-$VER.tar.gz
[ -d far2l-$VER ] && rm -rf far2l-$VER

mkdir -p far2l-$VER
cp -aR $CWD/* far2l-$VER
cp -aR $CWD/.git far2l-$VER
tar cf far2l-$VER.tar.gz far2l-$VER/

cp far2l-$VER.tar.gz ~/rpmbuild/SOURCES
cp $CWD/packaging/suse/far2l.spec ~/rpmbuild/SPECS/far2l.spec
cp $CWD/packaging/suse/copyright ~/rpmbuild/SPECS/far2l.spec
rpmbuild -bb  ~/rpmbuild/SPECS/far2l.spec

cp ~/rpmbuild/RPMS/x86_64/far2l-*$VER-*.x86_64.rpm .

cd $CWD/packaging/suse
echo $VER: Done
