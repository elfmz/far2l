#!/bin/sh

HASH="ref: HEAD";
while [ $HASH = "ref\:*" ];
do 
  HASH="$(cat ".git/$(echo $HASH | cut -d \  -f 2)")";
done;
echo $HASH

