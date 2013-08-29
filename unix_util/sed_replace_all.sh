#!/bin/sh
for ARG in "$@"
do
#  find $ARG -type f -exec echo "sed -i -e 's/before/after/g'" {} \;
   find $ARG -type f -exec sed -i -e 's/before/after/g' {} \;
done
