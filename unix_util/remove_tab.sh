#!/bin/sh
#
# use with 
#  grep -Prl "\t" path1 path2 ... | xargs 
# to remove tabs from a bunch of files
for ARG in "$@"
do
    cp $ARG tmp
    expand tmp > $ARG
    rm tmp
done