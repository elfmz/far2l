#!/bin/sh
NEW_VERSION=$1
if [ "$NEW_VERSION" = "" ]; then
	NEW_VERSION=$(perl -F'\.' -lane 'print $F[0].".".$F[1].".".($F[2]+1)' ./version)
fi

echo NEW_VERSION=$NEW_VERSION
NEW_TAG=v_$NEW_VERSION

echo $NEW_VERSION > ./version

sed -i "s|FAR2L_VERSION = .*|FAR2L_VERSION = $NEW_TAG|" buildroot/far2l/far2l.mk

sed -i "s|Master (current development)|Master (current development)\\n\\n## ${NEW_VERSION} beta ($(date -I))|" ../changelog.md

CURRENT_MONTH=$(LC_ALL=C.UTF-8 LC_TIME=C.UTF-8 date '+%B %Y')
sed -i "s|^\.TH.*$|.TH FAR2L 1 \"${CURRENT_MONTH}\" \"FAR2L Version ${NEW_VERSION}\" \"Linux fork of FAR Manager v2\"|" ../man/far2l.1 ../man/*/far2l.1

sed -i 's/^\([[:space:]]*\)version = ".*";/\1version = "'${NEW_VERSION}'";/' ./NixOS/far2lOverlays.nix

git add ./version ./buildroot/far2l/far2l.mk ../changelog.md ../man/far2l.1 ../man/*/far2l.1 ./NixOS/far2lOverlays.nix
git commit -m "Bump version to $NEW_VERSION"
git tag $NEW_TAG
