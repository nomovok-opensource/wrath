#!/bin/bash

set -e

input_filename="$1"
source_file_name="$2.cpp"
output_directory="$3"
if [ -z "$output_directory" ]; then
    output_directory="."
fi
source_file_name="$output_directory/$source_file_name"

thisdir="`dirname $0`"
decoder="$thisdir/decode_file_chars_to_numbers.pl"

if [ ! -f "$decoder" ]; then
    echo decode_file_chars_to_numbers.pl not found in "$thisdir"
    exit 1
fi

echo -e "\n#include \"WRATHConfig.hpp\"" > "$source_file_name"
echo -e "\n#include \"WRATHShaderSourceResource.hpp\"" >> "$source_file_name"

echo -e "namespace { namespace WRATH_SHADER_SOURCE_UNIQUE_ID(Private) { \n\tconst char values[]={ " >> "$source_file_name"
perl "$decoder" "$1" >> "$source_file_name" || exit 1
echo -e " 0 };\n" >> "$source_file_name"
echo -e "\n\n \n\tWRATHShaderSourceResource obj(\"$2\", values);\n}\n}\n" >> "$source_file_name"
