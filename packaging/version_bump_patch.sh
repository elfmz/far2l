#!/bin/sh
NEW_VERSION=$1
if [ "$NEW_VERSION" = "" ]; then
	NEW_VERSION=$(perl -F'\.' -lane 'print $F[0].".".$F[1].".".($F[2]+1)' ./version)
fi

echo NEW_VERSION=$NEW_VERSION
NEW_TAG=v_$NEW_VERSION

echo $NEW_VERSION > ./version

sed -i "s|FAR2L_VERSION = .*|FAR2L_VERSION = $NEW_TAG|" buildroot/far2l/far2l.mk

git add ./version ./buildroot/far2l/far2l.mk
git commit -m "Bump version to $NEW_VERSION"
git tag $NEW_TAG
