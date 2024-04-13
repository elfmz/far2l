#!/bin/sh
NEW_VERSION=$1
if [ "$NEW_VERSION" = "" ]; then
	NEW_VERSION=$(perl -F'\.' -lane 'print $F[0].".".$F[1].".".($F[2]+1)' ./version)
fi
echo NEW_VERSION=$NEW_VERSION
echo $NEW_VERSION > ./version
git add ./version
git commit -m "Bump version to $NEW_VERSION"
git tag v_$NEW_VERSION
