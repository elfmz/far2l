#!/bin/sh
NEW_VERSION=$1
AUTO_UPDATE=${2:-true}  # Set to false not to auto-update to latest revision in nix
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

# Check if nix is installed
if command -v nix &>/dev/null; then
    echo "Nix is installed, updating far2l in Nix file..."
    
    # Get latest revision if auto-update is enabled
    if [ "$AUTO_UPDATE" = "true" ]; then
        echo "Fetching latest revision from GitHub..."
        NEW_REV=$(git ls-remote https://github.com/elfmz/far2l.git HEAD | cut -f1)
        echo "Latest revision: $NEW_REV"
        
        # Update revision in far2l block
        sed -i '/^[[:space:]]*far2l = /,/^[[:space:]]*};/ {
            s|rev = ".*";|rev = "'"$NEW_REV"'";|
        }' ./NixOS/far2lOverlays.nix
    fi
    
    # Extract current revision from far2l block
    CURRENT_REV=$(sed -n '/^[[:space:]]*far2l = /,/^[[:space:]]*};/ {
        /rev = "/ s/.*"\(.*\)".*/\1/p
    }' ./NixOS/far2lOverlays.nix | head -1)
    
    echo "Current far2l revision: $CURRENT_REV"
    
    # Compute new hash for the current revision
    echo "Computing hash for revision $CURRENT_REV..."
    
    # Use nix-prefetch-url to get the hash
    if command -v nix-prefetch-url &>/dev/null; then
        HASH_BASE64=$(nix-prefetch-url --unpack "https://github.com/elfmz/far2l/archive/$CURRENT_REV.tar.gz" 2>/dev/null)
        if [ -n "$HASH_BASE64" ]; then
            HASH_SRI="sha256-$HASH_BASE64"
        fi
    fi
    
    # Fallback to nix hash with wget if nix-prefetch-url failed
    if [ -z "$HASH_SRI" ]; then
        TEMP_DIR=$(mktemp -d)
        cd "$TEMP_DIR" || exit
        
        echo "Downloading source to compute hash..."
        wget -q --show-progress "https://github.com/elfmz/far2l/archive/$CURRENT_REV.tar.gz" -O source.tar.gz
        
        if [ -f source.tar.gz ]; then
            HASH_BASE64=$(nix hash file --type sha256 --base64 source.tar.gz 2>/dev/null)
            if [ -n "$HASH_BASE64" ]; then
                HASH_SRI="sha256-$HASH_BASE64"
            fi
        fi
        
        cd - > /dev/null || exit
        rm -rf "$TEMP_DIR"
    fi
    
    if [ -n "$HASH_SRI" ] && [ "$HASH_SRI" != "sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=" ]; then
        echo "New hash: $HASH_SRI"
        
        # Update hash in far2l block only
        sed -i '/^[[:space:]]*far2l = /,/^[[:space:]]*};/ {
            s|sha256 = ".*";|sha256 = "'"$HASH_SRI"'";|
        }' ./NixOS/far2lOverlays.nix
    else
        echo "Warning: Could not compute valid hash for revision $CURRENT_REV"
        echo "Keeping existing hash"
    fi
    
    echo "Updated nix far2l version to $NEW_VERSION"

	# Create version string with revision suffix
    REV_SHORT=$(echo "$CURRENT_REV" | cut -c1-7)
    NIX_VERSION="${NEW_VERSION}-${REV_SHORT}"
    echo "Setting Nix version to: $NIX_VERSION"
    
    # Update version in far2l block with the new format
    sed -i '/^[[:space:]]*far2l = /,/^[[:space:]]*};/ {
        s/^\([[:space:]]*\)version = ".*";/\1version = "'"${NIX_VERSION}"'";/
    }' ./NixOS/far2lOverlays.nix
    
    echo "Updated far2l nix version to $NIX_VERSION"

else
    echo 'No nix installed, skipping Nix file update'
fi

git add ./version ./buildroot/far2l/far2l.mk ../changelog.md ../man/far2l.1 ../man/*/far2l.1
# Only add the Nix file if nix is installed
if command -v nix &>/dev/null; then
    git add ./NixOS/far2lOverlays.nix
fi
git commit -m "Bump version to $NEW_VERSION"
git tag $NEW_TAG
