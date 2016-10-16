#!/bin/sh

echo "m4_define(BUILD,$(date +"%y/%m/%d-")$(git rev-parse --short HEAD)-alpha)m4_dnl" > ./far2l/bootstrap/scripts/vbuild.m4
