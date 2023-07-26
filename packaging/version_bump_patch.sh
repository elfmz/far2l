#!/bin/sh
NEW_VERSION=$(perl -F'\.' -lane 'print $F[0].".".$F[1].".".($F[2]+1)' ./version)
echo NEW_VERSION=$NEW_VERSION
echo $NEW_VERSION > ./version
git add ./version
git commit -m "Bump version to $NEW_VERSION"
git tag v_$NEW_VERSION
