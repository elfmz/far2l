#!/bin/sh

echo "#pragma once" >./all.h
find . -name *.cpp -exec grep -i _export {} \; |grep -v -i "#define" >>./all.h
sed -i 's/)/);/' all.h 
