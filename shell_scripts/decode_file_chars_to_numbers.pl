#!/usr/bin/perl
 
# open file
open(FILE, "$ARGV[0]") or die("Unable to open file");

# read file into an array
@data = <FILE>;

foreach $line (@data)
{
    @ASCII = unpack("C*", $line);
    foreach (@ASCII) {
        print "$_,";
    }
}
