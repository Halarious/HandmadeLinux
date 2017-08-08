#!/bin/bash

echo --------
echo --------

WildCard=(*.{c,cpp,h,inl} --exclude stb_truetype.h)

echo STATICS FOUND:
grep -s -r -i -n "static" ${WildCard[@]} 
echo --------
echo --------
echo GLOBALS FOUND:
grep -s -r -i -n "local_persist"   ${WildCard[@]} 
grep -s -r -i -n "global_variable" ${WildCard[@]} 
echo --------
echo --------
