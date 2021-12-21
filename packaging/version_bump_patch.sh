#!/bin/sh
NEW_VERSION=$(awk -F'.' \
	' { print $1"."$2"."($3+1); } ' \
	./version)
echo NEW_VERSION=$NEW_VERSION
echo $NEW_VERSION > ./version
git add ./version
git commit -m 'Bump version to $NEW_VERSION'
git tag v_$NEW_VERSION
