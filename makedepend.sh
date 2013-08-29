#!/bin/sh

# This script generates dependency makefiles in the way WRATH wants them
# Probably reusable in other projects as well, but not tested separately
# See main Makefile for invocation examples

CXX="$1"
CPPFLAGS="$2"
BASEDIR="$3"
INPUTSTEM="$4"
INPUTFILE="$5"
OUTPUTFILE="$6"

INPUTDIR="$(dirname $INPUTSTEM)"
INPUTBASE="$(basename $INPUTSTEM)"

# Generate the deps with the preprocessor
# Edit the output with sed to convert
#   foo.o: foo.cpp foo.h
# into
#   foo.o foo.d : foo.cpp foo.h
#
# With the flag -MP we also generate stub rules for the prerequisites, like so:
#
#   foo.h:
#
# An empty rule with no commands or prerequisites of its own makes make continue if
# the file doesn't exist and cannot be made. This makes builds still work if old deps
# file exists and header files are removed.

"$CXX" -MM -MP $CPPFLAGS "$INPUTFILE" 2>/dev/null | sed "s@\($INPUTBASE\)\.o[ :]*@$BASEDIR/$INPUTDIR/\1.o $OUTPUTFILE : @g" > "$OUTPUTFILE"

# If the file didn't get generated, exit with an error status
if [ ! -s "$OUTPUTFILE" ]; then
    rm -f "$OUTPUTFILE"
    exit 1
fi
