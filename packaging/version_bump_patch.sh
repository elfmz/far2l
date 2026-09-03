#!/bin/bash
NEW_VERSION=$1
AUTO_UPDATE=${2:-true}  # Set to false not to auto-update to latest revision in nix
if [ "$NEW_VERSION" = "" ]; then
	NEW_VERSION=$(perl -F'\.' -lane 'print $F[0].".".$F[1].".".($F[2]+1)' ./version)
fi

echo NEW_VERSION=$NEW_VERSION
NEW_TAG=v_$NEW_VERSION

echo $NEW_VERSION > ./version

sed -i "s|Master (current development)|Master (current development)\\n\\n## ${NEW_VERSION} beta ($(date -I))|" ../changelog.md

CURRENT_MONTH=$(LC_ALL=C.UTF-8 LC_TIME=C.UTF-8 date '+%B %Y')
sed -i "s|^\.TH.*$|.TH FAR2L 1 \"${CURRENT_MONTH}\" \"FAR2L Version ${NEW_VERSION}\" \"Linux fork of FAR Manager v2\"|" ../man/far2l.1 ../man/*/far2l.1

source ./buildroot/update.sh
source ./NixOS/update.sh

git add ./version ../changelog.md ../man/far2l.1 ../man/*/far2l.1

git commit -m "Bump version to $NEW_VERSION"
git tag $NEW_TAG
